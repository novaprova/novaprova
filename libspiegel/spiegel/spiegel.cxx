#include "spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/dwarf/walker.hxx"
#include "spiegel/dwarf/entry.hxx"
#include "spiegel/dwarf/enumerations.hxx"

namespace spiegel {
using namespace std;

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
	return name;
    case DW_TAG_pointer_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string() + " *";
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
		}
	    }
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
