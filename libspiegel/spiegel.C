#include "commonp.H"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <bfd.h>
#include <map>
#include <vector>

namespace spiegel {
namespace dwarf {

using namespace std;

class string_table_t
{
public:
    string_table_t(const char *prefix, const char * const *names)
     :  prefix_(prefix),
        names_(names),
	prefix_len_(prefix ? strlen(prefix) : 0)
    {
	int i;
	for (i = 0 ; names_[i] ; i++)
	    ;
	names_count_ = i;
    }

    int to_index(const char *name) const
    {
	if (prefix_len_)
	{
	    if (strncmp(name, prefix_, prefix_len_))
		return -1;
	    name += prefix_len_;
	}

	for (int i = 0 ; names_[i] ; i++)
	{
	    if (!strcmp(names_[i], name))
		return i;
	}
	return -1;
    }

    const char *to_name(int i) const
    {
	if (i < 0 || i >= (int)names_count_ || names_[i][0] == '\0')
	    return "unknown";
	if (prefix_len_)
	{
	    // TODO: use an estring
	    static char buf[256];
	    strncpy(buf, prefix_, sizeof(buf));
	    strncat(buf, names_[i], sizeof(buf)-prefix_len_);
	    return buf;
	}
	else
	{
	    return names_[i];
	}
    }

private:
    const char *prefix_;
    const char * const * names_;
    unsigned prefix_len_;
    unsigned names_count_;
};

enum section_names
{
    /* These names are not defined in the DWARF standard
     * but use a similar naming style for consistency */
    DW_sec_none = -1,
    DW_sec_aranges,
    DW_sec_pubnames,
    DW_sec_info,
    DW_sec_abbrev,
    DW_sec_line,
    DW_sec_frame,
    DW_sec_str,
    DW_sec_loc,
    DW_sec_ranges,

    DW_sec_num
};

static const char * const _secnames[DW_sec_num+1] = {
    "aranges", "pubnames", "info",
    "abbrev", "line", "frame",
    "str", "loc", "ranges", 0
};

static string_table_t secnames(".debug_", _secnames);

static unsigned long
pagesize(void)
{
    static unsigned long ps;
    if (!ps)
	ps = sysconf(_SC_PAGESIZE);
    return ps;
}

static unsigned long
page_round_up(unsigned long x)
{
    unsigned long ps = pagesize();
    return ((x + ps - 1) / ps) * ps;
}

static unsigned long
page_round_down(unsigned long x)
{
    unsigned long ps = pagesize();
    return (x / ps) * ps;
}

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

    void expand_to_pages()
    {
	unsigned long end = page_round_up(offset + size);
	offset = page_round_down(offset);
	size = end - offset;
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

class compile_unit_t;
class abbrev_t;
class reader_t;
struct value_t;
class entry_t;
struct walker_t;
class state_t
{
public:
    state_t(const char *filename);
    ~state_t();

    void map_sections();
    void read_abbrevs(uint32_t offset);
    void dump_abbrevs();
    void dump_info();
    void dump_structs(walker_t &w);

private:

    char *filename_;
    section_t sections_[DW_sec_num];
    vector<section_t> mappings_;
    map<uint32_t, abbrev_t*> abbrevs_;
    vector<compile_unit_t> compile_units_;

    friend class walker_t;
};

state_t::state_t(const char *filename)
 :  filename_(xstrdup(filename))
{
    memset(sections_, 0, sizeof(sections_));
}

state_t::~state_t()
{
    free(filename_);
}

void
state_t::map_sections()
{
    int fd = -1;
    vector<section_t>::iterator m;

    bfd_init();

    /* Open a BFD */
    bfd *b = bfd_openr(filename_, NULL);
    if (!b)
    {
	bfd_perror(filename_);
	return;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	fprintf(stderr, "spiegel: %s: not an object\n", filename_);
	goto error;
    }

    /* Extract the file shape of the DWARF sections */
    asection *sec;
    for (sec = b->sections ; sec ; sec = sec->next)
    {
	int idx = secnames.to_index(sec->name);
	if (idx == DW_sec_none)
	    continue;
	printf("Section [%d], name=%s size=%lx filepos=%lx\n",
	    idx, sec->name, (unsigned long)sec->size,
	    (unsigned long)sec->filepos);
	sections_[idx].offset = (unsigned long)sec->filepos;
	sections_[idx].size = (unsigned long)sec->size;
    }

    /* Coalesce sections into a minimal number of mappings */
    struct section_t *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	tsec[idx] = &sections_[idx];
    qsort(tsec, DW_sec_num, sizeof(section_t *), section_t::compare_ptrs);
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	if (!tsec[idx]->size)
	    continue;
	if (mappings_.size() &&
	    page_round_up(mappings_.back().get_end()) >= tsec[idx]->offset)
	{
	    /* section abuts the last mapping, extend it */
	    mappings_.back().set_end(tsec[idx]->get_end());
	}
	else
	{
	    /* create a new mapping */
	    mappings_.push_back(*tsec[idx]);
	}
    }

    printf("Mappings:\n");
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	printf("offset=%lx size=%lx end=%lx\n",
	       m->offset, m->size, m->get_end());
    }

    /* mmap() the mappings */
    fd = open(filename_, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(filename_);
	goto error;
    }

    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	m->expand_to_pages();
	void *map = mmap(NULL, m->size,
			 PROT_READ, MAP_SHARED, fd,
			 m->offset);
	if (map == MAP_FAILED)
	{
	    perror("mmap");
	    goto error;
	}
	m->map = map;
    }

    /* setup sections[].map */
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
	{
	    if (m->contains(sections_[idx]))
	    {
		sections_[idx].map = (void *)(
		    (char *)m->map +
		    sections_[idx].offset -
		    m->offset);
		break;
	    }
	}
	assert(sections_[idx].map);
    }

    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=.debug_%s map=0x%lx size=0x%lx\n",
		idx, secnames.to_name(idx),
		(unsigned long)sections_[idx].map,
		sections_[idx].size);
    }

    goto out;
error:
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	if (m->map)
	{
	    munmap(m->map, m->size);
	    m->map = NULL;
	}
    }
out:
    if (fd >= 0)
	close(fd);
    bfd_close(b);
}

class reader_t
{
public:
    reader_t()
     :  p_(0), end_(0), base_(0) {}

    reader_t(const void *base, size_t len)
     :  p_((unsigned char *)base),
        end_(((unsigned char *)base) + len),
        base_((unsigned char *)base)
    {}

    reader_t(const section_t &s)
     :  p_((unsigned char *)s.map),
        end_(((unsigned char *)s.map) + s.size),
        base_((unsigned char *)s.map)
    {}

    reader_t initial_subset(size_t len) const
    {
	size_t remain = (end_ - p_);
	if (len > remain)
	    len = remain;
	return reader_t((void *)p_, len);
    }

    unsigned long get_offset() const
    {
	return p_ - base_;
    }
    unsigned long get_remains() const
    {
	return end_ - p_;
    }

    bool skip(size_t n)
    {
	if (p_ + n > end_)
	{
	    return false;
	}
	p_ += n;
	return true;
    }

    bool read_u32(uint32_t &v)
    {
	if (p_ + 4 > end_)
	    return false;
	v = ((uint32_t)p_[0]) |
	    ((uint32_t)p_[1] << 8) |
	    ((uint32_t)p_[2] << 16) |
	    ((uint32_t)p_[3] << 24);
	p_ += 4;
	return true;
    }

    bool read_u64(uint64_t &v)
    {
	if (p_ + 8 > end_)
	    return false;
	v = ((uint64_t)p_[0]) |
	    ((uint64_t)p_[1] << 8) |
	    ((uint64_t)p_[2] << 16) |
	    ((uint64_t)p_[3] << 24) |
	    ((uint64_t)p_[4] << 32) |
	    ((uint64_t)p_[5] << 40) |
	    ((uint64_t)p_[6] << 48) |
	    ((uint64_t)p_[7] << 56);
	p_ += 8;
	return true;
    }

    bool read_uleb128(uint32_t &v)
    {
	const unsigned char *pp = p_;
	uint32_t vv = 0;
	unsigned shift = 0;
	do
	{
	    if (pp == end_)
		return false;
	    vv |= ((*pp) & 0x7f) << shift;
	    shift += 7;
	} while ((*pp++) & 0x80);
	p_ = pp;
	v = vv;
	return true;
    }

    bool read_sleb128(int32_t &v)
    {
	const unsigned char *pp = p_;
	uint32_t vv = 0;
	unsigned shift = 0;

	do
	{
	    if (pp == end_)
		return false;
	    vv |= ((*pp) & 0x7f) << shift;
	    shift += 7;
	} while ((*pp++) & 0x80);

	// sign extend the result, from the last encoded bit
	if (pp[-1] & 0x40)
	    v = vv | (~0U << shift);
	else
	    v = vv;
	p_ = pp;
	return true;
    }

    bool read_u16(uint16_t &v)
    {
	if (p_ + 2 > end_)
	    return false;
	v = ((uint16_t)p_[0]) |
	    ((uint16_t)p_[1] << 8);
	p_ += 2;
	return true;
    }

    bool read_u8(uint8_t &v)
    {
	if (p_ >= end_)
	    return false;
	v = *p_++;
	return true;
    }

    bool read_string(const char *&v)
    {
	const unsigned char *e =
	    (const unsigned char *)memchr(p_, '\0', (end_ - p_));
	if (!e)
	    return false;
	v = (const char *)p_;
	p_ = e + 1;
	return true;
    }

    bool read_bytes(const unsigned char *&v, size_t len)
    {
	const unsigned char *e = p_ + len;
	if (e >= end_)
	    return false;
	v = (const unsigned char *)p_;
	p_ = e;
	return true;
    }

private:
    const unsigned char *p_;
    const unsigned char *end_;
    const unsigned char *base_;
};

class compile_unit_t
{
private:
    enum { header_length = 11 };	    // this might depend on version
public:
    compile_unit_t()
    {}

    ~compile_unit_t()
    {}

    bool read_header(reader_t &r)
    {
	reader_ = r;	    // sample offset of start of header

	uint32_t length;
	uint16_t version;
	if (!r.read_u32(length))
	    return false;
	if (length > r.get_remains())
	    fatal("Bad DWARF compilation unit length %u", length);

	if (!r.read_u16(version))
	    return false;
	if (version != 2)
	    fatal("Bad DWARF version %u, expecting 2", version);
	if (length < header_length-6/*read so far*/)
	    fatal("Bad DWARF compilation unit length %u", length);

	uint8_t addrsize;
	if (!r.read_u32(abbrevs_) ||
	    !r.read_u8(addrsize))
	    return false;
	if (addrsize != sizeof(void*))
	    fatal("Bad DWARF addrsize %u, expecting %u",
		  addrsize, sizeof(void*));

	printf("compilation unit\n");
	printf("    length %u\n", (unsigned)length);
	printf("    version %u\n", (unsigned)version);
	printf("    abbrevs %u\n", (unsigned)abbrevs_);
	printf("    addrsize %u\n", (unsigned)addrsize);

	length += 4;	// account for the `length' field of the header

	// setup reader_ to point to the whole compile
	// unit but not any bytes of the next one
	reader_ = reader_.initial_subset(length);

	// skip the outer reader over the body
	r.skip(length - header_length);

	return true;
    }

    reader_t get_body_reader() const
    {
	reader_t r = reader_;
	r.skip(header_length);
	return r;
    }

    unsigned long get_abbrevs() const { return abbrevs_; }

private:
    reader_t reader_;	    // for whole including header
    uint32_t abbrevs_;
};

enum children_values
{
    DW_CHILDREN_no = 0,
    DW_CHILDREN_yes = 1
};

static const char * const _childvals[] = {
    "no", "yes", 0
};

static string_table_t childvals("DW_CHILDREN_", _childvals);

enum form_values
{
    DW_FORM_addr = 0x01,
    DW_FORM_block2 = 0x03,
    DW_FORM_block4 = 0x04,
    DW_FORM_data2 = 0x05,
    DW_FORM_data4 = 0x06,
    DW_FORM_data8 = 0x07,
    DW_FORM_string = 0x08,
    DW_FORM_block = 0x09,
    DW_FORM_block1 = 0x0a,
    DW_FORM_data1 = 0x0b,
    DW_FORM_flag = 0x0c,
    DW_FORM_sdata = 0x0d,
    DW_FORM_strp = 0x0e,
    DW_FORM_udata = 0x0f,
    DW_FORM_ref_addr = 0x10,
    DW_FORM_ref1 = 0x11,
    DW_FORM_ref2 = 0x12,
    DW_FORM_ref4 = 0x13,
    DW_FORM_ref8 = 0x14,
    DW_FORM_ref_udata = 0x15,
    DW_FORM_indirect = 0x16
};

static const char * const _formvals[] = {
    "",
    "addr", /* 0x01, */
    "",
    "block2", /* 0x03, */
    "block4", /* 0x04, */
    "data2", /* 0x05, */
    "data4", /* 0x06, */
    "data8", /* 0x07, */
    "string", /* 0x08, */
    "block", /* 0x09, */
    "block1", /* 0x0a, */
    "data1", /* 0x0b, */
    "flag", /* 0x0c, */
    "sdata", /* 0x0d, */
    "strp", /* 0x0e, */
    "udata", /* 0x0f, */
    "ref_addr", /* 0x10, */
    "ref1", /* 0x11, */
    "ref2", /* 0x12, */
    "ref4", /* 0x13, */
    "ref8", /* 0x14, */
    "ref_udata", /* 0x15, */
    "indirect", /* 0x16 */
    0
};

static string_table_t formvals("DW_FORM_", _formvals);

enum tag_names
{
    DW_TAG_array_type = 0x01,
    DW_TAG_class_type = 0x02,
    DW_TAG_entry_point = 0x03,
    DW_TAG_enumeration_type = 0x04,
    DW_TAG_formal_parameter = 0x05,
    DW_TAG_imported_declaration = 0x08,
    DW_TAG_label = 0x0a,
    DW_TAG_lexical_block = 0x0b,
    DW_TAG_member = 0x0d,
    DW_TAG_pointer_type = 0x0f,
    DW_TAG_reference_type = 0x10,
    DW_TAG_compile_unit = 0x11,
    DW_TAG_string_type = 0x12,
    DW_TAG_structure_type = 0x13,
    DW_TAG_subroutine_type = 0x15,
    DW_TAG_typedef = 0x16,
    DW_TAG_union_type = 0x17,
    DW_TAG_unspecified_parameters = 0x18,
    DW_TAG_variant = 0x19,
    DW_TAG_common_block = 0x1a,
    DW_TAG_common_inclusion = 0x1b,
    DW_TAG_inheritance = 0x1c,
    DW_TAG_inlined_subroutine = 0x1d,
    DW_TAG_module = 0x1e,
    DW_TAG_ptr_to_member_type = 0x1f,
    DW_TAG_set_type = 0x20,
    DW_TAG_subrange_type = 0x21,
    DW_TAG_with_stmt = 0x22,
    DW_TAG_access_declaration = 0x23,
    DW_TAG_base_type = 0x24,
    DW_TAG_catch_block = 0x25,
    DW_TAG_const_type = 0x26,
    DW_TAG_constant = 0x27,
    DW_TAG_enumerator = 0x28,
    DW_TAG_file_type = 0x29,
    DW_TAG_friend = 0x2a,
    DW_TAG_namelist = 0x2b,
    DW_TAG_namelist_item = 0x2c,
    DW_TAG_packed_type = 0x2d,
    DW_TAG_subprogram = 0x2e,
    DW_TAG_template_type_param = 0x2f,
    DW_TAG_template_value_param = 0x30,
    DW_TAG_thrown_type = 0x31,
    DW_TAG_try_block = 0x32,
    DW_TAG_variant_part = 0x33,
    DW_TAG_variable = 0x34,
    DW_TAG_volatile_type = 0x35,

// DW_TAG_lo_user = 0x4080,
// DW_TAG_hi_user = 0xffff
};

static const char * const _tagnames[] = {
    "",
    "array_type",	    /* 0x01, */
    "class_type",	    /* 0x02, */
    "entry_point",	    /* 0x03, */
    "enumeration_type",	    /* 0x04, */
    "formal_parameter",	    /* 0x05, */
    "",
    "",
    "imported_declaration", /* 0x08, */
    "",
    "label",		    /* 0x0a, */
    "lexical_block",	    /* 0x0b, */
    "",
    "member",		    /* 0x0d, */
    "",
    "pointer_type",	    /* 0x0f, */
    "reference_type",	    /* 0x10, */
    "compile_unit",	    /* 0x11, */
    "string_type",	    /* 0x12, */
    "structure_type",	    /* 0x13, */
    "",
    "subroutine_type",	    /* 0x15, */
    "typedef",		    /* 0x16, */
    "union_type",	    /* 0x17, */
    "unspecified_parameters", /* 0x18, */
    "variant",		    /* 0x19, */
    "common_block",	    /* 0x1a, */
    "common_inclusion",	    /* 0x1b, */
    "inheritance",	    /* 0x1c, */
    "inlined_subroutine",   /* 0x1d, */
    "module",		    /* 0x1e, */
    "ptr_to_member_type",   /* 0x1f, */
    "set_type",		    /* 0x20, */
    "subrange_type",	    /* 0x21, */
    "with_stmt",	    /* 0x22, */
    "access_declaration",   /* 0x23, */
    "base_type",	    /* 0x24, */
    "catch_block",	    /* 0x25, */
    "const_type",	    /* 0x26, */
    "constant",		    /* 0x27, */
    "enumerator",	    /* 0x28, */
    "file_type",	    /* 0x29, */
    "friend",		    /* 0x2a, */
    "namelist",		    /* 0x2b, */
    "namelist_item",	    /* 0x2c, */
    "packed_type",	    /* 0x2d, */
    "subprogram",	    /* 0x2e, */
    "template_type_param",  /* 0x2f, */
    "template_value_param", /* 0x30, */
    "thrown_type",	    /* 0x31, */
    "try_block",	    /* 0x32, */
    "variant_part",	    /* 0x33, */
    "variable",		    /* 0x34, */
    "volatile_type",	    /* 0x35, */
    0
};

static string_table_t tagnames("DW_TAG_", _tagnames);

enum attribute_names
{
    /* DWARF2 values */
    DW_AT_sibling = 0x01,
    DW_AT_location = 0x02,
    DW_AT_name = 0x03,
    DW_AT_ordering = 0x09,
    DW_AT_byte_size = 0x0b,
    DW_AT_bit_offset = 0x0c,
    DW_AT_bit_size = 0x0d,
    DW_AT_stmt_list = 0x10,
    DW_AT_low_pc = 0x11,
    DW_AT_high_pc = 0x12,
    DW_AT_language = 0x13,
    DW_AT_discr = 0x15,
    DW_AT_discr_value = 0x16,
    DW_AT_visibility = 0x17,
    DW_AT_import = 0x18,
    DW_AT_string_length = 0x19,
    DW_AT_common_reference = 0x1a,
    DW_AT_comp_dir = 0x1b,
    DW_AT_const_value = 0x1c,
    DW_AT_containing_type = 0x1d,
    DW_AT_default_value = 0x1e,
    DW_AT_inline = 0x20,
    DW_AT_is_optional = 0x21,
    DW_AT_lower_bound = 0x22,
    DW_AT_producer = 0x25,
    DW_AT_prototyped = 0x27,
    DW_AT_return_addr = 0x2a,
    DW_AT_start_scope = 0x2c,
    DW_AT_stride_size = 0x2e,
    DW_AT_upper_bound = 0x2f,
    DW_AT_abstract_origin = 0x31,
    DW_AT_accessibility = 0x32,
    DW_AT_address_class = 0x33,
    DW_AT_artificial = 0x34,
    DW_AT_base_types = 0x35,
    DW_AT_calling_convention = 0x36,
    DW_AT_count = 0x37,
    DW_AT_data_member_location = 0x38,
    DW_AT_decl_column = 0x39,
    DW_AT_decl_file = 0x3a,
    DW_AT_decl_line = 0x3b,
    DW_AT_declaration = 0x3c,
    DW_AT_discr_list = 0x3d,
    DW_AT_encoding = 0x3e,
    DW_AT_external = 0x3f,
    DW_AT_frame_base = 0x40,
    DW_AT_friend = 0x41,
    DW_AT_identifier_case = 0x42,
    DW_AT_macro_info = 0x43,
    DW_AT_namelist_item = 0x44,
    DW_AT_priority = 0x45,
    DW_AT_segment = 0x46,
    DW_AT_specification = 0x47,
    DW_AT_static_link = 0x48,
    DW_AT_type = 0x49,
    DW_AT_use_location = 0x4a,
    DW_AT_variable_parameter = 0x4b,
    DW_AT_virtuality = 0x4c,
    DW_AT_vtable_elem_location = 0x4d,

    /* DWARF3 values */
    /* TODO: actually check the std */
    DW_AT_allocated = 0x4e,
    DW_AT_associated = 0x4f,
    DW_AT_data_location = 0x50,
    DW_AT_byte_stride = 0x51,
    DW_AT_entry_pc = 0x52,
    DW_AT_use_UTF8 = 0x53,
    DW_AT_extension = 0x54,
    DW_AT_ranges = 0x55,
    DW_AT_trampoline = 0x56,
    DW_AT_call_column = 0x57,
    DW_AT_call_file = 0x58,
    DW_AT_call_line = 0x59,
    DW_AT_description = 0x5a,
    DW_AT_binary_scale = 0x5b,
    DW_AT_decimal_scale = 0x5c,
    DW_AT_small = 0x5d,
    DW_AT_decimal_sign = 0x5e,
    DW_AT_digit_count = 0x5f,
    DW_AT_picture_string = 0x60,
    DW_AT_mutable = 0x61,
    DW_AT_threads_scaled = 0x62,
    DW_AT_explicit = 0x63,
    DW_AT_object_pointer = 0x64,
    DW_AT_endianity = 0x65,
    DW_AT_elemental = 0x66,
    DW_AT_pure = 0x67,
    DW_AT_recursive = 0x68,

//     DW_AT_lo_user = 0x2000,
    DW_AT_wacky_mangled_name = 0x2007,	    // observed in the wild
//     DW_AT_hi_user = 0x3fff

    DW_AT_max
};

static const char * const _attrnames[] = {
    "",
    "sibling",		/* 0x01, */
    "location",		/* 0x02, */
    "name",		/* 0x03, */
    "",
    "",
    "",
    "",
    "",
    "ordering",		/* 0x09, */
    "",
    "byte_size",	/* 0x0b, */
    "bit_offset",	/* 0x0c, */
    "bit_size",		/* 0x0d, */
    "",
    "",
    "stmt_list",	/* 0x10, */
    "low_pc",		/* 0x11, */
    "high_pc",		/* 0x12, */
    "language",		/* 0x13, */
    "",
    "discr",		/* 0x15, */
    "discr_value",	/* 0x16, */
    "visibility",	/* 0x17, */
    "import",		/* 0x18, */
    "string_length",	/* 0x19, */
    "common_reference",	/* 0x1a, */
    "comp_dir",		/* 0x1b, */
    "const_value",	/* 0x1c, */
    "containing_type",	/* 0x1d, */
    "default_value",	/* 0x1e, */
    "",
    "inline",		/* 0x20, */
    "is_optional",	/* 0x21, */
    "lower_bound",	/* 0x22, */
    "",
    "",
    "producer",		/* 0x25, */
    "",
    "prototyped",	/* 0x27, */
    "",
    "",
    "return_addr",	/* 0x2a, */
    "",
    "start_scope",	/* 0x2c, */
    "",
    "stride_size",	/* 0x2e, */
    "upper_bound",	/* 0x2f, */
    "",
    "abstract_origin",	/* 0x31, */
    "accessibility",	/* 0x32, */
    "address_class",	/* 0x33, */
    "artificial",	/* 0x34, */
    "base_types",	/* 0x35, */
    "calling_convention",/* 0x36, */
    "count",		/* 0x37, */
    "data_member_location",/* 0x38, */
    "decl_column",	/* 0x39, */
    "decl_file",	/* 0x3a, */
    "decl_line",	/* 0x3b, */
    "declaration",	/* 0x3c, */
    "discr_list",	/* 0x3d, */
    "encoding",		/* 0x3e, */
    "external",		/* 0x3f, */
    "frame_base",	/* 0x40, */
    "friend",		/* 0x41, */
    "identifier_case",	/* 0x42, */
    "macro_info",	/* 0x43, */
    "namelist_item",	/* 0x44, */
    "priority",		/* 0x45, */
    "segment",		/* 0x46, */
    "specification",	/* 0x47, */
    "static_link",	/* 0x48, */
    "type",		/* 0x49, */
    "use_location",	/* 0x4a, */
    "variable_parameter",/* 0x4b, */
    "virtuality",	/* 0x4c, */
    "vtable_elem_location",/* 0x4d, */
    "allocated",	/* 0x4e, */
    "associated",	/* 0x4f, */
    "data_location",	/* 0x50, */
    "byte_stride",	/* 0x51, */
    "entry_pc",		/* 0x52, */
    "use_UTF8",		/* 0x53, */
    "extension",	/* 0x54, */
    "ranges",		/* 0x55, */
    "trampoline",	/* 0x56, */
    "call_column",	/* 0x57, */
    "call_file",	/* 0x58, */
    "call_line",	/* 0x59, */
    "description",	/* 0x5a, */
    "binary_scale",	/* 0x5b, */
    "decimal_scale",	/* 0x5c, */
    "small",		/* 0x5d, */
    "decimal_sign",	/* 0x5e, */
    "digit_count",	/* 0x5f, */
    "picture_string",	/* 0x60, */
    "mutable",		/* 0x61, */
    "threads_scaled",	/* 0x62, */
    "explicit",		/* 0x63, */
    "object_pointer",	/* 0x64, */
    "endianity",	/* 0x65, */
    "elemental",	/* 0x66, */
    "pure",		/* 0x67, */
    "recursive",	/* 0x68, */
    0,
};

static string_table_t attrnames("DW_AT_", _attrnames);

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

    bool read(reader_t &r)
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

    uint32_t code;
    uint32_t tag;
    uint8_t children;
    vector<attr_spec_t> attr_specs;
};

struct reference_t
{
    uint32_t cu;	// index into compile unit table
    uint32_t offset;	// byte offset from start of compile unit
};

struct value_t
{
    enum
    {
	T_UNDEF,
	T_STRING,
	T_SINT32,
	T_UINT32,
	T_SINT64,
	T_UINT64,
	T_BYTES,
	T_REF
    } type;
    union
    {
	const char *string;
	int32_t sint32;
	uint32_t uint32;
	int64_t sint64;
	uint64_t uint64;
	struct {
	    const unsigned char *buf;
	    size_t len;
	} bytes;
	uint64_t ref;
    } val;

    value_t() : type(T_UNDEF) {}

    static value_t make_string(const char *v)
    {
	value_t val;
	val.type = T_STRING;
	val.val.string = v;
	return val;
    }
    static value_t make_uint32(uint32_t v)
    {
	value_t val;
	val.type = T_UINT32;
	val.val.uint32 = v;
	return val;
    }
    static value_t make_sint32(int32_t v)
    {
	value_t val;
	val.type = T_SINT32;
	val.val.sint32 = v;
	return val;
    }
    static value_t make_uint64(uint64_t v)
    {
	value_t val;
	val.type = T_UINT64;
	val.val.uint64 = v;
	return val;
    }
    static value_t make_sint64(int64_t v)
    {
	value_t val;
	val.type = T_SINT64;
	val.val.sint64 = v;
	return val;
    }
    static value_t make_bytes(const unsigned char *b, size_t l)
    {
	value_t val;
	val.type = T_BYTES;
	val.val.bytes.buf = b;
	val.val.bytes.len = l;
	return val;
    }
    static value_t make_ref(uint64_t v)
    {
	value_t val;
	val.type = T_REF;
	val.val.ref = v;
	return val;
    }

    void dump() const
    {
	switch (type)
	{
	case T_UNDEF:
	    printf("<undef>");
	    break;
	case T_STRING:
	    printf("\"%s\"", val.string);
	    break;
	case T_SINT32:
	    printf("%d", val.sint32);
	    break;
	case T_UINT32:
	    printf("0x%x", val.uint32);
	    break;
	case T_SINT64:
	    printf("%lld", (long long)val.sint64);
	    break;
	case T_UINT64:
	    printf("0x%llx", (unsigned long long)val.uint64);
	    break;
	case T_BYTES:
	    {
		const unsigned char *u = val.bytes.buf;
		unsigned len = val.bytes.len;
		const char *sep = "";
		while (len--)
		{
		    printf("%s0x%02x", sep, (unsigned)*u++);
		    sep = " ";
		}
	    }
	    break;
	case T_REF:
	    printf("(ref)0x%llx", (unsigned long long)val.ref);
	    break;
	}
    }
};

class entry_t
{
public:
    entry_t()
     :  offset_(0),
	level_(0),
	abbrev_(0)
    {}

    void setup(size_t offset, unsigned level, const abbrev_t *a)
    {
	offset_ = offset;
	level_ = level;
	abbrev_ = a;
	byattr_.clear();
	byattr_.resize(DW_AT_max, 0);
	values_.clear();
	values_.reserve(a->attr_specs.size());
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

private:
    size_t offset_;
    unsigned level_;
    const abbrev_t *abbrev_;
    vector<value_t> values_;
    vector<const value_t*> byattr_;
};

void entry_t::dump() const
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


void
state_t::read_abbrevs(uint32_t offset)
{
    abbrevs_.clear();	    /* TODO: memleak */

    reader_t r(sections_[DW_sec_abbrev]);
    r.skip(offset);

    uint32_t code;
    /* code 0 indicates end of compile unit */
    while (r.read_uleb128(code) && code)
    {
	abbrev_t *a = new abbrev_t(code);
	if (!a->read(r))
	{
	    delete a;
	    break;
	}
	abbrevs_[a->code] = a;
    }
}


void
state_t::dump_abbrevs()
{
    printf("Abbrevs {\n");

    map<uint32_t, abbrev_t*>::iterator itr;
    for (itr = abbrevs_.begin() ; itr != abbrevs_.end() ; ++itr)
    {
	abbrev_t *a = itr->second;
	printf("Code %u\n", a->code);
	printf("    tag 0x%x (%s)\n", a->tag, tagnames.to_name(a->tag));
	printf("    children %u (%s)\n",
		(unsigned)a->children,
		childvals.to_name(a->children));
	printf("    attribute specifications {\n");

	vector<abbrev_t::attr_spec_t>::iterator i;
	for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
	{
	    printf("        name 0x%x (%s)",
		    i->name, attrnames.to_name(i->name));
	    printf(" form 0x%x (%s)\n",
		    i->form, formvals.to_name(i->form));
	}
	printf("    }\n");
    }
    printf("}\n");
}

class walker_t
{
public:
    walker_t(state_t &s, reader_t r)
     :  state_(s),
	reader_(r),
	level_(0)
    {
    }

    const entry_t &get_entry() const { return entry_; }

    bool move_to_sibling();
    bool move_to_children();
    bool move_preorder();
//     bool move_to(reference_t);

private:
    int read_entry();

    state_t &state_;
    reader_t reader_;
    entry_t entry_;
    unsigned level_;
};

int
walker_t::read_entry()
{
    /* Refs are offsets relative to the start of the
     * compile unit header, but the reader starts
     * at the first byte after that header. */
    size_t offset = reader_.get_offset();
    uint32_t acode;

    if (!reader_.read_uleb128(acode))
    {
	return EOF;    // end of section
    }

    if (!acode)
    {
	if (!level_)
	    return EOF;   // end of subtree in scope
	level_--;
	return 0;
    }

    abbrev_t *a = state_.abbrevs_[acode];
    if (!a)
    {
	// TODO: bad DWARF info - throw an exception
	fatal("XXX wtf - no abbrev for code 0x%x\n", acode);
    }

    entry_.setup(offset, level_, a);

    vector<abbrev_t::attr_spec_t>::iterator i;
    for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
    {
	switch (i->form)
	{
	case DW_FORM_data1:
	    {
		uint8_t v;
		if (!reader_.read_u8(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data2:
	    {
		uint16_t v;
		if (!reader_.read_u16(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data4:
	    {
		uint32_t v;
		if (!reader_.read_u32(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_data8:
	    {
		uint64_t v;
		if (!reader_.read_u64(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint64(v));
		break;
	    }
	case DW_FORM_udata:
	    {
		uint32_t v;
		if (!reader_.read_uleb128(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_sdata:
	    {
		int32_t v;
		if (!reader_.read_sleb128(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_sint32(v));
		break;
	    }
	case DW_FORM_addr:
	    {
		if (sizeof(unsigned long) == 4)
		{
		    uint32_t v;
		    if (!reader_.read_u32(v))
			return EOF;
		    entry_.add_attribute(i->name, value_t::make_uint32(v));
		}
		else if (sizeof(unsigned long) == 8)
		{
		    uint64_t v;
		    if (!reader_.read_u64(v))
			return EOF;
		    entry_.add_attribute(i->name, value_t::make_uint64(v));
		}
		else
		{
		    fatal("Strange addrsize %u", sizeof(unsigned long));
		}
		break;
	    }
	case DW_FORM_flag:
	    {
		uint8_t v;
		if (!reader_.read_u8(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_uint32(v));
		break;
	    }
	case DW_FORM_ref1:
	    {
		uint8_t off;
		if (!reader_.read_u8(off))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_ref(off));
		break;
	    }
	case DW_FORM_ref2:
	    {
		uint16_t off;
		if (!reader_.read_u16(off))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_ref(off));
		break;
	    }
	case DW_FORM_ref4:
	    {
		uint32_t off;
		if (!reader_.read_u32(off))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_ref(off));
		break;
	    }
	case DW_FORM_ref8:
	    {
		uint64_t off;
		if (!reader_.read_u64(off))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_ref(off));
		break;
	    }
	case DW_FORM_string:
	    {
		const char *v;
		if (!reader_.read_string(v))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_string(v));
		break;
	    }
	case DW_FORM_strp:
	    {
		uint32_t off;
		if (!reader_.read_u32(off))
		    return EOF;
		const char *v = state_.sections_[DW_sec_str].offset_as_string(off);
		if (!v)
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_string(v));
		break;
	    }
	case DW_FORM_block1:
	    {
		uint8_t len;
		const unsigned char *v;
		if (!reader_.read_u8(len) ||
		    !reader_.read_bytes(v, len))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block2:
	    {
		uint16_t len;
		const unsigned char *v;
		if (!reader_.read_u16(len) ||
		    !reader_.read_bytes(v, len))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block4:
	    {
		uint32_t len;
		const unsigned char *v;
		if (!reader_.read_u32(len) ||
		    !reader_.read_bytes(v, len))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	case DW_FORM_block:
	    {
		uint32_t len;
		const unsigned char *v;
		if (!reader_.read_uleb128(len) ||
		    !reader_.read_bytes(v, len))
		    return EOF;
		entry_.add_attribute(i->name, value_t::make_bytes(v, len));
		break;
	    }
	default:
	    // TODO: bad DWARF info - throw an exception
	    fatal("XXX can't handle %s\n",
		  formvals.to_name(i->form));
	}
    }

    if (a->children)
	level_++;

    return 1;
}

// level_ is the level relative to the very first entry
// at which the walker was initialised or seeked to,
// of the next entry to be read from the file

bool
walker_t::move_preorder()
{
    int r;
    do
    {
	if ((r = read_entry()) == EOF)
	    return false;
    } while (!r);
    return true;
}

bool
walker_t::move_to_sibling()
{
    // TODO: use the DW_AT_sibling attribute if present
    unsigned target_level = entry_.get_level();
    int r;
    do
    {
	if ((r = read_entry()) == EOF)
	    return false;
    } while (r && entry_.get_level() > target_level);
    return !!r;
}

bool
walker_t::move_to_children()
{
    return (entry_.has_children() && read_entry() != EOF);
}

void
state_t::dump_structs(walker_t &w)
{
    const char *keyword;

    w.move_to_sibling();
    w.move_to_children();

    do
    {
	switch (w.get_entry().get_tag())
	{
	case DW_TAG_structure_type: keyword = "struct"; break;
	case DW_TAG_union_type: keyword = "union"; break;
	case DW_TAG_class_type: keyword = "class"; break;
	default: continue;
	}

	const char *name = w.get_entry().get_string_attribute(DW_AT_name);
	if (!name)
	    continue;
	printf("ZZZ %s %s;\n", keyword, name);

	// print members
	if (!w.move_to_children())
	    continue;
	do
	{
	    const char *name = w.get_entry().get_string_attribute(DW_AT_name);
	    if (!name)
		continue;

	    switch (w.get_entry().get_tag())
	    {
	    case DW_TAG_member: keyword = "member"; break;
	    case DW_TAG_subprogram: keyword = "function"; break;
	    default: continue;
	    }
	    printf("    %s %s;\n", keyword, name);

	} while (w.move_to_sibling());

    } while (w.move_to_sibling());
}

#if 1
#if 1
static void
preorder_dump2(walker_t &w, unsigned depth)
{
    do
    {
	assert(w.get_entry().get_abbrev());
	assert(depth == w.get_entry().get_level());
	w.get_entry().dump();
	if (w.move_to_children())
	    preorder_dump2(w, depth+1);
    } while (w.move_to_sibling());
}

static void
preorder_dump(walker_t &w)
{
    w.move_to_sibling();
    preorder_dump2(w, 0);
}

#else
static void
preorder_dump(walker_t &w)
{
    while (w.move_preorder())
	w.get_entry().dump();
}

#endif
#endif

void
state_t::dump_info()
{
    reader_t r(sections_[DW_sec_info]);

    printf(".debug_info section:\n");

    printf("Compile units:\n");
    compile_unit_t cu;
    while (cu.read_header(r))
    {
	compile_units_.push_back(cu);
    }
    printf("\n");

    vector<compile_unit_t>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	read_abbrevs(i->get_abbrevs());
// 	dump_abbrevs();

	walker_t w(*this, i->get_body_reader());
	preorder_dump(w);

// 	walker_t w2(*this, i->get_body_reader());
// 	dump_structs(w2);

	printf("} compile_unit\n");
    }
    printf("\n\n");
}

// close namespaces
} }

#if 0

int
main(int argc, char **argv)
{
#define TESTCASE(in, out) \
    { \
	printf(". read_uleb128(%u) ", out); fflush(stdout); \
	spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	uint32_t v; \
	if (!r.read_uleb128(v) || v != out) \
	{ \
	    printf("FAIL\n"); \
	    return 1; \
	} \
	printf("PASS\n"); \
    }

    TESTCASE("\x02", 2);
    TESTCASE("\x7f", 127);
    TESTCASE("\x80\x01", 128);
    TESTCASE("\x81\x01", 129);
    TESTCASE("\x82\x01", 130);
    TESTCASE("\xb9\x64", 12857);
#undef TESTCASE

#define TESTCASE(in, out) \
    { \
	printf(". read_sleb128(%d) ", out); fflush(stdout); \
	spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	int32_t v; \
	if (!r.read_sleb128(v) || v != out) \
	{ \
	    printf("FAIL\n"); \
	    return 1; \
	} \
	printf("PASS\n"); \
    }

    TESTCASE("\x02", 2);
    TESTCASE("\x7e", -2);
    TESTCASE("\xff\x00", 127);
    TESTCASE("\x81\x7f", -127);
    TESTCASE("\x80\x01", 128);
    TESTCASE("\x80\x7f", -128);
    TESTCASE("\x81\x01", 129);
    TESTCASE("\xff\x7e", -129);

#undef TESTCASE
    return 0;
}


#else

int
main(int argc, char **argv)
{
    argv0 = argv[0];
    if (argc != 2)
	fatal("Usage: spiegel EXE\n");

    spiegel::dwarf::state_t state(argv[1]);
    state.map_sections();
    state.dump_info();
    return 0;
}

#endif
