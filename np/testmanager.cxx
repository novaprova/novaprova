/*
 * Copyright 2011-2020 Gregory Banks
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
#include "np/event.hxx"
#include "np/spiegel/spiegel.hxx"
#include "np/util/log.hxx"

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

    delete root_;
    if (common_ != root_)
	delete common_;
    root_ = 0;
    common_ = 0;

    if (spiegel_)
	delete spiegel_;
    event_t::fini();

    assert(instance_ == this);
    instance_ = 0;
}


testmanager_t *
testmanager_t::instance()
{
    if (!instance_)
    {
	dprintf("creating testmanager_t instance\n");
	new testmanager_t();
	instance_->print_banner();
	instance_->setup_classifiers();
	instance_->discover_functions();
	instance_->setup_builtin_intercepts();
	/* TODO: check tree for a) leaves without FT_TEST
	 * and b) non-leaves with FT_TEST */
        if (np::log::is_enabled_for(np::log::DEBUG))
            instance_->root_->dump(string(""));
    }
    return instance_;
}

void
testmanager_t::print_banner()
{
    iprintf("NovaProva Copyright (c) Gregory Banks\n");
    iprintf("Built for O/S " _NP_OS " architecture " _NP_ARCH "\n");
    fflush(stderr);
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
    dprintf("adding classifier /%s/%s -> %s\n",
	    re, (case_sensitive ? "i" : ""), np::as_string(type));
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

/*
 * Returns true if `target` a well-known function name in libc which is
 * difficult to auto-mock because the symbol name that we know it by
 * (e.g. fopen) is not what the C library exports it as (e.g. fopen64)
 * due to various header file shenanigans like #defines and __REDIRECT.
 * Most of these are old-fashioned filesystem functions which need
 * foo64() versions to cope with large files in 32b ABIs.
 */
static bool warn_on_automock(const char *target)
{
    static const char *names[] = {
#ifdef __GLIBC__
        "alphasort",        /* alphasort, alphasort64 */
        "creat",            /* creat, creat64 */
        "fallocate",        /* fallocate, fallocate64 */
        "fclose",           /* fclose, _IO_fclose */
        "fdopen",           /* fdopen, _IO_fdopen */
        "fgetpos",          /* fgetpos, fgetpos64, _IO_fgetpos, _IO_fgetpos64 */
        "fopen",            /* fopen, fopen64, _IO_fopen */
        "fputs",            /* fputs, _IO_fputs */
        "freopen",          /* freopen, freopen64 */
        "fseeko",           /* fseeko, fseeko64 */
        "fsetpos",          /* fsetpos, fsetpos64, _IO_fsetpos, _IO_fsetpos64 */
        "fstat",            /* fstat, fstat64, __fxstat, __fxstat64 */
        "fstatat",          /* fstatat, fstatat64, __fxstatat, __fxstatat64 */
        "fstatfs",          /* fstatfs, fstatfs64 */
        "fstatvfs",         /* fstatvfs, fstatvfs64 */
        "ftello",           /* ftello, ftello64 */
        "ftruncate",        /* ftruncate, ftruncate64 */
        "ftw",              /* ftw, ftw64 */
        "getdirentries",    /* getdirentries, getdirentries64 */
        "getrlimit",        /* getrlimit, getrlimit64 */
        "glob",             /* glob, glob64 */
        "globfree",         /* globfree, globfree64 */
        "lockf",            /* lockf, lockf64 */
        "lseek",            /* lseek, lseek64 */
        "lstat",            /* lstat, lstat64, __lxstat , __lxstat64 */
        "mkostemp",         /* mkostemp, mkostemp64 */
        "mkostemps",        /* mkostemps, mkostemps64 */
        "mkstemp",          /* mkstemp, mkstemp64 */
        "mkstemps",         /* mkstemps, mkstemps64 */
        "mmap",             /* mmap, mmap64 */
        "nftw",             /* nftw, nftw64 */
        "open",             /* open, open64 */
        "openat",           /* openat, openat64 */
        "posix_fadvise",    /* posix_fadvise, posix_fadvise64 */
        "posix_fallocate",  /* posix_fallocate, posix_fallocate64 */
        "pread",            /* pread, pread64 */
        "preadv",           /* preadv, preadv64 */
        "prlimit",          /* prlimit, prlimit64 */
        "pwrite",           /* pwrite, pwrite64 */
        "pwritev",          /* pwritev, pwritev64 */
        "readdir",          /* readdir, readdir64 */
        "scandir",          /* scandir, scandir64 */
        "scandirat",        /* scandirat, scandirat64 */
        "sendfile",         /* sendfile, sendfile64 */
        "setrlimit",        /* setrlimit, setrlimit64 */
        "stat",             /* stat, stat64, __xstat, __xstat64 */
        "statfs",           /* statfs, statfs64 */
        "statvfs",          /* statvfs, statvfs64 */
        "tmpfile",          /* tmpfile, tmpfile64 */
        "truncate",         /* truncate, truncate64 */
        "versionsort"       /* versionsort, versionsort64 */
#endif
    };
    for (unsigned i = 0 ; i < sizeof(names)/sizeof(names[0]) ; i++)
        if (!strcmp(names[i], target))
            return true;
    return false;
}

np::spiegel::function_t *
testmanager_t::find_mock_target(string name)
{
    dprintf("Finding mock target %s", name.c_str());
    vector<np::spiegel::compile_unit_t *> units = spiegel_->get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	vector<np::spiegel::function_t *> fns = (*i)->get_functions();
	vector<np::spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
            if ((*j)->get_name() != name)
                continue;
            if ((*j)->is_declaration())
                continue;
            if (!(*j)->get_address())
                continue;
            dprintf("Found function %s in compile unit %s link object %s",
                    name.c_str(), (*i)->get_filename().c_str(),
                    (*i)->get_executable());
            return *j;
	}
    }
    dprintf("Failed to find mock target for %s", name.c_str());
    return 0;
}

static const struct __np_param_dec *
get_param_dec(np::spiegel::function_t *fn)
{
    vector<np::spiegel::value_t> args;
    np::spiegel::value_t ret = fn->invoke(args);
    return (const struct __np_param_dec *)ret.val.vpointer;
}

void
testmanager_t::discover_functions()
{
    if (!spiegel_)
    {
	dprintf("creating np::spiegel::state_t instance\n");
	spiegel_ = new np::spiegel::state_t();
	spiegel_->add_self();
        event_t::init(spiegel_);
	root_ = new testnode_t(0);
    }
    // else: splice common_ and root_ back together

    dprintf("scanning for test functions\n");
    vector<np::spiegel::compile_unit_t *> units = spiegel_->get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    unsigned int ntests = 0;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	dprintf("scanning compile unit %s\n", (*i)->get_absolute_path().c_str());
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
	    dprintf("function %s classified %s submatch \"%s\"\n",
		    fn->get_name().c_str(), np::as_string(type), submatch);
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
		root_->make_path(test_name(fn, submatch))->set_function(type, fn);
		ntests++;
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
                    if (warn_on_automock(submatch))
                        wprintf("Mock target function %s is a function in "
                                "libc which is commonly difficult to mock "
                                "using NovaProva's automatic mocks.  Please "
                                "read the section \"Automatic Mocks and The "
                                "C Library\" in the manual for details.", submatch);
		    np::spiegel::function_t *target = find_mock_target(submatch);
		    if (!target)
                    {
                        wprintf("Unable to find mock target function %s for "
                                "automatic mock function %s.  No mock will "
                                "be installed.",
                                submatch, fn->get_name().c_str());
			continue;
                    }
		    root_->make_path(test_name(fn, 0))->add_mock(target, fn);
		}
		break;
	    case FT_PARAM:
		// Parameters need a name
		if (!submatch[0])
		    continue;
		const struct __np_param_dec *dec = get_param_dec(fn);
		root_->make_path(test_name(fn, 0))->add_parameter(
				submatch, dec->var, dec->values);
		break;
	    }
	}
    }

    if (!ntests)
	wprintf("no tests discovered\n");
    // Calculate the effective root_ and common_
    common_ = root_;
    root_ = root_->detach_common();
}

extern void init_syslog_intercepts(testnode_t *);
extern void init_exit_intercepts(testnode_t *);

void
testmanager_t::setup_builtin_intercepts()
{
    init_syslog_intercepts(root_);
    init_exit_intercepts(root_);
}

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern "C" void __np_mock_by_name(const char *fname, void (*to)(void))
{
    np::spiegel::function_t *f = np::testmanager_t::instance()->find_mock_target(fname);
    __np_mock((void(*)(void))f->get_live_address(), f->get_full_name().c_str(), to);
}

extern "C" void np_unmock_by_name(const char *fname)
{
    np::spiegel::function_t *f = np::testmanager_t::instance()->find_mock_target(fname);
    __np_unmock((void(*)(void))f->get_live_address());
}

