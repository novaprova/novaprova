/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "np/runner.hxx"
#include "np/testnode.hxx"
#include "np/job.hxx"
#include "np/plan.hxx"
#include "np/text_listener.hxx"
#include "np/proxy_listener.hxx"
#include "np/junit_listener.hxx"
#include "np/child.hxx"
#include "np/spiegel/spiegel.hxx"
#include "np_priv.h"
#include "except.h"
#if HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

__np_exceptstate_t __np_exceptstate;

void __np_uncaught_event(void)
{
    fprintf(stderr, "np: uncaught event: %s\n%s\n",
	    __np_exceptstate.event.as_string().c_str(),
	    __np_exceptstate.event.get_long_location().c_str());
    fflush(stderr);
    abort();
}

namespace np {
using namespace std;
using namespace np::util;

#define dispatch_listeners(func, ...) \
    do { \
	vector<listener_t*>::iterator _i; \
	for (_i = listeners_.begin() ; _i != listeners_.end() ; ++_i) \
	    (*_i)->func(__VA_ARGS__); \
    } while(0)

runner_t *runner_t::running_;

static int
choose_timeout()
{
    if (np::spiegel::platform::is_running_under_debugger())
    {
	fprintf(stderr, "np: disabling test timeouts under debugger\n");
	return 0;
    }
    int64_t timeout = 30;
#if HAVE_VALGRIND
    if (RUNNING_ON_VALGRIND)
	timeout *= 3;
#endif
    return timeout;
}

runner_t::runner_t()
{
    maxchildren_ = 1;
    timeout_ = choose_timeout();
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
	plan = new plan_t();
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
    nrunning_ = 0;
    for (;;)
    {
	while (nrunning_ < maxchildren_ && pitr != pend)
	{
	    begin_job(new job_t(pitr));
	    ++pitr;
	}
	if (nrunning_ == 0)
	    break;
	wait();
    }
    if (children_.size() > 0)
    {
	/* Wait for and reap child processes
	 * until there are no more. */
	reap_children(-1, 0);
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
    needs_stdout_ = false;
}

void
runner_t::add_listener(listener_t *l)
{
    /* append to the list.  The order of adding is preserved for
     * dispatching */
    listeners_.push_back(l);
    needs_stdout_ |= l->needs_stdout();
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
    event_t n_ev;
    n_ev.normalise(ev);
    dispatch_listeners(add_event, j, &n_ev);
    return n_ev.get_result();
}

static const char tmpfile_template[] = "/tmp/novaprova.out.XXXXXX";
#define TMPFILE_MAX (sizeof(tmpfile_template))

child_t *
runner_t::fork_child(job_t *j)
{
    pid_t pid;
#define PIPE_READ 0
#define PIPE_WRITE 1
    int pipefd[2];
    int outfd = -1;
    int errfd = -1;
    char outpath[TMPFILE_MAX];
    char errpath[TMPFILE_MAX];
    child_t *child;
    int delay_ms = 10;
    int max_sleeps = 20;
    int r;

    r = pipe(pipefd);
    if (r < 0)
    {
	perror("np: pipe");
	exit(1);
    }

    if (needs_stdout_)
    {
	strcpy(outpath, tmpfile_template);
	outfd = mkstemp(outpath);
	if (outfd < 0)
	{
	    perror(outpath);
	    exit(1);
	}

	strcpy(errpath, tmpfile_template);
	errfd = mkstemp(errpath);
	if (errfd < 0)
	{
	    perror(errpath);
	    exit(1);
	}
    }

    for (;;)
    {
	pid = fork();
	if (pid < 0)
	{
	    if (errno == EAGAIN && max_sleeps-- > 0)
	    {
		/* rats, we fork-bombed, try again after a delay */
		fprintf(stderr, "np: fork bomb! sleeping %u ms.\n",
			delay_ms);
		poll(0, 0, delay_ms);
		delay_ms += (delay_ms>>1);	/* exponential backoff */
		continue;
	    }
	    perror("np: fork");
	    exit(1);
	}
	break;
    }

    if (!pid)
    {
	/* child process: return, will run the test */
	signal(SIGCHLD, SIG_DFL);
	close(pipefd[PIPE_READ]);
	event_pipe_ = pipefd[PIPE_WRITE];
	if (needs_stdout_)
	{
	    dup2(outfd, STDOUT_FILENO);
	    close(outfd);
	    dup2(errfd, STDERR_FILENO);
	    close(errfd);
	}
	return NULL;
    }

    /* parent process */

#if _NP_DEBUG
    fprintf(stderr, "np: spawned child process %d for %s\n",
	    (int)pid, j->as_string().c_str());
#endif
    close(pipefd[PIPE_WRITE]);
    child = new child_t(pid, pipefd[PIPE_READ], j);
    if (timeout_)
	child->set_deadline(j->get_start() + timeout_ * NANOSEC_PER_SEC);
    if (needs_stdout_)
    {
	close(outfd);
	close(errfd);
	j->set_stdout_path(outpath);
	j->set_stderr_path(errpath);
    }
    children_.push_back(child);
    nrunning_++;

    return child;
#undef PIPE_READ
#undef PIPE_WRITE
}

child_t *
runner_t::find_finished_child() const
{
    for (vector<child_t*>::const_iterator citr = children_.begin() ; citr != children_.end() ; ++citr)
    {
	if ((*citr)->is_finished())
	    return *citr;
    }
    return 0;
}

void
runner_t::wait()
{
    int r;
    unsigned int nzeroes = 0;
    unsigned int nfinished = 0;

    if (nrunning_ == 0)
	return;

    do
    {
#if _NP_DEBUG > 1
	fprintf(stderr, "np: handle_events() nchildren=%u caught_sigchld=%d nfinished=%u\n",
		(unsigned)children_.size(), caught_sigchld, nfinished);
#endif
	int64_t start = rel_now();
	int64_t timeout = -1;
	pfd_.clear();
	vector<child_t*>::iterator citr;
	for (citr = children_.begin() ; citr != children_.end() ; ++citr)
	{
	    if ((*citr)->is_finished())
		continue;
	    struct pollfd p;
	    memset(&p, 0, sizeof(struct pollfd));
	    int fd = (*citr)->get_input_fd();
	    if (fd >= 0)
	    {
		p.fd = fd;
		p.events |= POLLIN;
	    }
	    pfd_.push_back(p);

	    int64_t deadline = (*citr)->get_deadline();
	    if (deadline)
	    {
		int64_t to = deadline - start;
		if (to <= 0)
		    timeout = 0;	/* already overdue */
		else if (timeout < 0 || timeout > to)
		    timeout = to;
	    }
	}

	assert(pfd_.size() == nrunning_);
	assert(pfd_.size() > 0);

	if (timeout == 0)
	{
	    if (++nzeroes > 5)
		timeout = NANOSEC_PER_SEC;
	}
	else
	{
	    nzeroes = 0;
	}

	/* convert timeout for poll(): from nanosec to millisec
	 * and smash all negative values to -1 */
	timeout = (timeout < 0 ? -1 : (timeout+500000)/1000000);

#if _NP_DEBUG
	fprintf(stderr, "np: [%s] about to poll([%d fds] timeout=%lld msec)\n",
		rel_timestamp(), (int)pfd_.size(), (long long)timeout);
#endif
	r = poll(pfd_.data(), pfd_.size(), timeout);
#if _NP_DEBUG
	{
	    int e = errno;
	    fprintf(stderr, "np: [%s] poll returned %d errno %d(%s)\n",
		    rel_timestamp(), r, e, strerror(e));
	    vector<struct pollfd>::iterator pitr;
	    for (pitr = pfd_.begin() ; pitr != pfd_.end() ; ++pitr)
		fprintf(stderr, "np:     fd %d revents %d\n",
			pitr->fd, pitr->revents);
	    errno = e;
	}
#endif
	if (r < 0)
	{
	    if (errno == EINTR && caught_sigchld)
	    {
		/*
		 * Woken up from the poll() by a delivered SIGCHLD, so go
		 * see if there any newly exited children to reap.  Note
		 * that under Valgrind on Darwin, we don't get woken.
		 */
		reap_children(-1, WNOHANG);
		continue;
	    }
	    /* poll reported an error */
	    perror("np: poll");
	    return;
	}
	if (r == 0)
	{
	    /* poll() timed out */
	    int64_t end = rel_now();
	    for (citr = children_.begin() ; citr != children_.end() ; ++citr)
	    {
		if ((*citr)->is_finished())
		    continue;
		(*citr)->handle_timeout(end);
	    }
	}
	else
	{
	    /* poll indicated some fds are available */
	    vector<struct pollfd>::iterator pitr = pfd_.begin();
	    for (citr = children_.begin() ; citr != children_.end() ; ++citr)
	    {
		if ((*citr)->is_finished())
		    continue;
		if ((pitr->revents & POLLIN))
		    (*citr)->handle_input();
		if ((*citr)->is_finished())
		    nfinished++;
		++pitr;
	    }
	}
	if (nfinished)
	{
	    nrunning_ -= nfinished;
	    /* Synchronously reap all the finished children in their start
	     * order.  This preserves the order of result messages and test
	     * output seen when using the text listener, so that a -j1 run
	     * has a predictable order.  This is actually really important
	     * to ensure NP's own tests pass consistently.
	     */
	    child_t *child;
	    while ((child = find_finished_child()) != 0)
		reap_children(child->get_pid(), 0);
	}
    } while (nfinished == 0);
#if _NP_DEBUG > 1
    fprintf(stderr, "np: handle_events() returning, caught_sigchld=%d nfinished=%u\n",
	    caught_sigchld, nfinished);
#endif
}

void
runner_t::reap_children(pid_t reqpid, int waitflags)
{
    pid_t pid;
    int status;
    char msg[1024];

#if _NP_DEBUG
    fprintf(stderr, "np: [%s] reap_children()\n",
		rel_timestamp());
#endif
    caught_sigchld = 0;

    for (;;)
    {
#if _NP_DEBUG > 1
	fprintf(stderr, "np: [%s] about to call waitpid(%d, %u)\n",
		rel_timestamp(), (int)reqpid, waitflags);
#endif
	pid = waitpid(reqpid, &status, waitflags);
#if _NP_DEBUG > 1
	{
	    int e = errno;
	    fprintf(stderr, "np: [%s] waitpid returns %d, errno %d(%s)\n",
		    rel_timestamp(), (int)pid, e, strerror(errno));
	    errno = e;
	}
#endif
	if (pid == 0)
	    break;
	if (pid < 0)
	{
	    if (errno == ESRCH || errno == ECHILD)
		break;
	    perror("np: waitpid");
	    return;
	}
	if (WIFSTOPPED(status))
	{
	    fprintf(stderr, "np: process %d stopped on signal %d, ignoring\n",
		    (int)pid, WSTOPSIG(status));
	    continue;
	}
#if _NP_DEBUG
	fprintf(stderr, "np: [%s] reaped process %d\n",
		rel_timestamp(), (int)pid);
#endif
	vector<child_t*>::iterator itr;
	for (itr = children_.begin() ;
	     itr != children_.end() && (*itr)->get_pid() != pid ;
	     ++itr)
	    ;
	if (itr == children_.end())
	{
	    /* some other process */
	    fprintf(stderr, "np: reaped stray process %d\n", (int)pid);
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
	child->merge_result(np::R_PASS);

	/* notify listeners */
	nfailed_ += (child->get_result() == R_FAIL);
	nrun_++;
	child->get_job()->post_run(true);
	dispatch_listeners(end_job, child->get_job(), child->get_result());

	/* detach and clean up */
	children_.erase(itr);
	delete child;
    }

    /* nothing to reap here, move along */
}

void
runner_t::run_function(functype_t ft, np::spiegel::function_t *f)
{
    vector<np::spiegel::value_t> args;
    np::spiegel::value_t ret = f->invoke(args);

    if (ft == FT_TEST)
    {
	assert(ret.which == np::spiegel::type_t::TC_VOID);
    }
    else
    {
	assert(ret.which == np::spiegel::type_t::TC_SIGNED_LONG_LONG);
	int r = ret.val.vsint;

	if (r)
	{
	    static char cond[64];
	    snprintf(cond, sizeof(cond), "fixture returned %d", r);
	    np_throw(event_t(EV_FIXTURE, cond).in_function(f));
	}
    }
}

void
runner_t::run_fixtures(testnode_t *tn, functype_t type)
{
    list<np::spiegel::function_t*> fixtures = tn->get_fixtures(type);
    list<np::spiegel::function_t*>::iterator itr;
    for (itr = fixtures.begin() ; itr != fixtures.end() ; ++itr)
	run_function(type, *itr);
}

result_t
runner_t::valgrind_errors(job_t *j, result_t res)
{
#if HAVE_VALGRIND
    unsigned long leaked = 0;
    unsigned long dubious __attribute__((unused)) = 0;
    unsigned long reachable __attribute__((unused)) = 0;
    unsigned long suppressed __attribute__((unused)) = 0;
    unsigned long nerrors;
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
#endif

    return res;
}

result_t
runner_t::descriptor_leaks(job_t *j, const vector<string> &prefds, result_t res)
{
    vector<string> postfds = np::spiegel::platform::get_file_descriptors();
    unsigned int fd = 0;
    string none;
    char msg[1024];

    unsigned maxfd = max(prefds.size(), postfds.size());
    for (fd = 0 ; fd < maxfd ; fd++)
    {
	const string &pre = (fd < prefds.size() ? prefds[fd] : none);
	const string &post = (fd < postfds.size() ? postfds[fd] : none);

	if (pre == post)
	    continue;

	if (post == "")
	    snprintf(msg, sizeof(msg),
		     "test closed file descriptor %d -> %s\n",
		    fd, pre.c_str());
	else if (pre == "")
	    snprintf(msg, sizeof(msg),
		     "test leaked file descriptor %d -> %s\n",
		     fd, post.c_str());
	else
	    snprintf(msg, sizeof(msg),
		     "test reopened file descriptor %d -> %s as %s\n",
		     fd, pre.c_str(), post.c_str());
	event_t ev(EV_FDLEAK, msg);
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

    vector<string> prefds = np::spiegel::platform::get_file_descriptors();

    np_try
    {
	run_fixtures(tn, FT_BEFORE);
    }
    np_catch(ev)
    {
	ev->in_functype(FT_BEFORE);
	res = merge(res, raise_event(j, ev));
    }

    if (res == R_UNKNOWN)
    {
	np_try
	{
	    run_function(FT_TEST, tn->get_function(FT_TEST));
	}
	np_catch(ev)
	{
	    ev->in_functype(FT_TEST);
	    res = merge(res, raise_event(j, ev));
	}

	np_try
	{
	    run_fixtures(tn, FT_AFTER);
	}
	np_catch(ev)
	{
	    ev->in_functype(FT_AFTER);
	    res = merge(res, raise_event(j, ev));
	}

	/* If we got this far and nothing bad
	 * happened, we might have passed */
	res = merge(res, R_PASS);
    }

    j->post_run(false);

    res = descriptor_leaks(j, prefds, res);
    prefds.clear();

    res = valgrind_errors(j, res);

    return res;
}


void
runner_t::begin_job(job_t *j)
{
    child_t *child;
    result_t res;

//     {
// 	static int n = 0;
// 	if (++n > 60)
// 	    return;
//     }

#if _NP_DEBUG
    fprintf(stderr, "np: [%s] begin job %s\n",
	    rel_timestamp(), j->as_string().c_str());
#endif

    dispatch_listeners(begin_job, j);
    j->pre_run(true);

    child = fork_child(j);
    if (child)
	return; /* parent process */

    /* child process */
    set_listener(new proxy_listener_t(event_pipe_));
    res = run_test_code(j);
    dispatch_listeners(end_job, j, res);
#if _NP_DEBUG
    fprintf(stderr, "np: [%s] child process %d (%s) exiting\n",
	    rel_timestamp(), (int)getpid(), j->as_string().c_str());
#endif
    delete j;
    exit(0);
}

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
using namespace np;

/**
 * Set the limit on test job parallelism
 *
 * @param runner	the runner object
 * @param n		concurrency value to set
 *
 * Set the maximum number of test jobs which will be run at the same
 * time, to @a n.  The default value is 1, meaning tests will be run
 * serially.  A value of 0 is shorthand for one job per online CPU in
 * the system, which is likely to be the most efficient use of the
 * system.
 *
 * \ingroup main
 */
extern "C" void
np_set_concurrency(np_runner_t *runner, int n)
{
    runner->set_concurrency(n);
}

/**
 * Print the names of the tests in the plan to stdout.
 *
 * @param runner	the runner object
 * @param plan		optional plan object
 *
 * If @a plan is NULL, all the discovered tests will be
 * listed in testnode tree order.
 *
 * \ingroup main
 */
extern "C" void
np_list_tests(np_runner_t *runner, np_plan_t *plan)
{
    runner->list_tests(plan);
}

/**
 * Set the format in which test results will be emitted.  Available
 * formats are:
 *
 *  - @b "junit" a directory called @c reports/ will be created with XML
 *    files in jUnit format, suitable for use with upstream processors
 *    which accept jUnit files, such as the Jenkins CI server.
 *
 *  - @b "text" a stream of tests and events is emitted to stdout,
 *    co-mingled with anything emitted to stdout by the test code.
 *    This is the default if @c np_set_output_format is not called.
 *
 * Note that the function is a misnomer, it actually @b adds an output
 * format, so if you call it twice you will get two sets of output.
 *
 * Returns true if @c fmt is a valid format, or false on error.
 *
 * @param runner	the runner object
 * @param fmt		string naming the output format
 *
 * \ingroup main
 */
extern "C" bool
np_set_output_format(np_runner_t *runner, const char *fmt)
{
    if (!strcmp(fmt, "junit"))
    {
	runner->add_listener(new junit_listener_t);
	return true;
    }
    else if (!strcmp(fmt, "text"))
    {
	runner->add_listener(new text_listener_t);
	return true;
    }
    else
	return false;
}

/**
 * Runs all the tests described in the @a plan object.  If @a plan
 * is NULL, all the discovered tests will be run in testnode tree order.
 *
 * @param runner	the runner object
 * @param plan		optional plan object
 * @return		0 on success or non-zero if any tests failed.
 *
 * \ingroup main
 */
extern "C" int
np_run_tests(np_runner_t *runner, np_plan_t *plan)
{
    return runner->run_tests(plan);
}

/**
 * Get the timeout for the currently running test.
 * If called outside of a running test, returns 0.  Note that the
 * timeout for a test can vary depending on how it's run.  For
 * example, if the test executable is run under a debugger the
 * timeout is disabled, and if it's run under Valgrind (which is
 * the default) the timeout is tripled.
 *
 * @return	    timeout in seconds of currently running test
 *
 * \ingroup misc
 */
extern "C" int
np_get_timeout()
{
    np_runner_t *runner = runner_t::running();
    return runner ? runner->get_timeout() : 0;
}

