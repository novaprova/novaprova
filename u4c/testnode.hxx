#ifndef __U4C_TESTNODE_H__
#define __U4C_TESTNODE_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"
#include "spiegel/spiegel.hxx"
#include <string>
#include <list>

namespace u4c {

class testnode_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    testnode_t(const char *);
    ~testnode_t();

    std::string get_fullname() const;
    testnode_t *find(const char *name);
    testnode_t *make_path(std::string name);
    void set_function(functype_t, spiegel::function_t *);
    void add_mock(spiegel::function_t *target, spiegel::function_t *mock);
    void add_mock(spiegel::addr_t target, spiegel::addr_t mock);

    testnode_t *detach_common();
    spiegel::function_t *get_function(functype_t type) const
    {
	return funcs_[type];
    }
    std::list<spiegel::function_t*> get_fixtures(functype_t type) const;
    void pre_fixture() const;
    void post_fixture() const;

    void dump(int level) const;

    class preorder_iterator
    {
    public:
	preorder_iterator()
	    : base_(0), node_(0) {}
	preorder_iterator(testnode_t *n)
	    : base_(n), node_(n) {}
	testnode_t * operator* () const
	    { return node_; }
	preorder_iterator & operator++();
	int operator== (const preorder_iterator &o) const
	    { return o.node_ == node_; }
	preorder_iterator & operator=(testnode_t *n)
	    { base_ = node_ = n; return *this; }
    private:
	testnode_t *base_;
	testnode_t *node_;
    };
    preorder_iterator preorder_begin()
	{ return preorder_iterator(this); }
    preorder_iterator preorder_end()
	{ return preorder_iterator(); }

private:
    testnode_t *next_;
    testnode_t *parent_;
    testnode_t *children_;
    char *name_;
    spiegel::function_t *funcs_[FT_NUM_SINGULAR];
    std::vector<spiegel::intercept_t*> intercepts_;

    friend class preorder_iterator;
};

// close the namespace
};

#endif /* __U4C_TESTNODE_H__ */
