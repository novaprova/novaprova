#include "common.h"
#include "u4c_priv.h"
#include "except.h"
#include <sys/time.h>
#include <valgrind/valgrind.h>

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

static u4c_globalstate_t *
new_state(void)
{
    u4c_globalstate_t *state;

    state = (u4c_globalstate_t *)xmalloc(sizeof(*state));

    state->classifiers_tailp = &state->classifiers;
    state->funcs_tailp = &state->funcs;
    state->maxchildren = 1;

    return state;
}

static void
delete_node(u4c_testnode_t *tn)
{
    while (tn->children)
    {
	u4c_testnode_t *child = tn->children;
	tn->children = child->next;
	delete_node(child);
    }

    xfree(tn->name);
    xfree(tn);
}

static void
free_state(u4c_globalstate_t *state)
{
    u4c_function_t *f;
    u4c_classifier_t *cl;

    while (state->funcs)
    {
	f = state->funcs;
	state->funcs = f->next;

// 	xfree(f->name);
	xfree(f->submatch);
	delete f;
    }

    while (state->classifiers)
    {
	cl = state->classifiers;
	state->classifiers = cl->next;

	xfree(cl->re);
	regfree(&cl->compiled_re);
	xfree(cl);
    }

    while (state->plans)
	u4c_plan_delete(state->plans);

    delete_node(state->root);

    if (state->spiegel)
	delete state->spiegel;

    xfree(state->fixtures);
    xfree(state->common);
    xfree(state);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

enum u4c_functype
__u4c_classify_function(u4c_globalstate_t *state,
		        const char *func,
		        char *match_return,
		        size_t maxmatch)
{
    int r;
    u4c_classifier_t *cl;
    regmatch_t match[2];

    if (match_return)
	match_return[0] = '\0';

    for (cl = state->classifiers ; cl ; cl = cl->next)
    {
	r = regexec(&cl->compiled_re, func, 2, match, 0);

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
				    cl->re);
		    return FT_UNKNOWN;
		}
		memcpy(match_return, func+match[1].rm_so, len);
		match_return[len] = '\0';
// fprintf(stderr, "    match_return \"%s\"\n", match_return);
	    }
	    return cl->type;
	}
	else if (r != REG_NOMATCH)
	{
	    /* some runtime error */
	    char err[1024];
	    regerror(r, &cl->compiled_re, err, sizeof(err));
	    fprintf(stderr, "u4c: failed matching \"%s\": %s\n",
		    func, err);
	    continue;
	}
	/* else, no match: just keep looking */
    }
    return FT_UNKNOWN;
}

static void
u4c_add_classifier(u4c_globalstate_t *state,
	           const char *re,
	           bool case_sensitive,
	           enum u4c_functype type)
{
    u4c_classifier_t *cl;
    int r;

    cl = (u4c_classifier_t *)xmalloc(sizeof(*cl));
    cl->re = xstrdup(re);
    cl->type = type;
    r = regcomp(&cl->compiled_re, re,
	        REG_EXTENDED|(case_sensitive ? 0 : REG_ICASE));
    if (r)
    {
	char err[1024];
	regerror(r, &cl->compiled_re, err, sizeof(err));
	fprintf(stderr, "u4c: bad classifier %s: %s\n",
		re, err);
	xfree(cl->re);
	xfree(cl);
	return;
    }

    *state->classifiers_tailp = cl;
    state->classifiers_tailp = &cl->next;
}

static void
setup_classifiers(u4c_globalstate_t *state)
{
    u4c_add_classifier(state, "^test_([a-z0-9].*)", false, FT_TEST);
    u4c_add_classifier(state, "^[tT]est([A-Z].*)", false, FT_TEST);
    u4c_add_classifier(state, "^[sS]etup$", false, FT_BEFORE);
    u4c_add_classifier(state, "^set_up$", false, FT_BEFORE);
    u4c_add_classifier(state, "^[iI]nit$", false, FT_BEFORE);
    u4c_add_classifier(state, "^[tT]ear[dD]own$", false, FT_AFTER);
    u4c_add_classifier(state, "^tear_down$", false, FT_AFTER);
    u4c_add_classifier(state, "^[cC]leanup$", false, FT_AFTER);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_function_t *
__u4c_add_function(u4c_globalstate_t *state,
		   enum u4c_functype type,
		   spiegel::function_t *func,
		   const char *submatch)
{
    u4c_function_t *f;

    f = new u4c_function_t;
    f->type = type;
    f->func = func;

    f->filename = func->get_compile_unit()->get_absolute_path();

    f->submatch = xstrdup(submatch);

    /* append */
    *state->funcs_tailp = f;
    state->funcs_tailp = &f->next;

    return f;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
find_common_path(u4c_globalstate_t *state)
{
    u4c_function_t *f;
    char *common = NULL;
    unsigned int lastcommonlen = 0, commonlen = 0;
    char *cp, *ep;
    unsigned int clen, elen;

    for (f = state->funcs ; f ; f = f->next)
    {
	if (common == NULL)
	{
	    common = xstrdup(f->filename.c_str());
	    cp = strrchr(common, '/');
	    if (cp)
		*cp = '\0';
	    continue;
	}
	cp = common;
	ep = (char *)f->filename.c_str();

	for (;;)
	{
	    cp = strchr(cp+1, '/');
	    clen = cp - common;
	    ep = strchr(ep+1, '/');
	    elen = ep - f->filename.c_str();

	    if (clen != elen)
		break;
	    if (strncmp(common, f->filename.c_str(), clen))
		break;
	    lastcommonlen = commonlen;
	    commonlen = clen+1;
	}
    }

    state->common = common;
    state->commonlen = lastcommonlen;
}

static u4c_testnode_t *
new_testnode(const char *name)
{
    u4c_testnode_t *tn;

    tn = (u4c_testnode_t  *)xmalloc(sizeof(*tn));
    if (name)
	tn->name = xstrdup(name);

    return tn;
}

static void
add_testnode(u4c_globalstate_t *state,
	     char *name,
	     u4c_function_t *func)
{
    u4c_testnode_t *parent = state->root;
    char *part;
    u4c_testnode_t *child;
    u4c_testnode_t **tailp;
    unsigned int depth = 1;
    static const char sep[] = "/";

    for (part = strtok(name, sep) ; part ; part = strtok(0, sep))
    {
	for (child = parent->children, tailp = &parent->children ;
	     child ;
	     tailp = &child->next, child = child->next)
	{
	    if (!strcmp(child->name, part))
		break;
	}
	if (!child)
	{
	    child = new_testnode(part);
	    *tailp = child;
	    child->parent = parent;
	}

	parent = child;
	depth++;
    }

    if (child->funcs[func->type])
	fprintf(stderr, "u4c: WARNING: duplicate %s functions: "
			"%s:%s and %s:%s\n",
			__u4c_functype_as_string(func->type),
			child->funcs[func->type]->filename.c_str(),
			child->funcs[func->type]->func->get_name(),
			func->filename.c_str(),
			func->func->get_name());
    else
	child->funcs[func->type] = func;

    if (depth > state->maxdepth)
	state->maxdepth = depth;
}

static void
generate_nodes(u4c_globalstate_t *state)
{
    u4c_function_t *f;
    char *buf = NULL;
    unsigned int buflen = 0;
    unsigned int len;
    char *p;

    state->root = new_testnode(0);
    for (f = state->funcs ; f ; f = f->next)
    {
	/* TODO: use estring!! */
	len = strlen(f->filename.c_str() + state->commonlen) + 1;
	if (f->submatch)
	    len += strlen(f->submatch) + 1;
	if (len > buflen)
	{
	    buflen = (len | 0xff) + 1;
	    buf = (char *)realloc(buf, buflen);
	}
	strcpy(buf, f->filename.c_str() + state->commonlen);

	/* strip the .c extension */
	p = strrchr(buf, '.');
	if (p)
	    *p = '\0';

	if (f->submatch)
	{
	    strcat(buf, "/");
	    strcat(buf, f->submatch);
	}

// 	    fprintf(stderr, "u4c: \"%s\", \"%s\" -> \"%s\"\n",
// 		    f->name, f->filename+lastcommonlen, s);
	add_testnode(state, buf, f);
    }
    xfree(buf);
}

static void
indent(int level)
{
    for ( ; level ; level--)
	fputs("    ", stderr);
}

static void
dump_nodes(u4c_globalstate_t *state,
	   u4c_testnode_t *tn,
	   int level)
{
    u4c_testnode_t *child;
    int type;

    indent(level);
    fprintf(stderr, "%s\n", (tn->name ? tn->name : ""));

    for (type = 0 ; type < FT_NUM ; type++)
    {
	if (tn->funcs[type])
	{
	    indent(level);
	    fprintf(stderr, "  %s=%s:%s\n",
			    __u4c_functype_as_string((u4c_functype)type),
			    tn->funcs[type]->filename.c_str() + state->commonlen,
			    tn->funcs[type]->func->get_name());
	}
    }

    for (child = tn->children ; child ; child = child->next)
	dump_nodes(state, child, level+1);
}

char *
__u4c_testnode_fullname(const u4c_testnode_t *tn)
{
    const u4c_testnode_t *a;
    unsigned int len = 0;
    char *buf, *p;

    for (a = tn ; a ; a = a->parent)
	if (a->name)
	    len += strlen(a->name) + 1;

    buf = (char *)xmalloc(len);
    p = buf + len - 1;

    for (a = tn ; a ; a = a->parent)
    {
	if (!a->name)
	    continue;
	if (a != tn)
	    *--p = '.';
	len = strlen(a->name);
	p -= len;
	memcpy(p, a->name, len);
    }

    return buf;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static u4c_testnode_t *
find_node(u4c_testnode_t *tn, const char *name)
{
    u4c_testnode_t *child;
    u4c_testnode_t *found = 0;
    char *fullname = 0;

    if (tn->name)
    {
	fullname = __u4c_testnode_fullname(tn);
	if (!strcmp(name, fullname))
	{
	    found = tn;
	    goto out;
	}
    }

    for (child = tn->children ; child ; child = child->next)
    {
	found = find_node(child, name);
	if (found)
	    goto out;
    }

out:
    xfree(fullname);
    return found;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_plan_t *
u4c_plan_new(u4c_globalstate_t *state)
{
    u4c_plan_t *plan;

    plan = (u4c_plan_t  *)xmalloc(sizeof(*plan));
    plan->state = state;

    /* Prepend to the list of all plans; order doesn't
     * matter because all we use the list for is avoiding
     * memory leaks. */
    plan->next = state->plans;
    state->plans = plan;

    /* initialise iterator */
    plan->current.idx = -1;
    plan->current.node = 0;

    return plan;
}

void
u4c_plan_delete(u4c_plan_t *plan)
{
    u4c_plan_t **prevp;

    if (plan->state->rootplan == plan)
	plan->state->rootplan = 0;

    /* Remove from the list of all plans */
    for (prevp = &plan->state->plans ;
	 *prevp && *prevp != plan ;
	 prevp = &(*prevp)->next)
	;
    // assert(*prevp);
    if (*prevp)
	*prevp = plan->next;

    xfree(plan->nodes);
    xfree(plan);
}

static void
u4c_plan_add_node(u4c_plan_t *plan, u4c_testnode_t *tn)
{
    plan->nodes = (u4c_testnode_t **)xrealloc(plan->nodes,
			   sizeof(u4c_testnode_t *) * (plan->numnodes+1));
    plan->nodes[plan->numnodes++] = tn;
}

bool
u4c_plan_add(u4c_plan_t *plan, int nspec, const char **specs)
{
    u4c_testnode_t *tn;
    int i;

    for (i = 0 ; i < nspec ; i++)
    {
	tn = find_node(plan->state->root, specs[i]);
	if (!tn)
	    return false;
	u4c_plan_add_node(plan, tn);
    }
    return true;
}

void
u4c_plan_enable(u4c_plan_t *plan)
{
    plan->state->rootplan = plan;
}

static u4c_testnode_t *
u4c_plan_next(u4c_plan_t *plan)
{
    u4c_testnode_t *tn;

    u4c_plan_iterator_t *itr = &plan->current;

    tn = itr->node;

    /* advance tn */
    for (;;)
    {
	while (tn)
	{
	    if (tn->children)
		tn = tn->children;
	    else if (tn->next)
		tn = tn->next;
	    else if (tn->parent)
		tn = tn->parent->next;
	    if (tn && tn->funcs[FT_TEST])
		return itr->node = tn;
	}
	if (itr->idx >= plan->numnodes-1)
	    return itr->node = 0;
	tn = plan->nodes[++itr->idx];
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

u4c_globalstate_t *
u4c_init(void)
{
    u4c_globalstate_t *state;

    be_valground();
    u4c_reltimestamp();
    state = new_state();
    setup_classifiers(state);
    __u4c_discover_functions(state);
    find_common_path(state);
    generate_nodes(state);
    /* TODO: check tree for a) leaves without FT_TEST
     * and b) non-leaves with FT_TEST */
//     dump_nodes(state, state->root, 0);

    return state;
}

void
u4c_set_concurrency(u4c_globalstate_t *state, int n)
{
    if (n == 0)
    {
	/* shorthand for "best possible" */
	n = sysconf(_SC_NPROCESSORS_ONLN);
    }
    if (n < 1)
	n = 1;
    state->maxchildren = n;
}

void
u4c_list_tests(u4c_globalstate_t *state)
{
    u4c_plan_t *plan;
    u4c_testnode_t *tn;

    /* build a default plan with all the tests */
    plan = u4c_plan_new(state);
    u4c_plan_add_node(plan, state->root);

    /* iterate over all tests */
    while ((tn = u4c_plan_next(plan)))
    {
	char *fullname = __u4c_testnode_fullname(tn);
	printf("%s\n", fullname);
	xfree(fullname);
    }

    u4c_plan_delete(plan);
}

int
u4c_run_tests(u4c_globalstate_t *state)
{
    u4c_testnode_t *tn;

    if (!state->rootplan)
    {
	/* build a default plan with all the tests */
	u4c_plan_t *plan = u4c_plan_new(state);
	u4c_plan_add_node(plan, state->root);
	u4c_plan_enable(plan);
    }

    if (!state->listeners)
	__u4c_add_listener(state, __u4c_text_listener());

    __u4c_begin(state);
    for (;;)
    {
	while (state->nchildren < state->maxchildren &&
	       (tn = u4c_plan_next(state->rootplan)))
	    __u4c_begin_test(tn);
	if (!state->nchildren)
	    break;
	__u4c_wait();
    }
    __u4c_end();
    return !!state->nfailed;
}

void
u4c_done(u4c_globalstate_t *state)
{
    free_state(state);
}

