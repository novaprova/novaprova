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

    testnode_t *detach_common();
    testnode_t *next_preorder();
    spiegel::function_t *get_function(functype_t type) const
    {
	return funcs_[type];
    }
    std::list<spiegel::function_t*> get_fixtures(functype_t type) const;

    void dump(int level) const;

private:
    testnode_t *next_;
    testnode_t *parent_;
    testnode_t *children_;
    char *name_;
    spiegel::function_t *funcs_[FT_NUM];
};

// close the namespace
};

#endif /* __U4C_TESTNODE_H__ */
