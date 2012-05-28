#ifndef __NP_JOB_H__
#define __NP_JOB_H__ 1

#include "np/util/common.hxx"
#include "np/testnode.hxx"
#include "np/plan.hxx"

namespace np {

class job_t : public np::util::zalloc
{
public:
    job_t(const plan_t::iterator &);
    ~job_t();

    std::string as_string() const;
    testnode_t *get_node() const { return node_; }
    void pre_run(bool in_parent);
    void post_run(bool in_parent);

    int64_t get_start() const { return start_; }
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

#endif /* __NP_JOB_H__ */
