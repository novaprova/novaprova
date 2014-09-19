/* This changes the definition of ucontext_t */
#define _XOPEN_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

#define offsetof(type, fld)	((unsigned long)&((type *)0)->fld)

static void
handle_signal(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

    printf("handle_signal\n");
    printf("    sig=%d\n", sig);
    printf("    si=%p\n", si);
    printf("	    signo=%d\n", si->si_signo);
    printf("	    errno=%d\n", si->si_errno);
    printf("	    code=%d\n", si->si_code);
    printf("	    pid=%d\n", si->si_pid);
    printf("	    uid=%d\n", si->si_uid);
    printf("	    status=%d\n", si->si_status);
    printf("	    addr=%p\n", si->si_addr);
    printf("	    band=%ld\n", si->si_band);
    printf("    uc=%p\n", uc);
    printf("	    sizeof(*uc)=0x%lx\n", sizeof(*uc));
    printf("	    offsetof(*uc, __mcontext_data)=0x%lx\n", offsetof(ucontext_t, __mcontext_data));
    printf("	    sizeof(uc->__mcontext_data)=0x%lx\n", sizeof(uc->__mcontext_data));
    printf("	    RAX 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rax);
    printf("	    RBX 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rbx);
    printf("	    RCX 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rcx);
    printf("	    RDX 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rdx);
    printf("	    RDI 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rdi);
    printf("	    RSI 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rsi);
    printf("	    RBP 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rbp);
    printf("	    RSP 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rsp);
    printf("	    RIP 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rip);
    printf("	    R8 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r8);
    printf("	    R9 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r9);
    printf("	    R10 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r10);
    printf("	    R11 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r11);
    printf("	    R12 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r12);
    printf("	    R13 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r13);
    printf("	    R14 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r14);
    printf("	    R15 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__r15);
    printf("	    RFLAGS 0x%016llx\n", (unsigned long long)uc->uc_mcontext->__ss.__rflags);

    printf("Setting up to return past the HLT\n");
    fflush(stdout);

    uc->uc_mcontext->__ss.__rip += 1;
}

int this_function_halts(void)
{
    __asm__ volatile("hlt");
    return 42;
}

int main(int argc, char **argv)
{
    int r;
    struct sigaction act;
    bool using_int3 = false;

    printf("top of stack is near %p\n", &act);

    printf("Setting up signal handler\n");
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = handle_signal;
    act.sa_flags |= SA_SIGINFO;
//    if (RUNNING_ON_VALGRIND)
//	using_int3 = true;
    r = sigaction((using_int3 ? SIGTRAP : SIGSEGV), &act, NULL);

    printf("Calling function with a HLT insn in it\n");
    fflush(stdout);
    r = 0;
    r = this_function_halts();
    printf("Function returned %d\n", r);

    return 0;
}
