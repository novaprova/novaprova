#include "u4c/runner.hxx"
#include "u4c/testnode.hxx"
#include "u4c/job.hxx"
#include "u4c/plan.hxx"
#include "u4c/text_listener.hxx"
#include "u4c/proxy_listener.hxx"
#include "u4c/junit_listener.hxx"
#include "u4c/child.hxx"
#include "spiegel/spiegel.hxx"
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/memcheck.h>

__u4c_exceptstate_t __u4c_exceptstate;

namespace u4c {
using namespace std;

#define dispatch_listeners(func, ...) \
    do { \
	vector<listener_t*>::iterator _i; \
	for (_i = listeners_.begin() ; _i != listeners_.end() ; ++_i) \
	    (*_i)->func(__VA_ARGS__); \
    } while(0)

runner_t *runner_t::running_;

runner_t::runner_t()
{
    maxchildren_ = 1;
}

runner_t::~runner_t()
{
    destroy_listeners();
}

void
runner_t::set_concurrency(int n)
{
    if (n == 0)
    {
	/* shorthand for "best possible" */
	n = sysconf(_SC_NPROCESSORS_ONLN);
    }
    if (n < 1)
	n = 1;
    maxchildren_ = n;
}

void
runner_t::list_tests(plan_t *plan) const
{
    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	plan_t *plan = new plan_t();
	plan->add_node(testmanager_t::instance()->get_root());
	ourplan = true;
    }

    /* iterate over all tests */
    testnode_t *tn = 0;
    plan_t::iterator pitr = plan->begin();
    plan_t::iterator pend = plan->end();
    while (pitr != pend)
    {
	if (pitr.get_node() != tn)
	{
	    tn = pitr.get_node();
	    printf("%s\n", tn->get_fullname().c_str());
	}
	++pitr;
    }

    if (ourplan)
	delete plan;
}

int
runner_t::run_tests(plan_t *plan)
{
    bool ourplan = false;
    if (!plan)
    {
	/* build a default plan with all the tests */
	plan =  new plan_t();
	plan->add_node(testmanager_t::instance()->get_root());
	ourplan = true;
    }

    if (!listeners_.size())
	add_listener(new text_listener_t);

    begin();
    plan_t::iterator pitr = plan->begin();
    plan_t::iterator pend = plan->end();
    for (;;)
    {
	while (children_.size() < maxchildren_ && pitr != pend)
	{
	    begin_job(new job_t(pitr));
	    ++pitr;
	}
	if (!children_.size())
	    break;
	wait();
    }
    end();

    if (ourplan)
	delete plan;

    return !!nfailed_;
}

void
runner_t::destroy_listeners()
{
    vector<listener_t*>::iterator i;
    for (i = listeners_.begin() ; i != listeners_.end() ; ++i)
	delete *i;
    listeners_.clear();
}

void
runner_t::add_listener(listener_t *l)
{
    /* append to the list.  The order of adding is preserved for
     * dispatching */
    listeners_.push_back(l);
}

void
runner_t::set_listener(listener_t *l)
{
    destroy_listeners();
    listeners_.push_back(l);
}

static volatile int caught_sigchld = 0;
static void
handle_sigchld(int sig __attribute__((unused)))
{
    caught_sigchld = 1;
}

void
runner_t::begin()
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
runner_t::end()
{
    dispatch_listeners(end);
    running_ = 0;
}


result_t
runner_t::raise_event(job_t *j, const event_t *ev)
{
    ev = ev->normalise();
    dispatch_listeners(add_event, j, ev);
    return ev->get_result();
}

child_t *
runner_t::fork_child(job_t *j)
{
    pid_t pid;
#define PIPE_READ 0
#define PIPE_WRITE 1
    int pipefd[2];
    child_t *child;
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
	    (int)pid, j->as_string().c_str());
    close(pipefd[PIPE_WRITE]);
    child = new child_t(pid, pipefd[PIPE_READ], j);
    children_.push_back(child);

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

void
runner_t::handle_events()
{
    int r;

    if (!children_.size())
	return;

    while (!caught_sigchld)
    {
	pfd_.clear();
	vector<child_t*>::iterator citr;
	for (citr = children_.begin() ; citr != children_.end() ; ++citr)
	{
	    struct pollfd p;
	    memset(&p, 0, sizeof(struct pollfd));
	    int fd = (*citr)->get_input_fd();
	    if (fd >= 0)
	    {
		p.fd = fd;
		p.events |= POLLIN;
	    }
	    pfd_.push_back(p);
	}

	r = poll(pfd_.data(), pfd_.size(), -1);
	if (r < 0)
	{
	    if (errno == EINTR)
		continue;
	    perror("u4c: poll");
	    return;
	}
	/* TODO: implement test timeout handling here */

	if (r > 0)
	{
	    /* poll indicated some fds are available */
	    vector<struct pollfd>::iterator pitr;
	    for (pitr = pfd_.begin(), citr = children_.begin() ;
		 citr != children_.end() ; ++pitr, ++citr)
		if ((pitr->revents & POLLIN))
		    (*citr)->handle_input();
	}
    }
}

void
runner_t::reap_children()
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
	vector<child_t*>::iterator itr;
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
	child_t *child = *itr;

	if (WIFEXITED(status))
	{
	    if (WEXITSTATUS(status))
	    {
		snprintf(msg, sizeof(msg),
			 "child process %d exited with %d",
			 (int)pid, WEXITSTATUS(status));
		event_t ev(EV_EXIT, msg);
		child->merge_result(raise_event(child->get_job(), &ev));
	    }
	}
	else if (WIFSIGNALED(status))
	{
	    snprintf(msg, sizeof(msg),
		    "child process %d died on signal %d",
		    (int)pid, WTERMSIG(status));
	    event_t ev(EV_SIGNAL, msg);
	    child->merge_result(raise_event(child->get_job(), &ev));
	}

	/* test is finished; if nothing went wrong then PASS */
	child->merge_result(u4c::R_PASS);

	/* notify listeners */
	nfailed_ += (child->get_result() == R_FAIL);
	nrun_++;
	child->get_job()->post_run(true);
	dispatch_listeners(end_job, child->get_job(), child->get_result());

	/* detach and clean up */
	children_.erase(itr);
	delete child;
    }

    caught_sigchld = 0;
    /* nothing to reap here, move along */
}

void
runner_t::run_function(functype_t ft, spiegel::function_t *f)
{
    vector<spiegel::value_t> args;
    spiegel::value_t ret = f->invoke(args);

    if (ft == FT_TEST)
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
	    u4c_throw(event_t(EV_FIXTURE, cond).in_function(f));
	}
    }
}

void
runner_t::run_fixtures(testnode_t *tn, functype_t type)
{
    list<spiegel::function_t*> fixtures = tn->get_fixtures(type);
    list<spiegel::function_t*>::iterator itr;
    for (itr = fixtures.begin() ; itr != fixtures.end() ; ++itr)
	run_function(type, *itr);
}

result_t
runner_t::valgrind_errors(job_t *j)
{
    unsigned long leaked = 0, dubious = 0, reachable = 0, suppressed = 0;
    unsigned long nerrors;
    result_t res = R_UNKNOWN;
    char msg[1024];

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    if (leaked)
    {
	snprintf(msg, sizeof(msg),
		 "%lu bytes of memory leaked", leaked);
	event_t ev(EV_VALGRIND, msg);
	res = merge(res, raise_event(j, &ev));
    }

    nerrors = VALGRIND_COUNT_ERRORS;
    if (nerrors)
    {
	snprintf(msg, sizeof(msg),
		 "%lu unsuppressed errors found by valgrind", nerrors);
	event_t ev(EV_VALGRIND, msg);
	res = merge(res, raise_event(j, &ev));
    }

    return res;
}

result_t
runner_t::run_test_code(job_t *j)
{
    testnode_t *tn = j->get_node();
    result_t res = R_UNKNOWN;
    event_t *ev;

    j->pre_run(false);

    u4c_try
    {
	run_fixtures(tn, FT_BEFORE);
    }
    u4c_catch(ev)
    {
	ev->in_functype(FT_BEFORE);
	res = merge(res, raise_event(j, ev));
    }

    if (res == R_UNKNOWN)
    {
	u4c_try
	{
	    run_function(FT_TEST, tn->get_function(FT_TEST));
	}
	u4c_catch(ev)
	{
	    ev->in_functype(FT_TEST);
	    res = merge(res, raise_event(j, ev));
	}

	u4c_try
	{
	    run_fixtures(tn, FT_AFTER);
	}
	u4c_catch(ev)
	{
	    ev->in_functype(FT_AFTER);
	    res = merge(res, raise_event(j, ev));
	}

	/* If we got this far and nothing bad
	 * happened, we might have passed */
	res = merge(res, R_PASS);
    }

    j->post_run(false);
    res = merge(res, valgrind_errors(j));
    return res;
}


void
runner_t::begin_job(job_t *j)
{
    child_t *child;
    result_t res;

    {
	static int n = 0;
	if (++n > 60)
	    return;
    }

    fprintf(stderr, "%s: begin job %s\n",
	    rel_timestamp().c_str(), j->as_string().c_str());

    dispatch_listeners(begin_job, j);
    j->pre_run(true);

    child = fork_child(j);
    if (child)
	return; /* parent process */

    /* child process */
    set_listener(new proxy_listener_t(event_pipe_));
    res = run_test_code(j);
    dispatch_listeners(end_job, j, res);
    fprintf(stderr, "u4c: child process %d (%s) finishing\n",
	    (int)getpid(), j->as_string().c_str());
    delete j;
    exit(0);
}

void
runner_t::wait()
{
    handle_events();
    reap_children();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern "C" void
u4c_set_concurrency(u4c_runner_t *runner, int n)
{
    runner->set_concurrency(n);
}

extern "C" void
u4c_list_tests(u4c_runner_t *runner, u4c_plan_t *plan)
{
    runner->list_tests(plan);
}

extern "C" void
u4c_set_output_format(u4c_runner_t *runner, const char *fmt)
{
    if (!strcmp(fmt, "junit"))
	runner->add_listener(new junit_listener_t);
    else if (!strcmp(fmt, "text"))
	runner->add_listener(new text_listener_t);
}

extern "C" int
u4c_run_tests(u4c_runner_t *runner, u4c_plan_t *plan)
{
    return runner->run_tests(plan);
}

// close the namespace
};

