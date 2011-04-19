#include "common.h"
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

__u4c_exceptstate_t __u4c_exceptstate;

void __u4c_fail(const char *condition,
		const char *filename,
	        unsigned int lineno,
		const char *function)
{
    function = xstr(function);
    filename = xstr(filename);
    fprintf(stderr, "u4c: FAILED: %s%s%s at %s:%u\n",
	    condition,
	    (*function ? " in" : ""), function,
	    filename, lineno);
    u4c_throw;
}

static void
run_function(u4c_function_t *f)
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
    unsigned int i;

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
	    run_function(state->fixtures[i]);
}

static bool
valgrind_errors(void)
{
    unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
    unsigned long nerrors;
    bool fail = false;

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    if (leaked)
    {
	fprintf(stderr, "FAIL %lu bytes of memory leaked\n", leaked);
	fail = true;
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	fprintf(stderr, "FAIL %lu errors found by valgrind\n", nerrors);
	fail = true;
    }

    return fail;
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

    fullname = __u4c_testnode_fullname(tn);
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
	    run_function(tn->funcs[FT_TEST]);
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

    if (valgrind_errors())
	fail = true;

    state->nrun++;
    if (fail)
	state->nfailed++;
    else
	fprintf(stderr, "PASS %s\n", fullname);
    xfree(fullname);
}

void
__u4c_run_tests(u4c_globalstate_t *state,
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
	    __u4c_run_tests(state, child);
    }
}

void
__u4c_summarise_results(u4c_globalstate_t *state)
{
    fprintf(stderr, "u4c: %u run %u failed\n",
	    state->nrun, state->nfailed);
}


