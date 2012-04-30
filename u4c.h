#ifndef __U4C_H__
#define __U4C_H__ 1

#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
namespace u4c {
class plan_t;
class runner_t;
};
typedef class u4c::plan_t u4c_plan_t;
typedef class u4c::runner_t u4c_runner_t;
extern "C" {
#else
typedef struct u4c_runner u4c_runner_t;
typedef struct u4c_plan u4c_plan_t;
#endif

extern u4c_runner_t *u4c_init(void);
extern void u4c_list_tests(u4c_runner_t *, u4c_plan_t *);
extern void u4c_set_concurrency(u4c_runner_t *, int);
extern int u4c_run_tests(u4c_runner_t *, u4c_plan_t *);
extern void u4c_done(u4c_runner_t *);

extern u4c_plan_t *u4c_plan_new(void);
extern void u4c_plan_delete(u4c_plan_t *);
extern bool u4c_plan_add_specs(u4c_plan_t *, int nspec, const char **spec);

extern const char *u4c_reltimestamp(void);

/* Test-callable macros for ending a test with a result */
extern void __u4c_pass(const char *file, int line);
extern void __u4c_fail(const char *file, int line);
extern void __u4c_notapplicable(const char *file, int line);
extern void __u4c_assert_failed(const char *filename, int lineno,
				const char *fmt, ...);

#define U4C_PASS  \
    __u4c_pass(__FILE__, __LINE__)
#define U4C_FAIL \
    __u4c_fail(__FILE__, __LINE__)
#define U4C_NOTAPPLICABLE \
    __u4c_notapplicable(__FILE__, __LINE__)

#define U4C_ASSERT(cc) \
    do { \
	if (!(cc)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT(" #cc ")"); \
    } while(0)
#define U4C_ASSERT_TRUE(a) \
    do { \
	bool _a = (a); \
	if (!_a) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_TRUE(" #a "=%u)", _a); \
    } while(0)
#define U4C_ASSERT_FALSE(a) \
    do { \
	bool _a = (a); \
	if (_a) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_FALSE(" #a "=%u)", _a); \
    } while(0)
#define U4C_ASSERT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a == _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
#define U4C_ASSERT_NOT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a != _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NOT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
#define U4C_ASSERT_PTR_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a == _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_PTR_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
#define U4C_ASSERT_PTR_NOT_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a != _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_PTR_NOT_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
#define U4C_ASSERT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a == NULL)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NULL(" #a "=%p)", _a); \
    } while(0)
#define U4C_ASSERT_NOT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a != NULL)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NOT_NULL(" #a "=%p)", _a); \
    } while(0)
#define U4C_ASSERT_STR_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (strcmp(_a ? _a : "", _b ? _b : "")) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_STR_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)
#define U4C_ASSERT_STR_NOT_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (!strcmp(_a ? _a : "", _b ? _b : "")) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_STR_NOT_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)

/* syslog matching support */
extern void __u4c_syslog_fail(const char *re, const char *, int);
#define u4c_syslog_fail(re) \
    __u4c_syslog_fail((re), __FILE__, __LINE__)
extern void __u4c_syslog_ignore(const char *re, const char *, int);
#define u4c_syslog_ignore(re) \
    __u4c_syslog_ignore((re), __FILE__, __LINE__)
extern void __u4c_syslog_match(const char *re, int tag, const char *, int);
#define u4c_syslog_match(re, tag) \
    __u4c_syslog_match((re), (tag), __FILE__, __LINE__)
extern unsigned int __u4c_syslog_count(int tag, const char *, int);
#define u4c_syslog_count(tag) \
    __u4c_syslog_count((tag), __FILE__, __LINE__)

/* parameter support */
struct __u4c_param_dec
{
    char **var;
    const char *values;
};
#define U4C_PARAMETER(nm, vals) \
    static char * nm ;\
    static const struct __u4c_param_dec *__u4c_parameter_##nm(void) __attribute__((unused)); \
    static const struct __u4c_param_dec *__u4c_parameter_##nm(void) \
    { \
	static const struct __u4c_param_dec d = { & nm , vals }; \
	return &d; \
    }

#ifdef __cplusplus
};
#endif

#endif /* __U4C_H__ */
