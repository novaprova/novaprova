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
 */
#ifndef __np_spiegel_dwarf_entry_hxx__
#define __np_spiegel_dwarf_entry_hxx__ 1

#include "np/spiegel/common.hxx"
#include "value.hxx"
#include "abbrev.hxx"
#include "enumerations.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct abbrev_t;

class entry_t
{
public:
    entry_t()
     :  offset_(0),
	level_(0),
	abbrev_(0),
	latest_(0)
    {
	memset(generation_, 0x0, sizeof(generation_));
    }

    void setup(size_t offset, unsigned level, const abbrev_t *a)
    {
	offset_ = offset;
	level_ = level;
	abbrev_ = a;
	latest_++;
    }
    void partial_setup(const entry_t &o)
    {
	offset_ = o.offset_;
	level_ = o.level_;
	abbrev_ = 0;
    }
    void partial_setup(uint32_t off, uint32_t lev)
    {
	offset_ = off;
	level_ = lev;
	abbrev_ = 0;
    }
    void partial_setup(uint32_t off, uint32_t lev, const abbrev_t *a)
    {
	offset_ = off;
	level_ = lev;
	abbrev_ = a;
    }

    void add_attribute(uint32_t name, const value_t &val)
    {
	int i = name_to_index(name);
	if (i < 0)
	    return;	// silently ignore this
	values_[i] = val;
	generation_[i] = latest_;
    }

    unsigned get_offset() const { return offset_; }
    unsigned get_level() const { return level_; }
    const abbrev_t *get_abbrev() const { return abbrev_; }
    uint32_t get_tag() const { return abbrev_->tag; }
    bool has_children() const { return abbrev_->children; }
    std::string to_string() const;

    const value_t *get_attribute(uint32_t name) const
    {
	int i = name_to_index(name);
	return (i >= 0 && generation_[i] == latest_ ? &values_[i] : 0);
    }
    const char *get_string_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	return (v && v->type == value_t::T_STRING ? v->val.string : 0);
    }
    uint32_t get_uint32_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return v->val.uint32;
	else
	    return 0;
    }
    uint64_t get_uint64_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return v->val.uint32;
	else if (v->type == value_t::T_UINT64)
	    return v->val.uint64;
	else
	    return 0;
    }
    addr_t get_address_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	if (!v)
	    return 0;
	else if (v->type == value_t::T_UINT32)
	    return (addr_t)v->val.uint32;
	else if (v->type == value_t::T_UINT64)
	    return (addr_t)v->val.uint64;
	else
	    return 0;
    }
    const reference_t get_reference_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	return (v && v->type == value_t::T_REF ? v->val.ref : reference_t::null);
    }
    enum form_values get_attribute_form(uint32_t name) const;

private:
    enum
    {
	// we oversupply space, because entry_t is never
	// allocated and stored but handling a sparse mapping
	// was taking up too many cycles.
	MAX_VALUES = DW_AT_max_basic + (DW_AT_max_user - DW_AT_lo_user)
    };
    static int name_to_index(int name)
    {
	if (name < DW_AT_max_basic)
	    return name;
	else if (name >= DW_AT_lo_user && name < DW_AT_max_user)
	    return DW_AT_max_basic+name-DW_AT_lo_user;
	else
	    return -1;
    }

    unsigned offset_;
    unsigned level_;
    const abbrev_t *abbrev_;
    uint32_t latest_;
    uint32_t generation_[MAX_VALUES];
    value_t values_[MAX_VALUES];
};


// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_entry_hxx__
