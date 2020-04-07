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

class compile_unit_t;
struct reference_t;

// This would be std::tuple<compile_unit_t*, np::spiegel::offset_t>
// if I felt I could enable C++11 support.
struct compile_unit_offset_tuple_t
{
    compile_unit_offset_tuple_t(compile_unit_t *cu, np::spiegel::offset_t off)
     : _cu(cu), _off(off)
    {
    }
    compile_unit_t *_cu;
    np::spiegel::offset_t _off;
};

class reference_resolver_t
{
public:
    virtual ~reference_resolver_t() {}
    virtual compile_unit_offset_tuple_t resolve_reference(const reference_t &) const = 0;
    virtual std::string describe_resolver() const = 0;
};

struct reference_t
{
    /*
     * Types of reference, in the order defined in the standard
     * The standard does not define names for these types.
     *
     * - Not in the standard. A null reference, used in the code
     *   to indicate default values and failures, just like NULL
     *   pointers.  Both offset and resolver are zero.
     *
     * - Defined in Dwarf3.pdf p128.  Stored as DW_FORM_ref[1248].
     *   The offset is 1 to 8 byte offset from the first byte of
     *   the compile unit header for the containing compile unit.
     *   The resolver is the compile_unit_t.
     *
     * - Defined in Dwarf3.pdf p128.  Stored as DW_FORM_ref_addr.
     *   The offset is a 4 or 8 byte offset from the start of the
     *   containing executable or shared object's .debug_info
     *   section.  The resolver is the link_object_t.
     *
     * - Defined in DWARF4.pdf p150.  Stored as DW_FORM_ref_sig8.
     *   The offset is the 64bit type signature calculated for
     *   a type DIE.  This is not yet implemented.
     */

    const reference_resolver_t *resolver;
    uint64_t offset;	// byte offset, interpretation varies by type

    // reference_t is kept in a union so it cannot have user-defined c'tor or d'tors.
    // instead we provide the static make() function as a pseudo-c'tor

    static reference_t make(const reference_resolver_t *resolver, uint64_t off)
    {
        reference_t ref = { resolver, off };
        return ref;
    }

    static const reference_t null;

    compile_unit_offset_tuple_t resolve() const
    {
        return resolver ?
            resolver->resolve_reference(*this) :
            compile_unit_offset_tuple_t(0, 0);
    }

    int operator==(const reference_t &o) const
    {
	return (o.resolver == resolver && o.offset == offset);
    }
    // needs to be contained in a map
    int operator<(const reference_t &o) const
    {
	if (resolver < o.resolver)
	    return true;
	if (resolver == o.resolver && offset < o.offset)
	    return true;
	return false;
    }
    std::string as_string() const;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_reference_hxx__
