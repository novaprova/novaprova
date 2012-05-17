#include "u4c.h"
#include "u4c/testmanager.hxx"
#include "u4c/testnode.hxx"
#include "u4c/classifier.hxx"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"

namespace u4c {
using namespace std;

testmanager_t *testmanager_t::instance_ = 0;

testmanager_t::testmanager_t()
{
    assert(instance_ == 0);
    instance_ = this;
}

testmanager_t::~testmanager_t()
{
    while (classifiers_.size())
    {
	delete classifiers_.back();
	classifiers_.pop_back();
    }

    delete root_;
    root_ = 0;
    delete common_;
    common_ = 0;

    if (spiegel_)
	delete spiegel_;

    assert(instance_ == this);
    instance_ = 0;
}


testmanager_t *
testmanager_t::instance()
{
    if (!instance_)
    {
	new testmanager_t();
	instance_->setup_classifiers();
	instance_->discover_functions();
	instance_->setup_builtin_intercepts();
	/* TODO: check tree for a) leaves without FT_TEST
	 * and b) non-leaves with FT_TEST */
	instance_->root_->dump(0);
    }
    return instance_;
}

functype_t
testmanager_t::classify_function(const char *func,
				 char *match_return,
				 size_t maxmatch)
{
    if (match_return)
	match_return[0] = '\0';

    vector<classifier_t*>::iterator i;
    for (i = classifiers_.begin() ; i != classifiers_.end() ; ++i)
    {
	functype_t ft = (functype_t) (*i)->classify(func, match_return, maxmatch);
	if (ft != FT_UNKNOWN)
	    return ft;
	/* else, no match: just keep looking */
    }
    return FT_UNKNOWN;
}

void
testmanager_t::add_classifier(const char *re,
			      bool case_sensitive,
			      functype_t type)
{
    classifier_t *cl = new classifier_t;
    if (!cl->set_regexp(re, case_sensitive))
    {
	delete cl;
	return;
    }
    cl->set_results(FT_UNKNOWN, type);
    classifiers_.push_back(cl);
}

void
testmanager_t::setup_classifiers()
{
    add_classifier("^test_([a-z0-9].*)", false, FT_TEST);
    add_classifier("^[tT]est([A-Z].*)", false, FT_TEST);
    add_classifier("^[sS]etup$", false, FT_BEFORE);
    add_classifier("^set_up$", false, FT_BEFORE);
    add_classifier("^[iI]nit$", false, FT_BEFORE);
    add_classifier("^[tT]ear[dD]own$", false, FT_AFTER);
    add_classifier("^tear_down$", false, FT_AFTER);
    add_classifier("^[cC]leanup$", false, FT_AFTER);
    add_classifier("^mock_(.*)", false, FT_MOCK);
    add_classifier("^[mM]ock([A-Z].*)", false, FT_MOCK);
    add_classifier("^__u4c_parameter_(.*)", false, FT_PARAM);
}

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

spiegel::function_t *
testmanager_t::find_mock_target(string name)
{
    vector<spiegel::compile_unit_t *> units = spiegel::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	vector<spiegel::function_t *> fns = (*i)->get_functions();
	vector<spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    if ((*j)->get_name() == name)
		return *j;
	}
    }
    return 0;
}

static const struct __u4c_param_dec *
get_param_dec(spiegel::function_t *fn)
{
    vector<spiegel::value_t> args;
    spiegel::value_t ret = fn->invoke(args);
    return (const struct __u4c_param_dec *)ret.val.vpointer;
}

void
testmanager_t::discover_functions()
{
    if (!spiegel_)
    {
	spiegel_ = new spiegel::dwarf::state_t();
	spiegel_->add_self();
	root_ = new testnode_t(0);
    }
    // else: splice common_ and root_ back together

    vector<spiegel::compile_unit_t *> units = spiegel::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
// fprintf(stderr, "discover_functions: compile unit %s\n", (*i)->get_absolute_path().c_str());
	vector<spiegel::function_t *> fns = (*i)->get_functions();
	vector<spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    spiegel::function_t *fn = *j;
	    functype_t type;
	    char submatch[512];

	    // We want functions which are defined in this compile unit
	    if (!fn->get_address())
		continue;

	    type = classify_function(fn->get_name().c_str(),
				     submatch, sizeof(submatch));
	    switch (type)
	    {
	    case FT_UNKNOWN:
		continue;
	    case FT_TEST:
		// Test functions need a node name
		if (!submatch[0])
		    continue;
		// Test function return void
		if (fn->get_return_type()->get_classification() != spiegel::type_t::TC_VOID)
		    continue;
		// Test functions take no arguments
		if (fn->get_parameter_types().size() != 0)
		    continue;
		root_->make_path(test_name(fn, submatch))->set_function(type, fn);
		break;
	    case FT_BEFORE:
	    case FT_AFTER:
		// Before/after functions go into the parent node
		assert(!submatch[0]);
		// Before/after functions return int
		if (fn->get_return_type()->get_classification() != spiegel::type_t::TC_SIGNED_INT)
		    continue;
		// Before/after take no arguments
		if (fn->get_parameter_types().size() != 0)
		    continue;
		root_->make_path(test_name(fn, submatch))->set_function(type, fn);
		break;
	    case FT_MOCK:
		// Mock functions need a target name
		if (!submatch[0])
		    continue;
		{
		    spiegel::function_t *target = find_mock_target(submatch);
		    if (!target)
			continue;
		    root_->make_path(test_name(fn, 0))->add_mock(target, fn);
		}
		break;
	    case FT_PARAM:
		// Parameters need a name
		if (!submatch[0])
		    continue;
		const struct __u4c_param_dec *dec = get_param_dec(fn);
		root_->make_path(test_name(fn, 0))->add_parameter(
				submatch, dec->var, dec->values);
		break;
	    }
	}
    }

    // Calculate the effective root_ and common_
    common_ = root_;
    root_ = root_->detach_common();
}

extern void init_syslog_intercepts(testnode_t *);

void
testmanager_t::setup_builtin_intercepts()
{
    init_syslog_intercepts(root_);
}

// close the namespace
};
