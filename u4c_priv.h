#ifndef __U4C_PRIV_H__
#define __U4C_PRIV_H__ 1

#include "common.h"
#include "u4c.h"
#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/filename.hxx"
#include <regex.h>
#include <bfd.h>
#include <sys/poll.h>
#include <vector>
#include <list>

class u4c_child_t;
struct u4c_event_t;
struct u4c_classifier_t;
struct u4c_function_t;
class u4c_testnode_t;
struct u4c_plan_iterator_t;
struct u4c_plan;
class u4c_listener_t;
class u4c_globalstate_t;

enum u4c_result_t
{
    /* Note ordinal values: we use MAX() to combine
     * multiple results for a given test */
    R_UNKNOWN=0,
    R_PASS,
    R_NOTAPPLICABLE,
    R_FAIL
};

class u4c_child_t
{
public:
    u4c_child_t(pid_t pid, int fd, u4c_testnode_t *tn);
    ~u4c_child_t();

    pid_t get_pid() const { return pid_; }
    u4c_testnode_t *get_node() const { return node_; }
    u4c_result_t get_result() const { return result_; }

    void poll_setup(struct pollfd &);
    void poll_handle(struct pollfd &);
    void merge_result(u4c_result_t r);

private:
    pid_t pid_;
    int event_pipe_;	    /* read end of the pipe */
    u4c_testnode_t *node_;
    u4c_result_t result_;
    bool finished_;
};

enum u4c_functype
{
    FT_UNKNOWN,
    FT_BEFORE,
    FT_TEST,
    FT_AFTER,

    FT_NUM
};

struct u4c_classifier_t
{
public:
    u4c_classifier_t()
    {
	memset(this, 0, sizeof(*this));
    }
    ~u4c_classifier_t()
    {
	xfree(re_);
	regfree(&compiled_re_);
    }

    bool set_regexp(const char *, bool);
    void set_functype(enum u4c_functype type) { type_ = type; }
    enum u4c_functype classify(const char *, char *, size_t) const;

private:
    char *re_;
    regex_t compiled_re_;
    enum u4c_functype type_;
};

class u4c_testnode_t
{
public:
    u4c_testnode_t(const char *);
    ~u4c_testnode_t();

    std::string get_fullname() const;
    u4c_testnode_t *find(const char *name);
    u4c_testnode_t *make_path(std::string name);
    void set_function(enum u4c_functype, spiegel::function_t *);

    u4c_testnode_t *skip_common();
    u4c_testnode_t *next_preorder();
    spiegel::function_t *get_function(u4c_functype type) const
    {
	return funcs_[type];
    }
    std::list<spiegel::function_t*> get_fixtures(u4c_functype type) const;

    void dump(int level) const;

private:
    u4c_testnode_t *next_;
    u4c_testnode_t *parent_;
    u4c_testnode_t *children_;
    char *name_;
    spiegel::function_t *funcs_[FT_NUM];
};

struct u4c_plan_iterator_t
{
    int idx;
    u4c_testnode_t *node;
};

class u4c_plan_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    u4c_plan_t(u4c_globalstate_t *state);
    ~u4c_plan_t();

    void add_node(u4c_testnode_t *tn);
    bool add_specs(int nspec, const char **specs);

    u4c_testnode_t *next();

private:
    u4c_globalstate_t *state_;
    int numnodes_;
    u4c_testnode_t **nodes_;
    u4c_plan_iterator_t current_;

    friend void u4c_plan_enable(u4c_plan_t *plan);
};

class u4c_listener_t
{
public:
    u4c_listener_t() {}
    ~u4c_listener_t() {}

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void begin_node(const u4c_testnode_t *) = 0;
    virtual void end_node(const u4c_testnode_t *) = 0;
    virtual void add_event(const u4c_event_t *, enum u4c_functype ft) = 0;
    virtual void finished(u4c_result_t) = 0;
};

class u4c_text_listener_t : public u4c_listener_t
{
public:
    u4c_text_listener_t() {}
    ~u4c_text_listener_t() {}

    void begin();
    void end();
    void begin_node(const u4c_testnode_t *tn);
    void end_node(const u4c_testnode_t *tn);
    void add_event(const u4c_event_t *ev, enum u4c_functype ft);
    void finished(u4c_result_t res);

private:
    unsigned int nrun_;
    unsigned int nfailed_;
    u4c_result_t result_; /* for the current test */
};

class u4c_proxy_listener_t : public u4c_listener_t
{
public:
    u4c_proxy_listener_t(int);
    ~u4c_proxy_listener_t();

    void begin();
    void end();
    void begin_node(const u4c_testnode_t *);
    void end_node(const u4c_testnode_t *);
    void add_event(const u4c_event_t *ev, enum u4c_functype ft);
    void finished(u4c_result_t res);

    /* proxyl.c */
    static bool handle_call(int fd, u4c_result_t *resp);

private:
    int fd_;
};


class u4c_globalstate_t
{
public:
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }
    u4c_globalstate_t();
    ~u4c_globalstate_t();

    void initialise();
    void set_concurrency(int n);
    void list_tests();
    void add_listener(u4c_listener_t *);
    int run_tests();
    static u4c_globalstate_t *running() { return running_; }
    u4c_result_t raise_event(const u4c_event_t *, enum u4c_functype);

private:
    /* u4c.c */
    void discover_functions();
    void setup_classifiers();
    u4c_functype classify_function(const char *func, char *match_return, size_t maxmatch);
    void add_classifier(const char *re, bool case_sensitive, enum u4c_functype type);
    void dump_nodes(u4c_testnode_t *tn, int level);
    /* run.c */
    void begin();
    void end();
    void set_listener(u4c_listener_t *);
    const u4c_event_t *normalise_event(const u4c_event_t *ev);
    u4c_child_t *fork_child(u4c_testnode_t *tn);
    void handle_events();
    void reap_children();
    void run_function(enum u4c_functype ft, spiegel::function_t *f);
    void run_fixtures(u4c_testnode_t *tn, enum u4c_functype type);
    u4c_result_t valgrind_errors();
    u4c_result_t run_test_code(u4c_testnode_t *tn);
    void begin_test(u4c_testnode_t *);
    void wait();
    /* discover.c */

    static u4c_globalstate_t *running_;

    std::vector<u4c_classifier_t*> classifiers_;
    spiegel::dwarf::state_t *spiegel;
    u4c_testnode_t *root_;
    u4c_testnode_t *base_;
    u4c_plan_t *plan_;
    /* runtime state */
    std::vector<u4c_listener_t*> listeners_;
    unsigned int nrun_;
    unsigned int nfailed_;
    int event_pipe_;		/* only in child processes */
    std::vector<u4c_child_t*> children_;	// only in the parent process
    unsigned int maxchildren;
    std::vector<struct pollfd> pfd_;

    friend class u4c_plan_t;
    friend void u4c_plan_enable(u4c_plan_t *plan);
};

/* u4c.c */
extern const char *__u4c_functype_as_string(enum u4c_functype);

/* run.c */
#define __u4c_merge(r1, r2) \
    do { \
	u4c_result_t _r1 = (r1), _r2 = (u4c_result_t)(r2); \
	(r1) = (_r1 > _r2 ? _r1 : _r2); \
    } while(0)


#endif /* __U4C_PRIV_H__ */
