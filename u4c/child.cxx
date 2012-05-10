#include "u4c/child.hxx"
#include "u4c/testnode.hxx"
#include "u4c/job.hxx"
#include "u4c/proxy_listener.hxx"
#include "u4c_priv.h"

namespace u4c {

child_t::child_t(pid_t pid, int fd, job_t *j)
 :  pid_(pid),
    event_pipe_(fd),
    job_(j),
    result_(R_UNKNOWN),
    finished_(false)
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
    if (finished_)
	return;
    if (!proxy_listener_t::handle_call(event_pipe_, job_, &result_))
	finished_ = true;
}

void
child_t::merge_result(result_t r)
{
    result_ = merge(result_, r);
}

// close the namespace
};
