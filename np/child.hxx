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
#ifndef __NP_CHILD_H__
#define __NP_CHILD_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include <sys/poll.h>

namespace np {

class job_t;

class child_t : public np::util::zalloc
{
public:
    child_t(pid_t pid, int fd, job_t *);
    ~child_t();

    pid_t get_pid() const { return pid_; }
    job_t *get_job() const { return job_; }
    result_t get_result() const { return result_; }

    int get_input_fd() const { return (state_ == FINISHED ? -1 : event_pipe_); }
    void handle_input();
    int64_t get_deadline() const { return deadline_; }
    void set_deadline(int64_t d) { deadline_ = d; }
    void handle_timeout(int64_t);
    void merge_result(result_t r);
    bool is_finished() const { return state_ == FINISHED; }

private:
    pid_t pid_;
    int event_pipe_;	    /* read end of the pipe */
    job_t *job_;
    result_t result_;
    enum {
	RUNNING,
	TIMEOUT1,
	TIMEOUT2,
	FINISHED,
    } state_;
    int64_t deadline_;
};

// close the namespace
};

#endif /* __NP_CHILD_H__ */
