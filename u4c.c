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

const char *
__u4c_functype_as_string(enum u4c_functype type)
{
    switch (type)
    {
    case FT_UNKNOWN: return "unknown";
    case FT_BEFORE: return "before";
    case FT_TEST: return "test";
    case FT_AFTER: return "after";
    default: return "INTERNAL ERROR!";
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_globalstate_t::u4c_globalstate_t()
{
    maxchildren = 1;
}

u4c_globalstate_t::~u4c_globalstate_t()
{
    while (classifiers_.size())
    {
	delete classifiers_.back();
	classifiers_.pop_back();
    }

    delete root_;
    root_ = base_ = 0;

    if (spiegel)
	delete spiegel;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

enum u4c_functype
u4c_globalstate_t::classify_function(const char *func,
				     char *match_return,
				     size_t maxmatch)
{
    if (match_return)
	match_return[0] = '\0';

    vector<u4c_classifier_t*>::iterator i;
    for (i = classifiers_.begin() ; i != classifiers_.end() ; ++i)
    {
	enum u4c_functype ft = (*i)->classify(func, match_return, maxmatch);
	if (ft != FT_UNKNOWN)
	    return ft;
	/* else, no match: just keep looking */
    }
    return FT_UNKNOWN;
}

bool
u4c_classifier_t::set_regexp(const char *re, bool case_sensitive)
{
    re_ = xstrdup(re);
    int r = regcomp(&compiled_re_, re,
		    REG_EXTENDED|(case_sensitive ? 0 : REG_ICASE));
    if (r)
    {
	char err[1024];
	regerror(r, &compiled_re_, err, sizeof(err));
	fprintf(stderr, "u4c: bad classifier %s: %s\n",
		re, err);
	return false;
    }
    return true;
}

enum u4c_functype
u4c_classifier_t::classify(const char *func,
			   char *match_return,
			   size_t maxmatch) const
{
    regmatch_t match[2];
    int r = regexec(&compiled_re_, func, 2, match, 0);

    if (r == 0)
    {
// fprintf(stderr, "MATCHED \"%s\" to \"%s\"\n", func, cl->re);
// fprintf(stderr, "    submatch [ {%d %d} {%d %d}]\n",
// 	    match[0].rm_so, match[0].rm_eo,
// 	    match[1].rm_so, match[1].rm_eo);

	/* successful match */
	if (match_return && match[1].rm_so >= 0)
	{
	    size_t len = match[1].rm_eo - match[1].rm_so;
	    if (len >= maxmatch)
	    {
		fprintf(stderr, "u4c: match for classifier %s too long\n",
				re_);
		return FT_UNKNOWN;
	    }
	    memcpy(match_return, func+match[1].rm_so, len);
	    match_return[len] = '\0';
// fprintf(stderr, "    match_return \"%s\"\n", match_return);
	}
	return type_;
    }

    if (r != REG_NOMATCH)
    {
	/* some runtime error */
	char err[1024];
	regerror(r, &compiled_re_, err, sizeof(err));
	fprintf(stderr, "u4c: failed matching \"%s\": %s\n",
		func, err);
    }

    return FT_UNKNOWN;
}

void
u4c_globalstate_t::add_classifier(const char *re,
			          bool case_sensitive,
			          enum u4c_functype type)
{
    u4c_classifier_t *cl = new u4c_classifier_t;
    if (!cl->set_regexp(re, case_sensitive))
    {
	delete cl;
	return;
    }
    cl->set_functype(type);
    classifiers_.push_back(cl);
}

void
u4c_globalstate_t::setup_classifiers()
{
    add_classifier("^test_([a-z0-9].*)", false, FT_TEST);
    add_classifier("^[tT]est([A-Z].*)", false, FT_TEST);
    add_classifier("^[sS]etup$", false, FT_BEFORE);
    add_classifier("^set_up$", false, FT_BEFORE);
    add_classifier("^[iI]nit$", false, FT_BEFORE);
    add_classifier("^[tT]ear[dD]own$", false, FT_AFTER);
    add_classifier("^tear_down$", false, FT_AFTER);
    add_classifier("^[cC]leanup$", false, FT_AFTER);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_testnode_t::u4c_testnode_t(const char *name)
 :  next_(0),
    parent_(0),
    children_(0),
    name_(name ? xstrdup(name) : 0)
{
    memset(funcs_, 0, sizeof(funcs_));
}

u4c_testnode_t::~u4c_testnode_t()
{
    while (children_)
    {
	u4c_testnode_t *child = children_;
	children_ = child->next_;
	delete child;
    }

    xfree(name_);
}

u4c_testnode_t *
u4c_testnode_t::make_path(string name)
{
    u4c_testnode_t *parent = this;
    const char *part;
    u4c_testnode_t *child;
    u4c_testnode_t **tailp;
    spiegel::tok_t tok(name.c_str(), "/");

    while ((part = tok.next()))
    {
	for (child = parent->children_, tailp = &parent->children_ ;
	     child ;
	     tailp = &child->next_, child = child->next_)
	{
	    if (!strcmp(child->name_, part))
		break;
	}
	if (!child)
	{
	    child = new u4c_testnode_t(part);
	    *tailp = child;
	    child->parent_ = parent;
	}

	parent = child;
    }
    return child;
}

void
u4c_testnode_t::set_function(enum u4c_functype ft, spiegel::function_t *func)
{
    if (funcs_[ft])
	fprintf(stderr, "u4c: WARNING: duplicate %s functions: "
			"%s:%s and %s:%s\n",
			__u4c_functype_as_string(ft),
			funcs_[ft]->get_compile_unit()->get_absolute_path().c_str(),
			funcs_[ft]->get_name(),
			func->get_compile_unit()->get_absolute_path().c_str(),
			func->get_name());
    else
	funcs_[ft] = func;
}

static void
indent(int level)
{
    for ( ; level ; level--)
	fputs("    ", stderr);
}

void
u4c_testnode_t::dump(int level) const
{
    indent(level);
    fprintf(stderr, "%s\n", (name_ ? name_ : ""));

    for (int type = 0 ; type < FT_NUM ; type++)
    {
	if (funcs_[type])
	{
	    indent(level);
	    fprintf(stderr, "  %s=%s:%s\n",
			    __u4c_functype_as_string((u4c_functype)type),
			    funcs_[type]->get_compile_unit()->get_absolute_path().c_str(),
			    funcs_[type]->get_name());
	}
    }

    for (u4c_testnode_t *child = children_ ; child ; child = child->next_)
	child->dump(level+1);
}

string
u4c_testnode_t::get_fullname() const
{
    string full = "";

    for (const u4c_testnode_t *a = this ; a ; a = a->parent_)
    {
	if (!a->name_)
	    continue;
	if (a != this)
	    full = "." + full;
	full = a->name_ + full;
    }

    return full;
}

u4c_testnode_t *
u4c_testnode_t::next_preorder()
{
    u4c_testnode_t *tn = this;
    while (tn)
    {
	if (tn->children_)
	    tn = tn->children_;
	else if (tn->next_)
	    tn = tn->next_;
	else if (tn->parent_)
	    tn = tn->parent_->next_;
	if (tn && tn->funcs_[FT_TEST])
	    break;
    }
    return tn;
}

u4c_testnode_t *
u4c_testnode_t::skip_common()
{
    u4c_testnode_t *base;
    for (base = this ;
         base->children_ && !base->children_->next_ ;
	 base = base->children_)
	;
    return base;
}

list<spiegel::function_t*>
u4c_testnode_t::get_fixtures(u4c_functype type) const
{
    list<spiegel::function_t*> fixtures;

    /* Run FT_BEFORE from outermost in, and FT_AFTER
     * from innermost out */
    for (const u4c_testnode_t *a = this ; a ; a = a->parent_)
    {
	if (!a->funcs_[type])
	    continue;
	if (type == FT_BEFORE)
	    fixtures.push_front(a->funcs_[type]);
	else
	    fixtures.push_back(a->funcs_[type]);
    }
    return fixtures;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_testnode_t *
u4c_testnode_t::find(const char *nm)
{
    if (name_ && get_fullname() == nm)
	return this;

    for (u4c_testnode_t *child = children_ ; child ; child = child->next_)
    {
	u4c_testnode_t *found = child->find(nm);
	if (found)
	    return found;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_plan_t::u4c_plan_t(u4c_globalstate_t *state)
 :  state_(state)
{
    /* initialise iterator */
    current_.idx = -1;
    current_.node = 0;
}

u4c_plan_t::~u4c_plan_t()
{
}

extern "C" u4c_plan_t *
u4c_plan_new(u4c_globalstate_t *state)
{
    return new u4c_plan_t(state);
}

extern "C" void
u4c_plan_delete(u4c_plan_t *plan)
{
    delete plan;
}

void
u4c_plan_t::add_node(u4c_testnode_t *tn)
{
    nodes_.push_back(tn);
}

bool
u4c_plan_t::add_specs(int nspec, const char **specs)
{
    u4c_testnode_t *tn;
    int i;

    for (i = 0 ; i < nspec ; i++)
    {
	tn = state_->base_->find(specs[i]);
	if (!tn)
	    return false;
	add_node(tn);
    }
    return true;
}

extern "C" bool
u4c_plan_add_specs(u4c_plan_t *plan, int nspec, const char **spec)
{
    return plan->add_specs(nspec, spec);
}

u4c_testnode_t *
u4c_plan_t::next()
{
    u4c_plan_iterator_t *itr = &current_;

    u4c_testnode_t *tn = itr->node;

    /* advance tn */
    for (;;)
    {
	tn = tn->next_preorder();
	if (tn)
	    return itr->node = tn;
	if (itr->idx >= (int)nodes_.size()-1)
	    return itr->node = 0;
	tn = nodes_[++itr->idx];
    }
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

void
u4c_globalstate_t::initialise()
{
    setup_classifiers();
    discover_functions();
    /* TODO: check tree for a) leaves without FT_TEST
     * and b) non-leaves with FT_TEST */
    base_->dump(0);
}

extern "C" u4c_globalstate_t *
u4c_init(void)
{
    be_valground();
    u4c_reltimestamp();
    u4c_globalstate_t *state = new u4c_globalstate_t;
    state->initialise();
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
u4c_globalstate_t::list_tests(u4c_plan_t *plan)
{
    u4c_testnode_t *tn;

    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	u4c_plan_t *plan = new u4c_plan_t(this);
	plan->add_node(base_);
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
u4c_globalstate_t::run_tests(u4c_plan_t *plan)
{
    u4c_testnode_t *tn;

    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	plan =  new u4c_plan_t(this);
	plan->add_node(base_);
	ourplan = true;
    }

    if (!listeners_.size())
	add_listener(new u4c_text_listener_t);

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

