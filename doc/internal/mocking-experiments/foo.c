#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>

/*
 * First experiment.
 *
 * Trying to see whether we an INT3 insn will generate SIGTRAP,
 * whether we can handle that signal, and whether we can get
 * useful information out of the ucontext argument to the
 * signal handler.
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
    if (si->si_signo != SIGTRAP ||
	si->si_code != SI_KERNEL)
	return;
    printf("handle_sigtrap: faulted at EIP 0x%08lx ESP 0x%08lx\n",
	    (unsigned long)uc->uc_mcontext.gregs[REG_EIP],
	    (unsigned long)uc->uc_mcontext.gregs[REG_ESP]);
    got_sigtrap++;
}

void the_function(void)
{
    printf("At start of the_function\n");
woops: __asm__ volatile("int3");
    printf("At end of the_function\n");
    printf("The faulting address should have been 0x%08lx\n", (unsigned long)&&woops);
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
    the_function();
    printf("After call got_sigtrap=%d\n", got_sigtrap);

    return 0;
}
