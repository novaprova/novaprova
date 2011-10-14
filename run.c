#include "common.h"
#include <sys/wait.h>
#include <sys/poll.h>
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

__u4c_exceptstate_t __u4c_exceptstate;
static u4c_globalstate_t *state;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
__u4c_add_listener(u4c_globalstate_t *st, u4c_listener_t *l)
{
    u4c_listener_t **tailp;

    /* append to the list.  The order of adding is preserved for
     * dispatching */
    for (tailp = &st->listeners ; *tailp ; tailp = &(*tailp)->next)
	;
    *tailp = l;
    l->next = 0;
}

void
__u4c_set_listener(u4c_globalstate_t *st, u4c_listener_t *l)
{
    /* just throw away the old ones */
    st->listeners = l;
    l->next = 0;
}

void
__u4c_begin(u4c_globalstate_t *s)
{
    state = s;
    dispatch_listeners(state, begin);
}

void
__u4c_end(void)
{
    dispatch_listeners(state, end);
    state = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static const u4c_event_t *
normalise_event(const u4c_event_t *ev)
{
    static u4c_event_t norm;

    memset(&norm, 0, sizeof(norm));
    norm.which = ev->which;
    norm.description = xstr(ev->description);
    if (ev->lineno == ~0U)
    {
	unsigned long pc = (unsigned long)ev->filename;
	static char pcbuf[32];

	if (!__u4c_describe_address(state, pc,
				    &norm.filename,
				    &norm.lineno,
				    &norm.function))
	{
	    snprintf(pcbuf, sizeof(pcbuf), "(0x%lx)", pc);
	    norm.function = pcbuf;
	    norm.filename = "";
	}
    }
    else
    {
	norm.filename = xstr(ev->filename);
	norm.lineno = ev->lineno;
	norm.function = xstr(ev->function);
    }

    return &norm;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

u4c_result_t __u4c_raise_event(const u4c_event_t *ev, enum u4c_functype ft)
{
    ev = normalise_event(ev);
    dispatch_listeners(state, add_event, ev, ft);

    switch (ev->which)
    {
    case EV_ASSERT:
    case EV_EXIT:
    case EV_SIGNAL:
    case EV_FIXTURE:
    case EV_VALGRIND:
	return R_FAIL;
    default:
	return R_UNKNOWN;
    }
}

static u4c_child_t *
isolate_begin(void)
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
	__u4c_set_listener(state, __u4c_proxy_listener(state->event_pipe));
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


static void
isolate_end(u4c_child_t *special)
{
    if (!special)
    {
	fprintf(stderr, "u4c: child process %d finishing\n",
		(int)getpid());
	exit(0);
    }
}

static u4c_result_t
isolate_wait(u4c_child_t *special)
{
    pid_t pid;
    u4c_child_t **prevp, *child;
    int status;
    u4c_result_t res = R_UNKNOWN;
    int r;
    char msg[1024];

    /* Note: too lazy to handle more than 1 child here */
    do
    {
	struct pollfd pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = special->event_pipe;
	pfd.events = POLLIN;
	r = poll(&pfd, 1, -1);
	if (r < 0)
	{
	    perror("u4c: poll");
	    return res;
	}
	/* TODO: implement test timeout handling here */
    }
    while (__u4c_handle_proxy_call(special->event_pipe, &res));

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
	    return R_UNKNOWN;
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

	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		u4c_event_t ev = event(EV_EXIT, msg, NULL, 0, NULL);
		snprintf(msg, sizeof(msg),
			 "child process %d exited with %d",
			 (int)pid, WEXITSTATUS(status));
		__u4c_merge(res, __u4c_raise_event(&ev, FT_UNKNOWN));
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    u4c_event_t ev = event(EV_SIGNAL, msg, NULL, 0, NULL);
	    snprintf(msg, sizeof(msg),
		    "child process %d died on signal %d",
		    (int)pid, WTERMSIG(status));
	    __u4c_merge(res, __u4c_raise_event(&ev, FT_UNKNOWN));
	}

	/* detach and clean up */
	*prevp = child->next;
	state->nchildren--;
	close(child->event_pipe);
	free(child);

	/* Are you the one that I've been waiting for? */
	if (child == special)
	    return res;
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
	int r;

	r = call();

	if (r)
	{
	    static char cond[64];
	    snprintf(cond, sizeof(cond), "fixture retured %d", r);
	    u4c_throw(event(EV_FIXTURE, cond, f->filename,
			    0, f->name));
	}
    }
}

static void
run_fixtures(u4c_testnode_t *tn,
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

static u4c_result_t
valgrind_errors(void)
{
    unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
    unsigned long nerrors;
    u4c_result_t res = R_UNKNOWN;
    char msg[1024];

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    if (leaked)
    {
	u4c_event_t ev = event(EV_VALGRIND, msg, NULL, 0, NULL);
	snprintf(msg, sizeof(msg),
		 "%lu bytes of memory leaked", leaked);
	__u4c_merge(res, __u4c_raise_event(&ev, FT_UNKNOWN));
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	u4c_event_t ev = event(EV_VALGRIND, msg, NULL, 0, NULL);
	snprintf(msg, sizeof(msg),
		 "%lu unsuppressed errors found by valgrind", nerrors);
	__u4c_merge(res, __u4c_raise_event(&ev, FT_UNKNOWN));
    }

    return res;
}

static u4c_result_t
run_test_code(u4c_testnode_t *tn)
{
    u4c_result_t res = R_UNKNOWN;
    const u4c_event_t *ev;

    u4c_try
    {
	run_fixtures(tn, FT_BEFORE);
    }
    u4c_catch(ev)
    {
	__u4c_merge(res, __u4c_raise_event(ev, FT_BEFORE));
    }

    if (res == R_UNKNOWN)
    {
	u4c_try
	{
	    run_function(tn->funcs[FT_TEST]);
	}
	u4c_catch(ev)
	{
	    __u4c_merge(res, __u4c_raise_event(ev, FT_TEST));
	}

	u4c_try
	{
	    run_fixtures(tn, FT_AFTER);
	}
	u4c_catch(ev)
	{
	    __u4c_merge(res, __u4c_raise_event(ev, FT_AFTER));
	}

	/* If we got this far and nothing bad
	 * happened, we might have passed */
	__u4c_merge(res, R_PASS);
    }

    __u4c_merge(res, valgrind_errors());
    return res;
}

static void
run_test(u4c_testnode_t *tn)
{
    u4c_child_t *child;
    u4c_result_t res = R_UNKNOWN;

    {
	static int n = 0;
	if (++n > 10)
	    return;
    }

    dispatch_listeners(state, begin_node, tn);

    child = isolate_begin();
    if (!child)
    {
	/* child process */
	__u4c_merge(res, run_test_code(tn));
    }
    else
    {
	__u4c_merge(res, isolate_wait(child));
    }

    state->nfailed += (res == R_FAIL);
    state->nrun++;
    dispatch_listeners(state, finished, res);
    dispatch_listeners(state, end_node, tn);

    isolate_end(child);
}

void
__u4c_run_tests(u4c_testnode_t *tn)
{
    u4c_testnode_t *child;

    if (tn->funcs[FT_TEST])
    {
	run_test(tn);
    }
    else
    {
	for (child = tn->children ; child ; child = child->next)
	    __u4c_run_tests(child);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
