#ifndef __np_spiegel_dwarf_reference_hxx__
#define __np_spiegel_dwarf_reference_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct reference_t
{
    uint32_t cu;	// index into compile unit table
    uint32_t offset;	// byte offset from start of compile unit

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
