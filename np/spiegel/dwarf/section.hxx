
#ifndef __np_spiegel_dwarf_section_hxx__
#define __np_spiegel_dwarf_section_hxx__ 1

#include "np/spiegel/common.hxx"
#include "np/spiegel/mapping.hxx"
#include "reader.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct section_t : public np::spiegel::mapping_t
{
public:
    reader_t get_contents() const
    {
	reader_t r(map_, size_);
	return r;
    }

    const char *
    offset_as_string(unsigned long off) const
    {
	if (off >= size_)
	    return 0;
	const char *v = (const char *)map_ + off;
	// check that there's a \0 terminator
	if (!memchr(v, '\0', size_-off))
	    return 0;
	return v;
    }
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_section_hxx__
