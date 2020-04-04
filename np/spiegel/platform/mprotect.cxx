/*
 * Copyright 2011-2014 Gregory Banks
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
 * This file contains the code that uses the mprotect() system call.
 * So far it works just fine on Linux and Darwin.
 */
#include "np/spiegel/common.hxx"
#include "common.hxx"
#include "np/util/log.hxx"
#include <sys/mman.h>

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

/*
 * Functions to ensure some .text space is writable (as well as
 * executable) so we can insert breakpoint insns, and to undo the
 * effect.
 *
 * Uses per-page reference counting so that multiple calls can be
 * nested.  This is for tidiness so that we can re-map pages back to
 * their default state after we've finished, and still handle the case
 * of two intercepts in separate functions which are located in the
 * same page.
 *
 * These functions are more general than they need to be, as we only
 * ever need the len=1 case.
 */

static map<addr_t, unsigned int> pagerefs;

int
text_map_writable(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    addr_t a;
    int r;

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
    r = mprotect((void *)start,
		 (size_t)(end-start),
		 PROT_READ|PROT_WRITE|PROT_EXEC);
    if (r)
    {
        eprintf("Failed to call mprotect(PROT_READ|PROT_WRITE|PROT_EXEC): %s\n", strerror(errno));
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
	r = mprotect((void *)a, (size_t)page_size(), PROT_READ|PROT_EXEC);
	if (r)
	{
            eprintf("Failed to mprotect(PROT_READ|PROT_EXEC): %s\n", strerror(errno));
	    return -1;
	}
    }

    return 0;
}

// close namespaces
}; }; };
