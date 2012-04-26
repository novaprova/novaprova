#ifndef __U4C_JOB_H__
#define __U4C_JOB_H__ 1

#include "u4c/common.hxx"
#include "u4c/testnode.hxx"

namespace u4c {

class job_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    job_t(testnode_t *tn)
     :  id_(next_id_++),
        node_(tn)
    {}
    ~job_t() {}

    std::string as_string() const
    {
	return dec(id_) + ": " + node_->get_fullname();
    }

    testnode_t *get_node() const { return node_; }

private:
    static unsigned int next_id_;

    unsigned int id_;
    testnode_t *node_;
    // TODO: vector of parameter value indeces
};

// close the namespace
};

#endif /* __U4C_JOB_H__ */
