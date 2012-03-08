#include "common.h"
#include <sys/wait.h>
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

using namespace std;

__u4c_exceptstate_t __u4c_exceptstate;
u4c_globalstate_t *u4c_globalstate_t::running_;
static volatile int caught_sigchld = 0;

#define dispatch_listeners(func, ...) \
    do { \
	vector<u4c::listener_t*>::iterator _i; \
	for (_i = listeners_.begin() ; _i != listeners_.end() ; ++_i) \
	    (*_i)->func(__VA_ARGS__); \
    } while(0)

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
u4c_globalstate_t::add_listener(u4c::listener_t *l)
{
    /* append to the list.  The order of adding is preserved for
     * dispatching */
    listeners_.push_back(l);
}

void
u4c_globalstate_t::set_listener(u4c::listener_t *l)
{
    /* just throw away the old ones */
    listeners_.clear();
    listeners_.push_back(l);
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

    running_ = this;
    dispatch_listeners(begin);
}

void
u4c_globalstate_t::end()
{
    dispatch_listeners(end);
    running_ = 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const u4c_event_t *
u4c_globalstate_t::normalise_event(const u4c_event_t *ev)
{
    static u4c_event_t norm;

    static char filebuf[1024];
    static char funcbuf[1024];

    memset(&norm, 0, sizeof(norm));
    norm.which = ev->which;
    norm.description = xstr(ev->description);
    if (ev->lineno == ~0U)
    {
	unsigned long pc = (unsigned long)ev->filename;
	spiegel::location_t loc;

	if (spiegel::describe_address(pc, loc))
	{
	    snprintf(filebuf, sizeof(filebuf), "%s",
		     loc.compile_unit_->get_absolute_path().c_str());
	    norm.filename = filebuf;

	    norm.lineno = loc.line_;

	    if (loc.class_)
		snprintf(funcbuf, sizeof(funcbuf), "%s::%s",
			 loc.class_->get_name().c_str(),
			 loc.function_->get_name().c_str());
	    else
		snprintf(funcbuf, sizeof(funcbuf), "%s",
			 loc.function_->get_name().c_str());
	    norm.function = funcbuf;
	}
	else
	{
	    snprintf(funcbuf, sizeof(funcbuf), "(0x%lx)", pc);
	    norm.function = funcbuf;
	    norm.filename = "";
	}
    }
    else if (ev->lineno == ~0U-1)
    {
	spiegel::function_t *f = (spiegel::function_t *)ev->function;

	snprintf(filebuf, sizeof(filebuf), "%s",
		 f->get_compile_unit()->get_absolute_path().c_str());
	norm.filename = filebuf;

	snprintf(funcbuf, sizeof(funcbuf), "%s",
		 f->get_name().c_str());
	norm.function = funcbuf;
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

u4c::result_t
u4c_globalstate_t::raise_event(const u4c_event_t *ev, u4c::functype_t ft)
{
    ev = normalise_event(ev);
    dispatch_listeners(add_event, ev, ft);

    switch (ev->which)
    {
    case EV_ASSERT:
    case EV_EXIT:
    case EV_SIGNAL:
    case EV_FIXTURE:
    case EV_VALGRIND:
    case EV_SLMATCH:
	return u4c::R_FAIL;
    case EV_EXPASS:
	return u4c::R_PASS;
    case EV_EXFAIL:
	return u4c::R_FAIL;
    case EV_EXNA:
	return u4c::R_NOTAPPLICABLE;
    default:
	/* there was an event, but it makes no difference */
	return u4c::R_UNKNOWN;
    }
}

u4c::child_t *
u4c_globalstate_t::fork_child(u4c::testnode_t *tn)
{
    pid_t pid;
#define PIPE_READ 0
#define PIPE_WRITE 1
    int pipefd[2];
    u4c::child_t *child;
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
	event_pipe_ = pipefd[PIPE_WRITE];
	return NULL;
    }

    /* parent process */

    fprintf(stderr, "u4c: spawned child process %d for %s\n",
	    (int)pid, tn->get_fullname().c_str());
    close(pipefd[PIPE_WRITE]);
    child = new u4c::child_t(pid, pipefd[PIPE_READ], tn);
    children_.push_back(child);

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

void
u4c_globalstate_t::handle_events()
{
    int r;

    if (!children_.size())
	return;

    while (!caught_sigchld)
    {
	pfd_.resize(children_.size());
	vector<u4c::child_t*>::iterator citr;
	vector<struct pollfd>::iterator pitr;
	for (pitr = pfd_.begin(), citr = children_.begin() ;
	     citr != children_.end() ; ++pitr, ++citr)
	    (*citr)->poll_setup(*pitr);

	r = poll(pfd_.data(), pfd_.size(), -1);
	if (r < 0)
	{
	    if (errno == EINTR)
		continue;
	    perror("u4c: poll");
	    return;
	}
	/* TODO: implement test timeout handling here */

	for (pitr = pfd_.begin(), citr = children_.begin() ;
	     citr != children_.end() ; ++pitr, ++citr)
	    (*citr)->poll_handle(*pitr);
    }
}

void
u4c_globalstate_t::reap_children()
{
    pid_t pid;
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
	vector<u4c::child_t*>::iterator itr;
	for (itr = children_.begin() ;
	     itr != children_.end() && (*itr)->get_pid() != pid ;
	     ++itr)
	    ;
	if (itr == children_.end())
	{
	    /* some other process */
	    fprintf(stderr, "u4c: reaped stray process %d\n", (int)pid);
	    /* TODO: this is probably eventworthy */
	    continue;	    /* whatever */
	}
	u4c::child_t *child = *itr;

	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		u4c_event_t ev(EV_EXIT, msg, NULL, 0, NULL);
		snprintf(msg, sizeof(msg),
			 "child process %d exited with %d",
			 (int)pid, WEXITSTATUS(status));
		child->merge_result(raise_event(&ev, u4c::FT_UNKNOWN));
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    u4c_event_t ev(EV_SIGNAL, msg, NULL, 0, NULL);
	    snprintf(msg, sizeof(msg),
		    "child process %d died on signal %d",
		    (int)pid, WTERMSIG(status));
	    child->merge_result(raise_event(&ev, u4c::FT_UNKNOWN));
	}

	/* notify listeners */
	nfailed_ += (child->get_result() == u4c::R_FAIL);
	nrun_++;
	dispatch_listeners(finished, child->get_result());
	dispatch_listeners(end_node, child->get_node());

	/* detach and clean up */
	children_.erase(itr);
	delete child;
    }

    caught_sigchld = 0;
    /* nothing to reap here, move along */
}

void
u4c_globalstate_t::run_function(u4c::functype_t ft, spiegel::function_t *f)
{
    vector<spiegel::value_t> args;
    spiegel::value_t ret = f->invoke(args);

    if (ft == u4c::FT_TEST)
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
	    snprintf(cond, sizeof(cond), "fixture returned %d", r);
	    u4c_throw(u4c_event_t(EV_FIXTURE, cond, f));
	}
    }
}

void
u4c_globalstate_t::run_fixtures(u4c::testnode_t *tn, u4c::functype_t type)
{
    list<spiegel::function_t*> fixtures = tn->get_fixtures(type);
    list<spiegel::function_t*>::iterator itr;
    for (itr = fixtures.begin() ; itr != fixtures.end() ; ++itr)
	run_function(type, *itr);
}

u4c::result_t
u4c_globalstate_t::valgrind_errors()
{
    unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
    unsigned long nerrors;
    u4c::result_t res = u4c::R_UNKNOWN;
    char msg[1024];

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    if (leaked)
    {
	u4c_event_t ev(EV_VALGRIND, msg, NULL, 0, NULL);
	snprintf(msg, sizeof(msg),
		 "%lu bytes of memory leaked", leaked);
	__u4c_merge(res, raise_event(&ev, u4c::FT_UNKNOWN));
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	u4c_event_t ev(EV_VALGRIND, msg, NULL, 0, NULL);
	snprintf(msg, sizeof(msg),
		 "%lu unsuppressed errors found by valgrind", nerrors);
	__u4c_merge(res, raise_event(&ev, u4c::FT_UNKNOWN));
    }

    return res;
}

u4c::result_t
u4c_globalstate_t::run_test_code(u4c::testnode_t *tn)
{
    u4c::result_t res = u4c::R_UNKNOWN;
    const u4c_event_t *ev;

    u4c_try
    {
	run_fixtures(tn, u4c::FT_BEFORE);
    }
    u4c_catch(ev)
    {
	__u4c_merge(res, raise_event(ev, u4c::FT_BEFORE));
    }

    if (res == u4c::R_UNKNOWN)
    {
	u4c_try
	{
	    run_function(u4c::FT_TEST, tn->get_function(u4c::FT_TEST));
	}
	u4c_catch(ev)
	{
	    __u4c_merge(res, raise_event(ev, u4c::FT_TEST));
	}

	u4c_try
	{
	    run_fixtures(tn, u4c::FT_AFTER);
	}
	u4c_catch(ev)
	{
	    __u4c_merge(res, raise_event(ev, u4c::FT_AFTER));
	}

	/* If we got this far and nothing bad
	 * happened, we might have passed */
	__u4c_merge(res, u4c::R_PASS);
    }

    __u4c_merge(res, valgrind_errors());
    return res;
}


void
u4c_globalstate_t::begin_test(u4c::testnode_t *tn)
{
    u4c::child_t *child;
    u4c::result_t res;

    {
	static int n = 0;
	if (++n > 60)
	    return;
    }

    fprintf(stderr, "%s: begin test %s\n",
	    u4c_reltimestamp(), tn->get_fullname().c_str());

    dispatch_listeners(begin_node, tn);

    child = fork_child(tn);
    if (child)
	return; /* parent process */

    /* child process */
    set_listener(new u4c::proxy_listener_t(event_pipe_));
    res = run_test_code(tn);
    dispatch_listeners(finished, res);
    fprintf(stderr, "u4c: child process %d (%s) finishing\n",
	    (int)getpid(), tn->get_fullname().c_str());
    exit(0);
}

void
u4c_globalstate_t::wait()
{
    handle_events();
    reap_children();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
