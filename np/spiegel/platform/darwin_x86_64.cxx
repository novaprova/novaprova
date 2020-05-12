/*
 * Copyright 2014-2020 David Arnold, Greg Banks
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
#include "common.hxx"
#include "np/util/trace.h"
#include "np/util/log.hxx"
#include "np/util/valgrind.h"

#include <signal.h>
#include <sys/signal.h>
#include <ucontext.h>
#include <memory.h>

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif

namespace np {
    namespace spiegel {
	namespace platform {
using namespace std;
using namespace np::util;

/*
 * Darwin's setcontext() call is buggy.  Unless built with -DDEBUG, it
 * makes no attempt at all to restore any of the 6 argument registers.
 * Even with -DDEBUG, it fails to restore %rdx (the 3rd argument).
 * So we have to implement our own.
 */
extern "C" __attribute__((noreturn)) void _np_setcontext(void *);
static int
setcontext(ucontext_t *uc)
{
    sigsetmask(uc->uc_sigmask);
    _np_setcontext(&uc->__mcontext_data);
    return -1;
}

static unsigned long intercept_tramp(void);
extern "C" unsigned long __np_intercept_after(unsigned long, struct frame &);
extern "C" void __np_intercept_tramp0(void);
extern "C" void __np_intercept_tramp1(void);

#define offsetofp(ptype, fld)	    ((size_t)&(((ptype)0)->fld))

/* The x86_6 ABI passes the first 6 arguments in registers.  Here we
 * map the argument index to an offset into the mcontext_t structure */
static unsigned int abi_arg_offsets[6] = {
    (unsigned int)offsetofp(mcontext_t, __ss.__rdi),
    (unsigned int)offsetofp(mcontext_t, __ss.__rsi),
    (unsigned int)offsetofp(mcontext_t, __ss.__rdx),
    (unsigned int)offsetofp(mcontext_t, __ss.__rcx),
    (unsigned int)offsetofp(mcontext_t, __ss.__r8),
    (unsigned int)offsetofp(mcontext_t, __ss.__r9)
};

class x86_64_darwin_call_t : public np::spiegel::call_t
{
public:
    x86_64_darwin_call_t() {}
private:

    mcontext_t mcontext_;
    unsigned long *stack_;

    unsigned long *argp(unsigned int i) const
    {
	return (i < 6 ? (unsigned long *)((char *)mcontext_ + abi_arg_offsets[i]) : &stack_[i-6]);
    }

    unsigned long get_arg(unsigned int i) const
    {
	return *argp(i);
    }
    void set_arg(unsigned int i, unsigned long v)
    {
	*argp(i) = v;
    }

    friend unsigned long intercept_tramp(void);
    friend unsigned long __np_intercept_after(unsigned long, struct frame &);
};

#define INSN_PUSH_RBP	    0x55
#define INSN_INT3	    0xcc
#define INSN_HLT	    0xf4

static ucontext_t tramp_uc;
static np::spiegel::platform::intstate_t *tramp_intstate;
static bool using_int3 = false;
static const uint8_t int3_trap_bytes[] = {INSN_INT3};
static const uint8_t hlt_trap_bytes[] = {INSN_HLT};
#define trap_bytes      (using_int3 ? int3_trap_bytes : hlt_trap_bytes)
#define trap_len        (using_int3 ? sizeof(int3_trap_bytes) : sizeof(hlt_trap_bytes))

/*
 * We put all the local variables for the tramp into a struct so
 * that we can predict their relative locations on the stack.  This
 * is important because we are going to be calling the original
 * function with %rsp pointing at the stack[] field, and anything
 * which the compiler might decide to put below that on the stack is
 * going to be clobbered.  Also, for Darwin/clang we need to access
 * this stack frame from a separate function, __np_intercept_after().
 */
struct frame
{
    /* fake stack frame for the original function
     *
     *  [0] either the saved RBP (for intercept type PUSHRBP only, the
     *      result of the simulated push %rbp instruction), or an unused
     *      slot to preserve the ABI-mandated 16B stack alignment.
     *  [1] return address
     *  <arg0..arg5 passed in registers>
     *  [2] arg6
     *  [3] arg7...
     */
    unsigned long stack[28];
    /* any data that we need to keep safe past the call to the
     * original function, goes here */
    x86_64_darwin_call_t call;
    addr_t addr;
    unsigned long our_rsp;
};


static unsigned long intercept_tramp(void)
{
    struct frame frame __attribute__((aligned(0x10)));
    unsigned long parent_size;
    unsigned long *new_rsp;

    /* address of the breakpoint insn */
    frame.addr = tramp_uc.__mcontext_data.__ss.__rip - (using_int3 ? 1 : 0);

#if _NP_ENABLE_TRACE
    {
	unsigned long rsp;
	unsigned long rbp;
	__asm__ volatile("movq %%rsp, %0" : "=r"(rsp));
	__asm__ volatile("movq %%rbp, %0" : "=r"(rbp));
	np_trace("intercept_tramp: started frame=");
	np_trace_hex((unsigned long)&frame.stack[0]);
	np_trace(" %rsp=");
	np_trace_hex(rsp);
	np_trace(" %rbp=");
	np_trace_hex(rbp);
	np_trace("\n");
    }
#endif

    /* Point the RBP register at our own RBP register */
    __asm__ volatile("movq %%rbp, %0" : "=m"(tramp_uc.__mcontext_data.__ss.__rbp));

    np_trace("intercept_tramp: building fake stack frame\n");

    /*
     * Build a new fake stack frame for calling the original
     * function.
     *
     * GDB implements breakpoints by putting back the original
     * insn, arranging to single-step over it, and then re-inserting
     * the breakpoint insn.  This technique turns out to be a whole
     * lot harder to do in the same process than it is from a trace
     * parent process like GDB.
     *
     * The Linux i386 setcontext() calls a system call to set the
     * sigmask but implements all the register switching in
     * userspace and doesn't reload the EFLAGS register (which is how
     * single-stepping is enabled).  It probably could do so
     * if it used the IRET insn instead of RET, but it doesn't.  So
     * we can't use it to switch to single-step mode. We could write
     * our own, less broken, setcontext() which can enable single-
     * stepping, but we run into two further problems.
     *
     *  - single-stepping implies a second SIGTRAP and more context
     *    switches, which is expensive.
     *
     *  - more significantly, Valgrind doesn't simulate single-stepping.
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
    new_rsp = &frame.stack[2];
    switch (tramp_intstate->type_)
    {
    case intstate_t::PUSHBP:
	np_trace("intercept_tramp: simulating pushbp\n");
	/* simulate the callq insn pushing the return address */
	*(--new_rsp) = (unsigned long)__np_intercept_tramp1;
	/* simulate the push %rbp insn which the breakpoint replaced */
	*(--new_rsp) = (unsigned long)tramp_uc.__mcontext_data.__ss.__rbp;
	/* setup to start executing the insn after the breakpoint */
	tramp_uc.__mcontext_data.__ss.__rip = frame.addr + 1;
	break;

    case intstate_t::OTHER:
	np_trace("intercept_tramp: replacing original instruction\n");
	/* simulate the callq insn pushing the return address */
	*(--new_rsp) = (unsigned long)__np_intercept_tramp0;
	/* replace the breakpoint with the original insn */
	*(unsigned char *)frame.addr = tramp_intstate->orig_;
	VALGRIND_DISCARD_TRANSLATIONS(frame.addr, 1);
	/* setup to start executing it again */
	tramp_uc.__mcontext_data.__ss.__rip = frame.addr;
	break;

    case intstate_t::UNKNOWN:
	break;
    }

    /*
     * We setup the ucontext so the called function will return to the
     * start of the __np_intercept_after() function.  On Linux/gcc we
     * can get away with having that code at the end of this function
     * with a label, but on Darwin/clang the compiler seems to insist on
     * inserting instructions which trash %rax between the label and the
     * first __asm__ statement following it, which trashes the return
     * value from the original function.  So we return to some handcrafted
     * trampoline code, or rather one of two trampolines depending on
     * whether the simulated push %rbp insn needs to be undone.
     */

    /*
     * Copy enough of the original stack frame to make it look like
     * we have the original arguments from the seventh onwards.
     */
    np_trace("intercept_tramp: setting up new input area ");
    np_trace_hex((unsigned long)&frame.stack[2]);
    np_trace("\n");
    parent_size = tramp_uc.__mcontext_data.__ss.__rbp -
		  (tramp_uc.__mcontext_data.__ss.__rsp+8);
    memcpy(&frame.stack[2],
	   (void *)(tramp_uc.__mcontext_data.__ss.__rsp+8),
	   MIN(parent_size, sizeof(frame.stack)-2*sizeof(unsigned long)));

    /* setup the ucontext's RSP register to point at the new stack frame */
    np_trace("intercept_tramp: setting up new %rsp ");
    np_trace_hex((unsigned long)new_rsp);
    np_trace("\n");
    tramp_uc.__mcontext_data.__ss.__rsp = (unsigned long)new_rsp;

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
    np_trace("intercept_tramp: dispatching before method\n");
    frame.call.stack_ = &frame.stack[2];
    frame.call.mcontext_ = &tramp_uc.__mcontext_data;
    intercept_t::dispatch_before(frame.addr, frame.call);
    if (frame.call.skip_)
    {
	np_trace("intercept_tramp: skipping call to real function\n");
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
	};
	return frame.call.retval_;	/* before() requested skip() */
    }
    if (frame.call.redirect_)
    {
	/* before() requested redirect, so setup the context to call
	 * that function instead. */
	np_trace("intercept_tramp: setting up to redirect to another function\n");
	tramp_uc.__mcontext_data.__ss.__rip = frame.call.redirect_;
	switch (tramp_intstate->type_)
	{
	case intstate_t::PUSHBP:
	    /* The new function won't be intercepted (well, we hope not)
	     * so we need to undo emulation of 'push %rbp'.  */
	    tramp_uc.__mcontext_data.__ss.__rsp += 8;
	    break;
	case intstate_t::OTHER:
	    /* Re-insert the breakpoint */
            text_write(frame.addr, trap_bytes, trap_len);
	    break;
	case intstate_t::UNKNOWN:
	    break;
	};
    }
    /* Save our own %rsp and %rbp for later */
    __asm__ volatile("movq %%rsp, %0" : "=m"(frame.our_rsp));

    /* switch to the ucontext */
    np_trace("intercept_tramp: about to setcontext rip=");
    np_trace_hex(tramp_uc.__mcontext_data.__ss.__rip);
    np_trace(" rsp=");
    np_trace_hex(tramp_uc.__mcontext_data.__ss.__rsp);
    np_trace(" rbp=");
    np_trace_hex(tramp_uc.__mcontext_data.__ss.__rbp);
    np_trace("\n");

    setcontext(&tramp_uc);
    /* notreached - setcontext() should not return under any
     * circumstances on Darwin */
    eprintf("setcontext() somehow returned: %s\n", strerror(errno));
    exit(1);
    /* not reached */
    return 0;
}

/* This function does nothing except provide a name visible to DWARF
 * to appear in any stack trace that includes a return address
 * which points at the actual code at __np_intercept_tramp1. */
static void __np_intercept_tramp1z(void) __attribute__((used));
static void __np_intercept_tramp1z(void)
{
/*
 * The original function returns here with this state:
 *
 *   %rax   return value from original function
 *   %rsp   points to frame.stackp[2] in intercept_tramp()
 *	    above us on the stack
 *   %rbp   same as intercept_tramp()'s %rbp
 *
 * We do the least possible work to set up a call to the
 * __np_intercept_after() routine with
 *
 *   %rdi   arg0, the return value from the original function
 *   %rsi   arg1, a reference to intercept_tramp()s struct frame
 *
 * and then return to intercept_tramp()'s caller.
 */
__asm__ (
/* 5 nops are the shape of a CALL instruction
 * to ensure the function's entry symbol is far
 * back enough from where the RA will point. */
"nop; nop; nop; nop; nop;\n"
"___np_intercept_tramp1:\n"
"movq %rax,%rdi;\n"
"movq %rsp,%rsi;\n"
"subq $16,%rsi;\n"
"subq $32,%rsp;\n"
"callq ___np_intercept_after;\n"
"movq %rbp,%rsp;\n"
"popq %rbp;\n"
"retq;\n"
);
}

/* This function does nothing except provide a name visible to DWARF
 * to appear in any stack trace that includes a return address
 * which points at the actual code at __np_intercept_tramp0. */
static void __np_intercept_tramp0z(void) __attribute__((used));
static void __np_intercept_tramp0z(void)
{
/* This is the same as __np_intercept_tramp1 but handles the case where
 * %rsp points at frame.stack[1] instead because we didn't simulate a
 * pushbp instruction in intercept_tramp()
 */
__asm__ (
/* 5 nops are the shape of a CALL instruction
 * to ensure the function's entry symbol is far
 * back enough from where the RA will point. */
"nop; nop; nop; nop; nop;\n"
"___np_intercept_tramp0:\n"
"movq %rax,%rdi;\n"
"movq %rsp,%rsi;\n"
"subq $8,%rsi;\n"
"subq $24,%rsp;\n"
"callq ___np_intercept_after;\n"
"movq %rbp,%rsp;\n"
"popq %rbp;\n"
"retq;\n"
);
}


extern "C" unsigned long __np_intercept_after(unsigned long rax, struct frame &frame)
{
#if _NP_ENABLE_TRACE
    {
	unsigned long rsp;
	unsigned long rbp;
	__asm__ volatile("movq %%rsp, %0" : "=r"(rsp));
	__asm__ volatile("movq %%rbp, %0" : "=r"(rbp));
	np_trace("__np_intercept_after: started rax=");
	np_trace_hex(rax);
	np_trace(" frame=");
	np_trace_hex((unsigned long)&frame.stack[0]);
	np_trace(" %rsp=");
	np_trace_hex(rsp);
	np_trace(" %rbp=");
	np_trace_hex(rbp);
	np_trace("\n");
    }
#endif

    /* return value is in RAX, save it directly into the call_t */
    frame.call.retval_ = rax;

    /*
     * The compiler will emit (at least when unoptimised) instructions
     * in our epilogue that save %eax onto a temporary location on stack
     * relative to %rbp and then immediately load it again from there.
     * It's silly, but there you go.  After returning from the original
     * function our %rsp is pointing at @frame, and so that temporary
     * location is now below %rsp, i.e. outside the stack frame.  Of
     * course that's fine, no function care about that location, and we
     * just store and load from there.  Unfortunately Valgrind detects
     * such loads and stores below the stack pointer and warns about
     * them.  To avoid those two warnings we first restore the %rsp we
     * had before calling the original function.
     */
//    __asm__ volatile("movq %0, %%rsp" : : "m"(frame.our_rsp));


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
    };

    /*
     * Call the AFTER method.  The method can examine and modify the
     * return value from the original function using get_retval() and
     * set_retval().  It should also be able to longjmp() out without
     * drama, e.g. as a side effect of failing a NP_ASSERT().
     */
    np_trace("intercept_after: dispatch after method\n");
    intercept_t::dispatch_after(frame.addr, frame.call);
    np_trace("intercept_after: returning\n");
    return frame.call.retval_;
}

static void
handle_signal(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

    np_trace("handle_signal: starting, ucontext %rip=");
    np_trace_hex(uc->uc_mcontext->__ss.__rip);
    np_trace(" %rbp=");
    np_trace_hex(uc->uc_mcontext->__ss.__rbp);
    np_trace(" %rsp=");
    np_trace_hex(uc->uc_mcontext->__ss.__rsp);
    np_trace("\n");

    /* double-check that this is not some spurious signal */
    unsigned char *rip = (unsigned char *)uc->uc_mcontext->__ss.__rip;

    if (using_int3)
    {
	if (sig != SIGTRAP || si->si_signo != SIGTRAP)
	    return;	    /* we got a bogus signal, wtf? */
	/* Darwin seems to set neither si_code nor si_pid to anything useful?? */
	rip--;
	if (*rip != INSN_INT3)
	    goto wtf;	    /* not an INT3 */
    }
    else
    {
	if (sig != SIGSEGV || si->si_signo != SIGSEGV)
	    return;	    /* we got a bogus signal, wtf? */
	if (*rip != INSN_HLT)
	    goto wtf;	    /* not an HLT */
    }
    if (si->si_pid != 0)
	return;	    /* some process sent us SIGSEGV, wtf? */
    tramp_intstate = intercept_t::get_intstate((np::spiegel::addr_t)rip);
    if (!tramp_intstate)
	goto wtf;   /* not an installed intercept */
    np_trace("handle_signal: is installed\n");

    /* Stash the ucontext for the tramp.
     *
     * Note that in Darwin this is a little more complex than in Linux.
     * In Linux the standard-defined uc_mcontext field is an embedded
     * structure, so we just copy the whole ucontext_t.  In Darwin
     * uc_mcontext is a pointer, which can point to either the embedded
     * __mcontext_data or to a completely separate structure elsewhere.
     * It seems that the former happens when you build a ucontext_t from
     * whole cloth like getcontext() does, but when you get a ucontext_t
     * as the 3rd argument to a signal handler, the actual machine state
     * is stored further on up the signal stack, and the embedded
     * __mcontext_data is either empty or random bytes.
     */
    /* TODO: if we were concerned about the MP case we would
     * push build a stack frame containing this context */
    tramp_uc = *uc;
    tramp_uc.__mcontext_data = *uc->uc_mcontext;
    tramp_uc.uc_mcontext = &tramp_uc.__mcontext_data;

    /* munge the signal ucontext so we return from
     * the signal into the tramp instead of the
     * original function */
    uc->uc_mcontext->__ss.__rip = (unsigned long)&intercept_tramp;
    np_trace("handle_signal: returning to intercept_tramp\n");

    return;

wtf:
    np_trace("handle_signal: unexpected SEGV, unregistering handler\n");
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
    case INSN_PUSH_RBP:
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


	};
    };
};
