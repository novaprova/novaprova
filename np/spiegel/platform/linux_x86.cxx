/*
 * Copyright 2011-2021 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "np/spiegel/common.hxx"
#include "np/spiegel/intercept.hxx"
#include "np/util/log.hxx"
#include "common.hxx"

#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include "np/util/valgrind.h"

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

static unsigned long intercept_tramp(void);

class x86_linux_call_t : public np::spiegel::call_t
{
private:
    x86_linux_call_t() {}

    unsigned long *args_;
    unsigned long returned_edx_;

    unsigned long get_arg(unsigned int i) const
    {
	return args_[i];
    }
    void set_arg(unsigned int i, unsigned long v)
    {
	args_[i] = v;
    }
    uint64_t get_retval64() const
    {
	return (((uint64_t)returned_edx_)<<32) | retval_;
    }

    friend unsigned long intercept_tramp(void);
};

#define INSN_PUSH_EBP	    0x55
#define INSN_INT3	    0xcc
#define INSN_HLT	    0xf4

static ucontext_t tramp_uc;
static ucontext_t fpuc;
static np::spiegel::platform::intstate_t *tramp_intstate;
static bool hack1 = false;
static bool using_int3 = false;
static const uint8_t int3_trap_bytes[] = {INSN_INT3};
static const uint8_t hlt_trap_bytes[] = {INSN_HLT};
#define trap_bytes      (using_int3 ? int3_trap_bytes : hlt_trap_bytes)
#define trap_len        (using_int3 ? sizeof(int3_trap_bytes) : sizeof(hlt_trap_bytes))

static unsigned long
intercept_tramp(void)
{
    /*
     * We put all the local variables for the tramp into a struct so
     * that we can predict their relative locations on the stack.  This
     * is important because we are going to be calling the original
     * function with %esp pointing at the stack[] field, and anything
     * which the compiler might decide to put below that on the stack is
     * going to be clobbered.
     */
    struct
    {
	/* fake stack frame for the original function
	 *
	 *  - saved EBP (for intercept type PUSHBP only, the result of
	 *		 the simulated push %rbp instruction).
	 *  - return address
	 *  - arg0
	 *  - arg1
	 *  - arg2 ...
	 */
	unsigned long stack[36];
	/* any data that we need to keep safe past the call to the
	 * original function, goes here */
	x86_linux_call_t call;
	addr_t addr;
	unsigned long our_esp;
    } frame;
    unsigned long parent_size;
    int nstack = 0;

    /* address of the breakpoint insn */
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
        eprintf("Failed to call getcontext(): %s\n", strerror(errno));
	exit(1);
    }
    tramp_uc.uc_mcontext.fpregs = fpuc.uc_mcontext.fpregs;
    /* Point the EBP register at our own EBP register */
    tramp_uc.uc_mcontext.gregs[REG_EBP] = fpuc.uc_mcontext.gregs[REG_EBP];

//     printf("tramp: fpregs=%p\n", (void *)tramp_uc.uc_mcontext.fpregs);
//     printf("tramp: after=%p\n", (void *)&&after);

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
     * So we take one of two approaches:
     *
     *  - Many functions have a first instruction that is only one
     *    byte long, the same size as the breakpoint instruction.
     *    For x86 this applies to almost all non-leaf functions
     *    and the insn is 0x55 push %ebp.  For x86_64 the situation
     *    is more complex, but that insn (called push %rbp) is common.
     *    Functions like this are trivial to detect and the effect
     *    of the first insn can be trivially simulated here in the
     *    tramp by adjusting the newly constructed stack frame before
     *    calling the function.
     *
     *  - In the more complex case, we take advantage of the fact that
     *    our intercepts are function based not insn based like GDB's
     *    breakpoints.  So we can replace the original insn, call the
     *    function and wait for it to return, then re-insert the
     *    breakpoint.  This is efficient, but is not thread safe,
     *    and doesn't do the right thing if the function is recursive
     *    or if it longjmps out.
     *
     * "Trust me, I know what I'm doing."
     */
    switch (tramp_intstate->type_)
    {
    case intstate_t::PUSHBP:
	/* simulate the push %ebp insn which the breakpoint replaced */
	frame.stack[nstack++] = (unsigned long)tramp_uc.uc_mcontext.gregs[REG_EBP];
	/* setup to start executing the insn after the breakpoint */
	tramp_uc.uc_mcontext.gregs[REG_EIP] = frame.addr + 1;
	break;

    case intstate_t::OTHER:
	/* replace the breakpoint with the original insn */
        text_write(frame.addr, &tramp_intstate->orig_, trap_len);
	/* setup to start executing it again */
	tramp_uc.uc_mcontext.gregs[REG_EIP] = frame.addr;
	break;

    case intstate_t::UNKNOWN:
	break;
    }

    /* Setup the ucontext to look like we just called the
     * function from immediately before the 'after' label */
    frame.stack[nstack++] = (unsigned long)&&after; /* return address */

    /*
     * Copy enough of the original stack frame to make it look like
     * we have the original arguments.
     */
    parent_size = tramp_uc.uc_mcontext.gregs[REG_EBP] -
		  (tramp_uc.uc_mcontext.gregs[REG_ESP]+4);
    memcpy(&frame.stack[nstack],
	   (void *)(tramp_uc.uc_mcontext.gregs[REG_ESP]+4),
	   MIN(parent_size, sizeof(frame.stack)-nstack*sizeof(unsigned long)));
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
     * drama, e.g. as a side effect of failing a NP_ASSERT().
     */
    frame.call.args_ = frame.stack+nstack;
    intercept_t::dispatch_before(frame.addr, frame.call);
    if (frame.call.skip_)
    {
	switch (tramp_intstate->type_)
	{
	case intstate_t::PUSHBP:
	    break;
	case intstate_t::OTHER:
	    /* Re-insert the breakpoint */
            text_write(frame.addr, trap_bytes, trap_len);
	    break;
	case intstate_t::UNKNOWN:
	    break;
	}
	return frame.call.retval_;	/* before() requested skip() */
    }
    if (frame.call.redirect_)
    {
	/* before() requested redirect, so setup the context to call
	 * that function instead. */
	tramp_uc.uc_mcontext.gregs[REG_EIP] = frame.call.redirect_;
	switch (tramp_intstate->type_)
	{
	case intstate_t::PUSHBP:
	    /* The new function won't be intercepted (well, we hope not)
	     * so we need to undo emulation of 'push %ebp'.  */
	    tramp_uc.uc_mcontext.gregs[REG_ESP] += 4;
	    break;
	case intstate_t::OTHER:
	    /* Re-insert the breakpoint */
            text_write(frame.addr, trap_bytes, trap_len);
	    break;
	case intstate_t::UNKNOWN:
	    break;
	}
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
    eprintf("setcontext() somehow returned: %s\n", strerror(errno));
    exit(1);

after:
    /* the original function returns here */

    /* return value is in EAX, save it directly into the call_t */
    __asm__ volatile("movl %%eax, %0" : "=m"(frame.call.retval_));
    /* when the function returns a 64b integral type the high
     * 32 bits are in EDX, save that now just in case */
    __asm__ volatile("movl %%edx, %0" : "=m"(frame.call.returned_edx_));

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

    switch (tramp_intstate->type_)
    {
    case intstate_t::PUSHBP:
	/* we're cool */
	break;
    case intstate_t::OTHER:
	/* Re-insert the breakpoint */
        text_write(frame.addr, trap_bytes, trap_len);
	break;
    case intstate_t::UNKNOWN:
	break;
    }

    /*
     * Call the AFTER method.  The method can examine and modify the
     * return value from the original function using get_retval() and
     * set_retval().  It should also be able to longjmp() out without
     * drama, e.g. as a side effect of failing a NP_ASSERT().
     */
    intercept_t::dispatch_after(frame.addr, frame.call);
    /* Restore the original function's EDX in case we're
     * returning a 64bit result */
    __asm__ volatile("movl %0, %%edx" : "=m"(frame.call.returned_edx_));
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
    unsigned char *eip;

    eip = (unsigned char *)(uc->uc_mcontext.gregs[REG_EIP]);
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
    tramp_intstate = intercept_t::get_intstate((np::spiegel::addr_t)eip);
    if (!tramp_intstate)
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
install_intercept(np::spiegel::addr_t addr, intstate_t &state)
{
    int r;

    if (!addr)
        return -1;

    switch (*(unsigned char *)addr)
    {
    case INSN_PUSH_EBP:
	/* non-leaf function, all is good */
	state.type_ = intstate_t::PUSHBP;
	break;
    case INSN_HLT:
    case INSN_INT3:
	/* already intercepted, can handle this too */
	break;
    default:
	state.type_ = intstate_t::OTHER;
	break;
    }
    /* TODO: trap_len uses using_int3 before its correctly set,
     * but this doesn't matter as trap_len=1 always */
    memcpy((void *)&state.orig_, (const void *)addr, trap_len);

    static bool installed_sigaction = false;
    if (!installed_sigaction)
    {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = handle_signal;
	act.sa_flags |= SA_SIGINFO;
	if (RUNNING_ON_VALGRIND)
	    using_int3 = true;
	r = sigaction((using_int3 ? SIGTRAP : SIGSEGV), &act, NULL);
	if (r < 0)
	{
	    eprintf("Failed to call sigaction(): %s\n", strerror(errno));
	    return -1;
	}
	installed_sigaction = true;
    }

    /* TODO: install the sig handler only when there are
     * any installed intercepts, or the pid has changed */
    r = text_write(addr, trap_bytes, trap_len);
    if (r)
    {
        eprintf("Cannot write trap bytes to text address 0x%lx", addr);
        return r;
    }

    return 0;
}

int
uninstall_intercept(np::spiegel::addr_t addr, intstate_t &state)
{
    if (!addr)
        return -1;

    if (*(unsigned char *)addr != trap_bytes[0])
    {
        eprintf("Intercept not installed at 0x%lx", addr);
	return -1;
    }

    int r = text_write(addr, &state.orig_, trap_len);
    if (r)
        eprintf("Failed to uninstall intercept at 0x%lx", addr);

    return r;
}

bool
is_intercept_installed(
    np::spiegel::addr_t addr,
    const intstate_t &state __attribute__((unused)))
{
    if (!addr)
        return false;
    return (*(unsigned char *)addr == trap_bytes[0]);
}

// close namespaces
}; }; };

