#include "common.h"
#include "u4c_priv.h"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static string
test_name(spiegel::function_t *fn, char *submatch)
{
    string name = fn->get_compile_unit()->get_absolute_path();

    /* strip the .c or .cxx extension */
    size_t p = name.find_last_of('.');
    if (p != string::npos)
	name.resize(p);

    if (submatch && submatch[0])
    {
	name += "/";
	name += submatch;
    }

    return name;
}

void
u4c_globalstate_t::discover_functions()
{
    if (!spiegel)
    {
	spiegel = new spiegel::dwarf::state_t();
	spiegel->add_self();
	root_ = new u4c_testnode_t(0);
    }
    // else: splice common_ and root_ back together

    vector<spiegel::compile_unit_t *> units = spiegel::compile_unit_t::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
fprintf(stderr, "__u4c_discover_functions: scanning %s\n", (*i)->get_absolute_path().c_str());
	vector<spiegel::function_t *> fns = (*i)->get_functions();
	vector<spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    spiegel::function_t *fn = *j;
	    u4c::functype_t type;
	    char submatch[512];

	    // We want functions which are defined in this compile unit
	    if (!fn->get_address())
		continue;

	    // We want functions returning void or int
	    switch (fn->get_return_type()->get_classification())
	    {
	    case spiegel::type_t::TC_VOID:
	    case spiegel::type_t::TC_SIGNED_INT:
		break;
	    default:
		continue;
	    }

	    // We want functions taking no arguments
	    if (fn->get_parameter_types().size() != 0)
		continue;

	    type = classify_function(fn->get_name(),
				     submatch, sizeof(submatch));
	    if (type == u4c::FT_UNKNOWN)
		continue;
	    if (type == u4c::FT_TEST && !submatch[0])
		continue;

	    root_->make_path(test_name(fn, submatch))->set_function(type, fn);
	}
    }

    // Calculate the effective root_ and common_
    common_ = root_;
    root_ = root_->detach_common();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
