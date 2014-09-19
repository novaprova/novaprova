/* This changes the definition of ucontext_t */
#define _XOPEN_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <ucontext.h>

extern __attribute__((noreturn)) void fixed__setcontext(void *);

int
fixed_setcontext(ucontext_t *uc)
{
    fixed__setcontext(&uc->__mcontext_data);
    return -1;
}

static void print_and_die(
    uint64_t a1,
    uint64_t a2,
    uint64_t a3,
    uint64_t a4,
    uint64_t a5,
    uint64_t a6,
    uint64_t a7,
    uint64_t a8,
    uint64_t a9,
    uint64_t a10,
    uint64_t a11,
    uint64_t a12)
{
    printf("print_and_die: a1=0x%016llx\n", a1);
    printf("print_and_die: a2=0x%016llx\n", a2);
    printf("print_and_die: a3=0x%016llx\n", a3);
    printf("print_and_die: a4=0x%016llx\n", a4);
    printf("print_and_die: a5=0x%016llx\n", a5);
    printf("print_and_die: a6=0x%016llx\n", a6);
    printf("print_and_die: a7=0x%016llx\n", a7);
    printf("print_and_die: a8=0x%016llx\n", a8);
    printf("print_and_die: a9=0x%016llx\n", a9);
    printf("print_and_die: a10=0x%016llx\n", a10);
    printf("print_and_die: a11=0x%016llx\n", a11);
    printf("print_and_die: a12=0x%016llx\n", a12);
    exit(0);
}

static ucontext_t uc;

int main(int argc, char **argv)
{
    int r;

    r = getcontext(&uc);
    if (r < 0)
    {
	perror("getcontext");
	exit(1);
    }

    printf("main: &uc=%p\n", &uc);

    /*
     * Woops, it seems that Darwin's setcontext() doesn't restore
     * the argument registers unless built with -DDEBUG.  See
     * http://opensource.apple.com/source/Libc/Libc-825.40.1/x86_64/gen/_setcontext.S
     * No wonder they deprecated it; it's broken!!
     */
    uc.uc_mcontext->__ss.__rdi = 0xa1a1a1a1a1a1a1a1LL;
    uc.uc_mcontext->__ss.__rsi = 0xa2a2a2a2a2a2a2a2LL;
    uc.uc_mcontext->__ss.__rdx = 0xa3a3a3a3a3a3a3a3LL;
    uc.uc_mcontext->__ss.__rcx = 0xa4a4a4a4a4a4a4a4LL;
    uc.uc_mcontext->__ss.__r8 = 0xa5a5a5a5a5a5a5a5LL;
    uc.uc_mcontext->__ss.__r9 = 0xa6a6a6a6a6a6a6a6LL;
    uint64_t more_stack[7];
    more_stack[0] = 0xf0f0f0f0f0f0f0f0LL;	/* fake return address */
    more_stack[1] = 0xa7a7a7a7a7a7a7a7LL;	/* fake 7th arg */
    more_stack[2] = 0xa8a8a8a8a8a8a8a8LL;
    more_stack[3] = 0xa9a9a9a9a9a9a9a9LL;
    more_stack[4] = 0xaaaaaaaaaaaaaaaaLL;
    more_stack[5] = 0xababababababababLL;
    more_stack[6] = 0xacacacacacacacacLL;
    uc.uc_mcontext->__ss.__rsp = (unsigned long)&more_stack[0];
    uc.uc_mcontext->__ss.__rip = (unsigned long)&print_and_die;

    printf("main: about to call setcontext()\n");
    fixed_setcontext(&uc);
    perror("setcontext");

    return 0;
}
