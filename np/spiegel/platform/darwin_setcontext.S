/*
 * Copyright (c) 2014-2020 Greg Banks.  Derived from
 * http://opensource.apple.com/source/Libc/Libc-825.40.1/x86_64/gen/_setcontext.S
 * which bore the following message
 *
 * Copyright (c) 2007,2009 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#if defined(__x86_64__)

#include <architecture/i386/asm_help.h>

#define MCONTEXT_SS_RAX     16
#define MCONTEXT_SS_RBX     24
#define MCONTEXT_SS_RCX     32
#define MCONTEXT_SS_RDX     40
#define MCONTEXT_SS_RDI     48
#define MCONTEXT_SS_RSI     56
#define MCONTEXT_SS_RBP     64
#define MCONTEXT_SS_RSP     72
#define MCONTEXT_SS_R8      80
#define MCONTEXT_SS_R9      88
#define MCONTEXT_SS_R10     96
#define MCONTEXT_SS_R11     104
#define MCONTEXT_SS_R12     112
#define MCONTEXT_SS_R13     120
#define MCONTEXT_SS_R14     128
#define MCONTEXT_SS_R15     136
#define MCONTEXT_SS_RIP     144

TEXT
LABEL(__np_setcontext)
	/* struct mcontext_t * %rdi */
	movq  MCONTEXT_SS_RAX(%rdi),   %rax
	movq  MCONTEXT_SS_RBX(%rdi),   %rbx
	movq  MCONTEXT_SS_RCX(%rdi),   %rcx
	movq  MCONTEXT_SS_RDX(%rdi),   %rdx
	/* we deal with %rdi later */
	movq  MCONTEXT_SS_RSI(%rdi),   %rsi
	movq  MCONTEXT_SS_RBP(%rdi),   %rbp
	movq  MCONTEXT_SS_RSP(%rdi),   %rsp
	movq  MCONTEXT_SS_R8(%rdi),    %r8
	movq  MCONTEXT_SS_R9(%rdi),    %r9
	movq  MCONTEXT_SS_R10(%rdi),   %r10
	movq  MCONTEXT_SS_R11(%rdi),   %r11
	movq  MCONTEXT_SS_R12(%rdi),   %r12
	movq  MCONTEXT_SS_R13(%rdi),   %r13
	movq  MCONTEXT_SS_R14(%rdi),   %r14
	movq  MCONTEXT_SS_R15(%rdi),   %r15

	pushq MCONTEXT_SS_RIP(%rdi)
	pushq MCONTEXT_SS_RDI(%rdi)
	popq %rdi
	retq

#endif /* __x86_64__ */
