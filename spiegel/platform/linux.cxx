#include "spiegel/common.hxx"
#include "spiegel/intercept.hxx"
#include "common.hxx"

#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <valgrind/valgrind.h>

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif

namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

char *
self_exe()
{
    // We could find the ELF auxv_t vector which is above the environ[]
    // passed to us by the kernel, there is usually a AT_EXECFN entry
    // there which points at the executable filename.  But, when running
    // under Valgrind, the aux vector is synthetically built by Valgrind
    // and doesn't contain AT_EXECFN.  However, Valgrind *does*
    // intercept a readlink() of the magical Linux /proc file and
    // substitute the answer we want (instead of what the real kernel
    // readlink() call would return, which is the Valgrind executable
    // itself).  So, we have a fallback that works in all cases, we may
    // as well use it.
    char buf[PATH_MAX];
    static const char filename[] = "/proc/self/exe";
    int r = readlink(filename, buf, sizeof(buf)-1);
    if (r < 0)
    {
	perror(filename);
	return 0;
    }
    // readlink() doesn't terminate the buffer
    buf[r] = '\0';
    return xstrdup(buf);
}

static int
add_one_linkobj(struct dl_phdr_info *info,
		size_t size __attribute__((unused)),	// sizeof(*info)
		void *closure)
{
    vector<linkobj_t> *vec = (vector<linkobj_t> *)closure;

    const char *name = info->dlpi_name;
    if (name && !*name)
	name = NULL;

    if (!name && info->dlpi_addr)
	return 0;
    if (!info->dlpi_phnum)
	return 0;

#if 0
    fprintf(stderr, "dl_phdr_info { addr=%p name=%s }\n",
	    (void *)info->dlpi_addr, info->dlpi_name);
#endif

    linkobj_t lo;
    lo.name = name;

    for (int i = 0 ; i < info->dlpi_phnum ; i++)
    {
	if (!info->dlpi_phdr[i].p_memsz)
	    continue;

	const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
	lo.mappings.push_back(mapping_t(
		(unsigned long)ph->p_offset, (unsigned long)ph->p_memsz,
		(void *)((unsigned long)info->dlpi_addr + ph->p_vaddr)));
    }
    vec->push_back(lo);

    return 0;
}

vector<linkobj_t> get_linkobjs()
{
    vector<linkobj_t> vec;
    dl_iterate_phdr(add_one_linkobj, &vec);
    return vec;
}

static vector<spiegel::mapping_t> plts;

void add_plt(const spiegel::mapping_t &m)
{
    plts.push_back(m);
}

static bool is_in_plt(spiegel::addr_t addr)
{
    vector<spiegel::mapping_t>::const_iterator i;
    for (i = plts.begin() ; i != plts.end() ; ++i)
    {
	if (i->contains((void *)addr))
	    return true;
    }
    return false;
}

spiegel::addr_t normalise_address(spiegel::addr_t addr)
{
    if (is_in_plt(addr))
    {
	Dl_info info;
	memset(&info, 0, sizeof(info));
	int r = dladdr((void *)addr, &info);
	if (r)
	    return (spiegel::addr_t)dlsym(RTLD_NEXT, info.dli_sname);
    }
    return addr;
}

/*
 * Functions to ensure some .text space is writable (as well as
 * executable) so we can insert breakpoint insns, and to undo the
 * effect.
 *
 * Uses per-page reference counting so that multiple calls can be
 * nested.  This is for tidiness so that we can re-map pages back to
 * their default state after we've finished, and still handle the case
 * of two intercepts in separate functions which are located in the
 * same page.
 *
 * These functions are more general than they need to be, as we only
 * ever need the len=1 case.
 */

map<addr_t, unsigned int> pagerefs;

int
text_map_writable(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    addr_t a;
    int r;

    /* increment the reference counts on every page we hit */
    for (a = start ; a < end ; a += page_size())
    {
	map<addr_t, unsigned int>::iterator itr = pagerefs.find(a);
	if (itr == pagerefs.end())
	    pagerefs[a] = 1;
	else
	    itr->second++;
    }

    /* actually change the underlying mapping in one
     * big system call. */
    r = mprotect((void *)start,
		 (size_t)(end-start),
		 PROT_READ|PROT_WRITE|PROT_EXEC);
    if (r)
    {
	perror("spiegel: mprotect");
	return -1;
    }
    return 0;
}

int
text_restore(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    addr_t a;
    int r;

    /* decrement the reference counts on every page we hit */
    for (a = start ; a < end ; a += page_size())
    {
	map<addr_t, unsigned int>::iterator itr = pagerefs.find(a);
	if (itr == pagerefs.end())
	    continue;
	if (--itr->second)
	    continue;	/* still other references */
	pagerefs.erase(itr);

	/* change the underlying mapping one page at a time */
	r = mprotect((void *)a, (size_t)page_size(), PROT_READ|PROT_EXEC);
	if (r)
	{
	    perror("spiegel: mprotect");
	    return -1;
	}
    }

    return 0;
}

static unsigned long intercept_tramp(void);

class i386_linux_call_t : public spiegel::call_t
{
private:
    i386_linux_call_t() {}

    unsigned long *args_;

    unsigned long get_arg(unsigned int i) const
    {
	return args_[i];
    }
    void set_arg(unsigned int i, unsigned long v)
    {
	args_[i] = v;
    }

    friend unsigned long intercept_tramp(void);
};

#define INSN_PUSH_EBP	    0x55
#define INSN_INT3	    0xcc
#define INSN_HLT	    0xf4

static ucontext_t tramp_uc;
static ucontext_t fpuc;
static bool hack1 = false;
static bool using_int3 = false;

static unsigned long
intercept_tramp(void)
{
    /*
     * We put all the local variables for the tramp into a struct so
     * that we can predict their relative locations on the stack.  This
     * is important because we are going to be calling the original
     * function with %esp pointing at the saved_ebp field, and anything
     * which the compiler might decide to put below that on the stack is
     * going to be clobbered.
     */
    struct
    {
	/* fake stack frame for the original function */
	unsigned long saved_ebp;
	unsigned long return_addr;
	unsigned long args[32];
	/* any data that we need to keep safe past the call to the
	 * original function, goes here */
	i386_linux_call_t call;
	addr_t addr;
	unsigned long our_esp;
    } frame;
    unsigned long parent_size;

    frame.addr = tramp_uc.uc_mcontext.gregs[REG_EIP] - (using_int3 ? 1 : 0);

    /*
     * This branch is never taken because the variable 'hack1' is never
     * set.  However having 'goto after' here prevents gcc from eliding
     * the code following the after: label.  Presumably this happens
     * because the single path reaching that code ends in a call to
     * exit() which is declared __attribute__((noreturn)).  I'm
     * surprised that taking the address of the after: label using the
     * '&&after' syntax does not force the code to be emitted but
     * instead generates a bogus address in the middle of the function!
     * This was needlessly confusing.
     */
    if (hack1)
	goto after;

//     printf("intercept_tramp: starting, RA=%p\n", __builtin_return_address(0));

    /*
     * The ucontext we get from the signal handler has no floating point
     * state in it, but setcontext() expects floating point state and
     * will handily SEGV if it's not there.  So, we use getcontext() to
     * get floating point state and copy it into the ucontext we
     * actually want to use.  Or more precisely, we copy the pointer
     * fpuc.uc_mcontect.fpregs which points to fpuc.__fpregs_mem.
     */
    memset(&fpuc, 0, sizeof(fpuc));
    if (getcontext(&fpuc))
    {
	perror("getcontext");
	exit(1);
    }
    tramp_uc.uc_mcontext.fpregs = fpuc.uc_mcontext.fpregs;
    /* Point the EBP register at our own EBP register */
    tramp_uc.uc_mcontext.gregs[REG_EBP] = fpuc.uc_mcontext.gregs[REG_EBP];

//     printf("tramp: fpregs=0x%08lx\n", (unsigned long)tramp_uc.uc_mcontext.fpregs);
//     printf("tramp: after=0x%08lx\n", (unsigned long)&&after);

    /* Setup the ucontext to look like we just called the
     * function from immediately before the 'after' label */
    tramp_uc.uc_mcontext.gregs[REG_EIP] = frame.addr + 1;
    /*
     * Build a new fake stack frame for calling the original
     * function.
     *
     * The Linux i386 setcontext() calls a system call to set the
     * sigmask but implements all the register switching in
     * userspace and doesn't reload EFLAGS.  It probably could do so
     * if it used the IRET insn instead of RET, but it doesn't.  So
     * we can't use it to switch to single-step mode.  Rather than
     * write our own, less broken, setcontext() we avoid
     * single-stepping (and the second SIGTRAP it implies) entirely
     * by noting that we only support intercepts at the first byte
     * of a function and that for non-leaf functions the first byte
     * will be the 1-byte insn "push %ebp".  We can easily simulate
     * the effect of that insn right here by adjusting the newly
     * constructed stack frame.  Another good reason to avoid single-
     * stepping is that Valgrind doesn't simulate it.
     *
     * "Trust me, I know what I'm doing."
     */
    frame.saved_ebp = (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EBP];
    frame.return_addr = (unsigned long)&&after;
    /*
     * Copy enough of the original stack frame to make it look like
     * we have the original arguments.
     */
    parent_size = tramp_uc.uc_mcontext.gregs[REG_EBP] -
		  (tramp_uc.uc_mcontext.gregs[REG_ESP]+4);
    memcpy(frame.args,
	   (void *)(tramp_uc.uc_mcontext.gregs[REG_ESP]+4),
	   MIN(parent_size, sizeof(frame.args)));
    /* setup the ucontext's ESP register to point at the new stack frame */
    tramp_uc.uc_mcontext.gregs[REG_ESP] = (unsigned long)&frame;

    /*
     * Call the BEFORE method.  This call happens late enough that
     * we have an entire new stack frame which the method can
     * examine and modify using get_arg() and set_arg().  The method
     * can also request that we return early without calling the
     * original function, by calling skip().  It can also request
     * that we call a different function instead using redirect().
     * Finally it should also be able to longjmp() out without
     * drama, e.g. as a side effect of failing a U4C_ASSERT().
     */
    frame.call.args_ = frame.args;
    intercept_t::dispatch_before(frame.addr, frame.call);
    if (frame.call.skip_)
	return frame.call.retval_;	/* before() requested skip() */
    if (frame.call.redirect_)
    {
	/* before() requested redirect, so setup the context to call
	 * that function instead. */
	tramp_uc.uc_mcontext.gregs[REG_EIP] = frame.call.redirect_;
	/* The new function won't be intercepted (well, we hope not)
	 * so we need to undo emulation of 'push %ebp'.  */
	tramp_uc.uc_mcontext.gregs[REG_ESP] += 4;
    }
    /* Save our own %esp for later */
    __asm__ volatile("movl %%esp, %0" : "=m"(frame.our_esp));

    /* switch to the ucontext */
//     printf("tramp: about to setcontext(EIP=0x%08lx ESP=0x%08lx EBP=0x%08lx)\n",
// 	   (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EIP],
// 	   (unsigned long)tramp_uc.uc_mcontext.gregs[REG_ESP],
// 	   (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EBP]);
    setcontext(&tramp_uc);
    /* notreached - setcontext() should not return, unless setting
     * the signal mask failed, which it doesn't */
    perror("setcontext");
    exit(1);

after:
    /* the original function returns here */

    /* return value is in EAX, save it directly into the call_t */
    __asm__ volatile("movl %%eax, %0" : "=m"(frame.call.retval_));

    /*
     * The compiler will emit (at least when unoptimised) instructions
     * in our epilogue that save %eax onto a temporary location on stack
     * relative to %ebp and then immediately load it again from there.
     * It's silly, but there you go.  After returning from the original
     * function our %esp is pointing at @frame, and so that temporary
     * location is now below %esp, i.e. outside the stack frame.  Of
     * course that's fine, no function care about that location, and we
     * just store and load from there.  Unfortunately Valgrind detects
     * such loads and stores below the stack pointer and warns about
     * them.  To avoid those two warnings we first restore the %esp we
     * had before calling the original function.
     */
    __asm__ volatile("movl %0, %%esp" : : "m"(frame.our_esp));

    /*
     * Call the AFTER method.  The method can examine and modify the
     * return value from the original function using get_retval() and
     * set_retval().  It should also be able to longjmp() out without
     * drama, e.g. as a side effect of failing a U4C_ASSERT().
     */
    intercept_t::dispatch_after(frame.addr, frame.call);
    return frame.call.retval_;
}

static void
handle_signal(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

//     printf("handle_signal: signo=%d code=%d pid=%d EIP 0x%08lx EBP 0x%08lx ESP 0x%08lx\n",
// 	    si->si_signo,
// 	    si->si_code,
// 	    (int)si->si_pid,
// 	    (unsigned long)uc->uc_mcontext.gregs[REG_EIP],
// 	    (unsigned long)uc->uc_mcontext.gregs[REG_EBP],
// 	    (unsigned long)uc->uc_mcontext.gregs[REG_ESP]);

    /* double-check that this is not some spurious signal */
    unsigned char *eip = (unsigned char *)(uc->uc_mcontext.gregs[REG_EIP]);
    if (using_int3)
    {
	if (sig != SIGTRAP || si->si_signo != SIGTRAP)
	    return;	    /* we got a bogus signal, wtf? */
	if (si->si_code != SI_KERNEL /* natural */ &&
	    si->si_code != TRAP_BRKPT /* via Valgrind */)
	    goto wtf;	    /* this is the code we expect from HLT traps */
	eip--;
	if (*eip != INSN_INT3)
	    goto wtf;	    /* not an INT3 */
    }
    else
    {
	if (sig != SIGSEGV || si->si_signo != SIGSEGV)
	    return;	    /* we got a bogus signal, wtf? */
	if (si->si_code != SI_KERNEL)
	    goto wtf;	    /* this is the code we expect from HLT traps */
	if (*eip != INSN_HLT)
	    goto wtf;	    /* not an HLT */
    }
    if (si->si_pid != 0)
	return;	    /* some process sent us SIGSEGV, wtf? */
    if (!intercept_t::is_intercepted((spiegel::addr_t)eip))
	goto wtf;   /* not an installed intercept */

//     printf("handle_signal: trap from intercept breakpoint\n");
    /* stash the ucontext for the tramp */
    /* TODO: if we were concerned about the MP case we would
     * push build a stack frame containing this context */
    tramp_uc = *uc;
    /* munge the signal ucontext so we return from
     * the signal into the tramp instead of the
     * original function */
    uc->uc_mcontext.gregs[REG_EIP] = (unsigned long)&intercept_tramp;
//     printf("handle_signal: ending\n");

    return;

wtf:
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(sig, &act, NULL);
}

int
install_intercept(spiegel::addr_t addr)
{
    int r;

    switch (*(unsigned char *)addr)
    {
    case INSN_PUSH_EBP:
	/* non-leaf function, all is good */
	break;
    case INSN_HLT:
    case INSN_INT3:
	/* already intercepted, can handle this too */
	break;
    default:
	fprintf(stderr, "spiegel: sorry, cannot intercept leaf functions\n");
	return -1;
    }

    r = text_map_writable(addr, 1);
    if (r)
	return -1;

    static bool installed_sigaction = false;
    if (!installed_sigaction)
    {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = handle_signal;
	act.sa_flags |= SA_SIGINFO;
	if (RUNNING_ON_VALGRIND)
	    using_int3 = true;
	sigaction((using_int3 ? SIGTRAP : SIGSEGV), &act, NULL);
	installed_sigaction = true;
    }

    /* TODO: install the sig handler only when there are
     * any installed intercepts, or the pid has changed */
    *(unsigned char *)addr = (using_int3 ? INSN_INT3 : INSN_HLT);
    VALGRIND_DISCARD_TRANSLATIONS(addr, 1);

    return 0;
}

int
uninstall_intercept(spiegel::addr_t addr)
{
    if (*(unsigned char *)addr != (using_int3 ? INSN_INT3 : INSN_HLT))
    {
	fprintf(stderr, "spiegel: attempting to uninstall an unintercepted function\n");
	return -1;
    }
    *(unsigned char *)addr = INSN_PUSH_EBP;
    VALGRIND_DISCARD_TRANSLATIONS(addr, 1);
    return text_restore(addr, 1);
}

/* This trick doesn't work - Valgrind actively prevents
 * the simulated program from hijacking it's log fd */
#if 0
    if (RUNNING_ON_VALGRIND)
    {
	/* ok, this is cheating */
	int old_stderr = -1;
	int fd;
	int r;

	strncpy(buf, "/tmp/spiegel-stack-XXXXXX", maxlen);
	fd = mkstemp(buf);
	if (fd < 0)
	    return -errno;
	unlink(buf);

	old_stderr = dup(STDERR_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);

	VALGRIND_PRINTF_BACKTRACE("\n");

	lseek(STDERR_FILENO, 0, SEEK_SET);
	r = read(STDERR_FILENO, buf, sizeof(buf)-1);
	if (r < 0)
	{
	    r = -errno;
	    goto out;
	}
	buf[r] = '\0';
	r = 0;
out:
	if (old_stderr > 0)
	{
	    dup2(old_stderr, STDERR_FILENO);
	    close(old_stderr);
	}
	return r;
    }
#endif

vector<spiegel::addr_t> get_stacktrace()
{
    /* This only works if a frame pointer is used, i.e. it breaks
     * with -fomit-frame-pointer.
     *
     * TODO: use DWARF2 unwind info and handle -fomit-frame-pointer
     *
     * TODO: terminating the unwind loop is tricky to do properly,
     *       we need to estimate the stack boundaries.  Instead
     *       we approximate
     *
     * TODO: should return a vector of {ip=%eip,fp=%ebp,sp=%esp}
     */
    unsigned long bp;
    vector<spiegel::addr_t> stack;

    __asm__ volatile("movl %%ebp, %0" : "=r"(bp));
    for (;;)
    {
	stack.push_back(((unsigned long *)bp)[1]-5);
	unsigned long nextbp = ((unsigned long *)bp)[0];
	if (!nextbp)
	    break;
	if (nextbp < bp)
	    break;	// moving in the wrong direction
	if ((nextbp - bp) > 16384)
	    break;	// moving a heuristic "too far"
	bp = nextbp;
    };
    return stack;
}


// close namespace
} }
