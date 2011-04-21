#include "common.h"
#include <sys/wait.h>
#include <sys/poll.h>
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


static u4c_child_t *
isolate_begin(u4c_globalstate_t *state)
{
    pid_t pid;
#define PIPE_READ 0
#define PIPE_WRITE 1
    int pipefd[2];
    u4c_child_t *child;
    int delay_ms = 10;
    int max_sleeps = 20;
    int r;

    r = pipe(pipefd);
    if (r < 0)
    {
	perror("u4c: pipe");
	exit(1);
    }

    for (;;)
    {
	pid = fork();
	if (pid < 0)
	{
	    if (errno == EAGAIN && max_sleeps-- > 0)
	    {
		/* rats, we fork-bombed, try again after a delay */
		fprintf(stderr, "u4c: fork bomb! sleeping %u ms.\n",
			delay_ms);
		poll(0, 0, delay_ms);
		delay_ms += (delay_ms>>1);	/* exponential backoff */
		continue;
	    }
	    perror("u4c: fork");
	    exit(1);
	}
	break;
    }

    if (!pid)
    {
	/* child process: return, will run the test */
	close(pipefd[PIPE_READ]);
	state->event_pipe = pipefd[PIPE_WRITE];
	return 0;
    }

    /* parent process */

    fprintf(stderr, "u4c: spawned child process %d\n",
	    (int)pid);
    child = xmalloc(sizeof(*child));
    child->pid = pid;
    close(pipefd[PIPE_WRITE]);
    child->event_pipe = pipefd[PIPE_READ];
    child->next = state->children;
    state->children = child;
    state->nchildren++;

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

static bool
isolate_end(u4c_globalstate_t *state,
	    u4c_child_t *special,
	    bool fail)
{
    pid_t pid;
    u4c_child_t **prevp, *child;
    int status;

    if (!special)
    {
	fprintf(stderr, "u4c: child process %d finishing\n",
		(int)getpid());
	exit(fail);
    }

    for (;;)
    {
	pid = waitpid(-1, &status, 0);
	if (pid < 0)
	{
	    if (errno == ESRCH || errno == ECHILD)
		fprintf(stderr, "u4c: child %d already reaped!?\n",
			special->pid);
	    else
		perror("u4c: waitpid");
	    return false;
	}
	if (WIFSTOPPED(status))
	{
	    fprintf(stderr, "u4c: process %d stopped on signal %d, ignoring\n",
		    (int)pid, WSTOPSIG(status));
	    continue;
	}
	for (child = state->children, prevp = &state->children ;
	     child && child->pid != pid ;
	     prevp = &child->next, child = child->next)
	    ;
	if (!child)
	{
	    /* some other process */
	    fprintf(stderr, "u4c: reaped stray process %d\n", (int)pid);
	    continue;	    /* whatever */
	}

	fail = false;
	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		fprintf(stderr, "FAIL child process %d exited with %d\n",
			(int)pid, WEXITSTATUS(status));
		fail = true;
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    fprintf(stderr, "FAIL child process %d died on signal %d\n",
		    (int)pid, WTERMSIG(status));
	    fail = true;
	}

	/* detach and clean up */
	*prevp = child->next;
	state->nchildren--;
	close(child->event_pipe);
	free(child);

	/* Are you the one that I've been waiting for? */
	if (child == special)
	    return fail;
    }
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

static bool
run_test_code(u4c_globalstate_t *state,
	      u4c_testnode_t *tn,
	      const char *fullname)
{
    bool fail = false;

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

    return fail;
}

static void
run_test(u4c_globalstate_t *state,
	 u4c_testnode_t *tn)
{
    char *fullname;
    u4c_child_t *child;
    bool fail = false;

    {
	static int n = 0;
	if (++n > 10)
	    return;
    }

    fullname = __u4c_testnode_fullname(tn);
    child = isolate_begin(state);
    if (!child)
    {
	/* child process */
	fail = run_test_code(state, tn, fullname);
    }
    fail = isolate_end(state, child, fail);

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


