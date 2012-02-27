#include "common.h"
#include <sys/wait.h>
#include <sys/poll.h>
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

using namespace std;

__u4c_exceptstate_t __u4c_exceptstate;
u4c_globalstate_t *u4c_globalstate_t::state = 0;
static volatile int caught_sigchld = 0;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
u4c_globalstate_t::add_listener(u4c_listener_t *l)
{
    u4c_listener_t **tailp;

    /* append to the list.  The order of adding is preserved for
     * dispatching */
    for (tailp = &listeners ; *tailp ; tailp = &(*tailp)->next)
	;
    *tailp = l;
    l->next = 0;
}

void
u4c_globalstate_t::set_listener(u4c_listener_t *l)
{
    /* just throw away the old ones */
    listeners = l;
    l->next = 0;
}

static void
handle_sigchld(int sig __attribute__((unused)))
{
    caught_sigchld = 1;
}

void
u4c_globalstate_t::begin()
{
    static bool init = false;

    if (!init)
    {
	signal(SIGCHLD, handle_sigchld);
	init = true;
    }

    state = this;
    dispatch_listeners(state, begin);
}

void
u4c_globalstate_t::end()
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
	const char *classname = 0;

	if (u4c_globalstate_t::state->spiegel->describe_address(pc, &norm.filename,
			&norm.lineno, &classname, &norm.function))
	{
	    static char fullname[1024];
	    if (classname)
	    {
		snprintf(fullname, sizeof(fullname), "%s::%s",
			 classname, norm.function);
		norm.function = fullname;
	    }
	}
	else
	{
	    static char pcbuf[32];
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
    dispatch_listeners(u4c_globalstate_t::state, add_event, ev, ft);

    switch (ev->which)
    {
    case EV_ASSERT:
    case EV_EXIT:
    case EV_SIGNAL:
    case EV_FIXTURE:
    case EV_VALGRIND:
    case EV_SLMATCH:
	return R_FAIL;
    case EV_EXPASS:
	return R_PASS;
    case EV_EXFAIL:
	return R_FAIL;
    case EV_EXNA:
	return R_NOTAPPLICABLE;
    default:
	/* there was an event, but it makes no difference */
	return R_UNKNOWN;
    }
}

static u4c_child_t *
fork_child(u4c_testnode_t *tn)
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
	u4c_globalstate_t::state->event_pipe = pipefd[PIPE_WRITE];
	return NULL;
    }

    /* parent process */

    {
	char *nm = __u4c_testnode_fullname(tn);
	fprintf(stderr, "u4c: spawned child process %d for %s\n",
		(int)pid, nm);
	xfree(nm);
    }
    child = (u4c_child_t *)xmalloc(sizeof(*child));
    child->pid = pid;
    child->node = tn;
    child->result = R_UNKNOWN;
    close(pipefd[PIPE_WRITE]);
    child->event_pipe = pipefd[PIPE_READ];
    child->next = u4c_globalstate_t::state->children;
    u4c_globalstate_t::state->children = child;
    u4c_globalstate_t::state->nchildren++;

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

static void
handle_events(void)
{
    u4c_globalstate_t *state = u4c_globalstate_t::state;
    u4c_child_t *child;
    int r;
    int i;

    if (!state->nchildren)
	return;

    while (!caught_sigchld)
    {
	if (state->npfd != state->nchildren)
	{
	    state->npfd = state->nchildren;
	    state->pfd = (struct pollfd *)xrealloc(state->pfd, state->npfd * sizeof(struct pollfd));
	}
	memset(state->pfd, 0, state->npfd * sizeof(struct pollfd));
	for (i = 0, child = state->children ; child ; i++, child = child->next)
	{
	    if (child->finished)
		continue;
	    state->pfd[i].fd = child->event_pipe;
	    state->pfd[i].events = POLLIN;
	}

	r = poll(state->pfd, state->npfd, -1);
	if (r < 0)
	{
	    if (errno == EINTR)
		continue;
	    perror("u4c: poll");
	    return;
	}
	/* TODO: implement test timeout handling here */

	for (i = 0, child = state->children ; child ; i++, child = child->next)
	{
	    if (child->finished)
		continue;
	    if (!(state->pfd[i].revents & POLLIN))
		continue;
	    if (!__u4c_handle_proxy_call(child->event_pipe, &child->result))
		child->finished = true;
	}
    }
}

static void
reap_children(void)
{
    u4c_globalstate_t *state = u4c_globalstate_t::state;
    pid_t pid;
    u4c_child_t **prevp, *child;
    int status;
    char msg[1024];

    for (;;)
    {
	pid = waitpid(-1, &status, WNOHANG);
	if (pid == 0)
	    break;
	if (pid < 0)
	{
	    if (errno == ESRCH || errno == ECHILD)
		break;
	    perror("u4c: waitpid");
	    return;
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
	    /* TODO: this is probably eventworthy */
	    continue;	    /* whatever */
	}

	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		u4c_event_t ev(EV_EXIT, msg, NULL, 0, NULL);
		snprintf(msg, sizeof(msg),
			 "child process %d exited with %d",
			 (int)pid, WEXITSTATUS(status));
		__u4c_merge(child->result, __u4c_raise_event(&ev, FT_UNKNOWN));
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    u4c_event_t ev(EV_SIGNAL, msg, NULL, 0, NULL);
	    snprintf(msg, sizeof(msg),
		    "child process %d died on signal %d",
		    (int)pid, WTERMSIG(status));
	    __u4c_merge(child->result, __u4c_raise_event(&ev, FT_UNKNOWN));
	}

	/* notify listeners */
	state->nfailed += (child->result == R_FAIL);
	state->nrun++;
	dispatch_listeners(state, finished, child->result);
	dispatch_listeners(state, end_node, child->node);

	/* detach and clean up */
	*prevp = child->next;
	state->nchildren--;
	close(child->event_pipe);
	free(child);
    }

    caught_sigchld = 0;
    /* nothing to reap here, move along */
}

static void
run_function(u4c_function_t *f)
{
    vector<spiegel::value_t> args;
    spiegel::value_t ret = f->func->invoke(args);

    if (f->type == FT_TEST)
    {
	assert(ret.which == spiegel::type_t::TC_VOID);
    }
    else
    {
	assert(ret.which == spiegel::type_t::TC_SIGNED_INT);
	int r = ret.val.vsint;

	if (r)
	{
	    static char cond[64];
	    snprintf(cond, sizeof(cond), "fixture retured %d", r);
	    u4c_throw(u4c_event_t(EV_FIXTURE, cond, f->filename.c_str(),
			    0, f->func->get_name()));
	}
    }
}

static void
run_fixtures(u4c_testnode_t *tn,
	     enum u4c_functype type)
{
    u4c_globalstate_t *state = u4c_globalstate_t::state;
    u4c_testnode_t *a;
    unsigned int i;

    if (!state->fixtures)
	state->fixtures = (u4c_function_t **)xmalloc(sizeof(u4c_function_t*) *
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
	u4c_event_t ev(EV_VALGRIND, msg, NULL, 0, NULL);
	snprintf(msg, sizeof(msg),
		 "%lu bytes of memory leaked", leaked);
	__u4c_merge(res, __u4c_raise_event(&ev, FT_UNKNOWN));
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	u4c_event_t ev(EV_VALGRIND, msg, NULL, 0, NULL);
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


void
__u4c_begin_test(u4c_testnode_t *tn)
{
    u4c_globalstate_t *state = u4c_globalstate_t::state;
    u4c_child_t *child;
    u4c_result_t res;

    {
	static int n = 0;
	if (++n > 60)
	    return;
    }

    {
	char *nm = __u4c_testnode_fullname(tn);
	fprintf(stderr, "%s: begin test %s\n",
		u4c_reltimestamp(), nm);
	xfree(nm);
    }

    dispatch_listeners(state, begin_node, tn);

    child = fork_child(tn);
    if (child)
	return; /* parent process */

    /* child process */
    state->set_listener(new u4c_proxy_listener_t(state->event_pipe));
    res = run_test_code(tn);
    dispatch_listeners(state, finished, res);
    {
	char *nm = __u4c_testnode_fullname(tn);
	fprintf(stderr, "u4c: child process %d (%s) finishing\n",
		(int)getpid(), nm);
	xfree(nm);
    }
    exit(0);
}

void
__u4c_wait(void)
{
    handle_events();
    reap_children();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
