#include "spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"
#include "np/spiegel/dwarf/walker.hxx"
#include "np/spiegel/dwarf/entry.hxx"
#include "np/spiegel/dwarf/enumerations.hxx"
#include "np/spiegel/platform/common.hxx"

namespace np {
namespace spiegel {
using namespace std;
using namespace np::util;

value_t
value_t::make_invalid()
{
    value_t v;
    memset(&v, 0, sizeof(v));
    return v;
}

value_t
value_t::make_void()
{
    value_t v;
    memset(&v, 0, sizeof(v));
    v.which = type_t::TC_VOID;
    return v;
}

value_t
value_t::make_sint(int64_t i)
{
    value_t v;
    memset(&v, 0, sizeof(v));
    v.which = type_t::TC_SIGNED_LONG_LONG;
    v.val.vsint = i;
    return v;
}

value_t
value_t::make_sint(int32_t i)
{
    value_t v;
    memset(&v, 0, sizeof(v));
    v.which = type_t::TC_SIGNED_LONG_LONG;
    v.val.vsint = i;
    return v;
}

value_t
value_t::make_pointer(void *p)
{
    value_t v;
    memset(&v, 0, sizeof(v));
    v.which = type_t::TC_POINTER;
    v.val.vpointer = p;
    return v;
}

unsigned int
type_t::get_classification() const
{
    if (ref_ == np::spiegel::dwarf::reference_t::null)
	return TC_VOID;

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
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
    if (ref_ == np::spiegel::dwarf::reference_t::null)
	return 0;   // sizeof(void)

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
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
	return SPIEGEL_ADDRSIZE;
    }
    return byte_size;
}

string
type_t::get_name() const
{
    if (ref_ == np::spiegel::dwarf::reference_t::null)
	return "";

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return "wtf?";

    const char *name = e->get_string_attribute(DW_AT_name);
    switch (e->get_tag())
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_class_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_namespace_type:
	return (name ? name : "");
	break;
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_volatile_type:
    case DW_TAG_const_type:
    case DW_TAG_array_type:
    case DW_TAG_subroutine_type:
	return "";
    default:
	return np::spiegel::dwarf::tagnames.to_name(e->get_tag());
    }
}

string
type_t::to_string(string inner) const
{
    if (ref_ == np::spiegel::dwarf::reference_t::null)
	return (inner.length() ? "void " + inner : "void");

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return (inner.length() ? "wtf? " + inner : "wtf?");

    string s;

    const char *name = e->get_string_attribute(DW_AT_name);
    switch (e->get_tag())
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
	s = (name ? name : "wtf?");
	break;
    case DW_TAG_pointer_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string("*" + inner);
    case DW_TAG_reference_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string("&" + inner);
    case DW_TAG_volatile_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string("volatile " + inner);
    case DW_TAG_const_type:
	return type_t(e->get_reference_attribute(DW_AT_type)).to_string("const " + inner);
    case DW_TAG_structure_type:
	s = string("struct ") + (name ? name : "{...}");
	break;
    case DW_TAG_union_type:
	s = string("union ") + (name ? name : "{...}");
	break;
    case DW_TAG_class_type:
	s = string("class ") + (name ? name : "{...}");
	break;
    case DW_TAG_enumeration_type:
	s = string("enum ") + (name ? name : "{...}");
	break;
    case DW_TAG_namespace_type:
	s = string("namespace ") + (name ? name : "{...}");
	break;
    case DW_TAG_array_type:
	{
	    type_t element_type(e->get_reference_attribute(DW_AT_type));
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
		    inner += buf;
		    found = true;
		}
	    }
	    if (!found)
		inner += "[]";
	    return element_type.to_string(inner);
	}
    case DW_TAG_subroutine_type:
	{
	    // TODO: elide the parenethese around inner if it's an identifier
	    type_t return_type(e->get_reference_attribute(DW_AT_type));
	    inner = "(" + inner + ")(";
	    bool ellipsis = false;
	    unsigned int nparam = 0;
	    for (e = w.move_down() ; e ; e = w.move_next())
	    {
		if (!ellipsis && e->get_tag() == DW_TAG_formal_parameter)
		{
		    if (nparam++)
			inner += ", ";
		    const char *param = e->get_string_attribute(DW_AT_name);
		    if (!param)
			param = "";
		    inner += type_t(e->get_reference_attribute(DW_AT_type)).to_string(param);
		}
		else if (e->get_tag() == DW_TAG_unspecified_parameters)
		{
		    ellipsis = true;
		    if (nparam++)
			inner += ", ";
		    inner += "...";
		}
	    }
	    inner += ")";
	    return return_type.to_string(inner);
	}
    default:
	s = np::spiegel::dwarf::tagnames.to_name(e->get_tag());
	break;
    }
    if (inner.length())
	s += " " + inner;
    return s;
}

string
type_t::to_string() const
{
    return to_string(string(""));
}

filename_t
compile_unit_t::get_absolute_path() const
{
    return filename_t(name_).make_absolute_to_dir(filename_t(comp_dir_));
}

vector<compile_unit_t *>
get_compile_units()
{
    vector<np::spiegel::compile_unit_t *> res;

    const vector<np::spiegel::dwarf::compile_unit_t*> &units =
	np::spiegel::dwarf::state_t::instance()->get_compile_units();
    vector<np::spiegel::dwarf::compile_unit_t*>::const_iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	compile_unit_t *cu = _cacher_t::make_compile_unit((*i)->make_root_reference());
	if (cu)
	    res.push_back(cu);
    }
    return res;
}

bool
compile_unit_t::populate()
{
    np::spiegel::dwarf::walker_t w(ref_);
    // move to DW_TAG_compile_unit
    const np::spiegel::dwarf::entry_t *e = w.move_next();
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
    np::spiegel::dwarf::walker_t w(ref_);
    // move to DW_TAG_compile_unit
    w.move_next();

    vector<function_t *> res;

    // scan children of DW_TAG_compile_unit for functions
    for (const np::spiegel::dwarf::entry_t *e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() != DW_TAG_subprogram ||
	    !e->get_string_attribute(DW_AT_name))
	    continue;

	res.push_back(_cacher_t::make_function(w));
    }
    return res;
}

const char *
compile_unit_t::get_executable() const
{
    np::spiegel::dwarf::compile_unit_t *cu =
	np::spiegel::dwarf::state_t::instance()->get_compile_unit(ref_);
    return cu->get_executable();
}

void
compile_unit_t::dump_types()
{
    np::spiegel::dwarf::walker_t w(ref_);
    w.move_next();
    for (const np::spiegel::dwarf::entry_t *e = w.move_down() ; e ; e = w.move_next())
    {
	// filter for all types
	const char *tagname = np::spiegel::dwarf::tagnames.to_name(e->get_tag());
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

member_t::member_t(np::spiegel::dwarf::walker_t &w)
 :  _cacheable_t(w.get_reference()),
    name_(w.get_entry()->get_string_attribute(DW_AT_name))
{
    if (!name_)
	name_ = "";
}

const compile_unit_t *
member_t::get_compile_unit() const
{
    np::spiegel::dwarf::state_t *state = np::spiegel::dwarf::state_t::instance();
    np::spiegel::dwarf::compile_unit_t *dcu = state->get_compile_unit(ref_);
    return _cacher_t::make_compile_unit(dcu->make_root_reference());
}

string
function_t::get_full_name() const
{
    np::spiegel::dwarf::state_t *state = np::spiegel::dwarf::state_t::instance();
    return state->get_full_name(ref_);
}

type_t *
function_t::get_return_type() const
{
    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    return _cacher_t::make_type(e->get_reference_attribute(DW_AT_type));
}

vector<type_t*>
function_t::get_parameter_types() const
{
    vector<type_t *> res;

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    for (e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() == DW_TAG_formal_parameter)
	    res.push_back(_cacher_t::make_type(e->get_reference_attribute(DW_AT_type)));
	else if (e->get_tag() == DW_TAG_unspecified_parameters)
	    break;
    }
    return res;
}

vector<const char *>
function_t::get_parameter_names() const
{
    vector<const char *> res;

    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    for (e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() == DW_TAG_formal_parameter)
	{
	    const char *name = e->get_string_attribute(DW_AT_name);
	    if (!name || !*name)
		name = "<unknown>";
	    res.push_back(name);
	}
	else if (e->get_tag() == DW_TAG_unspecified_parameters)
	    break;
    }
    return res;
}

bool
function_t::has_unspecified_parameters() const
{
    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    for (e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() == DW_TAG_unspecified_parameters)
	    return true;
    }
    return false;
}

string
function_t::to_string() const
{
    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    type_t return_type(e->get_reference_attribute(DW_AT_type));
    string inner = name_;
    inner += "(";
    unsigned int nparam = 0;
    for (e = w.move_down() ; e ; e = w.move_next())
    {
	if (e->get_tag() == DW_TAG_formal_parameter)
	{
	    if (nparam++)
		inner += ", ";
	    const char *param = e->get_string_attribute(DW_AT_name);
	    if (!param)
		param = "";
	    inner += type_t(e->get_reference_attribute(DW_AT_type)).to_string(param);
	}
	else if (e->get_tag() == DW_TAG_unspecified_parameters)
	{
	    if (nparam++)
		inner += ", ";
	    inner += "...";
	    break;
	}
    }
    inner += ")";
    return return_type.to_string(inner);
}

// Return the address of the function, or 0 if the function is not
// defined in this compile unit (in which case, good luck finding it in
// some other compile unit).
addr_t
function_t::get_address() const
{
    np::spiegel::dwarf::walker_t w(ref_);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    return e->get_address_attribute(DW_AT_low_pc);
}

#if SPIEGEL_DYNAMIC
value_t
function_t::invoke(vector<value_t> args __attribute__((unused))) const
{
    // TODO: check that DW_AT_calling_convention == DW_CC_normal
    // TODO: check that we're talking to self
    addr_t addr = get_address();
    if (!addr)
	return value_t::make_invalid();

    // Hacky special cases, enough to get NP working without
    // writing the general purpose platform ABI invoke()
    if (get_parameter_types().size() > 0)
	return value_t::make_invalid();
    switch (get_return_type()->get_classification())
    {
    case type_t::TC_VOID:
	((void (*)(void))addr)();
	return value_t::make_void();
    case type_t::TC_SIGNED_INT:
	return value_t::make_sint(((int32_t (*)(void))addr)());
    case type_t::TC_POINTER:
	return value_t::make_pointer(((void *(*)(void))addr)());
    }
    return value_t::make_invalid();
//     return np::spiegel::platform::invoke(addr, args);
}
#endif

bool describe_address(addr_t addr, class location_t &loc)
{
    np::spiegel::dwarf::state_t *state = np::spiegel::dwarf::state_t::instance();

    np::spiegel::dwarf::reference_t curef;
    np::spiegel::dwarf::reference_t funcref;
    if (!state->describe_address(addr, curef, loc.line_,
				 funcref, loc.offset_))
	return false;

    loc.compile_unit_ = _cacher_t::make_compile_unit(curef);
    loc.function_ = _cacher_t::make_function(funcref);
    return true;
}

map<np::spiegel::dwarf::reference_t, _cacheable_t*> _cacher_t::cache_;

_cacheable_t *
_cacher_t::find(np::spiegel::dwarf::reference_t ref)
{
    std::map<np::spiegel::dwarf::reference_t, _cacheable_t*>::const_iterator i = cache_.find(ref);
    if (i == cache_.end())
	return 0;
    return i->second;
}

_cacheable_t *
_cacher_t::add(_cacheable_t *cc)
{
    cache_[cc->ref_] = cc;
    return cc;
}

compile_unit_t *
_cacher_t::make_compile_unit(np::spiegel::dwarf::reference_t ref)
{
    if (ref == np::spiegel::dwarf::reference_t::null)
	return 0;
    _cacheable_t *cc = find(ref);
    if (!cc)
	cc = add(new compile_unit_t(ref));
    compile_unit_t *cu = (compile_unit_t *)cc;
    if (!cu->populate())
    {
	delete cu;
	return 0;
    }
    return cu;
}

type_t *
_cacher_t::make_type(np::spiegel::dwarf::reference_t ref)
{
    _cacheable_t *cc = find(ref);
    if (!cc)
	cc = add(new type_t(ref));
    return (type_t *)cc;
}

function_t *
_cacher_t::make_function(np::spiegel::dwarf::walker_t &w)
{
    _cacheable_t *cc = find(w.get_reference());
    if (!cc)
	cc = add(new function_t(w));
    return (function_t *)cc;
}

function_t *
_cacher_t::make_function(np::spiegel::dwarf::reference_t ref)
{
    if (ref == np::spiegel::dwarf::reference_t::null)
	return 0;
    np::spiegel::dwarf::walker_t w(ref);
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    return (e ? make_function(w) : 0);
}

std::string describe_stacktrace()
{
    string s;
    vector<addr_t> stack = np::spiegel::platform::get_stacktrace();
    vector<addr_t>::iterator i;
    bool first = true;
    bool done = false;
    for (i = stack.begin() ; !done && i != stack.end() ; ++i)
    {
	s += (first ? "at " : "by ");
	s += HEX(*i);
	s += ":";
	location_t loc;
	if (describe_address(*i, loc))
	{
	    if (loc.function_)
	    {
		s += " ";
		s += loc.function_->get_full_name();
	    }

	    if (loc.compile_unit_)
	    {
		s += " (";
		s += loc.compile_unit_->get_name();
		if (loc.line_)
		{
		    s += ":";
		    s += dec(loc.line_);
		}
		s += ")";
	    }
	    if (loc.function_ && loc.function_->get_name() == "main")
		done = true;
	}
	s += "\n";
	first = false;
    }
    return s;
}

// close the namespaces
}; };
