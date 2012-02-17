#include "spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/dwarf/walker.hxx"
#include "spiegel/dwarf/entry.hxx"
#include "spiegel/dwarf/enumerations.hxx"

namespace spiegel {
using namespace std;

unsigned int
type_t::get_classification() const
{
    if (ref_ == spiegel::dwarf::reference_t::null)
	return TC_VOID;

    spiegel::dwarf::walker_t w(ref_);
    const spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return TC_INVALID;	// TODO: exception

    switch (e->get_tag())
    {
    case DW_TAG_base_type:
	{
	    uint32_t encoding = e->get_uint32_attribute(DW_AT_encoding);
	    uint32_t byte_size = e->get_uint32_attribute(DW_AT_byte_size);
	    switch (encoding)
	    {
	    case DW_ATE_float:
		return TC_MAJOR_FLOAT|byte_size;
	    case DW_ATE_signed:
	    case DW_ATE_signed_char:
		return TC_MAJOR_INTEGER|_TC_SIGNED|byte_size;
	    case DW_ATE_unsigned:
	    case DW_ATE_unsigned_char:
		return TC_MAJOR_INTEGER|_TC_UNSIGNED|byte_size;
	    }
	    break;
	}
    case DW_TAG_typedef:
    case DW_TAG_volatile_type:
    case DW_TAG_const_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).get_classification();
    case DW_TAG_pointer_type:
	return TC_POINTER;
    case DW_TAG_reference_type:
	return TC_REFERENCE;
    case DW_TAG_structure_type:
	return TC_STRUCT;
    case DW_TAG_union_type:
	return TC_UNION;
    case DW_TAG_class_type:
	return TC_CLASS;
    case DW_TAG_enumeration_type:
	{
	    uint32_t byte_size = e->get_uint32_attribute(DW_AT_byte_size);
	    if (!byte_size)
		byte_size = sizeof(int);
	    return TC_MAJOR_INTEGER|_TC_SIGNED|byte_size;
	}
    case DW_TAG_array_type:
	return TC_ARRAY;
    }
    return TC_INVALID;	// TODO: exception
}

string
type_t::get_classification_as_string() const
{
    unsigned int cln = get_classification();
    if (cln == TC_INVALID)
	return "invalid";

    static const char * const major_names[] =
	{ "0", "void", "pointer", "array", "integer", "float", "compound", "7" };
    string s = major_names[(cln >> 8) & 0x7];

    if ((cln & _TC_MAJOR_MASK) == TC_MAJOR_INTEGER)
	s += (cln & _TC_SIGNED ? "|signed" : "|unsigned");

    cln &= ~(_TC_MAJOR_MASK|_TC_SIGNED);

    char buf[32];
    snprintf(buf, sizeof(buf), "|%u", cln);
    s += buf;

    return s;
}

unsigned int
type_t::get_sizeof() const
{
    if (ref_ == spiegel::dwarf::reference_t::null)
	return 0;   // sizeof(void)

    spiegel::dwarf::walker_t w(ref_);
    const spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return 0;	// TODO: exception

    uint32_t byte_size = e->get_uint32_attribute(DW_AT_byte_size);

    switch (e->get_tag())
    {
    case DW_TAG_array_type:
	if (!byte_size)
	{
	    uint32_t element_size = type_t(e->get_reference_attribute(DW_AT_type)).get_sizeof();
	    for (e = w.move_down() ; e ; e = w.move_next())
	    {
		uint32_t count;
		if (e->get_tag() == DW_TAG_subrange_type &&
		    ((count = e->get_uint32_attribute(DW_AT_count)) ||
		     (count = e->get_uint32_attribute(DW_AT_upper_bound))))
		{
		    byte_size = (byte_size ? byte_size : 1) * count;
		}
	    }
	    byte_size *= element_size;
	}
	break;
    case DW_TAG_typedef:
    case DW_TAG_volatile_type:
    case DW_TAG_const_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).get_sizeof();
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
	return sizeof(void*);
    }
    return byte_size;
}

string
type_t::to_string() const
{
    if (ref_ == spiegel::dwarf::reference_t::null)
	return "void";

    spiegel::dwarf::walker_t w(ref_);
    const spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return "???";

    const char *name = e->get_string_attribute(DW_AT_name);
    switch (e->get_tag())
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
	if (!name)
	    name = "wtf?";
	return name;
    case DW_TAG_pointer_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string() + " *";
    case DW_TAG_reference_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string() + " &";
    case DW_TAG_volatile_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string() + " volatile";
    case DW_TAG_const_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string() + " const";
    case DW_TAG_structure_type:
	return string("struct ") + (name ? name : "{...}");
    case DW_TAG_union_type:
	return string("union ") + (name ? name : "{...}");
    case DW_TAG_class_type:
	return string("class ") + (name ? name : "{...}");
    case DW_TAG_enumeration_type:
	return string("enum ") + (name ? name : "{...}");
    case DW_TAG_array_type:
	{
	    string s = type_t(e->get_reference_attribute(DW_AT_type)).to_string();
	    bool found = false;
	    for (e = w.move_down() ; e ; e = w.move_next())
	    {
		uint32_t count;
		if (e->get_tag() == DW_TAG_subrange_type &&
		    ((count = e->get_uint32_attribute(DW_AT_count)) ||
		     (count = e->get_uint32_attribute(DW_AT_upper_bound))))
		{
		    // TODO: unuglify this
		    char buf[32];
		    snprintf(buf, sizeof(buf), "[%u]", count);
		    s += buf;
		    found = true;
		}
	    }
	    if (!found)
		s += "[]";
	    return s;
	}
    case DW_TAG_subroutine_type:
	{
	    string s = "(";
	    s += type_t(e->get_reference_attribute(DW_AT_type)).to_string();
	    s += " foo(";
	    bool ellipsis = false;
	    unsigned int nparam = 0;
	    for (e = w.move_down() ; e ; e = w.move_next())
	    {
		if (!ellipsis && e->get_tag() == DW_TAG_formal_parameter)
		{
		    if (nparam++)
			s += ", ";
		    s += type_t(e->get_reference_attribute(DW_AT_type)).to_string();
		}
		else if (e->get_tag() == DW_TAG_unspecified_parameters)
		{
		    ellipsis = true;
		    if (nparam++)
			s += ", ";
		    s += "...";
		}
	    }
	    s += "))";
	    return s;
	}
    default:
	return spiegel::dwarf::tagnames.to_name(e->get_tag());
    }
}

vector<compile_unit_t *>
compile_unit_t::get_compile_units()
{
    vector<spiegel::compile_unit_t *> res;

    const vector<spiegel::dwarf::compile_unit_t*> &units =
	spiegel::dwarf::state_t::instance()->get_compile_units();
    vector<spiegel::dwarf::compile_unit_t*>::const_iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	compile_unit_t *cu = new compile_unit_t((*i)->make_root_reference());
	if (!cu->populate())
	    delete cu;
	else
	    res.push_back(cu);
    }
    return res;
}

bool
compile_unit_t::populate()
{
    spiegel::dwarf::walker_t w(ref_);
    // move to DW_TAG_compile_unit
    const spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return false;

    name_ = e->get_string_attribute(DW_AT_name);
    comp_dir_ = e->get_string_attribute(DW_AT_comp_dir);
    low_pc_ = e->get_uint64_attribute(DW_AT_low_pc);
    high_pc_ = e->get_uint64_attribute(DW_AT_high_pc);
    language_ = e->get_uint32_attribute(DW_AT_language);

#if 0
    printf("    name %s\n", name_);
    printf("    comp_dir %s\n", comp_dir_);
    printf("    low_pc 0x%llx\n", (unsigned long long)low_pc_);
    printf("    high_pc 0x%llx\n", (unsigned long long)high_pc_);
    printf("    language %u\n", (unsigned)language_);
#endif

    return true;
}

vector<function_t *>
compile_unit_t::get_functions()
{
    spiegel::dwarf::walker_t w(ref_);
    // move to DW_TAG_compile_unit
    w.move_next();

    vector<function_t *> res;

    // scan children of DW_TAG_compile_unit for functions
    for (const spiegel::dwarf::entry_t *e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() != DW_TAG_subprogram)
	    continue;

	const char *name = e->get_string_attribute(DW_AT_name);
	if (!name)
	    continue;

	function_t *fn = new function_t;
	if (!fn->populate(w))
	    delete fn;
	else
	    res.push_back(fn);
    }
    return res;
}

function_t::parameter_t::parameter_t(const spiegel::dwarf::entry_t *e)
{
    name = e->get_string_attribute(DW_AT_name),
    type = e->get_reference_attribute(DW_AT_type);
}

void
compile_unit_t::dump_types()
{
    spiegel::dwarf::walker_t w(ref_);
    w.move_next();
    for (const spiegel::dwarf::entry_t *e = w.move_down() ; e ; e = w.move_next())
    {
	// filter for all types
	const char *tagname = spiegel::dwarf::tagnames.to_name(e->get_tag());
	switch (e->get_tag())
	{
	case DW_TAG_base_type:
	case DW_TAG_reference_type:
	case DW_TAG_pointer_type:
	case DW_TAG_array_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	case DW_TAG_class_type:
	case DW_TAG_const_type:
	case DW_TAG_volatile_type:
	case DW_TAG_enumeration_type:
	case DW_TAG_subroutine_type:
	case DW_TAG_typedef:
	    break;
	case DW_TAG_subprogram:
	case DW_TAG_variable:
	    continue;
	default:
	    printf("    WTF??? %s\n", tagname);
	    continue;
	}
	printf("    Type %s at 0x%x\n", tagname, e->get_offset());
	type_t type(w.get_reference());
	printf("        to_string=\"%s\"\n", type.to_string().c_str());
	printf("        sizeof=%u\n", type.get_sizeof());
	printf("        classification=%u (%s)\n",
		type.get_classification(),
		type.get_classification_as_string().c_str());
    }
}

bool
function_t::populate(spiegel::dwarf::walker_t &w)
{
    const spiegel::dwarf::entry_t *e = w.get_entry();

    name_ = e->get_string_attribute(DW_AT_name);
    type_ = e->get_reference_attribute(DW_AT_type);

    for (e = w.move_down() ; e ; e = w.move_next())
    {
	if (!ellipsis_ && e->get_tag() == DW_TAG_formal_parameter)
	{
	    parameters_.push_back(parameter_t(e));
	}
	else if (e->get_tag() == DW_TAG_unspecified_parameters)
	{
	    ellipsis_ = true;
	}
    }

    return true;
}

type_t *
function_t::get_return_type() const
{
    return new type_t(type_);
}

vector<type_t*>
function_t::get_parameter_types() const
{
    vector<type_t *> res;

    vector<parameter_t>::const_iterator i;
    for (i = parameters_.begin() ; i != parameters_.end() ; ++i)
    {
	res.push_back(new type_t(i->type));
    }
    return res;
}

vector<const char *>
function_t::get_parameter_names() const
{
    vector<const char *> res;

    vector<parameter_t>::const_iterator i;
    for (i = parameters_.begin() ; i != parameters_.end() ; ++i)
    {
	res.push_back(i->name);
    }
    return res;
}

string
function_t::to_string() const
{
    int n = 0;
    string s;

    s += type_t(type_).to_string();
    s += " ";
    s += get_name();
    s += "(";

    vector<parameter_t>::const_iterator i;
    for (i = parameters_.begin() ; i != parameters_.end() ; ++i)
    {
	if (n++)
	    s += ", ";
	s += type_t(i->type).to_string();
    }

    if (ellipsis_)
    {
	if (n)
	    s += ", ";
	s += "...";
    }

    s += ")";
    return s;
}

// close namespace
};
