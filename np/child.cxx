/*
 * Copyright 2011-2021 Gregory Banks
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
#include "np/child.hxx"
#include "np/testnode.hxx"
#include "np/job.hxx"
#include "np/event.hxx"
#include "np/proxy_listener.hxx"
#include "np_priv.h"
#include "np/util/log.hxx"

namespace np {

child_t::child_t(pid_t pid, int fd, job_t *j)
 :  pid_(pid),
    event_pipe_(fd),
    job_(j),
    result_(R_UNKNOWN),
    state_(RUNNING)
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
    dprintf("pid %d job %s handle_input() state=%d\n",
	    (int)pid_, job_->as_string().c_str(), (int)state_);
    if (state_ == FINISHED)
	return;
    if (!proxy_listener_t::handle_call(event_pipe_, job_, &result_))
    {
	dprintf("child now finished\n");
	state_ = FINISHED;
    }
}

void
child_t::handle_hangup()
{
    dprintf("pid %d job %s handle_hangup() state=%d\n",
	    (int)pid_, job_->as_string().c_str(), (int)state_);
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
	    static char buf[80];
	    snprintf(buf, sizeof(buf), "Child process %d timed out, killing", (int)pid_);
	    event_t ev(EV_TIMEOUT, buf);
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
