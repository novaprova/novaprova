#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <sys/mman.h>

/*
 * Fourth experiment.
 *
 * Improved breakpoint implementation, using gnarly tricks
 * to bump from the signal handler to a trampoline function
 * outside of signal handler context.  Basically we don't
 * want to be calling interceptor functions from signal handler
 * context if we can avoid it.
 */

struct breakpoint
{
    /* saved parameters */
    void *addr;
    void (*before)(struct breakpoint *);
    void (*after)(struct breakpoint *);

    /* These are here for the intercept routines to
     * examine and/or modify */

    unsigned long *args;
    unsigned long retval;
    int skip;

    /* internal state */
    unsigned char old_insn;
    int state;
    ucontext_t uc;
};

static volatile int got_sigtrap = 0;
static int hack1 = 0;	    /* this is always zero */

static struct breakpoint thebp;

static void setup_breakpoint(struct breakpoint *bp, void *addr,
			     void (*before)(struct breakpoint *),
			     void (*after)(struct breakpoint *))
{
    static unsigned long page_size = 0;
    int r;

    if (*(unsigned char *)addr != 0x55)
    {
	fprintf(stderr, "Cannot intercept leaf function, sorry\n");
	exit(1);
    }

    memset(bp, 0, sizeof(*bp));
    bp->addr = addr;
    bp->before = before;
    bp->after = after;
    bp->state = 0;

    printf("Making the function writable\n");

    /* adjust region to the enclosing page */
    if (!page_size)
	page_size = sysconf(_SC_PAGESIZE);
    addr = (void *)((unsigned long)addr - ((unsigned long)addr % page_size));

    r = mprotect(addr, page_size, PROT_READ|PROT_WRITE|PROT_EXEC);
    if (r)
    {
	perror("mprotect");
	exit(1);
    }

    printf("Inserting an INT3 instruction at the first byte of the function\n");
    bp->old_insn = *(unsigned char *)bp->addr;
    *(unsigned char *)bp->addr = 0xcc;
}

#if 0
__asm__ (
"    .text\n"
"    .type	tramp2, @function\n"
"tramp2:\n"
"    mov    $0x0,%eax\n"
"    movb   $0x0,(%eax)\n"
"    .size	tramp2, .-tramp2\n"
);
extern void tramp2(void);
#endif

static unsigned long tramp(void)
{
    struct breakpoint *bp = &thebp;
    ucontext_t fpuc;
    unsigned long rv;
    unsigned long *esp;

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

    printf("tramp: starting, RA=%p\n", __builtin_return_address(0));

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
    bp->uc.uc_mcontext.fpregs = fpuc.uc_mcontext.fpregs;
    /* Point the EBP register at our own EBP register */
    bp->uc.uc_mcontext.gregs[REG_EBP] = fpuc.uc_mcontext.gregs[REG_EBP];
//     /* setup the EFLAGS register to enable single-step mode */
//     bp->uc.uc_mcontext.gregs[REG_EFL] |= 0x100;    /* TF Trace Flag */
    printf("tramp: fpregs=0x%08lx\n", (unsigned long)bp->uc.uc_mcontext.fpregs);

    printf("tramp: after=0x%08lx\n", (unsigned long)&&after);

    /* Setup the ucontext to look like we just called the
     * function from immediately before the 'after' label */
    bp->uc.uc_mcontext.gregs[REG_EIP] = (unsigned long)bp->addr + 1;
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
	 * by noting that we only support breakpoints at the first byte
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
	frame.saved_ebp = (unsigned long)bp->uc.uc_mcontext.gregs[REG_EBP];
	frame.return_addr = (unsigned long)&&after;
	/*
	 * Copy enough of the original stack frame to make it look like
	 * we have the original arguments.
	 */
	esp = (unsigned long *)bp->uc.uc_mcontext.gregs[REG_ESP];
	frame.args[0] = esp[1];
	frame.args[1] = esp[2];
	/* setup the ucontext's ESP register to point at the new stack frame */
	bp->uc.uc_mcontext.gregs[REG_ESP] = (unsigned long)&frame;

	/*
	 * Call the BEFORE intercept function.  This call happens late
	 * enough that we have an entire new stack frame which the
	 * intercept function can examine and modify, in bp->args[].
	 * The intercept function can also request that we return
	 * early without calling the original function, by setting
	 * bp->skip = 1 and bp->retval to the requested return value.
	 * It should also be able to longjmp() out without drama,
	 * e.g. as a side effect of failing a U4C_ASSERT().
	 */
	if (bp->before)
	{
	    bp->args = frame.args;
	    bp->skip = 0;
	    bp->retval = 0;
	    bp->before(bp);
	    if (bp->skip)
		return bp->retval;
	}

	/* switch to the ucontext */
	printf("tramp: about to setcontext(EIP=0x%08lx ESP=0x%08lx EBP=0x%08lx)\n",
	       (unsigned long)bp->uc.uc_mcontext.gregs[REG_EIP],
	       (unsigned long)bp->uc.uc_mcontext.gregs[REG_ESP],
	       (unsigned long)bp->uc.uc_mcontext.gregs[REG_EBP]);
	setcontext(&bp->uc);
	/* notreached - setcontext() should not return */
	perror("setcontext");
	exit(1);
    }

after:
    /* the original function returns here */
    /* return value is in EAX, save it in 'rv' */
    __asm__ volatile("pushl %%eax; popl %0" : "=r"(rv));
    printf("tramp: at 'after', rv=%lu\n", rv);
    /*
     * Call the AFTER intercept routine if it's defined.  The intercept
     * routine can examine and modify the return value from the original
     * function in bp->retval.  It should also be able to longjmp() out
     * without drama, e.g. as a side effect of failing a U4C_ASSERT().
     */
    if (bp->after)
    {
	bp->retval = rv;
	bp->after(bp);
	rv = bp->retval;
    }
    return rv;
}

static void handle_sigtrap(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;
    struct breakpoint *bp = &thebp;

    printf("handle_sigtrap: signo=%d errno=%d code=%d\n",
	    si->si_signo,
	    si->si_errno,
	    si->si_code);
    // it turns out that neither si_pid nor si_address are meaningful
    // for SIGTRAP
    if (si->si_signo != SIGTRAP)
	return;
//     if (si->si_code != SI_KERNEL)
// 	return;
    printf("handle_sigtrap: faulted at EIP 0x%08lx ESP 0x%08lx\n",
	    (unsigned long)uc->uc_mcontext.gregs[REG_EIP],
	    (unsigned long)uc->uc_mcontext.gregs[REG_ESP]);
    switch (bp->state)
    {
    case 0:
	/* first trap, from INT3 insn */
	printf("handle_sigtrap: trap from int3\n");
// 	/* temporarily remove the INT3 insn */
// 	*(unsigned char *)bp->addr = bp->old_insn;
	/* stash the ucontext for the tramp */
	bp->uc = *uc;
	/* munge the signal ucontext so we return from
	 * the signal into the tramp instead of the
	 * original function */
	uc->uc_mcontext.gregs[REG_EIP] = (unsigned long)&tramp;
// 	bp->state = 1;
	break;
//     case 1:
// 	printf("handle_sigtrap: trap from single-stepping\n");
// 	/* single step trap due to TF in EFLAGS */
// 	bp->state = 0;
// 	/* restore the INT3 insn */
// 	*(unsigned char *)bp->addr = 0xcc;
// 	/* stop single-stepping */
// 	uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;    /* TF Trace Flag */
// 	break;
    }
    printf("handle_sigtrap: ending\n");
    got_sigtrap++;
}

char pad1[8192] __attribute__((section(".text"))) = {0};

int the_function(int x, int y)
{
    int i;

    printf("Start of the_function, RA=%p x=%d y=%d\n",
	    __builtin_return_address(0), x, y);
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    printf("End of the_function\n");
    return y;
}

char pad2[8192] __attribute__((section(".text"))) = {0};

static void before(struct breakpoint *bp)
{
    printf("In BEFORE interceptor arg0=%lu arg1=%lu\n",
	   bp->args[0], bp->args[1]);
}

static void after(struct breakpoint *bp)
{
    printf("In AFTER interceptor ret=%lu\n",
	   bp->retval);
}

void do_the_calling(void)
{
    int x;

    printf("Before call got_sigtrap=%d\n", got_sigtrap);
    x = the_function(3, 42);
    printf("After call got_sigtrap=%d x=%d\n", got_sigtrap, x);

    /* do it again just to be sure */
    printf("Before call got_sigtrap=%d\n", got_sigtrap);
    x = the_function(7, 27);
    printf("After call got_sigtrap=%d x=%d\n", got_sigtrap, x);
}

int main(int argc, char **argv)
{
    struct sigaction act;
    int doit = (argc > 1 && !strcmp(argv[1], "--doit"));

    printf("Registering signal handler\n");
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handle_sigtrap;
    act.sa_flags |= SA_SIGINFO;
    sigaction(SIGTRAP, &act, NULL);

    if (doit)
	setup_breakpoint(&thebp, &the_function, before, after);

    do_the_calling();

    return 0;
}
