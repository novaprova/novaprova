#include "common.h"
#include "u4c_priv.h"
#include "except.h"

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

    state = xmalloc(sizeof(*state));

    state->classifiers_tailp = &state->classifiers;
    state->objects_tailp = &state->objects;
    state->funcs_tailp = &state->funcs;

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
    u4c_object_t *o;
    u4c_classifier_t *cl;

    while (state->objects)
    {
	o = state->objects;
	state->objects = o->next;

	xfree(o->name);
	xfree(o);
    }

    while (state->funcs)
    {
	f = state->funcs;
	state->funcs = f->next;

	xfree(f->name);
	xfree(f->filename);
	xfree(f->submatch);
	xfree(f);
    }

    while (state->classifiers)
    {
	cl = state->classifiers;
	state->classifiers = cl->next;

	xfree(cl->re);
	regfree(&cl->compiled_re);
	xfree(cl);
    }

    delete_node(state->root);

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

    cl = xmalloc(sizeof(*cl));
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

u4c_object_t *
__u4c_add_object(u4c_globalstate_t *state,
		 const char *name,
		 unsigned long base)
{
    u4c_object_t *o;

//     fprintf(stderr, "u4c: addr=0x%lx name=\"%s\"\n",
// 	    base, name);

    o = xmalloc(sizeof(*o));
    o->base = base;
    o->name = xstrdup(xstr(name));

    /* append to the objects list */
    o->next = NULL;
    *state->objects_tailp = o;
    state->objects_tailp = &o->next;

    return 0;
}

u4c_function_t *
__u4c_add_function(u4c_globalstate_t *state,
		   enum u4c_functype type,
		   const char *name,
		   const char *filename,
		   const char *submatch,
		   void (*addr)(void),
		   u4c_object_t *o)
{
    u4c_function_t *f;

    f = xmalloc(sizeof(*f));
    f->type = type;
    f->name = xstrdup(name);
    f->filename = xstrdup(filename);
    f->submatch = xstrdup(submatch);
    f->addr = addr;
    f->object = o;

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
	    common = xstrdup(f->filename);
	    cp = strrchr(common, '/');
	    if (cp)
		*cp = '\0';
	    continue;
	}
	cp = common;
	ep = f->filename;

	for (;;)
	{
	    cp = strchr(cp+1, '/');
	    clen = cp - common;
	    ep = strchr(ep+1, '/');
	    elen = ep - f->filename;

	    if (clen != elen)
		break;
	    if (strncmp(common, f->filename, clen))
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

    tn = xmalloc(sizeof(*tn));
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
			child->funcs[func->type]->filename,
			child->funcs[func->type]->name,
			func->filename,
			func->name);
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
	len = strlen(f->filename + state->commonlen) + 1;
	if (f->submatch)
	    len += strlen(f->submatch) + 1;
	if (len > buflen)
	{
	    buflen = (len | 0xff) + 1;
	    buf = realloc(buf, buflen);
	}
	strcpy(buf, f->filename + state->commonlen);

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
			    __u4c_functype_as_string(type),
			    tn->funcs[type]->filename + state->commonlen,
			    tn->funcs[type]->name);
	}
    }

    for (child = tn->children ; child ; child = child->next)
	dump_nodes(state, child, level+1);
}

bool CU_assertImplementation(bool bValue,
			     unsigned int uiLine,
			     char strCondition[],
			     char strFile[],
			     char strFunction[] __attribute__((unused)),
			     bool bFatal __attribute__((unused)))
{
//     fprintf(stderr, "    %s at %s:%u\n", strCondition, strFile, uiLine);
    if (!bValue)
    {
	fprintf(stderr, "u4c: FAILED: %s at %s:%u\n",
		strCondition, strFile, uiLine);
	u4c_throw;
    }
    return true;
}

void exit(int status)
{
    fprintf(stderr, "u4c: exit(%d) called\n", status);
    __u4c_throw;
    _exit(status);
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

    buf = xmalloc(len);
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

void
list_tests(u4c_globalstate_t *state,
	   u4c_testnode_t *tn)
{
    u4c_testnode_t *child;

    if (tn->funcs[FT_TEST])
    {
	char *fullname = __u4c_testnode_fullname(tn);
	printf("%s\n", fullname);
	xfree(fullname);
    }
    else
    {
	for (child = tn->children ; child ; child = child->next)
	    list_tests(state, child);
    }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_globalstate_t *
u4c_init(void)
{
    u4c_globalstate_t *state;

    fprintf(stderr, "u4c: running\n");

    state = new_state();
    setup_classifiers(state);
    __u4c_discover_objects(state);
    __u4c_discover_functions(state);
    find_common_path(state);
    generate_nodes(state);
    /* TODO: check tree for a) leaves without FT_TEST
     * and b) non-leaves with FT_TEST */
//     dump_nodes(state, state->root, 0);

    return state;
}

void
u4c_list_tests(u4c_globalstate_t *state)
{
    list_tests(state, state->root);
}

int
u4c_run_tests(u4c_globalstate_t *state)
{
    __u4c_run_tests(state, state->root);
    __u4c_summarise_results(state);
    return !!state->nfailed;
}

void
u4c_done(u4c_globalstate_t *state)
{
    free_state(state);
}

