#include "u4c/plan.hxx"
#include "u4c/testnode.hxx"
#include "u4c/job.hxx"
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

job_t *
plan_t::next()
{
    iterator_t *itr = &current_;
    testnode_t *tn = 0;

    do
    {
	if (itr->nitr == nodes_[0]->preorder_end())
	{
	    if (itr->idx >= (int)nodes_.size()-1)
		return 0;
	    itr->nitr = nodes_[++itr->idx];
	}
	tn = *itr->nitr;
	++itr->nitr;
    } while (tn && !tn->get_function(FT_TEST));

    if (tn)
	return new job_t(tn);
    return 0;
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
