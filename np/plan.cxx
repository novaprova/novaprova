#include "np/plan.hxx"
#include "np/testnode.hxx"
#include "np/job.hxx"
#include "np/testmanager.hxx"
#include "np_priv.h"

namespace np {
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

/**
 * Create a new `np_plan_t` object.
 *
 * A plan object can be used to configure a `np_runner_t` object to run
 * (or list to stdout) a subset of all the discovered tests.  Note that
 * if you want to run all tests, you do not need to create a plan at
 * all; passing NULL to `np_run_tests` has that effect.
 */
extern "C" np_plan_t *
np_plan_new(void)
{
    return new plan_t();
}

/**
 * Delete a plan object.
 */
extern "C" void
np_plan_delete(np_plan_t *plan)
{
    delete plan;
}

/**
 * Add test specifications to a plan object.
 *
 * Add a sequence of test specifications to the plan object.  Each test
 * specification is a string which matches a testnode in the discovered
 * testnode hierarchy, and will cause that node plus all of its
 * descendants to be added to the plan.  The interface is designed to
 * take command-line arguments from your test runner program after
 * options have been parsed with `getopt`.
 *
 * Returns false if any of the test specifications could not be found,
 * true on success.
 */
extern "C" bool
np_plan_add_specs(np_plan_t *plan, int nspec, const char **spec)
{
    return plan->add_specs(nspec, spec);
}


// close the namespace
};
