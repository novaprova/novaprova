#ifndef __U4C_PRIV_H__
#define __U4C_PRIV_H__ 1

#include "common.h"
#include "u4c.h"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/filename.hxx"
#include <bfd.h>
#include <sys/poll.h>
#include <vector>
#include <list>

struct u4c_event_t;
struct u4c_function_t;
class u4c_globalstate_t;

#include "u4c/types.hxx"
#include "u4c/classifier.hxx"
#include "u4c/child.hxx"
#include "u4c/testnode.hxx"
#include "u4c/listener.hxx"
#include "u4c/text_listener.hxx"
#include "u4c/proxy_listener.hxx"
#include "u4c/plan.hxx"

class u4c_testmanager_t
{
public:
    /* testmanager is a singleton */
    static u4c_testmanager_t *instance();

    u4c::testnode_t *find_node(const char *nm) const
    {
	return root_ ? root_->find(nm) : 0;
    }
    u4c::testnode_t *get_root() { return root_; }

private:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }
    u4c_testmanager_t();
    ~u4c_testmanager_t();

    u4c::functype_t classify_function(const char *func, char *match_return, size_t maxmatch);
    void add_classifier(const char *re, bool case_sensitive, u4c::functype_t type);
    void setup_classifiers();
    void discover_functions();

    static u4c_testmanager_t *instance_;

    std::vector<u4c::classifier_t*> classifiers_;
    spiegel::dwarf::state_t *spiegel_;
    u4c::testnode_t *root_;
    u4c::testnode_t *common_;	// nodes from filesystem root down to root_
};

class u4c_globalstate_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }
    u4c_globalstate_t();
    ~u4c_globalstate_t();

    void set_concurrency(int n);
    void add_listener(u4c::listener_t *);
    void list_tests(u4c::plan_t *) const;
    int run_tests(u4c::plan_t *);
    static u4c_globalstate_t *running() { return running_; }
    u4c::result_t raise_event(const u4c_event_t *, u4c::functype_t);

private:
    /* u4c.c */
    /* run.c */
    void begin();
    void end();
    void set_listener(u4c::listener_t *);
    const u4c_event_t *normalise_event(const u4c_event_t *ev);
    u4c::child_t *fork_child(u4c::testnode_t *tn);
    void handle_events();
    void reap_children();
    void run_function(u4c::functype_t ft, spiegel::function_t *f);
    void run_fixtures(u4c::testnode_t *tn, u4c::functype_t type);
    u4c::result_t valgrind_errors();
    u4c::result_t run_test_code(u4c::testnode_t *tn);
    void begin_test(u4c::testnode_t *);
    void wait();

    static u4c_globalstate_t *running_;

    /* runtime state */
    std::vector<u4c::listener_t*> listeners_;
    unsigned int nrun_;
    unsigned int nfailed_;
    int event_pipe_;		/* only in child processes */
    std::vector<u4c::child_t*> children_;	// only in the parent process
    unsigned int maxchildren;
    std::vector<struct pollfd> pfd_;
};

/* run.c */
#define __u4c_merge(r1, r2) \
    do { \
	u4c::result_t _r1 = (r1), _r2 = (u4c::result_t)(r2); \
	(r1) = (_r1 > _r2 ? _r1 : _r2); \
    } while(0)


#endif /* __U4C_PRIV_H__ */
