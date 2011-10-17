#ifndef __U4C_H__
#define __U4C_H__ 1

#include <stdbool.h>

typedef struct u4c_globalstate u4c_globalstate_t;
typedef struct u4c_plan u4c_plan_t;

extern u4c_globalstate_t *u4c_init(void);
extern void u4c_list_tests(u4c_globalstate_t *);
extern int u4c_run_tests(u4c_globalstate_t *);
extern void u4c_done(u4c_globalstate_t *);

extern u4c_plan_t *u4c_plan_new(u4c_globalstate_t *);
extern void u4c_plan_delete(u4c_plan_t *);
extern bool u4c_plan_add(u4c_plan_t *, int nspec, const char **spec);
extern void u4c_plan_enable(u4c_plan_t *);

extern const char *u4c_reltimestamp(void);

/* macros for ending a test immediately with a given result */
extern void __u4c_pass(const char *file, int line);
extern void __u4c_fail(const char *file, int line);
extern void __u4c_notapplicable(const char *file, int line);

#define U4C_PASS  \
    __u4c_pass(__FILE__, __LINE__)
#define U4C_FAIL \
    __u4c_fail(__FILE__, __LINE__)
#define U4C_NOTAPPLICABLE \
    __u4c_notapplicable(__FILE__, __LINE__)

#endif /* __U4C_H__ */
