#include "common.h"
#include "u4c_priv.h"
#include <assert.h>
#include <elf.h>
#include <dlfcn.h>
#include <link.h>
#include <bfd.h>
#include <setjmp.h>


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

static void
init_state(u4c_globalstate_t *state)
{
    memset(state, 0, sizeof(*state));
    state->classifiers_tailp = &state->classifiers;
    state->objects_tailp = &state->objects;
    state->funcs_tailp = &state->funcs;
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
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char **environ;

/*
 * Find and return the filename of the currently running executable.
 *
 * Uses the aux vector given us by the kernel, on the primary stack
 * above the argv[] and environ[] arrays.
 */
static const char *
u4c_exe_filename(void)
{
    char **p;
    ElfW(auxv_t) *a;

    /* skip environ[] */
    for (p = environ ; *p ; p++)
	;
    p++;

    /* p now points to the aux array.  probably */
    for (a = (ElfW(auxv_t) *)p ; a->a_type != AT_NULL ; a++)
    {
	if (a->a_type == AT_EXECFN)
	    return (const char *)a->a_un.a_val;
    }

    /* This basically never happens...unless we're running under Valgrind */
    {
	static char buf[PATH_MAX];
	int r = readlink("/proc/self/exe", buf, sizeof(buf));
	if (r > 0)
	    return buf;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static bool
u4c_filename_is_ignored(const char *filename)
{
    static const char * const prefixes[] =
    {
	"/bin/",
	"/lib/",
	"/usr/bin/",
	"/usr/lib/",
	"/opt/",
	NULL
    };
    const char * const *p;

    for (p = prefixes ; *p ; p++)
    {
	if (!strncmp(filename, *p, strlen(*p)))
	    return true;
    }
    return false;
}

static enum u4c_functype
u4c_classify_function(u4c_globalstate_t *state,
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

#if 0
static const char *sym_flags_as_string(unsigned int flags)
{
    static char buf[2048];
    char *p = buf;
    int remain = sizeof(buf)-1;
    unsigned int i;
    static const struct
    {
	unsigned int flag;
	const char *name;
    }
    flagdesc[] =
    {
	{ BSF_LOCAL, "local" },
	{ BSF_GLOBAL, "global" },
	{ BSF_DEBUGGING, "debugging" },
	{ BSF_FUNCTION, "function" },
	{ BSF_KEEP, "keep" },
	{ BSF_KEEP_G, "keep_g" },
	{ BSF_WEAK, "weak" },
	{ BSF_SECTION_SYM, "section_sym" },
	{ BSF_OLD_COMMON, "old_common" },
	{ BFD_FORT_COMM_DEFAULT_VALUE, "fortran_common" },
	{ BSF_NOT_AT_END, "not_at_end" },
	{ BSF_CONSTRUCTOR, "constructor" },
	{ BSF_WARNING, "warning" },
	{ BSF_INDIRECT, "indirect" },
	{ BSF_FILE, "file" },
	{ BSF_DYNAMIC, "dynamic" },
	{ BSF_OBJECT, "object" },
	{ BSF_DEBUGGING_RELOC, "debugging_reloc" },
	{ BSF_THREAD_LOCAL, "thread_local" },
	{ BSF_RELC, "relc" },
	{ BSF_SRELC, "srelc" },
	{ BSF_SYNTHETIC, "synthetic" },
	{ 0, NULL },
    };

    buf[0] = '\0';
    for (i = 0 ; flagdesc[i].name && remain > 0 ; i++)
    {
	if (!(flags & flagdesc[i].flag))
	    continue;
	flags &= ~flagdesc[i].flag;
	if (p != buf) {
	    *p++ = '|';
	    remain--;
	}
	strcpy(p, flagdesc[i].name);
	remain -= strlen(p);
	p += strlen(p);
    }

    return buf;
}
#endif

static int
add_one_object(struct dl_phdr_info *info,
	       size_t size __attribute__((unused)),
	       void *closure)
{
    u4c_globalstate_t *state = closure;
    u4c_object_t *o;

//     fprintf(stderr, "u4c: addr=0x%lx name=\"%s\"\n",
// 	    (unsigned long)info->dlpi_addr,
// 	    info->dlpi_name);

    o = xmalloc(sizeof(*o));
    o->base = (unsigned long)info->dlpi_addr;
    o->name = xstrdup(xstr(info->dlpi_name));

    /* append to the objects list */
    o->next = NULL;
    *state->objects_tailp = o;
    state->objects_tailp = &o->next;

    return 0;
}

static int
discover_objects(u4c_globalstate_t *state)
{
    return dl_iterate_phdr(add_one_object, state);
}

static u4c_function_t *
add_function(u4c_globalstate_t *state,
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

static int
compare_syms_by_value(const void *v1, const void *v2)
{
    const asymbol *s1 = *(const asymbol **)v1;
    const asymbol *s2 = *(const asymbol **)v2;

    if (s1->value > s2->value)
	return 1;
    if (s1->value < s2->value)
	return -1;
    return 0;
}

static void
discover_functions(u4c_globalstate_t *state)
{
    u4c_object_t *o;
    bfd *b;
    bfd_init();

    fprintf(stderr, "u4c: exe_filename=\"%s\"\n", u4c_exe_filename());

    for (o = state->objects ; o ; o = o->next)
    {
	const char *filename;
	int nsyms;
	asymbol **syms;
	asymbol *s;
	int i;

	if (!o->base && !*o->name)
	    filename = u4c_exe_filename();
	else if (o->base && *o->name)
	    filename = o->name;
	else
	    continue;

	if (u4c_filename_is_ignored(filename))
	    continue;

	b = bfd_openr(filename, NULL);
	if (!b)
	{
	    bfd_perror(filename);
	    continue;
	}
	if (!bfd_check_format(b, bfd_object))
	{
	    fprintf(stderr, "u4c: %s: not an object\n", filename);
	    bfd_close(b);
	    continue;
	}
// 	fprintf(stderr, "u4c: filename=\"%s\" b=%p\n", filename, b);

	nsyms = bfd_get_symtab_upper_bound(b);
// 	fprintf(stderr, "u4c: nsyms=%d\n", nsyms);
	syms = xmalloc(sizeof(asymbol*) * nsyms);
	nsyms = bfd_canonicalize_symtab(b, syms);
// 	fprintf(stderr, "u4c: nsyms=%d\n", nsyms);

	qsort(syms, nsyms, sizeof(asymbol *), compare_syms_by_value);

#if 0
	for (i = 0 ; i < nsyms ; i++)
	{
	    s = syms[i];
	    fprintf(stderr, "%08lx %s %s %s\n",
		    (unsigned long)s->value,
		    sym_flags_as_string(s->flags),
		    s->section->name,
		    s->name);
	}
#endif
	for (i = 0 ; i < nsyms ; i++)
	{
	    enum u4c_functype type;
	    const char *filename = NULL;
	    unsigned int lineno = 0;
	    unsigned long addr;
	    char submatch[512];

	    s = syms[i];

	    if (!(s->flags & BSF_FUNCTION))
		continue;
	    if (strcmp(s->section->name, ".text"))
		continue;
	    type = u4c_classify_function(state, s->name,
					 submatch, sizeof(submatch));
	    if (type == FT_UNKNOWN)
		continue;
	    if (type == FT_TEST && !submatch[0])
		continue;
	    if (!bfd_find_line(b, syms, s, &filename, &lineno))
		continue;
	    addr = (unsigned long)bfd_asymbol_value(s);

// 	    fprintf(stderr, "u4c: function \"%s\" type %d location \"%s\":%u\n",
// 			    s->name, type, filename, lineno);

	    add_function(state, type, s->name, filename, submatch,
			 (void(*)(void))addr, o);
	}
	xfree(syms);
	bfd_close(b);
    }
}

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

static jmp_buf u4c_jbuf;
static bool u4c_catching;
static int u4c_caught;

#define u4c_try \
	u4c_catching = true; \
	if (!(u4c_caught = setjmp(u4c_jbuf)))
#define u4c_catch \
	u4c_catching = false; \
	if (u4c_caught)

#define __u4c_throw \
	if (u4c_catching) \
	    longjmp(u4c_jbuf, 1);
#define u4c_throw \
    do { \
	__u4c_throw; \
	abort(); \
    } while(0)

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

static void
run_function(u4c_globalstate_t *state,
	     u4c_function_t *f)
{
    if (f->type == FT_TEST)
    {
	void (*call)(void) = f->addr;
	call();
    }
    else
    {
	int (*call)(void) = (int(*)(void))f->addr;
	if (call())
	    u4c_throw;
    }
}

static void
run_fixtures(u4c_globalstate_t *state,
	     u4c_testnode_t *tn,
	     enum u4c_functype type)
{
    u4c_testnode_t *a;
    int i;

    if (!state->fixtures)
	state->fixtures = xmalloc(sizeof(u4c_function_t*) *
				  state->maxdepth);
    else
	memset(state->fixtures, 0, sizeof(u4c_function_t*) *
				   state->maxdepth);

    /* Run FT_BEFORE from outermost in, and FT_AFTER
     * from innermost out */
    if (type == FT_BEFORE)
    {
	i = state->maxdepth;
	for (a = tn ; a ; a = a->parent)
	    state->fixtures[--i] = a->funcs[type];
    }
    else
    {
	i = 0;
	for (a = tn ; a ; a = a->parent)
	    state->fixtures[i++] = a->funcs[type];
    }

    for (i = 0 ; i < state->maxdepth ; i++)
	if (state->fixtures[i])
	    run_function(state, state->fixtures[i]);
}

static char *
u4c_testnode_fullname(const u4c_testnode_t *tn)
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

static void
run_test(u4c_globalstate_t *state,
	 u4c_testnode_t *tn)
{
    char *fullname;
    bool fail = false;

    {
	static int n = 0;
	if (++n > 10)
	    return;
    }

    fullname = u4c_testnode_fullname(tn);
    fprintf(stderr, "u4c: running: \"%s\"\n", fullname);

    u4c_try
    {
	run_fixtures(state, tn, FT_BEFORE);
    }
    u4c_catch
    {
	fprintf(stderr, "FAIL %s in before fixture\n", fullname);
	fail = true;
    }

    if (!fail)
    {
	u4c_try
	{
	    run_function(state, tn->funcs[FT_TEST]);
	}
	u4c_catch
	{
	    fprintf(stderr, "FAIL %s\n", fullname);
	    fail = true;
	}

	u4c_try
	{
	    run_fixtures(state, tn, FT_AFTER);
	}
	u4c_catch
	{
	    fprintf(stderr, "FAIL %s in after fixture\n", fullname);
	    fail = true;
	}
    }

    state->nrun++;
    if (fail)
	state->nfailed++;
    else
	fprintf(stderr, "PASS %s\n", fullname);
    xfree(fullname);
}

static void
run_tests(u4c_globalstate_t *state,
	  u4c_testnode_t *tn)
{
    u4c_testnode_t *child;

    if (tn->funcs[FT_TEST])
    {
	run_test(state, tn);
    }
    else
    {
	for (child = tn->children ; child ; child = child->next)
	    run_tests(state, child);
    }
}

static void
summarise_results(u4c_globalstate_t *state)
{
    fprintf(stderr, "u4c: %u run %u failed\n",
	    state->nrun, state->nfailed);
}

int
u4c(void)
{
    u4c_globalstate_t state;
    int ec;

    fprintf(stderr, "u4c: running\n");

    init_state(&state);
    setup_classifiers(&state);
    discover_objects(&state);
    discover_functions(&state);
    find_common_path(&state);
    generate_nodes(&state);
    /* TODO: check tree for a) leaves without FT_TEST
     * and b) non-leaves with FT_TEST */
//     dump_nodes(&state, state.root, 0);

    run_tests(&state, state.root);
    summarise_results(&state);

    ec = !!state.nfailed;
    free_state(&state);

    return ec;
}

