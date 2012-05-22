#ifndef __NP_PLAN_H__
#define __NP_PLAN_H__ 1

#include "np/util/common.hxx"
#include "np/testnode.hxx"
#include <vector>

class np_testmanager_t;

namespace np {


class plan_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    plan_t();
    ~plan_t();

    void add_node(testnode_t *tn);
    bool add_specs(int nspec, const char **specs);

    class iterator
    {
    public:
	iterator() {}
	iterator(const iterator &o)
	 :  vitr_(o.vitr_),
	    vend_(o.vend_),
	    nitr_(o.nitr_),
	    assigns_(o.assigns_)
	{}
	iterator &operator++();
	iterator &operator=(const iterator &o)
	{
	    vitr_ = o.vitr_;
	    vend_ = o.vend_;
	    nitr_ = o.nitr_;
	    assigns_ = o.assigns_;
	    return *this;
	}
	int operator==(const iterator &o) const
	{
	    return (vitr_ == o.vitr_ &&
		    nitr_ == o.nitr_ &&
		    assigns_ == o.assigns_);
	}
	int operator!=(const iterator &o) const
	{
	    return !operator==(o);
	}

	testnode_t *get_node() const { return *nitr_; }
	std::vector<testnode_t::assignment_t> const &get_assignments() const { return assigns_; }

    private:
	iterator(std::vector<testnode_t*>::iterator first,
		 std::vector<testnode_t*>::iterator last);
	void find_testable_node();

	std::vector<testnode_t*>::iterator vitr_;
	std::vector<testnode_t*>::iterator vend_;
	testnode_t::preorder_iterator nitr_;
	std::vector<testnode_t::assignment_t> assigns_;

	friend class plan_t;
    };
    iterator begin() { return iterator(nodes_.begin(), nodes_.end()); }
    iterator end() { return iterator(nodes_.end(), nodes_.end()); }

private:
    std::vector<testnode_t*> nodes_;
};

// close the namespace
};

#endif /* __NP_PLAN_H__ */
