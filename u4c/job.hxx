#ifndef __U4C_JOB_H__
#define __U4C_JOB_H__ 1

#include "u4c/common.hxx"
#include "u4c/testnode.hxx"
#include "u4c/plan.hxx"

namespace u4c {

class job_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    job_t(const plan_t::iterator &);
    ~job_t();

    std::string as_string() const;
    testnode_t *get_node() const { return node_; }
    void pre_run(bool in_parent);
    void post_run(bool in_parent);

    int64_t get_elapsed() const;

private:
    static unsigned int next_id_;

    unsigned int id_;
    testnode_t *node_;
    std::vector<testnode_t::assignment_t> assigns_;
    int64_t start_;
    int64_t end_;
};

// close the namespace
};

#endif /* __U4C_JOB_H__ */
