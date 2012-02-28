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

struct u4c_child_t;
struct u4c_event_t;
struct u4c_classifier_t;
struct u4c_function_t;
struct u4c_testnode_t;
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

struct u4c_child_t
{
    u4c_child_t *next;
    pid_t pid;
    int event_pipe;	    /* read end of the pipe */
    u4c_testnode_t *node;
    u4c_result_t result;
    bool finished;
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
    u4c_classifier_t *next;
    char *re;
    regex_t compiled_re;
    enum u4c_functype type;
};

struct u4c_function_t
{
    u4c_function_t *next;
    enum u4c_functype type;
    spiegel::function_t *func;
    spiegel::filename_t filename;
    char *submatch;

    u4c_function_t()
     :  next(0),
        type(FT_UNKNOWN),
	func(0),
	submatch(0)
    {
    }

};

struct u4c_testnode_t
{
    u4c_testnode_t *next;
    u4c_testnode_t *parent;
    u4c_testnode_t *children;
    char *name;
    u4c_function_t *funcs[FT_NUM];

    std::string get_fullname() const;
    u4c_testnode_t *find(const char *name);
};

struct u4c_plan_iterator_t
{
    int idx;
    u4c_testnode_t *node;
};

struct u4c_plan_t
{
    u4c_plan_t *next;
    u4c_globalstate_t *state;
    int numnodes;
    u4c_testnode_t **nodes;
    u4c_plan_iterator_t current;
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

    /* u4c.c */
    u4c_functype classify_function(const char *func, char *match_return, size_t maxmatch);
    u4c_function_t *add_function(enum u4c_functype type, spiegel::function_t *func, const char *submatch);
    void add_classifier(const char *re, bool case_sensitive, enum u4c_functype type);
    void setup_classifiers();
    void find_common_path();
    void add_testnode(char *name, u4c_function_t *func);
    void generate_nodes();
    void dump_nodes(u4c_testnode_t *tn, int level);
    void set_concurrency(int n);
    void list_tests();
    int run_tests();
    /* run.c */
    void begin();
    void end();
    void add_listener(u4c_listener_t *);
    void set_listener(u4c_listener_t *);
    /* discover.c */
    void discover_functions();

    static u4c_globalstate_t *state;

    u4c_classifier_t *classifiers, **classifiers_tailp;
    spiegel::dwarf::state_t *spiegel;
    u4c_function_t *funcs, **funcs_tailp;
    char *common;
    unsigned int commonlen;
    u4c_testnode_t *root;
    unsigned int maxdepth;
    u4c_plan_t *rootplan;
    u4c_plan_t *plans;
    /* runtime state */
    std::vector<u4c_listener_t*> listeners_;
    u4c_function_t **fixtures;
    unsigned int nrun;
    unsigned int nfailed;
    int event_pipe;		/* only in child processes */
    u4c_child_t *children;	/* only in the parent process */
    int nchildren;
    int maxchildren;
    int npfd;
    struct pollfd *pfd;
};

#define dispatch_listeners(st, func, ...) \
    do { \
	vector<u4c_listener_t*>::iterator _i; \
	for (_i = (st)->listeners_.begin() ; \
	     _i != (st)->listeners_.end() ; \
	     ++_i) \
	    (*_i)->func(__VA_ARGS__); \
    } while(0)

/* u4c.c */
extern const char *__u4c_functype_as_string(enum u4c_functype);

/* run.c */
extern void __u4c_begin_test(u4c_testnode_t *);
extern void __u4c_wait(void);
extern u4c_result_t __u4c_raise_event(const u4c_event_t *, enum u4c_functype);
#define __u4c_merge(r1, r2) \
    do { \
	u4c_result_t _r1 = (r1), _r2 = (u4c_result_t)(r2); \
	(r1) = (_r1 > _r2 ? _r1 : _r2); \
    } while(0)

/* proxyl.c */
extern bool __u4c_handle_proxy_call(int fd, u4c_result_t *resp);


#endif /* __U4C_PRIV_H__ */
