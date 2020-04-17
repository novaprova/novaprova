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
 * This file contains the code that uses the Mach vm_protect() system
 * call, needed on MacOS Catalina and above.
 */
#include "np/spiegel/common.hxx"
#include "common.hxx"
#include "np/util/log.hxx"
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

static map<addr_t, unsigned int> pagerefs;

int
text_map_writable(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    addr_t a;
    kern_return_t r;

    /* increment the reference counts on every page we hit */
    for (a = start ; a < end ; a += page_size())
    {
	map<addr_t, unsigned int>::iterator itr = pagerefs.find(a);
	if (itr == pagerefs.end())
	    pagerefs[a] = 1;
	else
	    itr->second++;
    }

    /* actually change the underlying mapping in one
     * big system call. */
    r = ::vm_protect(
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

int
text_restore(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    addr_t a;
    int r;

    /* decrement the reference counts on every page we hit */
    for (a = start ; a < end ; a += page_size())
    {
	map<addr_t, unsigned int>::iterator itr = pagerefs.find(a);
	if (itr == pagerefs.end())
	    continue;
	if (--itr->second)
	    continue;	/* still other references */
	pagerefs.erase(itr);

	/* change the underlying mapping one page at a time */
	r = ::vm_protect(
            mach_task_self(),
            (vm_address_t)a,
            (vm_size_t)page_size(),
            /*set_maximum*/false,
            VM_PROT_READ|VM_PROT_EXECUTE);
	if (r != KERN_SUCCESS)
	{
            eprintf("Failed to vm_protect(VM_PROT_READ|VM_PROT_EXECUTE): %d\n", r);
	    return -1;
	}
    }

    return 0;
}

// close namespaces
}; }; };
