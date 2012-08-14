/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __NP_PLAN_H__
#define __NP_PLAN_H__ 1

#include "np/util/common.hxx"
#include "np/testnode.hxx"
#include <vector>

class np_testmanager_t;

namespace np {


class plan_t : public np::util::zalloc
{
public:
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
