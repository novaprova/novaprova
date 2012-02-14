#ifndef __libspiegel_dwarf_entry_hxx__
#define __libspiegel_dwarf_entry_hxx__ 1

#include "spiegel/commonp.hxx"
#include <vector>
#include "value.hxx"
#include "abbrev.hxx"

namespace spiegel {
namespace dwarf {

class abbrev_t;

class entry_t
{
public:
    entry_t()
     :  offset_(0),
	level_(0),
	abbrev_(0)
    {}

    void setup(size_t offset, unsigned level, const abbrev_t *a);
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

    void add_attribute(uint32_t name, value_t val)
    {
	values_.push_back(val);
	byattr_[name] = &values_.back();
    }

    size_t get_offset() const { return offset_; }
    unsigned get_level() const { return level_; }
    const abbrev_t *get_abbrev() const { return abbrev_; }
    uint32_t get_tag() const { return abbrev_->tag; }
    bool has_children() const { return abbrev_->children; }
    void dump() const;

    const value_t *get_attribute(uint32_t name) const
    {
	return (name < byattr_.size() ? byattr_[name] : 0);
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
    const reference_t get_reference_attribute(uint32_t name) const
    {
	const value_t *v = get_attribute(name);
	return (v && v->type == value_t::T_REF ? v->val.ref : reference_t::null);
    }

private:
    size_t offset_;
    unsigned level_;
    const abbrev_t *abbrev_;
    std::vector<value_t> values_;
    std::vector<const value_t*> byattr_;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_entry_hxx__
