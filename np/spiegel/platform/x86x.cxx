/*
 * Copyright 2011-2020 Greg Banks
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
 * Code for x86 and x86_64 architectures on all platforms.
 */
#include "np/spiegel/common.hxx"
#include "np/util/log.hxx"
#include "common.hxx"

#include <vector>


namespace np {
    namespace spiegel {
        namespace platform {

using namespace std;
using namespace np::util;

static unsigned
call_instruction_length_for_ra(unsigned long ra)
{
    // dprintf("XXX trying to discover a CALL instruction for return address 0x%lx", ra);
#define LENGTH(_x)      (_x)
#define MASK(_m, _v)    (((_m)<<8)|((_v)))
#define EXACT(_x)       MASK(0xff, _x)
#define ANY             MASK(0, 0)
    /* The forms of CALL instruction we expect to see, extracted from
     * Intel 32 and IA-32 Architectures Software Developer's Manual Vol 2A */
    static uint16_t insn_forms[] = {
        /* E8 XX XX XX XX XX        Call near, relative, displacement relative
         *                          to next instruction. 32-bit displacement sign
         *                          extended to 64-bits in 64-bit mode.  */
        LENGTH(5), EXACT(0xE8), ANY, ANY, ANY, ANY,

        /* See Table 2.2 32-Bit Addressing Forms with the ModR/M Bytes */

        /* FF (mod=00, r/m=000..011) */
        LENGTH(2), EXACT(0xFF), MASK(0xE0, 0x00),
        /* FF (mod=00, r/m=100) sib */
        LENGTH(3), EXACT(0xFF), MASK(0xF8, 0x20), ANY,
        /* FF (mod=00, r/m=101) disp32 */
        LENGTH(6), EXACT(0xFF), MASK(0xF8, 0x28), ANY, ANY, ANY, ANY,
        /* FF (mod=00 r/m=110,111) */
        LENGTH(2), EXACT(0xFF), MASK(0xF0, 0x30),

        /* FF (mod=01, r/m=000..011) disp8 */
        LENGTH(3), EXACT(0xFF), MASK(0xE0, 0x40), ANY,
        /* FF (mod=01, r/m=100) sib disp8 */
        LENGTH(4), EXACT(0xFF), MASK(0xF8, 0x60), ANY, ANY,
        /* FF (mod=01, r/m=101) disp8 */
        LENGTH(3), EXACT(0xFF), MASK(0xF8, 0x68), ANY,
        /* FF (mod=01 r/m=110,111) disp8 */
        LENGTH(3), EXACT(0xFF), MASK(0xF0, 0x70), ANY,

        /* FF (mod=10, r/m=000..011) disp32 */
        LENGTH(6), EXACT(0xFF), MASK(0xE0, 0x80), ANY, ANY, ANY, ANY,
        /* FF (mod=10, r/m=100) sib disp32 */
        LENGTH(7), EXACT(0xFF), MASK(0xF8, 0xA0), ANY, ANY, ANY, ANY, ANY,
        /* FF (mod=10, r/m=101) disp32 */
        LENGTH(6), EXACT(0xFF), MASK(0xF8, 0xA8), ANY, ANY, ANY, ANY,
        /* FF (mod=10 r/m=110,111) disp32 */
        LENGTH(6), EXACT(0xFF), MASK(0xF0, 0xB0), ANY, ANY, ANY, ANY,

        /* FF (mod=11, r/m=000..011) */
        LENGTH(2), EXACT(0xFF), MASK(0xE0, 0xC0),
        /* FF (mod=11, r/m=100) */
        LENGTH(2), EXACT(0xFF), MASK(0xF8, 0xE0),
        /* FF (mod=11, r/m=101) */
        LENGTH(2), EXACT(0xFF), MASK(0xF8, 0xE8),
        /* FF (mod=11 r/m=110,111) */
        LENGTH(2), EXACT(0xFF), MASK(0xF0, 0xF0)
    };
    const uint16_t *forms_end = (const uint16_t *)((const char*)insn_forms + sizeof(insn_forms));
    const uint16_t *formp = insn_forms;
    while (formp < forms_end)
    {
        uint16_t len = *formp++;
        const uint16_t *next_formp = formp + len;
        const uint8_t *bytep = (const uint8_t *)ra - len;
        bool match = true;

        for ( ; match && formp < next_formp ; formp++, bytep++)
        {
            uint8_t mask = *formp >> 8;
            uint8_t value = *formp & 0xFF;
            if ((*bytep & mask) != value)
                match = false;
        }
        if (match)
        {
//            dprintf("XXX CALL instruction for return address 0x%lx "
//                    "was %u bytes long, starting at 0x%lx",
//                    ra, len, (unsigned long)ra - len);
            return len;
        }
        formp = next_formp;
    }
    dprintf("Unable to match any CALL instruction for return address 0x%lx", ra);
    return 0;
#undef LENGTH
#undef MASK
#undef EXACT
#undef ANY
}

vector<np::spiegel::addr_t> get_stacktrace()
{
    /* This only works if a frame pointer is used, i.e. it breaks
     * with -fomit-frame-pointer.
     *
     * TODO: use DWARF2 unwind info and handle -fomit-frame-pointer
     *
     * TODO: terminating the unwind loop is tricky to do properly,
     *       we need to estimate the stack boundaries.  Instead
     *       we approximate
     *
     * TODO: should return a vector of {ip=%eip,fp=%ebp,sp=%esp}
     */
    unsigned long bp;
    vector<np::spiegel::addr_t> stack;

#if _NP_ADDRSIZE == 4
    __asm__ volatile("movl %%ebp, %0" : "=r"(bp));
#else
    __asm__ volatile("movq %%rbp, %0" : "=r"(bp));
#endif
    for (;;)
    {
        unsigned long ra = ((unsigned long *)bp)[1];
        if (ra > 4096)
            ra -= call_instruction_length_for_ra(ra);
	stack.push_back(ra);
	unsigned long nextbp = ((unsigned long *)bp)[0];
	if (!nextbp)
	    break;
	if (nextbp < bp)
	    break;	// moving in the wrong direction
	if ((nextbp - bp) > 16384)
	    break;	// moving a heuristic "too far"
	bp = nextbp;
    };
    return stack;
}

// Close namespaces
}; }; };
