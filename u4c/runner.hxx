#ifndef __U4C_RUNNER_H__
#define __U4C_RUNNER_H__ 1

#include "common.h"
#include "u4c/types.hxx"
#include <vector>

namespace spiegel { class function_t; };

struct u4c_event_t;

namespace u4c {

class listener_t;
class plan_t;
class child_t;
class testnode_t;

class runner_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }
    runner_t();
    ~runner_t();

    void set_concurrency(int n);
    void add_listener(listener_t *);
    void list_tests(plan_t *) const;
    int run_tests(plan_t *);
    static runner_t *running() { return running_; }
    result_t raise_event(const u4c_event_t *, functype_t);

private:
    void begin();
    void end();
    void set_listener(listener_t *);
    const u4c_event_t *normalise_event(const u4c_event_t *ev);
    child_t *fork_child(testnode_t *tn);
    void handle_events();
    void reap_children();
    void run_function(functype_t ft, spiegel::function_t *f);
    void run_fixtures(testnode_t *tn, functype_t type);
    result_t valgrind_errors();
    result_t run_test_code(testnode_t *tn);
    void begin_test(testnode_t *);
    void wait();

    static runner_t *running_;

    /* runtime state */
    std::vector<listener_t*> listeners_;
    unsigned int nrun_;
    unsigned int nfailed_;
    int event_pipe_;		/* only in child processes */
    std::vector<child_t*> children_;	// only in the parent process
    unsigned int maxchildren_;
    std::vector<struct pollfd> pfd_;
};

// close the namespace
};

#endif /* __U4C_RUNNER_H__ */

