#ifndef __libspiegel_dwarf_reference_hxx__
#define __libspiegel_dwarf_reference_hxx__ 1

#include "spiegel/common.hxx"

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
};

// close namespaces
} }

#endif // __libspiegel_dwarf_reference_hxx__
