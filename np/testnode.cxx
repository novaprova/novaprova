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
#include "np/testnode.hxx"
#include "np/redirect.hxx"
#include "np/util/tok.hxx"

static std::vector<np::spiegel::intercept_t*> dynamic_intercepts;

namespace np {
using namespace std;
using namespace np::util;

testnode_t::testnode_t(const char *name)
 :  name_(name ? xstrdup(name) : 0)
{
}

testnode_t::~testnode_t()
{
    while (children_)
    {
	testnode_t *child = children_;
	children_ = child->next_;
	delete child;
    }

    xfree(name_);
}

testnode_t *
testnode_t::make_path(string name)
{
    testnode_t *parent = this;
    const char *part;
    testnode_t *child;
    testnode_t **tailp;
    tok_t tok(name.c_str(), "/");

#if _NP_DEBUG
    fprintf(stderr, "np: testnode_t::make_path(%s)\n", name.c_str());
#endif
    while ((part = tok.next()))
    {
	for (child = parent->children_, tailp = &parent->children_ ;
	     child ;
	     tailp = &child->next_, child = child->next_)
	{
	    if (!strcmp(child->name_, part))
		break;
	}
	if (!child)
	{
	    child = new testnode_t(part);
	    *tailp = child;
	    child->parent_ = parent;
	}

	parent = child;
    }
    return child;
}

void
testnode_t::set_function(functype_t ft, np::spiegel::function_t *func)
{
    if (funcs_[ft])
	fprintf(stderr, "np: WARNING: duplicate %s functions: "
			"%s:%s and %s:%s\n",
			as_string(ft),
			funcs_[ft]->get_compile_unit()->get_absolute_path().c_str(),
			funcs_[ft]->get_name().c_str(),
			func->get_compile_unit()->get_absolute_path().c_str(),
			func->get_name().c_str());
    else
	funcs_[ft] = func;
}

void
testnode_t::add_mock(np::spiegel::function_t *target, np::spiegel::function_t *mock)
{
#if _NP_DEBUG
    fprintf(stderr, "np: testnode_t::add_mock: target_addr=%p target_name=%s mock_addr=%p\n", 
            (void*)target->get_address(),
	    target->get_full_name().c_str(),
	    (void*)mock->get_address());
#endif
    add_mock(target->get_address(),
	     target->get_full_name().c_str(),
	     mock->get_address());
}

void
testnode_t::add_mock(np::spiegel::addr_t target, const char *name, np::spiegel::addr_t mock)
{
    intercepts_.push_back(new redirect_t(target, name, mock));
}

void
testnode_t::add_mock(np::spiegel::addr_t target, np::spiegel::addr_t mock)
{
    intercepts_.push_back(new redirect_t(target, 0, mock));
}

static void
indent(int level)
{
    for ( ; level ; level--)
	fputs("    ", stderr);
}

void
testnode_t::dump(int level) const
{
    indent(level);
    if (name_)
    {
	fprintf(stderr, "%s (full %s)\n",
		name_, get_fullname().c_str());
    }

    for (int type = 0 ; type < FT_NUM_SINGULAR ; type++)
    {
	if (funcs_[type])
	{
	    indent(level);
	    fprintf(stderr, "  %s=%s:%s\n",
			    as_string((functype_t)type),
			    funcs_[type]->get_compile_unit()->get_absolute_path().c_str(),
			    funcs_[type]->get_name().c_str());
	}
    }

    for (testnode_t *child = children_ ; child ; child = child->next_)
	child->dump(level+1);
}

string
testnode_t::get_fullname() const
{
    string full = "";

    for (const testnode_t *a = this ; a ; a = a->parent_)
    {
	if (!a->name_)
	    continue;
	if (a != this)
	    full = "." + full;
	full = a->name_ + full;
    }

    return full;
}

bool
testnode_t::is_elidable() const
{
    /* nodes with mocks or other intercepts cannot be elided */
    if (intercepts_.size() > 0)
	return false;
    /* nodes with parameters cannot be elided */
    if (parameters_.size() > 0)
	return false;
    /* nodes with tests or fixtures cannot be elided */
    if (funcs_[FT_BEFORE] || funcs_[FT_TEST] || funcs_[FT_AFTER])
	return false;
    /* nodes with more than a single child cannot be elided */
    if (children_ && children_->next_)
	return false;
    return true;
}

testnode_t *
testnode_t::detach_common()
{
    testnode_t *tn;

    for (tn = this ; tn->children_ && tn->is_elidable() ; tn = tn->children_)
	;
    /* tn now points at the highest non-elidable node */

    /* corner case: we have exactly one test; give ourselves at least a
     * two-deep hierarchy so we don't see a sudden jump in naming when
     * adding the second test */
    if (!tn->children_ && tn->parent_)
	tn = tn->parent_;

    if (tn->parent_)
    {
	tn->parent_->children_ = 0;
	assert(!tn->next_);
	tn->parent_ = 0;
    }

    return tn;
}

list<np::spiegel::function_t*>
testnode_t::get_fixtures(functype_t type) const
{
    list<np::spiegel::function_t*> fixtures;

    /* Run FT_BEFORE from outermost in, and FT_AFTER
     * from innermost out */
    for (const testnode_t *a = this ; a ; a = a->parent_)
    {
	if (!a->funcs_[type])
	    continue;
	if (type == FT_BEFORE)
	    fixtures.push_front(a->funcs_[type]);
	else
	    fixtures.push_back(a->funcs_[type]);
    }
    return fixtures;
}

testnode_t *
testnode_t::find(const char *nm)
{
    if (name_ && get_fullname() == nm)
	return this;

    for (testnode_t *child = children_ ; child ; child = child->next_)
    {
	testnode_t *found = child->find(nm);
	if (found)
	    return found;
    }

    return 0;
}

void
testnode_t::pre_run() const
{
    /* Install intercepts from innermost out */
    for (const testnode_t *a = this ; a ; a = a->parent_)
    {
	vector<np::spiegel::intercept_t*>::const_iterator itr;
	for (itr = a->intercepts_.begin() ; itr != a->intercepts_.end() ; ++itr)
	    (*itr)->install();
    }
}

void
testnode_t::post_run() const
{
    /*
     * Uninstall intercepts from innermost out.  Probably we should do
     * this from outermost in to do it in the opposite order to
     * pre_fixture(), but the order doesn't really matter.  The order
     * *does* matter for installation, as the install order will be the
     * execution order should any intercepts double up.
     */
    for (const testnode_t *a = this ; a ; a = a->parent_)
    {
	vector<np::spiegel::intercept_t*>::const_iterator itr;
	for (itr = a->intercepts_.begin() ; itr != a->intercepts_.end() ; ++itr)
	    (*itr)->uninstall();
    }

    /* uninstall all dynamic intercepts installed by this test */
    vector<np::spiegel::intercept_t*>::const_iterator itr;
    for (itr = dynamic_intercepts.begin() ; itr != dynamic_intercepts.end() ; ++itr)
    {
	np::spiegel::intercept_t *ii = *itr;
	ii->uninstall();
	delete ii;
    }
    dynamic_intercepts.clear();
}

testnode_t::preorder_iterator &
testnode_t::preorder_iterator::operator++()
{
    if (node_->children_)
	node_ = node_->children_;	    // down
    else if (node_ != base_ && node_->next_)
	node_ = node_->next_;		    // across
    else
    {
	// up and across
	for (;;)
	{
	    if (node_ == base_)
	    {
		node_ = 0;
		break;
	    }
	    if (node_->next_)
	    {
		node_ = node_->next_;
		break;
	    }
	    node_ = node_->parent_;
	}
    }
    return *this;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

testnode_t::parameter_t::parameter_t(const char *n, char **v, const char *vals)
 :  name_(xstrdup(n)),
    variable_(v)
{
    /* TODO: need a split() function */
    np::util::tok_t valtok(vals, ", \t");
    const char *val;
    while ((val = valtok.next()))
	values_.push_back(xstrdup(val));
}

testnode_t::parameter_t::~parameter_t()
{
    xfree(name_);
    vector<char*>::iterator i;
    for (i = values_.begin() ; i != values_.end() ; ++i)
	free(*i);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void testnode_t::assignment_t::apply() const
{
    free(*param_->variable_);
    *param_->variable_ = xstrdup(param_->values_[idx_]);
}

void testnode_t::assignment_t::unapply() const
{
    free(*param_->variable_);
    *param_->variable_ = 0;
}

string testnode_t::assignment_t::as_string() const
{
    return string(param_->name_) + "=" + param_->values_[idx_];
}

// Bump the assignment vector to the next next value in order, clearing
// the vector and returning true if we run off the end of the values.
bool bump(std::vector<testnode_t::assignment_t> &a)
{
    vector<testnode_t::assignment_t>::iterator i;
    for (i = a.begin() ; i != a.end() ; ++i)
    {
	if (++i->idx_ < i->param_->values_.size())
	    return false;
	i->idx_ = 0;
    }
    a.clear();
    return true;
}

int operator==(const std::vector<testnode_t::assignment_t> &a,
	       const std::vector<testnode_t::assignment_t> &b)
{
    if (a.size() != b.size())
	return 0;

    std::vector<testnode_t::assignment_t>::const_iterator ia;
    std::vector<testnode_t::assignment_t>::const_iterator ib;
    for (ia = a.begin(), ib = b.begin() ; ia != a.end() ; ++ia, ++ib)
	if (ia->idx_ != ib->idx_)
	    return 0;
    return 1;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
testnode_t::add_parameter(const char *name, char **var, const char *vals)
{
    parameters_.push_back(new parameter_t(name, var, vals));
}

vector<testnode_t::assignment_t>
testnode_t::create_assignments() const
{
    vector<assignment_t> assigns;

    for (const testnode_t *a = this ; a ; a = a->parent_)
    {
	vector<parameter_t*>::const_iterator i;
	for (i = a->parameters_.begin() ; i != a->parameters_.end() ; ++i)
	    assigns.push_back(assignment_t(*i));
    }
    return assigns;
}

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern "C" void __np_mock(void (*from)(void), const char *name, void (*to)(void))
{
    np::redirect_t *mock = new np::redirect_t((np::spiegel::addr_t)from,
					      name,
					      (np::spiegel::addr_t)to);
    // TODO: should we search the dynamic_intercepts list here
    // to be entirely sure the caller doesn't double-mock
    dynamic_intercepts.push_back(mock);
    mock->install();
}

extern "C" void __np_unmock(void (*from)(void))
{
    std::vector<np::spiegel::intercept_t*>::iterator itr;
    for (itr = dynamic_intercepts.begin() ; itr != dynamic_intercepts.end() ; ++itr)
    {
	np::spiegel::intercept_t *ii = *itr;
	if (ii->get_address() == (np::spiegel::addr_t)from)
	{
	    dynamic_intercepts.erase(itr);
	    ii->uninstall();
	    delete ii;
	    return;
	}
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
