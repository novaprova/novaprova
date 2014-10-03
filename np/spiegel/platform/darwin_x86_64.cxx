/*
 * Copyright 2014 David Arnold, Greg Banks
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

#include <signal.h>
#include <sys/signal.h>
#include <ucontext.h>
#include <memory.h>

#if HAVE_VALGRIND
#include <valgrind/valgrind.h>
#else
#define RUNNING_ON_VALGRIND 0
#define VALGRIND_DISCARD_TRANSLATIONS(a, l)
#endif

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

#define offsetofp(ptype, fld)	    ((size_t)&(((ptype)0)->fld))

/* The x86_6 ABI passes the first 6 arguments in registers.  Here we
 * map the argument index to an offset into the mcontext_t structure */
static int abi_arg_offsets[6] = {
    offsetofp(mcontext_t, __ss.__rdi),
    offsetofp(mcontext_t, __ss.__rsi),
    offsetofp(mcontext_t, __ss.__rdx),
    offsetofp(mcontext_t, __ss.__rcx),
    offsetofp(mcontext_t, __ss.__r8),
    offsetofp(mcontext_t, __ss.__r9)
};

class x86_64_darwin_call_t : public np::spiegel::call_t
{
private:
    x86_64_darwin_call_t() {}

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
};

#define INSN_PUSH_RBP	    0x55
#define INSN_INT3	    0xcc
#define INSN_HLT	    0xf4

static ucontext_t tramp_uc;
static np::spiegel::platform::intstate_t *tramp_intstate;
static bool hack1 = false;
static bool using_int3 = false;

static unsigned long
intercept_tramp(void)
{
    /*
     * We put all the local variables for the tramp into a struct so
     * that we can predict their relative locations on the stack.  This
     * is important because we are going to be calling the original
     * function with %rsp pointing at the stack[] field, and anything
     * which the compiler might decide to put below that on the stack is
     * going to be clobbered.
     */
    struct
    {
	/* fake stack frame for the original function
	 *
	 *  - saved RBP (for intercept type PUSHRBP only, the result of
	 *		 the simulated push %rbp instruction).
	 *  - return address
	 *  <arg0..arg5 passed in registers>
	 *  - arg6
	 *  - arg7...
	 */
	unsigned long stack[28];
	/* any data that we need to keep safe past the call to the
	 * original function, goes here */
	x86_64_darwin_call_t call;
	addr_t addr;
	unsigned long our_rsp;
    } frame;
    unsigned long parent_size;
    int nstack = 0;

    /* address of the breakpoint insn */
    frame.addr = tramp_uc.__mcontext_data.__ss.__rip - (using_int3 ? 1 : 0);

//    printf("intercept_tramp: found saved tramp_uc RIP=0x%016lx RSP=0x%016lx RBP=0x%016lx\n",
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rip,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rsp,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rbp);

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

    /* Point the RBP register at our own RBP register */
    __asm__ volatile("movq %%rbp, %0" : "=m"(tramp_uc.__mcontext_data.__ss.__rbp));

//     printf("tramp: fpregs=%p\n", (void *)tramp_uc.uc_mcontext.fpregs);
//     printf("tramp: after=%p\n", (void *)&&after);

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
    switch (tramp_intstate->type_)
    {
    case intstate_t::PUSHBP:
	/* simulate the push %rbp insn which the breakpoint replaced */
	frame.stack[nstack++] = (unsigned long)tramp_uc.__mcontext_data.__ss.__rbp;
	/* setup to start executing the insn after the breakpoint */
	tramp_uc.__mcontext_data.__ss.__rip = frame.addr + 1;
	break;

    case intstate_t::OTHER:
	/* replace the breakpoint with the original insn */
	*(unsigned char *)frame.addr = tramp_intstate->orig_;
	VALGRIND_DISCARD_TRANSLATIONS(frame.addr, 1);
	/* setup to start executing it again */
	tramp_uc.__mcontext_data.__ss.__rip = frame.addr;
	break;

    case intstate_t::UNKNOWN:
	break;
    }

    /* Setup the ucontext to look like we just called the
     * function from immediately before the 'after' label */
    frame.stack[nstack++] = (unsigned long)&&after; /* return address */

    /*
     * Copy enough of the original stack frame to make it look like
     * we have the original arguments from the seventh onwards.
     */
    parent_size = tramp_uc.__mcontext_data.__ss.__rbp -
		  (tramp_uc.__mcontext_data.__ss.__rsp+8);
    memcpy(&frame.stack[nstack],
	   (void *)(tramp_uc.__mcontext_data.__ss.__rsp+8),
	   MIN(parent_size, sizeof(frame.stack)-nstack*sizeof(unsigned long)));
    /* setup the ucontext's RSP register to point at the new stack frame */
    tramp_uc.__mcontext_data.__ss.__rsp = (unsigned long)&frame;

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
    frame.call.stack_ = frame.stack+nstack;
    frame.call.mcontext_ = &tramp_uc.__mcontext_data;
    intercept_t::dispatch_before(frame.addr, frame.call);
    if (frame.call.skip_)
	return frame.call.retval_;	/* before() requested skip() */
    if (frame.call.redirect_)
    {
	/* before() requested redirect, so setup the context to call
	 * that function instead. */
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
	    *( char *)frame.addr = (using_int3 ? INSN_INT3 : INSN_HLT);
	    VALGRIND_DISCARD_TRANSLATIONS(frame.addr, 1);
	    break;
	case intstate_t::UNKNOWN:
	    break;
	};
    }
    /* Save our own %rsp for later */
    __asm__ volatile("movq %%rsp, %0" : "=m"(frame.our_rsp));

    /* switch to the ucontext */
//    printf("tramp: about to setcontext(RIP=0x%016lx RSP=0x%016lx RBP=0x%016lx)\n",
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rip,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rsp,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rbp);
    setcontext(&tramp_uc);
    /* notreached - setcontext() should not return under any
     * circumstances on Darwin */
    perror("setcontext");
    exit(1);

after:
    /* the original function returns here */

    /* return value is in RAX, save it directly into the call_t */
    __asm__ volatile("movq %%rax, %0" : "=m"(frame.call.retval_));

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
    __asm__ volatile("movq %0, %%rsp" : : "m"(frame.our_rsp));

    switch (tramp_intstate->type_)
    {
    case intstate_t::PUSHBP:
	/* we're cool */
	break;
    case intstate_t::OTHER:
	/* Re-insert the breakpoint */
	*(unsigned char *)frame.addr = (using_int3 ? INSN_INT3 : INSN_HLT);
	VALGRIND_DISCARD_TRANSLATIONS(frame.addr, 1);
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
    intercept_t::dispatch_after(frame.addr, frame.call);
    return frame.call.retval_;
}

static void
handle_signal(int sig, siginfo_t *si, void *vuc)
{
    ucontext_t *uc = (ucontext_t *)vuc;

//    printf("handle_signal: signo=%d code=%d pid=%d RIP 0x%016lx RBP 0x%016lx RSP 0x%016lx\n",
//	    si->si_signo,
//	    si->si_code,
//	    (int)si->si_pid,
//	    (unsigned long)uc->uc_mcontext->__ss.__rip,
//	    (unsigned long)uc->uc_mcontext->__ss.__rbp,
//	    (unsigned long)uc->uc_mcontext->__ss.__rsp);

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
//    printf("tramp: saved tramp_uc RIP=0x%016lx RSP=0x%016lx RBP=0x%016lx\n",
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rip,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rsp,
//	   (unsigned long)tramp_uc.__mcontext_data.__ss.__rbp);

    /* munge the signal ucontext so we return from
     * the signal into the tramp instead of the
     * original function */
    uc->uc_mcontext->__ss.__rip = (unsigned long)&intercept_tramp;

//     printf("handle_signal: ending\n");

    return;

wtf:
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    sigaction(sig, &act, NULL);
}


int
install_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)
{
    int r;
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
    state.orig_ = *(unsigned char *)addr;

    r = text_map_writable(addr, 1);
    if (r)
    {
	err = "cannot make text page writable";
	return -1;
    }

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
	if (r < 0)
	{
	    perror("np: sigaction");
	    err = "cannot install signal handler";
	    return -1;
	}
	installed_sigaction = true;
    }

    /* TODO: install the sig handler only when there are
     * any installed intercepts, or the pid has changed */
    *(unsigned char *)addr = (using_int3 ? INSN_INT3 : INSN_HLT);
    VALGRIND_DISCARD_TRANSLATIONS(addr, 1);

    return 0;
}

int
uninstall_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)
{
    if (*(unsigned char *)addr != (using_int3 ? INSN_INT3 : INSN_HLT))
    {
	err = "intercept not installed";
	return -1;
    }
    *(unsigned char *)addr = state.orig_;
    VALGRIND_DISCARD_TRANSLATIONS(addr, 1);
    int r = text_restore(addr, 1);
    if (r < 0)
	err = "cannot restore text page";
    return r;
}



	};
    };
};
