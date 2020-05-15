/*
 * Copyright 2011-2020 Gregory Banks
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
 * This file contains a replacement for the vm_protect() system call.
 */

.section __TEXT,__text,regular,pure_instructions

/* Align the vm_protect stub to a page, to ensure it cannot
 * accidentally render itself unexecutable while making
 * some other text page writable. */
.balign 4096

/*
 * Call the vm_protect() system call.  This is exactly the code
 * in libsystem_kernel.dylib.  Unlike traditional Unixes, Mach
 * doesn't do the wacky errno dance and instead returns a status
 * in %eax, so we don't need to compromise with our simulation
 * of the stub's behavior like we do on Linux.
 * Equivalent C declaration is identical to the real one, namely
 *
 * kern_return_t __np_vm_protect(vm_map_t target_task,
 *                               vm_address_t address,
 *                               vm_size_t size,
 *                               boolean_t set_maximum,
 *                               vm_prot_t new_protection);
 */
.globl ___np_vm_protect
___np_vm_protect:
    movq %rcx, %r10
    /* TODO: presumably a define exists for this system call number */
    movl $0x100000e, %eax
    syscall
    retq

/* Pad out to the next page, see comment above */
.balign 4096
