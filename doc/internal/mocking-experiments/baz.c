#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <sys/mman.h>

/*
 * Third experiment.
 *
 * Trying most of a breakpoint implementation.  Put together
 * handling SIGTRAP from an INT3 plus using mprotect() to
 * make the code writable so we can insert the insn at runtime
 * instead of using asm() to do so at compile time, plus the
 * two-step dance needed to re-insert the original insn and
 * single step over it.
 */

struct breakpoint
{
    void *addr;
    unsigned char old_insn;
    int state;
};

static volatile int got_sigtrap = 0;

static struct breakpoint thebp;

static void setup_breakpoint(struct breakpoint *bp, void *addr)
{
    static unsigned long page_size = 0;
    int r;

    memset(bp, 0, sizeof(*bp));
    bp->addr = addr;
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
	printf("handle_sigtrap: HIT BREAKPOINT\n");
	/* temporarily remove the INT3 insn */
	*(unsigned char *)bp->addr = bp->old_insn;
	/* setup the EIP register so we execute that insn */
	uc->uc_mcontext.gregs[REG_EIP]--;
	/* setup the EFLAGS register to single-step */
	uc->uc_mcontext.gregs[REG_EFL] |= 0x100;    /* TF Trace Flag */
	bp->state = 1;
	break;
    case 1:
	/* single step trap due to TF in EFLAGS */
	bp->state = 0;
	/* restore the INT3 insn */
	*(unsigned char *)bp->addr = 0xcc;
	/* stop single-stepping */
	uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;    /* TF Trace Flag */
	break;
    }
    printf("handle_sigtrap: ending\n");
    got_sigtrap++;
}

char pad1[8192] __attribute__((section(".text"))) = {0};

int the_function(int x, int y)
{
    int i;

    printf("Start of the_function\n");
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    printf("End of the_function\n");
    return y;
}

char pad2[8192] __attribute__((section(".text"))) = {0};

int main(int argc, char **argv)
{
    struct sigaction act;

    printf("Registering signal handler\n");
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handle_sigtrap;
    act.sa_flags |= SA_SIGINFO;
    sigaction(SIGTRAP, &act, NULL);

    setup_breakpoint(&thebp, &the_function);

    printf("Before call got_sigtrap=%d\n", got_sigtrap);
    the_function(3, 42);
    printf("After call got_sigtrap=%d\n", got_sigtrap);

    /* do it again just to be sure */
    printf("Before call got_sigtrap=%d\n", got_sigtrap);
    the_function(7, 27);
    printf("After call got_sigtrap=%d\n", got_sigtrap);

    return 0;
}
