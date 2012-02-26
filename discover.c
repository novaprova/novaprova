#include "common.h"
#include "u4c_priv.h"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
__u4c_discover_functions(u4c_globalstate_t *state)
{
    if (!state->spiegel)
    {
	state->spiegel = new spiegel::dwarf::state_t();
	state->spiegel->add_self();
    }

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
	    enum u4c_functype type;
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

	    type = state->classify_function(fn->get_name(),
					    submatch, sizeof(submatch));
	    if (type == FT_UNKNOWN)
		continue;
	    if (type == FT_TEST && !submatch[0])
		continue;

	    __u4c_add_function(state, type, fn, submatch);
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
