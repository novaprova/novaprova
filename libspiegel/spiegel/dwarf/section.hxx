
#ifndef __libspiegel_dwarf_section_hxx__
#define __libspiegel_dwarf_section_hxx__ 1

#include "spiegel/common.hxx"
#include "reader.hxx"

namespace spiegel {
namespace dwarf {

struct section_t
{
    unsigned long offset;
    unsigned long size;
    void *map;

    unsigned long get_end() const { return offset + size; }
    void set_end(unsigned long e) { size = e - offset; }
    bool contains(const section_t &o)
    {
	return (offset <= o.offset &&
		get_end() >= o.get_end());
    }
    void map_from(const section_t &o)
    {
	map = (void *)((char *)o.map + offset - o.offset);
    }

    void expand_to_pages()
    {
	unsigned long end = page_round_up(offset + size);
	offset = page_round_down(offset);
	size = end - offset;
    }

    reader_t get_contents() const
    {
	reader_t r(map, size);
	return r;
    }

    static int
    compare_ptrs(const void *v1, const void *v2)
    {
	const struct section_t **s1 = (const struct section_t **)v1;
	const struct section_t **s2 = (const struct section_t **)v2;
	return u64cmp((*s1)->offset, (*s2)->offset);
    }

    const char *
    offset_as_string(unsigned long off) const
    {
	if (off >= size)
	    return 0;
	const char *v = (const char *)map + off;
	// check that there's a \0 terminator
	if (!memchr(v, '\0', size-off))
	    return 0;
	return v;
    }
};

// close namespaces
} }

#endif // __libspiegel_dwarf_section_hxx__
