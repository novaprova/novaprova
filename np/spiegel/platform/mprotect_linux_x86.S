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
 *
 * This file contains a replacement for the mprotect() system call.
 */
#include <sys/syscall.h>

.text

/* Align the mprotect stub to a page, to ensure it cannot
 * accidentally render itself unexecutable while making
 * some other text page writable. */
.align 4096

/*
 * Call the mprotect() system call.  Does not report errors
 * in the "global" errno, as that requires some very ABI-specific
 * magic involving TLS; instead it returns the raw result from
 * the kernel in %eax which is 0 for success and a negative
 * error number on failure.  Equivalent C declaration is
 *
 * int __np_mprotect(void *addr, size_t len, int prot);
 */
.globl __np_mprotect
.type __np_mprotect, @function
__np_mprotect:
    push %ebx
    mov 0x10(%esp), %edx
    mov 0xc(%esp), %ecx
    mov 0x8(%esp), %ebx
    movl $__NR_mprotect, %eax
    /* The recommended standard is the SYSENTER insn instead of the
     * old int 80h.  But SYSENTER seems to be failing on 32b Xenial,
     * so old-fashioned it is. */
    int $0x80
    pop %ebx
    ret
    .size __np_mprotect, .-__np_mprotect

/* Pad out to the next page, see comment above */
.align 4096
