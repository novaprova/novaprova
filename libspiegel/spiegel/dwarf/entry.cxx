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
    nvalues_ = 0;
    // invalidate the by-attribute indexes
    memset(byattr_basic_, 0xff, sizeof(byattr_basic_));
    memset(byattr_user_, 0xff, sizeof(byattr_user_));
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
