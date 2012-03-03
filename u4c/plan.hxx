#ifndef __U4C_PLAN_H__
#define __U4C_PLAN_H__ 1

#include "common.h"
#include <vector>

class u4c_globalstate_t;

namespace u4c {

class testnode_t;

class plan_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    plan_t(u4c_globalstate_t *state);
    ~plan_t();

    void add_node(testnode_t *tn);
    bool add_specs(int nspec, const char **specs);

    testnode_t *next();

private:
    struct iterator_t
    {
	iterator_t() : idx(-1), node(0) { }
	~iterator_t() { }

	int idx;
	testnode_t *node;
    };

    u4c_globalstate_t *state_;
    std::vector<testnode_t*> nodes_;
    iterator_t current_;
};

// close the namespace
};

#endif /* __U4C_PLAN_H__ */
