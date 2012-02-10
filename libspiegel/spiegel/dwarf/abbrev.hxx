#ifndef __libspiegel_dwarf_abbrev_hxx__
#define __libspiegel_dwarf_abbrev_hxx__ 1

#include "spiegel/commonp.hxx"
#include <vector>

namespace spiegel {
namespace dwarf {

class reader_t;

struct abbrev_t
{
    struct attr_spec_t
    {
	uint32_t name;
	uint32_t form;
    };

    // default c'tor
    abbrev_t()
     :  code(0),
        tag(0),
        children(0)
    {}

    // c'tor with code
    abbrev_t(uint32_t c)
     :  code(c),
        tag(0),
	children(0)
    {}

    bool read(reader_t &r);

    uint32_t code;
    uint32_t tag;
    uint8_t children;
    std::vector<attr_spec_t> attr_specs;
};

// close namespaces
} }

#endif // __libspiegel_dwarf_abbrev_hxx__
