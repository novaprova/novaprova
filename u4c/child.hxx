#ifndef __U4C_CHILD_H__
#define __U4C_CHILD_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"
#include <sys/poll.h>

namespace u4c {

class testnode_t;

class child_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    child_t(pid_t pid, int fd, testnode_t *tn);
    ~child_t();

    pid_t get_pid() const { return pid_; }
    testnode_t *get_node() const { return node_; }
    result_t get_result() const { return result_; }

    void poll_setup(struct pollfd &);
    void poll_handle(struct pollfd &);
    void merge_result(result_t r);

private:
    pid_t pid_;
    int event_pipe_;	    /* read end of the pipe */
    testnode_t *node_;
    result_t result_;
    bool finished_;
};

// close the namespace
};

#endif /* __U4C_CHILD_H__ */
