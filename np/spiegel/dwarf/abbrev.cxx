#include "abbrev.hxx"
#include "reader.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

bool
abbrev_t::read(reader_t &r)
{
    if (!r.read_uleb128(tag) ||
	!r.read_u8(children))
	return false;
    for (;;)
    {
	attr_spec_t as;
	if (!r.read_uleb128(as.name) ||
	    !r.read_uleb128(as.form))
	    return false;
	if (!as.name && !as.form)
	    break;	    /* name=0, form=0 indicates end
			 * of attribute specifications */
	attr_specs.push_back(as);
    }
    return true;
}

// close namespaces
}; }; };
