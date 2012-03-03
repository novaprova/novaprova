#include "u4c/testnode.hxx"
#include "spiegel/tok.hxx"

namespace u4c {
using namespace std;

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
    spiegel::tok_t tok(name.c_str(), "/");

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
testnode_t::set_function(functype_t ft, spiegel::function_t *func)
{
    if (funcs_[ft])
	fprintf(stderr, "u4c: WARNING: duplicate %s functions: "
			"%s:%s and %s:%s\n",
			as_string(ft),
			funcs_[ft]->get_compile_unit()->get_absolute_path().c_str(),
			funcs_[ft]->get_name(),
			func->get_compile_unit()->get_absolute_path().c_str(),
			func->get_name());
    else
	funcs_[ft] = func;
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

    for (int type = 0 ; type < FT_NUM ; type++)
    {
	if (funcs_[type])
	{
	    indent(level);
	    fprintf(stderr, "  %s=%s:%s\n",
			    as_string((functype_t)type),
			    funcs_[type]->get_compile_unit()->get_absolute_path().c_str(),
			    funcs_[type]->get_name());
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

testnode_t *
testnode_t::next_preorder()
{
    testnode_t *tn = this;
    while (tn)
    {
	if (tn->children_)
	    tn = tn->children_;
	else if (tn->next_)
	    tn = tn->next_;
	else if (tn->parent_)
	    tn = tn->parent_->next_;
	if (tn && tn->funcs_[FT_TEST])
	    break;
    }
    return tn;
}

testnode_t *
testnode_t::detach_common()
{
    testnode_t *tn;

    for (tn = this ;
         tn->children_ && !tn->children_->next_ ;
	 tn = tn->children_)
	;
    /* tn now points at the highest node with more than 1 child */

    tn->parent_->children_ = 0;
    assert(!tn->next_);
    tn->parent_ = 0;

    return tn;
}

list<spiegel::function_t*>
testnode_t::get_fixtures(functype_t type) const
{
    list<spiegel::function_t*> fixtures;

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

// close the namespace
};
