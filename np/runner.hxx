#ifndef __NP_RUNNER_H__
#define __NP_RUNNER_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include <vector>

namespace np { namespace spiegel { class function_t; }; };

namespace np {

struct event_t;
class listener_t;
class plan_t;
class child_t;
class testnode_t;
class job_t;

class runner_t : public np::util::zalloc
{
public:
    runner_t();
    ~runner_t();

    void set_concurrency(int n);
    void add_listener(listener_t *);
    void list_tests(plan_t *) const;
    int run_tests(plan_t *);
    static runner_t *running() { return running_; }
    result_t raise_event(job_t *, const event_t *);

private:
    void destroy_listeners();
    void begin();
    void end();
    void set_listener(listener_t *);
    child_t *fork_child(job_t *);
    void handle_events();
    void reap_children();
    void run_function(functype_t ft, spiegel::function_t *f);
    void run_fixtures(testnode_t *tn, functype_t type);
    result_t valgrind_errors(job_t *);
    result_t run_test_code(job_t *);
    void begin_job(job_t *);
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

#define np_raise(ev) \
    do { \
	event_t _ev = (ev); \
	runner_t::running()->raise_event(0, &_ev); \
    } while(0)

// close the namespace
};

#endif /* __NP_RUNNER_H__ */

