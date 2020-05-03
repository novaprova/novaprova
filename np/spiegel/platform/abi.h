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
 */
#ifndef __np_spiegel_platform_abi_h__
#define __np_spiegel_platform_abi_h__ 1

#if defined(_NP_x86)

#define capture_pc(nm) \
    do { \
        unsigned long _eip; \
        __asm__ volatile("call 1f; 1: popl %0" : "=r"(_eip)); \
        (nm) = _eip; \
    } while(0)
#define capture_sp(nm) \
    do { \
        unsigned long _esp; \
        __asm__ volatile("mov %%esp, %0" : "=r"(_esp)); \
        (nm) = _esp; \
    } while(0)
#define capture_bp(nm) \
    do { \
        unsigned long _ebp; \
        __asm__ volatile("mov %%ebp, %0" : "=r"(_ebp)); \
        (nm) = _ebp; \
    } while(0)

#define _NP_STACK_ALIGNMENT_BYTES     4

#elif defined(_NP_x86_64)

#define capture_pc(nm) \
    do { \
        unsigned long _rip; \
        __asm__ volatile("call 1f; 1: popq %0" : "=r"(_rip)); \
        (nm) = _rip; \
    } while(0)
#define capture_sp(nm) \
    do { \
        unsigned long _rsp; \
        __asm__ volatile("movq %%rsp, %0" : "=r"(_rsp)); \
        (nm) = _rsp; \
    } while(0)
#define capture_bp(nm) \
    do { \
        unsigned long _rbp; \
        __asm__ volatile("movq %%rbp, %0" : "=r"(_rbp)); \
        (nm) = _rbp; \
    } while(0)

#define _NP_STACK_ALIGNMENT_BYTES     16

#else
#error "Sorry, no definition for capture_pc() et al on this platform"
#endif

#endif /* __np_spiegel_platform_abi_h__  */
