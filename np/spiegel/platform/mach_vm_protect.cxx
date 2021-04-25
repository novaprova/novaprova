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
 * This file contains the code that uses the Mach vm_protect() system
 * call, needed on MacOS Catalina and above.
 */
#include "np/spiegel/common.hxx"
#include "common.hxx"
#include "np/util/log.hxx"
#include "np/util/trace.h"
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>
#include "np/util/valgrind.h"

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

extern "C" kern_return_t __np_vm_protect(vm_map_t target_task,
                                         vm_address_t address,
                                         vm_size_t size,
                                         boolean_t set_maximum,
                                         vm_prot_t new_protection);


static int
text_map_writable(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    kern_return_t r;


    /* actually change the underlying mapping in one
     * big system call. */
    r = __np_vm_protect(
        mach_task_self(),
        (vm_address_t)start,
        (vm_size_t)(end-start),
        /*set_maximum*/false,
        VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY);
    if (r != KERN_SUCCESS)
    {
        eprintf("Failed to call vm_protect(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY): %d\n", r);
	return -1;
    }
    return 0;
}

static int
text_restore(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    int r;

    /* actually change the underlying mapping in one
     * big system call. */
    r = __np_vm_protect(
        mach_task_self(),
        (vm_address_t)start,
        (vm_size_t)(end-start),
        /*set_maximum*/false,
        VM_PROT_READ|VM_PROT_EXECUTE);
    if (r != KERN_SUCCESS)
    {
        eprintf("Failed to vm_protect(VM_PROT_READ|VM_PROT_EXECUTE): %d\n", r);
        return -1;
    }

    return 0;
}

int
text_write(addr_t to, const uint8_t *from, size_t len)
{
    np_trace("text_write: to=");
    np_trace_hex(to);
    np_trace(" from=");
    np_trace_hex((uint64_t)from);
    np_trace(" len=");
    np_trace_hex(len);
    np_trace("\n");

    int r = text_map_writable(to, len);
    if (r)
        return r;
    /*
     * On Linux, this would be the start of a CRITICAL SECTION
     * where there is no guarantee that calls to libc (or any
     * other shared library) will work.
     *
     * That issues hasn't happened on Mac.  Yet.  But we will
     * hew to the same basic design Just In Case.
     */
    for (unsigned int i = 0 ; i < len ; i++)
        ((char *)to)[i] = ((const char *)from)[i];

    r = text_restore(to, len);
    /* end CRITICAL SECTION */

    VALGRIND_DISCARD_TRANSLATIONS(to, len);

    return r;
}

// close namespaces
}; }; };
