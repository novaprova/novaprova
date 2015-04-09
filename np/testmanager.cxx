/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "np.h"
#include "np/testmanager.hxx"
#include "np/testnode.hxx"
#include "np/classifier.hxx"
#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"

namespace np {
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

    decorator_descs_.clear();

    delete root_;
    if (common_ != root_)
	delete common_;
    root_ = 0;
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
#if _NP_DEBUG
	fprintf(stderr, "np: creating testmanager_t instance\n");
#endif
	new testmanager_t();
	instance_->print_banner();
	instance_->setup_classifiers();
	instance_->discover_functions();
	instance_->setup_builtin_intercepts();
	/* TODO: check tree for a) leaves without FT_TEST
	 * and b) non-leaves with FT_TEST */
// 	instance_->root_->dump(0);
    }
    return instance_;
}

void
testmanager_t::print_banner()
{
    printf("np: NovaProva Copyright (c) Gregory Banks\n");
    printf("np: Built for O/S "_NP_OS" architecture "_NP_ARCH"\n");
    fflush(stdout);
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
#if _NP_DEBUG
    fprintf(stderr, "np: adding classifier /%s/%s -> %s\n",
	    re, (case_sensitive ? "i" : ""), np::as_string(type));
#endif
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
    add_classifier("^__np_parameter_(.*)", false, FT_PARAM);
    add_classifier("^__np_decorator_(.*)", false, FT_DECORATOR);
}

static string
test_name(np::spiegel::function_t *fn, char *submatch)
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

np::spiegel::function_t *
testmanager_t::find_mock_target(string name)
{
    vector<np::spiegel::compile_unit_t *> units = np::spiegel::get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	vector<np::spiegel::function_t *> fns = (*i)->get_functions();
	vector<np::spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    if ((*j)->get_name() == name)
		return *j;
	}
    }
    return 0;
}

static void *
call_func(np::spiegel::function_t *fn)
{
    vector<np::spiegel::value_t> args;
    np::spiegel::value_t ret = fn->invoke(args);
    return ret.val.vpointer;
}

int
testmanager_t::decorator_desc_t::compare(const decorator_desc_t *a,
					 const decorator_desc_t *b)
{
    int r = a->priority_ - b->priority_;
    /* force a stable ordering which is not sensitive to link order */
    if (!r)
	r = strcmp(a->name_.c_str(), b->name_.c_str());
    return r;
}


void
testmanager_t::discover_functions()
{
    if (!spiegel_)
    {
#if _NP_DEBUG
	fprintf(stderr, "np: creating np::spiegel::dwarf::state_t instance\n");
#endif
	spiegel_ = new np::spiegel::dwarf::state_t();
	spiegel_->add_self();
	root_ = new testnode_t(0);
    }
    // else: splice common_ and root_ back together

#if _NP_DEBUG
    fprintf(stderr, "np: scanning for test functions\n");
#endif
    vector<np::spiegel::compile_unit_t *> units = np::spiegel::get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    unsigned int ntests = 0;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
#if _NP_DEBUG
	fprintf(stderr, "np: scanning compile unit %s\n", (*i)->get_absolute_path().c_str());
#endif
	vector<np::spiegel::function_t *> fns = (*i)->get_functions();
	vector<np::spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    np::spiegel::function_t *fn = *j;
	    functype_t type;
	    char submatch[512];

	    // We want functions which are defined in this compile unit
	    if (!fn->get_address())
		continue;

	    type = classify_function(fn->get_name().c_str(),
				     submatch, sizeof(submatch));
#if _NP_DEBUG
	    fprintf(stderr, "np: function %s classified %s submatch \"%s\"\n",
		    fn->get_name().c_str(), np::as_string(type), submatch);
#endif
	    switch (type)
	    {
	    case FT_UNKNOWN:
		continue;
	    case FT_TEST:
		// Test functions need a node name
		if (!submatch[0])
		    continue;
		// Test function return void
		if (fn->get_return_type()->get_classification() != np::spiegel::type_t::TC_VOID)
		    continue;
		// Test functions take no arguments
		if (fn->get_parameter_types().size() != 0)
		    continue;
		{
		    testnode_t *tn = root_->make_path(test_name(fn, submatch));
		    tn->set_function(type, fn);
		    ntests++;

		    std::vector<const char*> tags;
		    spiegel_->get_annotations(fn->get_address(), "T", tags);
		    if (tags.size())
			tn->set_tags(tags);
		}
		break;
	    case FT_BEFORE:
	    case FT_AFTER:
		// Before/after functions go into the parent node
		assert(!submatch[0]);
		// Before/after functions return int
		if (fn->get_return_type()->get_classification() != np::spiegel::type_t::TC_SIGNED_INT)
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
		    np::spiegel::function_t *target = find_mock_target(submatch);
		    if (!target)
			continue;
		    root_->make_path(test_name(fn, 0))->add_mock(target, fn);
		}
		break;
	    case FT_PARAM:
		// Parameters need a name
		if (!submatch[0])
		    continue;
		{
		    const struct __np_param_dec *dec = (const struct __np_param_dec *)call_func(fn);
		    root_->make_path(test_name(fn, 0))->add_parameter(
				    submatch, dec->var, dec->values);
		}
		break;
	    case FT_DECORATOR:
		// Decorators should have a numeric priority and a string
		// name encoded as two underscore-separated fields.
		if (!submatch[0])
		    continue;
		char *name = strchr(submatch, '_');
		if (!name)
		    continue;
		*name++ = '\0';
		decorator_descs_.push_back(decorator_desc_t(name, atoi(submatch), fn));
		break;
	    }
	}
    }

    if (!ntests)
	fprintf(stderr, "np: WARNING: no tests discovered\n");
    // Calculate the effective root_ and common_
    common_ = root_;
    root_ = root_->detach_common();

    // Sort decorator descriptors by their priority
    qsort(decorator_descs_.data(),
	  decorator_descs_.size(),
	  sizeof(decorator_desc_t),
	  (int(*)(const void *, const void *))decorator_desc_t::compare);
#if _NP_DEBUG
    vector<decorator_desc_t>::const_iterator itr;
    for (itr = decorator_descs_.begin() ; itr != decorator_descs_.end() ; itr++)
	fprintf(stderr, "np: decorator %s priority %d\n", itr->name_.c_str(), itr->priority_);
#endif
}

extern void init_syslog_intercepts(testnode_t *);

void
testmanager_t::setup_builtin_intercepts()
{
    init_syslog_intercepts(root_);
}

std::vector<decorator_t*>
testmanager_t::create_decorators() const
{
    vector<decorator_t*> decs;
    decs.reserve(decorator_descs_.size());

    vector<decorator_desc_t>::const_iterator i;
    for (i = decorator_descs_.begin() ; i != decorator_descs_.end() ; i++)
    {
	decorator_t *dec = (decorator_t *)call_func(i->create_);
	if (dec)
	    decs.push_back(dec);
    }

    return decs;
}

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern "C" void __np_mock_by_name(const char *fname, void (*to)(void))
{
    np::spiegel::function_t *f = np::testmanager_t::instance()->find_mock_target(fname);
    __np_mock((void(*)(void))f->get_address(), f->get_full_name().c_str(), to);
}

extern "C" void np_unmock_by_name(const char *fname)
{
    np::spiegel::function_t *f = np::testmanager_t::instance()->find_mock_target(fname);
    __np_unmock((void(*)(void))f->get_address());
}

