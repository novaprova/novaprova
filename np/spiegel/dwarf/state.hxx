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
#ifndef __np_spiegel_dwarf_state_hxx__
#define __np_spiegel_dwarf_state_hxx__ 1

#include "np/spiegel/common.hxx"
#include "np/spiegel/spiegel.hxx"
#include "np/util/rangetree.hxx"
#include "section.hxx"
#include "reference.hxx"
#include "enumerations.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class walker_t;
class compile_unit_t;
class link_object_t;

class state_t
{
public:
    state_t();
    ~state_t();

    bool add_self();
    bool add_executable(const char *filename);

    void dump_structs();
    void dump_functions();
    void dump_variables();
    void dump_info(bool preorder, bool paths);
    void dump_abbrevs();

    /* Input is any address generated from inside the program, e.g. by
     * taking the address of a function (which might actually be the
     * address of a function descriptor or a PLT slot.  Returns the
     * address of the actual code, i.e. the first byte of the function
     * prolog, which can actually be usefully instrumented. */
    np::spiegel::addr_t instrumentable_address(np::spiegel::addr_t) const;

    /* Input is any address seen at runtime in the program, e.g. from
     * a stack trace or a signal context.  Returns an address which
     * can be used to look up recorded debug information.  Note that the
     * two may differ due to Address Space Layout Randomisation. */
    np::spiegel::addr_t recorded_address(np::spiegel::addr_t) const;

    bool describe_address(np::spiegel::addr_t addr,
			  reference_t &curef,
			  unsigned int &lineno,
			  reference_t &funcref,
			  unsigned int &offset) const;
    std::string get_full_name(reference_t ref);

    // state_t is a Singleton
    static state_t *instance() { return instance_; }

    const std::vector<compile_unit_t*> &get_compile_units() const { return compile_units_; }

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
    compile_unit_offset_tuple_t resolve_reference(reference_t ref) const;

    const std::vector<link_object_t*> &get_link_objects() const { return link_objects_; }
    link_object_t *get_link_object(uint32_t loidx) { return loidx < link_objects_.size() ? link_objects_[loidx] : 0; }
    link_object_t *get_link_object(const char *filename);

private:
    bool read_link_objects();
    bool read_compile_units(link_object_t *);
    compile_unit_t *get_compile_unit_by_offset(uint32_t loindex, np::spiegel::offset_t off) const;
    /* Prepare an index which will speed up all later calls to describe_address(). */
    void prepare_address_index();

    void insert_ranges(const walker_t &w, reference_t funcref);
    bool is_within(np::spiegel::addr_t addr, const walker_t &w,
		   unsigned int &offset) const;

    static state_t *instance_;

    std::vector<link_object_t*> link_objects_;
    std::vector<compile_unit_t*> compile_units_;
    np::util::rangetree<addr_t, reference_t> address_index_;
    /* Index from real address ranges to link_object_t */
    np::util::rangetree<addr_t, link_object_t*> link_object_index_;

    friend class walker_t;
    friend class compile_unit_t;
};


// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_state_hxx__
