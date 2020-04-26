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
 * This file contains the code that uses the mprotect() system call.
 * So far it works just fine on Linux and Darwin.
 */
#include "np/spiegel/common.hxx"
#include "common.hxx"
#include "np/util/log.hxx"
#include <sys/mman.h>
#include <dlfcn.h>
#include "np/util/valgrind.h"

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;


/*
 * Call the mprotect() system call without using the PLT.
 * This horrible workaround is necessary to avoid a SEGV
 * if the mprotect() in text_map_writable() happens to
 * accidentally remove execute protection from the PLT.
 */
static int mprotect(void *start, size_t len, int prot)
{
    static int (*real_mprotect)(void*, size_t, int) = 0;
    if (!real_mprotect)
        real_mprotect = (int (*)(void*, size_t, int))dlsym(RTLD_NEXT, "mprotect");
    return real_mprotect(start, len, prot);
}

static int
text_map_writable(addr_t addr, size_t len)
{
    addr_t start = page_round_down(addr);
    addr_t end = page_round_up(addr+len);
    int r;

    /* actually change the underlying mapping in one
     * big system call. */
    r = mprotect((void *)start,
		 (size_t)(end-start),
		 PROT_READ|PROT_WRITE);
    if (r)
    {
        eprintf("Failed to call mprotect(PROT_READ|PROT_WRITE): %s\n", strerror(errno));
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
    r = mprotect((void *)start,
                 (size_t)(end-start),
                 PROT_READ|PROT_EXEC);
    if (r)
    {
        eprintf("Failed to mprotect(PROT_READ|PROT_EXEC): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int
text_write(addr_t to, const uint8_t *from, size_t len)
{
    int r = text_map_writable(to, len);
    if (r)
        return r;
    /*
     * If mprotect succeeded, we're now in a CRITICAL SECTION
     * where there is no guarantee that calls to libc (or any
     * other shared library) will work.  This is because it's
     * possible for the mprotect() to accidentally remove
     * execute protection from the PLT, depending on accidents
     * of the layout of the executable.
     *
     * It also means we can't rely on memcpy() to copy the bytes,
     * because it might be a libc call or inlined depending on
     * how smart the compiler is or the optimization level.  On
     * x86_64 this doesn't matter because the trap instruction
     * sequences are a single byte anyway.
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
