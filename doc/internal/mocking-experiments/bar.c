#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>

/*
 * Second experiment.
 *
 * Trying to see whether we can cause the CPU to single-step
 * through code in our own process, by setting the TF (Trace
 * Flag) in the EFLAGS register in the ucontext which gets
 * passed to the SIGTRAP handler.  The assumption is that
 * the kernel will take the new EFLAGS value when it context
 * switches back on signal return, and the TF will survive
 * that, and work when we resume, and be automatically
 * cleared in the signal handler (so we're not single stepping
 * through the signal handler, which would suck).
 */

static volatile int got_sigtrap = 0;

static void handle_sigtrap(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

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
    if (got_sigtrap < 5)
	uc->uc_mcontext.gregs[REG_EFL] |= 0x100;    /* TF Trace Flag */
    else
	uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;    /* TF Trace Flag */
    got_sigtrap++;
}

int the_function(int x, int y)
{
    int i;

    __asm__ volatile("int3;");
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    return y;
}

int main(int argc, char **argv)
{
    struct sigaction act;

    printf("Registering signal handler\n");
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handle_sigtrap;
    act.sa_flags |= SA_SIGINFO;
    sigaction(SIGTRAP, &act, NULL);

    printf("Before call got_sigtrap=%d\n", got_sigtrap);
    the_function(3, 42);
    printf("After call got_sigtrap=%d\n", got_sigtrap);

    return 0;
}
