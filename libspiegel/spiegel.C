#include "commonp.H"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <bfd.h>
#include <map>
#include <vector>

namespace spiegel {
namespace dwarf {

using namespace std;

class string_table
{
public:
    string_table(const char *prefix, const char * const *names)
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

static string_table secnames(".debug_", _secnames);

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

struct section
{
    unsigned long offset;
    unsigned long size;
    void *map;

    unsigned long get_end() const { return offset + size; }
    void set_end(unsigned long e) { size = e - offset; }
    bool contains(const section &o)
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
	const struct section **s1 = (const struct section **)v1;
	const struct section **s2 = (const struct section **)v2;
	return u64cmp((*s1)->offset, (*s2)->offset);
    }
};

static struct section sections[DW_sec_num];
static struct section mappings[DW_sec_num];
static int nmappings;

static void
map_sections(const char *filename)
{
    int fd = -1;

    bfd_init();

    /* Open a BFD */
    bfd *b = bfd_openr(filename, NULL);
    if (!b)
    {
	bfd_perror(filename);
	return;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	fprintf(stderr, "spiegel: %s: not an object\n", filename);
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
	sections[idx].offset = (unsigned long)sec->filepos;
	sections[idx].size = (unsigned long)sec->size;
    }

    /* Coalesce sections into a minimal number of mappings */
    struct section *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	tsec[idx] = &sections[idx];
    qsort(tsec, DW_sec_num, sizeof(struct section *),
	  section::compare_ptrs);
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	if (nmappings &&
	    page_round_up(mappings[nmappings-1].get_end()) >= tsec[idx]->offset)
	{
	    /* section abuts the last mapping, extend it */
	    mappings[nmappings-1].set_end(tsec[idx]->get_end());
	}
	else
	{
	    /* create a new mapping */
	    mappings[nmappings++] = *tsec[idx];
	}
    }

    printf("Mappings:\n");
    for (int m = 0 ; m < nmappings ; m++)
    {
	printf("[%d] offset=%lx size=%lx end=%lx\n",
	    m, mappings[m].offset, mappings[m].size,
	    mappings[m].get_end());
    }

    /* mmap() the mappings */
    fd = open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(filename);
	goto error;
    }

    for (int m = 0 ; m < nmappings ; m++)
    {
	mappings[m].expand_to_pages();
	void *map = mmap(NULL, mappings[m].size,
			 PROT_READ, MAP_SHARED, fd,
			 mappings[m].offset);
	if (map == MAP_FAILED)
	{
	    perror("mmap");
	    goto error;
	}
	mappings[m].map = map;
    }

    /* setup sections[].map */
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	for (int m = 0 ; m < nmappings ; m++)
	{
	    if (mappings[m].contains(sections[idx]))
	    {
		sections[idx].map = (void *)(
		    (char *)mappings[m].map +
		    sections[idx].offset -
		    mappings[m].offset);
		break;
	    }
	}
	assert(sections[idx].map);
    }

    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=.debug_%s map=0x%lx size=0x%lx\n",
		idx, secnames.to_name(idx),
		(unsigned long)sections[idx].map,
		sections[idx].size);
    }

    goto out;
error:
    for (int m = 0 ; m < nmappings ; m++)
    {
	if (mappings[m].map)
	{
	    munmap(mappings[m].map, mappings[m].size);
	    mappings[m].map = NULL;
	}
    }
out:
    if (fd >= 0)
	close(fd);
    bfd_close(b);
}

class reader
{
public:
    reader()
     :  p_(0), end_(0) {}

    reader(void *base, size_t len)
     :  p_((unsigned char *)base),
        end_(((unsigned char *)base) + len)
    {}

    reader(const section &s)
     :  p_((unsigned char *)s.map),
        end_(((unsigned char *)s.map) + s.size)
    {}

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
	v = (p_[0]) |
	    (p_[1] << 8) |
	    (p_[2] << 16) |
	    (p_[3] << 24);
	p_ += 4;
	return true;
    }

    bool read_uleb128(uint32_t &v)
    {
	unsigned char *pp = p_;
	uint32_t vv = 0;
	for (;;)
	{
	    if (pp == end_)
		return false;
	    vv = (vv << 7) | ((*pp) & 0x7f);
	    if (!((*pp++) & 0x80))
		break;
	}
	p_ = pp;
	v = vv;
	return true;
    }

    bool read_u16(uint16_t &v)
    {
	if (p_ + 2 > end_)
	    return false;
	v = (p_[0]) |
	    (p_[1] << 8);
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

private:
    unsigned char *p_;
    unsigned char *end_;
};

struct info_compilation_unit_header
{
    uint32_t length;
    uint16_t version;
    uint32_t abbrevs;
    uint8_t addrsize;

    bool read(reader &r)
    {
	return (r.read_u32(length) &&
		r.read_u16(version) &&
		r.read_u32(abbrevs) &&
		r.read_u8(addrsize));
    };
};

enum children_values
{
    DW_CHILDREN_no = 0,
    DW_CHILDREN_yes = 1
};

static const char * const _childvals[] = {
    "no", "yes", 0
};

static string_table childvals("DW_CHILDREN_", _childvals);

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

static string_table formvals("DW_FORM_", _formvals);

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

static string_table tagnames("DW_TAG_", _tagnames);

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
//     DW_AT_hi_user = 0x3fff
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

static string_table attrnames("DW_AT_", _attrnames);

struct abbrev
{
    struct attr_spec
    {
	uint32_t name;
	uint32_t form;
    };

    // default c'tor
    abbrev()
     :  code(0),
        tag(0),
        children(0)
    {}

    // c'tor with code
    abbrev(uint32_t c)
     :  code(c),
        tag(0),
	children(0)
    {}

    bool read(reader &r)
    {
	if (!r.read_uleb128(tag) ||
	    !r.read_u8(children))
	    return false;
	for (;;)
	{
	    attr_spec as;
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
    vector<attr_spec> attr_specs;
};

static void
read_abbrevs(uint32_t offset, map<uint32_t, abbrev*> &table)
{
    reader r(sections[DW_sec_abbrev]);
    r.skip(offset);

    uint32_t code;
    /* code 0 indicates end of compilation unit */
    while (r.read_uleb128(code) && code)
    {
	abbrev *a = new abbrev(code);
	if (!a->read(r))
	{
	    delete a;
	    break;
	}
	table[a->code] = a;
    }
}


static void
dump_abbrevs(map<uint32_t, abbrev*> &table)
{
    printf("Abbrevs {\n");

    map<uint32_t, abbrev*>::iterator itr;
    for (itr = table.begin() ; itr != table.end() ; ++itr)
    {
	abbrev *a = itr->second;
	printf("Code %u\n", a->code);
	printf("    tag 0x%x (%s)\n", a->tag, tagnames.to_name(a->tag));
	printf("    children %u (%s)\n",
		(unsigned)a->children,
		childvals.to_name(a->children));
	printf("    attribute specifications {\n");

	vector<abbrev::attr_spec>::iterator i;
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

static void
dump_info(void)
{
    reader r(sections[DW_sec_info]);

    printf(".debug_info section:\n");
    info_compilation_unit_header cuh;
    while (cuh.read(r))
    {
	if (cuh.version != 2)
	    fatal("Bad DWARF version %u, expecting 2", cuh.version);
	if (cuh.addrsize != sizeof(void*))
	    fatal("Bad DWARF addrsize %u, expecting %u",
		  cuh.addrsize, sizeof(void*));
	printf("compilation unit\n");
	printf("    length %u\n", (unsigned)cuh.length);
	printf("    version %u\n", (unsigned)cuh.version);
	printf("    abbrevs %u\n", (unsigned)cuh.abbrevs);
	printf("    addrsize %u\n", (unsigned)cuh.addrsize);

	map<uint32_t, abbrev*> abbrevs;
	read_abbrevs(cuh.abbrevs, abbrevs);
	dump_abbrevs(abbrevs);

	r.skip(cuh.length-7);
    }
    printf("\n\n");
}

// close namespaces
} }

int
main(int argc, char **argv)
{
    argv0 = argv[0];
    if (argc != 2)
	fatal("Usage: spiegel EXE\n");
    spiegel::dwarf::map_sections(argv[1]);
    spiegel::dwarf::dump_info();
    return 0;
}

