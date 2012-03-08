#include "u4c/plan.hxx"
#include "u4c/testnode.hxx"
#include "u4c/testmanager.hxx"
#include "u4c_priv.h"

namespace u4c {

plan_t::plan_t()
{
}

plan_t::~plan_t()
{
}

void
plan_t::add_node(testnode_t *tn)
{
    nodes_.push_back(tn);
}

bool
plan_t::add_specs(int nspec, const char **specs)
{
    testnode_t *tn;
    int i;

    for (i = 0 ; i < nspec ; i++)
    {
	tn = testmanager_t::instance()->find_node(specs[i]);
	if (!tn)
	    return false;
	add_node(tn);
    }
    return true;
}

testnode_t *
plan_t::next()
{
    iterator_t *itr = &current_;

    testnode_t *tn = itr->node;

    /* advance tn */
    for (;;)
    {
	tn = tn->next_preorder();
	if (tn)
	    return itr->node = tn;
	if (itr->idx >= (int)nodes_.size()-1)
	    return itr->node = 0;
	tn = nodes_[++itr->idx];
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern "C" u4c_plan_t *
u4c_plan_new(void)
{
    return new plan_t();
}

extern "C" void
u4c_plan_delete(u4c_plan_t *plan)
{
    delete plan;
}

extern "C" bool
u4c_plan_add_specs(u4c_plan_t *plan, int nspec, const char **spec)
{
    return plan->add_specs(nspec, spec);
}


// close the namespace
};
