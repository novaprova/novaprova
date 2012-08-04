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
