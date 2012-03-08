#include "common.h"
#include "u4c_priv.h"
#include "except.h"
#include "spiegel/tok.hxx"
#include <sys/time.h>
#include <valgrind/valgrind.h>

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
u4c_reltimestamp(void)
{
    static char buf[32];
    static struct timeval first;
    struct timeval now;
    struct timeval delta;
    gettimeofday(&now, NULL);
    if (!first.tv_sec)
	first = now;
    timersub(&now, &first, &delta);
    snprintf(buf, sizeof(buf), "%lu.%06lu",
	     (unsigned long)delta.tv_sec,
	     (unsigned long)delta.tv_usec);
    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_globalstate_t::u4c_globalstate_t()
{
    maxchildren = 1;
}

u4c_globalstate_t::~u4c_globalstate_t()
{
}

u4c_testmanager_t::u4c_testmanager_t()
{
}

u4c_testmanager_t::~u4c_testmanager_t()
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
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c::functype_t
u4c_testmanager_t::classify_function(const char *func,
				     char *match_return,
				     size_t maxmatch)
{
    if (match_return)
	match_return[0] = '\0';

    vector<u4c::classifier_t*>::iterator i;
    for (i = classifiers_.begin() ; i != classifiers_.end() ; ++i)
    {
	u4c::functype_t ft = (u4c::functype_t) (*i)->classify(func, match_return, maxmatch);
	if (ft != u4c::FT_UNKNOWN)
	    return ft;
	/* else, no match: just keep looking */
    }
    return u4c::FT_UNKNOWN;
}

void
u4c_testmanager_t::add_classifier(const char *re,
			          bool case_sensitive,
			          u4c::functype_t type)
{
    u4c::classifier_t *cl = new u4c::classifier_t;
    if (!cl->set_regexp(re, case_sensitive))
    {
	delete cl;
	return;
    }
    cl->set_results(u4c::FT_UNKNOWN, type);
    classifiers_.push_back(cl);
}

void
u4c_testmanager_t::setup_classifiers()
{
    add_classifier("^test_([a-z0-9].*)", false, u4c::FT_TEST);
    add_classifier("^[tT]est([A-Z].*)", false, u4c::FT_TEST);
    add_classifier("^[sS]etup$", false, u4c::FT_BEFORE);
    add_classifier("^set_up$", false, u4c::FT_BEFORE);
    add_classifier("^[iI]nit$", false, u4c::FT_BEFORE);
    add_classifier("^[tT]ear[dD]own$", false, u4c::FT_AFTER);
    add_classifier("^tear_down$", false, u4c::FT_AFTER);
    add_classifier("^[cC]leanup$", false, u4c::FT_AFTER);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char **environ;

static bool
discover_args(int *argcp, char ***argvp)
{
    char **p;
    int n;

    /* This early, environ[] points at the area
     * above argv[], so walk down from there */
    for (p = environ-2, n = 1;
	 ((int *)p)[-1] != n ;
	 --p, ++n)
	;
    *argcp = n;
    *argvp = p;
    return true;
}

static void
be_valground(void)
{
    int argc;
    char **argv;
    const char **newargv;
    const char **p;

    if (RUNNING_ON_VALGRIND)
	return;
    fprintf(stderr, "u4c: starting valgrind\n");

    if (!discover_args(&argc, &argv))
	return;

    p = newargv = (const char **)xmalloc(sizeof(char *) * (argc+6));
    *p++ = "/usr/bin/valgrind";
    *p++ = "-q";
    *p++ = "--tool=memcheck";
//     *p++ = "--leak-check=full";
//     *p++ = "--suppressions=../../../u4c/valgrind.supp";
    while (*argv)
	*p++ = *argv++;

    execv(newargv[0], (char * const *)newargv);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_testmanager_t *u4c_testmanager_t::instance_ = 0;

u4c_testmanager_t *
u4c_testmanager_t::instance()
{
    if (!instance_)
    {
	instance_->setup_classifiers();
	instance_->discover_functions();
	/* TODO: check tree for a) leaves without FT_TEST
	 * and b) non-leaves with FT_TEST */
	instance_->root_->dump(0);
    }
    return instance_;
}

extern "C" u4c_globalstate_t *
u4c_init(void)
{
    be_valground();
    u4c_reltimestamp();
    u4c_testmanager_t::instance();
    u4c_globalstate_t *state = new u4c_globalstate_t;
    return state;
}

void
u4c_globalstate_t::set_concurrency(int n)
{
    if (n == 0)
    {
	/* shorthand for "best possible" */
	n = sysconf(_SC_NPROCESSORS_ONLN);
    }
    if (n < 1)
	n = 1;
    maxchildren = n;
}

extern "C" void
u4c_set_concurrency(u4c_globalstate_t *state, int n)
{
    state->set_concurrency(n);
}

void
u4c_globalstate_t::list_tests(u4c::plan_t *plan) const
{
    u4c::testnode_t *tn;

    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	u4c::plan_t *plan = new u4c::plan_t();
	plan->add_node(u4c_testmanager_t::instance()->get_root());
	ourplan = true;
    }

    /* iterate over all tests */
    while ((tn = plan->next()))
	printf("%s\n", tn->get_fullname().c_str());

    if (ourplan)
	delete plan;
}

extern "C" void
u4c_list_tests(u4c_globalstate_t *state, u4c_plan_t *plan)
{
    state->list_tests(plan);
}

int
u4c_globalstate_t::run_tests(u4c::plan_t *plan)
{
    u4c::testnode_t *tn;

    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	plan =  new u4c::plan_t();
	plan->add_node(u4c_testmanager_t::instance()->get_root());
	ourplan = true;
    }

    if (!listeners_.size())
	add_listener(new u4c::text_listener_t);

    begin();
    for (;;)
    {
	while (children_.size() < maxchildren &&
	       (tn = plan->next()))
	    begin_test(tn);
	if (!children_.size())
	    break;
	wait();
    }
    end();

    if (ourplan)
	delete plan;

    return !!nfailed_;
}

extern "C" int
u4c_run_tests(u4c_globalstate_t *state, u4c_plan_t *plan)
{
    return state->run_tests(plan);
}

extern "C" void
u4c_done(u4c_globalstate_t *state)
{
    delete state;
}

