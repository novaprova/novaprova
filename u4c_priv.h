#ifndef __U4C_PRIV_H__
#define __U4C_PRIV_H__ 1

#include <regex.h>

typedef struct u4c_object u4c_object_t;
typedef struct u4c_classifier u4c_classifier_t;
typedef struct u4c_function u4c_function_t;
typedef struct u4c_testnode u4c_testnode_t;
typedef struct u4c_globalstate u4c_globalstate_t;

struct u4c_object
{
    u4c_object_t *next;
    unsigned long base;
    char *name;
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

struct u4c_globalstate
{
    u4c_classifier_t *classifiers, **classifiers_tailp;
    u4c_object_t *objects, **objects_tailp;
    u4c_function_t *funcs, **funcs_tailp;
    char *common;
    unsigned int commonlen;
    u4c_testnode_t *root;
    unsigned int maxdepth;
    /* runtime state */
    u4c_function_t **fixtures;
    unsigned int nrun;
    unsigned int nfailed;
};

extern const char *__u4c_functype_as_string(enum u4c_functype);

#endif /* __U4C_PRIV_H__ */
