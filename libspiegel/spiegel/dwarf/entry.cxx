#include "entry.hxx"
#include "enumerations.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

void
entry_t::setup(size_t offset, unsigned level, const abbrev_t *a)
{
    offset_ = offset;
    level_ = level;
    abbrev_ = a;
    byattr_.clear();
    byattr_.resize(DW_AT_max, 0);
    values_.clear();
    values_.reserve(a->attr_specs.size());
}

void
entry_t::dump() const
{
    if (!abbrev_)
	return;

    printf("Entry 0x%x [%u] %s {\n",
	offset_,
	level_,
	tagnames.to_name(abbrev_->tag));

    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = abbrev_->attr_specs.begin() ; i != abbrev_->attr_specs.end() ; ++i)
    {
	printf("    %s = ", attrnames.to_name(i->name));
	const value_t *v = get_attribute(i->name);
	assert(v);
	v->dump();
	printf("\n");
    }
    printf("}\n");
}

// close namespace
} }
