#include "np/child.hxx"
#include "np/testnode.hxx"
#include "np/job.hxx"
#include "np/event.hxx"
#include "np/proxy_listener.hxx"
#include "np_priv.h"

namespace np {

child_t::child_t(pid_t pid, int fd, job_t *j)
 :  pid_(pid),
    event_pipe_(fd),
    job_(j),
    result_(R_UNKNOWN),
    state_(RUNNING),
    deadline_(j->get_start() + 30 * NANOSEC_PER_SEC)
{
}

child_t::~child_t()
{
    close(event_pipe_);
    delete job_;
}

void
child_t::handle_input()
{
    if (state_ == FINISHED)
	return;
    if (!proxy_listener_t::handle_call(event_pipe_, job_, &result_))
	state_ = FINISHED;
}

void
child_t::handle_timeout(int64_t end)
{
    switch (state_)
    {
    case RUNNING:
	if (deadline_ <= end)
	{
	    event_t ev(EV_TIMEOUT, "Child timed out");
	    merge_result(np::runner_t::running()->raise_event(job_, &ev));

	    kill(pid_, SIGTERM);
	    state_ = TIMEOUT1;
	    deadline_ = end + 3 * NANOSEC_PER_SEC;
	}
	break;
    case TIMEOUT1:
	kill(pid_, SIGKILL);
	state_ = TIMEOUT2;
	deadline_ = 0;
	break;
    default:
	break;
    }
}

void
child_t::merge_result(result_t r)
{
    result_ = merge(result_, r);
}

// close the namespace
};
