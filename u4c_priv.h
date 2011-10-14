#ifndef __U4C_PRIV_H__
#define __U4C_PRIV_H__ 1

#include "common.h"
#include "u4c.h"
#include <regex.h>
#include <bfd.h>

typedef struct u4c_object u4c_object_t;
typedef struct u4c_classifier u4c_classifier_t;
typedef struct u4c_function u4c_function_t;
typedef struct u4c_testnode u4c_testnode_t;
typedef struct u4c_child u4c_child_t;
typedef struct u4c_event u4c_event_t;
typedef struct u4c_listener u4c_listener_t;
typedef struct u4c_listener_ops u4c_listener_ops_t;
typedef struct u4c_plan_iterator u4c_plan_iterator_t;
typedef enum u4c_result u4c_result_t;

struct u4c_child
{
    u4c_child_t *next;
    pid_t pid;
    int event_pipe;	    /* read end of the pipe */
};

struct u4c_object
{
    u4c_object_t *next;
    unsigned long base;
    char *name;

    bfd *bfd;
    unsigned int nsyms;
    asymbol **syms;
};

enum u4c_functype
{
    FT_UNKNOWN,
    FT_BEFORE,
    FT_TEST,
    FT_AFTER,

    FT_NUM
};

struct u4c_classifier
{
    u4c_classifier_t *next;
    char *re;
    regex_t compiled_re;
    enum u4c_functype type;
};

struct u4c_function
{
    u4c_function_t *next;
    enum u4c_functype type;
    char *name;
    char *filename;
    char *submatch;
    void (*addr)(void);
    u4c_object_t *object;
};

struct u4c_testnode
{
    u4c_testnode_t *next;
    u4c_testnode_t *parent;
    u4c_testnode_t *children;
    char *name;
    u4c_function_t *funcs[FT_NUM];
};

struct u4c_plan_iterator
{
    int idx;
    u4c_testnode_t *node;
};

struct u4c_plan
{
    u4c_plan_t *next;
    u4c_globalstate_t *state;
    int numnodes;
    u4c_testnode_t **nodes;
    u4c_plan_iterator_t current;
};

enum u4c_result
{
    /* Note ordinal values: we use MAX() to combine
     * multiple results for a given test */
    R_UNKNOWN=0,
    R_PASS,
    R_NOTAPPLICABLE,
    R_FAIL
};

struct u4c_listener_ops
{
    void (*begin)(u4c_listener_t *);
    void (*end)(u4c_listener_t *);
    void (*begin_node)(u4c_listener_t *, const u4c_testnode_t *);
    void (*end_node)(u4c_listener_t *, const u4c_testnode_t *);
    void (*add_event)(u4c_listener_t *, const u4c_event_t *, enum u4c_functype ft);
    void (*finished)(u4c_listener_t *, u4c_result_t);
};

struct u4c_listener
{
    u4c_listener_t *next;
    u4c_listener_ops_t *ops;
};

struct u4c_globalstate
{
    u4c_classifier_t *classifiers, **classifiers_tailp;
    u4c_object_t *objects, **objects_tailp;
    u4c_function_t *funcs, **funcs_tailp;
    char *common;
    unsigned int commonlen;
    u4c_testnode_t *root;
    unsigned int maxdepth;
    u4c_plan_t *rootplan;
    u4c_plan_t *plans;
    /* runtime state */
    u4c_listener_t *listeners;
    u4c_function_t **fixtures;
    unsigned int nrun;
    unsigned int nfailed;
    int event_pipe;		/* only in child processes */
    u4c_child_t *children;	/* only in the parent process */
    int nchildren;
};

#define dispatch_listeners(st, func, ...) \
    do { \
	u4c_listener_t *_l, *_n; \
	for (_l = (st)->listeners ; _l ; _l = _n) \
	{ \
	    _n = _l->next; \
	    if ((_l)->ops->func) \
		(_l)->ops->func((_l), ## __VA_ARGS__); \
	} \
    } while(0)

/* u4c.c */
extern const char *__u4c_functype_as_string(enum u4c_functype);
extern char *__u4c_testnode_fullname(const u4c_testnode_t *tn);
extern enum u4c_functype __u4c_classify_function(u4c_globalstate_t *,
	const char *func, char *match_return, size_t maxmatch);
extern u4c_object_t *__u4c_add_object(u4c_globalstate_t *,
	const char *name, unsigned long base);
extern u4c_function_t *__u4c_add_function(u4c_globalstate_t *,
	enum u4c_functype type, const char *name,
	const char *filename, const char *submatch,
	void (*addr)(void), u4c_object_t *o);
extern u4c_listener_t *__u4c_new_text_listener(void);

/* run.c */
extern void __u4c_add_listener(u4c_globalstate_t *, u4c_listener_t *);
extern void __u4c_set_listener(u4c_globalstate_t *, u4c_listener_t *);
extern void __u4c_begin(u4c_globalstate_t *);
extern void __u4c_end(void);
extern void __u4c_run_test(u4c_testnode_t *);
extern u4c_result_t __u4c_raise_event(const u4c_event_t *, enum u4c_functype);
#define __u4c_merge(r1, r2) \
    do { \
	u4c_result_t _r1 = (r1), _r2 = (r2); \
	(r1) = (_r1 > _r2 ? _r1 : _r2); \
    } while(0)

/* textl.c */
extern u4c_listener_t *__u4c_text_listener(void);

/* proxyl.c */
extern u4c_listener_t *__u4c_proxy_listener(int fd);
extern bool __u4c_handle_proxy_call(int fd, u4c_result_t *resp);

/* discover.c */
extern bool __u4c_describe_address(u4c_globalstate_t *state,
				   unsigned long addr,
				   const char **filenamep,
				   unsigned int *linenop,
				   const char **functionp);
extern void __u4c_discover_objects(u4c_globalstate_t *state);
extern void __u4c_discover_functions(u4c_globalstate_t *state);

#endif /* __U4C_PRIV_H__ */
