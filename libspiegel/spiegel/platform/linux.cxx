#include "spiegel/common.hxx"
#include "common.hxx"

#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <sys/mman.h>

namespace spiegel { namespace platform {
using namespace std;

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
		size_t size, void *closure)
{
    vector<linkobj_t> *vec = (vector<linkobj_t> *)closure;

    linkobj_t lo;
    lo.name = info->dlpi_name;
    lo.addr = (unsigned long)info->dlpi_addr;
    lo.size = (unsigned long)size;
    vec->push_back(lo);

    return 0;
}

vector<linkobj_t> self_linkobjs()
{
    vector<linkobj_t> vec;
    dl_iterate_phdr(add_one_linkobj, &vec);
    return vec;
}


/*
 * Functions to ensure some .text space is writable (as well as
 * executable) so we can insert INT3 insns, and to undo the effect.
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
    for (a = start ; a < end ; a++)
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
    for (a = start ; a < end ; a++)
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

#define INSN_PUSH_EBP	    0x55
#define INSN_INT3	    0xcc

static ucontext_t tramp_uc;
static bool hack1 = false;
vector<intercept_t*> intercept_t::installed_;

intercept_t *
intercept_t::find_installed(addr_t addr)
{
    vector<intercept_t*>::iterator itr;
    for (itr = installed_.begin() ; itr != installed_.end() ; ++itr)
	if ((*itr)->addr_ == addr)
	    return *itr;
    return 0;
}

static unsigned long
intercept_tramp(void)
{
    intercept_t *icpt = intercept_t::find_installed(
			    tramp_uc.uc_mcontext.gregs[REG_EIP]-1);
    if (!icpt)
	return 0;

    ucontext_t fpuc;
    unsigned long rv;
    unsigned long *esp;
    intercept_t::runtime_t rt;

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
    tramp_uc.uc_mcontext.gregs[REG_EIP] = icpt->get_address() + 1;
    {
	/*
	 * Build a new fake stack frame for calling the original
	 * function.  We're doing this in a new {} block so that it will
	 * be the lowermost object on the stack.
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
	 * constructed stack frame.
	 *
	 * Trust me, I know what I'm doing.
	 */
	struct
	{
	    unsigned long saved_ebp;
	    unsigned long return_addr;
	    unsigned long args[2];
	} frame;
	frame.saved_ebp = (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EBP];
	frame.return_addr = (unsigned long)&&after;
	/*
	 * Copy enough of the original stack frame to make it look like
	 * we have the original arguments.
	 */
	esp = (unsigned long *)tramp_uc.uc_mcontext.gregs[REG_ESP];
	frame.args[0] = esp[1];
	frame.args[1] = esp[2];
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
	memset(&rt, 0, sizeof(rt));
	rt.args = frame.args;
	icpt->call_before(&rt);
	if (rt.skip)
	    return rt.retval;	/* before() requested skip() */
	if (rt.redirect)
	{
	    /* before() requested redirect, so setup the context to call
	     * that function instead. */
	    tramp_uc.uc_mcontext.gregs[REG_EIP] = rt.redirect;
	    /* The new function won't be intercepted (well, we hope not)
	     * so we need to undo emulation of 'push %ebp'.  */
	    tramp_uc.uc_mcontext.gregs[REG_ESP] += 4;
	}

	/* switch to the ucontext */
// 	printf("tramp: about to setcontext(EIP=0x%08lx ESP=0x%08lx EBP=0x%08lx)\n",
// 	       (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EIP],
// 	       (unsigned long)tramp_uc.uc_mcontext.gregs[REG_ESP],
// 	       (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EBP]);
	setcontext(&tramp_uc);
	/* notreached - setcontext() should not return */
	perror("setcontext");
	exit(1);
    }

after:
    /* the original function returns here */
    /* return value is in EAX, save it in 'rv' */
    __asm__ volatile("pushl %%eax; popl %0" : "=r"(rv));
//     printf("tramp: at 'after', rv=%lu\n", rv);
    /*
     * Call the AFTER method.  The method can examine and modify the
     * return value from the original function using get_retval() and
     * set_retval().  It should also be able to longjmp() out without
     * drama, e.g. as a side effect of failing a U4C_ASSERT().
     */
    rt.retval = rv;
    icpt->call_after(&rt);
    return rt.retval;
}


static void
handle_sigtrap(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

//     printf("handle_sigtrap: signo=%d errno=%d code=%d EIP 0x%08lx ESP 0x%08lx\n",
// 	    si->si_signo,
// 	    si->si_errno,
// 	    si->si_code,
// 	    (unsigned long)uc->uc_mcontext.gregs[REG_EIP],
// 	    (unsigned long)uc->uc_mcontext.gregs[REG_ESP]);

    if (sig != SIGTRAP)
	return;	    /* we got a bogus signal, wtf? */
    if (si->si_signo != SIGTRAP)
	return;	    /* we got a bogus signal, wtf? */
    if (si->si_code != SI_KERNEL)
	return;	    /* this is the code we expect from INT3 traps */
    if (si->si_pid != 0)
	return;	    /* some process sent us SIGTRAP, wtf? */
    if (*(unsigned char *)(uc->uc_mcontext.gregs[REG_EIP]-1) != INSN_INT3)
	return;	    /* not an INT3, wtf? */

//     printf("handle_sigtrap: trap from intercept int3\n");
    /* stash the ucontext for the tramp */
    /* TODO: if we were concerned about the MP case we would
     * push build a stack frame containing this context */
    tramp_uc = *uc;
    /* munge the signal ucontext so we return from
     * the signal into the tramp instead of the
     * original function */
    uc->uc_mcontext.gregs[REG_EIP] = (unsigned long)&intercept_tramp;
//     printf("handle_sigtrap: ending\n");
}

int
intercept_t::install()
{
    int r;

    switch (*(unsigned char *)addr_)
    {
    case INSN_PUSH_EBP:
	/* non-leaf function, all is good */
	break;
    case INSN_INT3:
	/* already intercepted, can handle this too */
	break;
    default:
	fprintf(stderr, "spiegel: sorry, cannot intercept leaf functions\n");
	return -1;
    }

    r = text_map_writable(addr_, 1);
    if (r)
	return -1;

    static bool installed_sigaction = false;
    if (!installed_sigaction)
    {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = handle_sigtrap;
	act.sa_flags |= SA_SIGINFO;
	sigaction(SIGTRAP, &act, NULL);
	installed_sigaction = true;
    }
    /* TODO: install the sig handler only when there are
     * any installed intercepts, or the pid has changed */

    installed_.push_back(this);
    *(unsigned char *)addr_ = INSN_INT3;

    return 0;
}

int
intercept_t::uninstall()
{
    /* TODO */
//     installed_.erase(itr);
    return 0;
}

// close namespace
} }
