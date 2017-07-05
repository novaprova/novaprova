/*
 * Copyright 2011-2012 Gregory Banks
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
#ifndef __np_spiegel_dwarf_reference_hxx__
#define __np_spiegel_dwarf_reference_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct reference_t
{
    // Types of reference, in the order defined in the standard
    // The standard does not define names for these types.
    enum type_t {
        // Used only for the null reference
        REF_NULL=0,
        // Dwarf3.pdf p128
        // The offset field is 1 to 8 byte offset from the first byte
        // of the compile unit header for the containing compile unit.
        // The cu field is state_t's index of the containing compile unit.
        REF_CU,
        // Dwarf3.pdf p128
        // The offset field is a 4 or 8 byte offset from the start of the
        // containing executable or shared object's .debug_info section.
        // The cu field is state_t's index of the containing link object.
        // The name comes from DW_FORM_ref_addr but is a misnomer as
        // the offset is actually a exe file offset which is not the
        // same concept at all.
        REF_ADDR,
        // DWARF4.pdf p150
        // The offset field is the 64bit type signature calculated for
        // a type DIE.
        // The cu field...who knows ???
        // This is not yet implemented.
        REF_SIG8
    };
    type_t type:32;     // what type of reference this is
                        // expanded to 32b to avoid anonymous padding to
                        // avoid anonymous padding which makes Valgrind sad
    uint32_t cu;	// compile unit index, interpretation varies by type
    uint64_t offset;	// byte offset, interpretation varies by type

    // reference_t is kept in a union so it cannot have user-defined c'tor or d'tors.
    // instead we provide static make() functions as pseudo-c'tors

    // make a compile-unit reference
    static reference_t make_cu(uint32_t cu, uint32_t off)
    {
        reference_t ref;
        ref.type = REF_CU;
        ref.cu = cu;
        ref.offset = off;
        return ref;
    }
    // make an address reference
    static reference_t make_addr(uint32_t lo, np::spiegel::offset_t off)
    {
        reference_t ref;
        ref.type = REF_ADDR;
        ref.cu = lo;
        ref.offset = off;
        return ref;
    }
#if 0
    // make a signature reference
    static reference_t make_sig8(uint64_t sig8)
    {
        reference_t ref;
        ref.type = REF_SIG8;
        ref.cu = 0;
        ref.offset = sig8;
        return ref;
    }
#endif

    static const reference_t null;

    int operator==(const reference_t &o) const
    {
	return (o.cu == cu && o.offset == offset);
    }
    int operator<(const reference_t &o) const
    {
	if (cu < o.cu)
	    return true;
	if (cu == o.cu && offset < o.offset)
	    return true;
	return false;
    }
    std::string as_string() const;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_reference_hxx__
