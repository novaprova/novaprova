#ifndef __U4C_CHILD_H__
#define __U4C_CHILD_H__ 1

#include "common.h"
#include "u4c/types.hxx"
#include <sys/poll.h>

class u4c_testnode_t;

namespace u4c {

class child_t
{
public:
    child_t(pid_t pid, int fd, u4c_testnode_t *tn);
    ~child_t();

    pid_t get_pid() const { return pid_; }
    u4c_testnode_t *get_node() const { return node_; }
    result_t get_result() const { return result_; }

    void poll_setup(struct pollfd &);
    void poll_handle(struct pollfd &);
    void merge_result(result_t r);

private:
    pid_t pid_;
    int event_pipe_;	    /* read end of the pipe */
    u4c_testnode_t *node_;
    result_t result_;
    bool finished_;
};

// close the namespace
};

#endif /* __U4C_CHILD_H__ */
