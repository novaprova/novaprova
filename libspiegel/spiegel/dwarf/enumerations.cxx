#include "enumerations.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

static const char * const _secnames[DW_sec_num+1] = {
    "aranges", "pubnames", "info",
    "abbrev", "line", "frame",
    "str", "loc", "ranges", 0
};
string_table_t secnames(".debug_", _secnames);

static const char * const _childvals[] = {
    "no", "yes", 0
};
string_table_t childvals("DW_CHILDREN_", _childvals);

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
string_table_t formvals("DW_FORM_", _formvals);

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
string_table_t tagnames("DW_TAG_", _tagnames);

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
string_table_t attrnames("DW_AT_", _attrnames);

// close namespace
} }
