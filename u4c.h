#ifndef __U4C_H__
#define __U4C_H__ 1

#include <stdbool.h>
#include <string.h>

/** @file u4c.h NovaProva C API */

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
extern void u4c_set_output_format(u4c_runner_t *, const char *);
extern int u4c_run_tests(u4c_runner_t *, u4c_plan_t *);
extern void u4c_done(u4c_runner_t *);

extern u4c_plan_t *u4c_plan_new(void);
extern bool u4c_plan_add_specs(u4c_plan_t *, int nspec, const char **spec);
extern void u4c_plan_delete(u4c_plan_t *);

/* Test-callable macros for ending a test with a result */
extern void __u4c_pass(const char *file, int line);
extern void __u4c_fail(const char *file, int line);
extern void __u4c_notapplicable(const char *file, int line);
extern void __u4c_assert_failed(const char *filename, int lineno,
				const char *fmt, ...);

/** Causes the running test to terminate immediately with a PASS result.
 *
 * Causes the running test to terminate immediately with a PASS result.
 * You will probably never need to call this, as merely reaching the end
 * of a test function without FAILing is considered a PASS result.
 */
#define U4C_PASS  \
    __u4c_pass(__FILE__, __LINE__)
/** Causes the running test to terminate immediately with a FAIL result.
 *
 * Causes the running test to terminate immediately with a FAIL result. */
#define U4C_FAIL \
    __u4c_fail(__FILE__, __LINE__)
/** Causes the running test to terminate immediately with a NOTAPPLICABLE result.
 *
 * Causes the running test to terminate immediately with a NOTAPPLICABLE result.
 * The NOTAPPLICABLE result is not counted towards either failures or
 * successes and is useful for tests whose preconditions are not
 * satisfied and have thus not actually run. */
#define U4C_NOTAPPLICABLE \
    __u4c_notapplicable(__FILE__, __LINE__)

/** Test that a given boolean condition is true, otherwise FAIL the test.
 *
 * Test that a given boolean condition is true, otherwise FAIL the test. */
#define U4C_ASSERT(cc) \
    do { \
	if (!(cc)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT(" #cc ")"); \
    } while(0)
/** Test that a given boolean condition is true, otherwise FAIL the test.
 *
 * Test that a given boolean condition is true, otherwise FAIL the test.
 * This is the same as `U4C_ASSERT` except that the message printed on
 * failure is slightly more helpful.  */
#define U4C_ASSERT_TRUE(a) \
    do { \
	bool _a = (a); \
	if (!_a) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_TRUE(" #a "=%u)", _a); \
    } while(0)
/** Test that a given boolean condition is false, otherwise FAIL the test.
 *
 * Test that a given boolean condition is false, otherwise FAIL the test. */
#define U4C_ASSERT_FALSE(a) \
    do { \
	bool _a = (a); \
	if (_a) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_FALSE(" #a "=%u)", _a); \
    } while(0)
/** Test that two signed integers are equal, otherwise FAIL the test.
 *
 * Test that two signed integers are equal, otherwise FAIL the test. */
#define U4C_ASSERT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a == _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
/** Test that two signed integers are not equal, otherwise FAIL the test.
 *
 * Test that two signed integers are not equal, otherwise FAIL the test. */
#define U4C_ASSERT_NOT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a != _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NOT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
/** Test that two pointers are equal, otherwise FAIL the test.
 *
 * Test that two pointers are equal, otherwise FAIL the test. */
#define U4C_ASSERT_PTR_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a == _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_PTR_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
/** Test that two pointers are not equal, otherwise FAIL the test.
 *
 * Test that two pointers are not equal, otherwise FAIL the test. */
#define U4C_ASSERT_PTR_NOT_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a != _b)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_PTR_NOT_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
/** Test that a pointer is NULL, otherwise FAIL the test.
 *
 * Test that a pointer is NULL, otherwise FAIL the test. */
#define U4C_ASSERT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a == NULL)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NULL(" #a "=%p)", _a); \
    } while(0)
/** Test that a pointer is not NULL, otherwise FAIL the test.
 *
 * Test that a pointer is not NULL, otherwise FAIL the test. */
#define U4C_ASSERT_NOT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a != NULL)) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_NOT_NULL(" #a "=%p)", _a); \
    } while(0)
/** Test that two strings are equal, otherwise FAIL the test.
 *
 * Test that two strings are equal, otherwise FAIL the test.
 * Either string can be NULL; NULL compares like the empty string.
 */
#define U4C_ASSERT_STR_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (strcmp(_a ? _a : "", _b ? _b : "")) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_STR_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)
/** Test that two strings are not equal, otherwise FAIL the test.
 *
 * Test that two strings are not equal, otherwise FAIL the test.
 * Either string can be NULL, it compares like the empty string.
 */
#define U4C_ASSERT_STR_NOT_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (!strcmp(_a ? _a : "", _b ? _b : "")) \
	    __u4c_assert_failed(__FILE__, __LINE__, \
	    "U4C_ASSERT_STR_NOT_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)

/* syslog matching support */
extern void __u4c_syslog_fail(const char *re, const char *, int);
/** Set up to FAIL the test on syslog messages matching a regexp.
 *
 * From this point until the end of the test, if any code emits a
 * message to `syslog` which matches the given regular expression, the
 * test will FAIL immediately as if `U4C_FAIL` had been called from
 * inside `syslog`.
 */
#define u4c_syslog_fail(re) \
    __u4c_syslog_fail((re), __FILE__, __LINE__)
extern void __u4c_syslog_ignore(const char *re, const char *, int);
/** Set up to ignore syslog messages matching a regexp.
 *
 * From this point until the end of the test function, if any code emits
 * a message to `syslog` which matches the given regular expression,
 * nothing will happen.  Note that this is the default behaviour, so
 * this call is only useful in complex cases where there are multiple
 * overlapping regexps being used for syslog matching.
 */
#define u4c_syslog_ignore(re) \
    __u4c_syslog_ignore((re), __FILE__, __LINE__)
extern void __u4c_syslog_match(const char *re, int tag, const char *, int);
/** Set up to count syslog messages matching a regexp.
 *
 * From this point until the end of the test function, if any code emits
 * a message to `syslog` which matches the given regular expression, a
 * counter will be incremented and no other action will be taken.  The
 * counts can be retrieved by calling `u4c_syslog_count`.  Note that the
 * `tag` argument does not need to be unique; in fact passing 0 for all
 * tags is reasonable.
 */
#define u4c_syslog_match(re, tag) \
    __u4c_syslog_match((re), (tag), __FILE__, __LINE__)
extern unsigned int __u4c_syslog_count(int tag, const char *, int);
/** Return the number of syslog matches for the given tag.
 *
 * Returns the number of messages emited to `syslog` which matched a
 * regexp set up earlier using `u4c_syslog_match`.  If `tag` is less
 * than zero, all match counts will be returned, otherwise only the
 * match counts for regexps registered with the same tag will be
 * returned.
 */
#define u4c_syslog_count(tag) \
    __u4c_syslog_count((tag), __FILE__, __LINE__)

/* parameter support */
struct __u4c_param_dec
{
    char **var;
    const char *values;
};
/**
 * Statically define a test parameter and its values.
 *
 * Define a `static char *` variable called `nm`, and declare it as a
 * test parameter on the testnode corresponding to the source file in
 * which it appears, with a set of values defined by splitting up the
 * string literal `vals` on whitespace and commas.  For example:
 *
 *     U4C_PARAMETER(db_backend, "mysql,postgres");
 *
 * Declares a variable called `db_backend` in the current file, and at
 * runtime every test function in this file will be run twice, once with
 * the variable `db_backend` set to `"mysql"` and once with it set to
 * `"postgres"`.
 */
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
