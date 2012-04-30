#include "u4c/plan.hxx"
#include "u4c/testnode.hxx"
#include "u4c/job.hxx"
#include "u4c/testmanager.hxx"
#include "u4c_priv.h"

namespace u4c {
using namespace std;

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

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

plan_t::iterator::iterator(vector<testnode_t*>::iterator first,
			   vector<testnode_t*>::iterator last)
 :  vitr_(first),
    vend_(last)
{
    if (vitr_ != vend_)
    {
	nitr_ = *vitr_;
	find_testable_node();
    }
}

plan_t::iterator &
plan_t::iterator::operator++()
{
    // walk to the next assignment state
    if (!bump(assigns_))
	return *this;
    assigns_.clear();

    // no more assignment states: walk to the next node
    // in preorder which has a test function
    ++nitr_;
    find_testable_node();
    return *this;
}

// walk to the first testnode at or after the iterator's
// current state, which has a test function
void plan_t::iterator::find_testable_node()
{
    for (;;)
    {
	if (nitr_ == (*vitr_)->preorder_end())
	{
	    // no more nodes in this subtree: walk to the
	    // next subtree in the vector
	    ++vitr_;
	    if (vitr_ == vend_)
		return;	    // end of iteration
	}
	if ((*nitr_)->get_function(FT_TEST))
	{
	    assigns_ = (*nitr_)->create_assignments();
	    return;		    // found a node
	}
	++nitr_;
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
