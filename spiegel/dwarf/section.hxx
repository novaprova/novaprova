
#ifndef __libspiegel_dwarf_section_hxx__
#define __libspiegel_dwarf_section_hxx__ 1

#include "spiegel/common.hxx"
#include "spiegel/mapping.hxx"
#include "reader.hxx"

namespace spiegel {
namespace dwarf {

struct section_t : public spiegel::mapping_t
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
} }

#endif // __libspiegel_dwarf_section_hxx__
