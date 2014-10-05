/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __NP_H__
#define __NP_H__ 1

#include <stdbool.h>
#include <string.h>

/** @file np.h
 * NovaProva C API.
 * @author Greg Banks <gnb@fastmail.fm>
 */

#ifdef __cplusplus
namespace np {
class plan_t;
class runner_t;
};
typedef class np::plan_t np_plan_t;
typedef class np::runner_t np_runner_t;
extern "C" {
#else
typedef struct np_runner np_runner_t;
typedef struct np_plan np_plan_t;
#endif

extern np_runner_t *np_init(void);
extern void np_list_tests(np_runner_t *, np_plan_t *);
extern void np_set_concurrency(np_runner_t *, int);
extern bool np_set_output_format(np_runner_t *, const char *);
extern int np_run_tests(np_runner_t *, np_plan_t *);
extern int np_get_timeout(void);   /* in seconds, or zero */
extern void np_done(np_runner_t *);

extern np_plan_t *np_plan_new(void);
extern bool np_plan_add_specs(np_plan_t *, int nspec, const char **spec);
extern void np_plan_delete(np_plan_t *);

extern const char *np_rel_timestamp(void);

/* Test-callable macros for ending a test with a result */
extern void __np_pass(const char *file, int line);
extern void __np_fail(const char *file, int line);
extern void __np_notapplicable(const char *file, int line);
extern void __np_assert_failed(const char *filename, int lineno,
				const char *fmt, ...);

/** Causes the running test to terminate immediately with a PASS result.
 *
 * Causes the running test to terminate immediately with a PASS result.
 * You will probably never need to call this, as merely reaching the end
 * of a test function without FAILing is considered a PASS result.
 */
#define NP_PASS  \
    __np_pass(__FILE__, __LINE__)
/** Causes the running test to terminate immediately with a FAIL result.
 *
 * Causes the running test to terminate immediately with a FAIL result. */
#define NP_FAIL \
    __np_fail(__FILE__, __LINE__)
/** Causes the running test to terminate immediately with a NOTAPPLICABLE result.
 *
 * Causes the running test to terminate immediately with a NOTAPPLICABLE result.
 * The NOTAPPLICABLE result is not counted towards either failures or
 * successes and is useful for tests whose preconditions are not
 * satisfied and have thus not actually run. */
#define NP_NOTAPPLICABLE \
    __np_notapplicable(__FILE__, __LINE__)

/** Test that a given boolean condition is true, otherwise FAIL the test.
 *
 * Test that a given boolean condition is true, otherwise FAIL the test. */
#define NP_ASSERT(cc) \
    do { \
	if (!(cc)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT(" #cc ")"); \
    } while(0)
/** Test that a given boolean condition is true, otherwise FAIL the test.
 *
 * Test that a given boolean condition is true, otherwise FAIL the test.
 * This is the same as @c NP_ASSERT except that the message printed on
 * failure is slightly more helpful.  */
#define NP_ASSERT_TRUE(a) \
    do { \
	bool _a = (a); \
	if (!_a) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_TRUE(" #a "=%u)", _a); \
    } while(0)
/** Test that a given boolean condition is false, otherwise FAIL the test.
 *
 * Test that a given boolean condition is false, otherwise FAIL the test. */
#define NP_ASSERT_FALSE(a) \
    do { \
	bool _a = (a); \
	if (_a) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_FALSE(" #a "=%u)", _a); \
    } while(0)
/** Test that two signed integers are equal, otherwise FAIL the test.
 *
 * Test that two signed integers are equal, otherwise FAIL the test. */
#define NP_ASSERT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a == _b)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
/** Test that two signed integers are not equal, otherwise FAIL the test.
 *
 * Test that two signed integers are not equal, otherwise FAIL the test. */
#define NP_ASSERT_NOT_EQUAL(a, b) \
    do { \
	long long _a = (a), _b = (b); \
	if (!(_a != _b)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_NOT_EQUAL(" #a "=%lld, " #b "=%lld)", _a, _b); \
    } while(0)
/** Test that two pointers are equal, otherwise FAIL the test.
 *
 * Test that two pointers are equal, otherwise FAIL the test. */
#define NP_ASSERT_PTR_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a == _b)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_PTR_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
/** Test that two pointers are not equal, otherwise FAIL the test.
 *
 * Test that two pointers are not equal, otherwise FAIL the test. */
#define NP_ASSERT_PTR_NOT_EQUAL(a, b) \
    do { \
	const void *_a = (a), *_b = (b); \
	if (!(_a != _b)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_PTR_NOT_EQUAL(" #a "=%p, " #b "=%p)", _a, _b); \
    } while(0)
/** Test that a pointer is NULL, otherwise FAIL the test.
 *
 * Test that a pointer is NULL, otherwise FAIL the test. */
#define NP_ASSERT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a == NULL)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_NULL(" #a "=0x%lx)", (unsigned long)_a); \
    } while(0)
/** Test that a pointer is not NULL, otherwise FAIL the test.
 *
 * Test that a pointer is not NULL, otherwise FAIL the test. */
#define NP_ASSERT_NOT_NULL(a) \
    do { \
	const void *_a = (a); \
	if (!(_a != NULL)) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_NOT_NULL(" #a "=0x%lx)", (unsigned long)_a); \
    } while(0)
/** Test that two strings are equal, otherwise FAIL the test.
 *
 * Test that two strings are equal, otherwise FAIL the test.
 * Either string can be NULL; NULL compares like the empty string.
 */
#define NP_ASSERT_STR_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (strcmp(_a ? _a : "", _b ? _b : "")) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_STR_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)
/** Test that two strings are not equal, otherwise FAIL the test.
 *
 * Test that two strings are not equal, otherwise FAIL the test.
 * Either string can be NULL, it compares like the empty string.
 */
#define NP_ASSERT_STR_NOT_EQUAL(a, b) \
    do { \
	const char *_a = (a), *_b = (b); \
	if (!strcmp(_a ? _a : "", _b ? _b : "")) \
	    __np_assert_failed(__FILE__, __LINE__, \
	    "NP_ASSERT_STR_NOT_EQUAL(" #a "=\"%s\", " #b "=\"%s\")", _a, _b); \
    } while(0)

/* syslog matching support */
/** Set up to FAIL the test on syslog messages matching a regexp.
 *
 * @param re	    POSIX extended regular expression to match
 *
 * From this point until the end of the test, if any code emits a
 * message to @c syslog which matches the given regular expression, the
 * test will FAIL immediately as if @c NP_FAIL had been called from
 * inside @c syslog.
 */
extern void np_syslog_fail(const char *re);
/** Set up to ignore syslog messages matching a regexp.
 *
 * @param re	    POSIX extended regular expression to match
 *
 * From this point until the end of the test function, if any code emits
 * a message to @c syslog which matches the given regular expression,
 * nothing will happen.  Note that this is the default behaviour, so
 * this call is only useful in complex cases where there are multiple
 * overlapping regexps being used for syslog matching.
 */
extern void np_syslog_ignore(const char *re);
/** Set up to count syslog messages matching a regexp.
 *
 * @param re	    POSIX extended regular expression to match
 * @param tag	    tag for later matching of counts
 *
 * From this point until the end of the test function, if any code emits
 * a message to @c syslog which matches the given regular expression, a
 * counter will be incremented and no other action will be taken.  The
 * counts can be retrieved by calling @c np_syslog_count.  Note that
 * @a tag does not need to be unique; in fact always passing 0 is reasonable.
 */
extern void np_syslog_match(const char *re, int tag);
/** Return the number of syslog matches for the given tag.
 *
 * @param tag	    tag to choose which matches to count, or -1 for all
 * @return	    count of matched messages
 *
 * Calculate and return the number of messages emitted to @c syslog which
 * matched a regexp set up earlier using @c np_syslog_match.  If @a tag is
 * less than zero, all match counts will be returned, otherwise only the
 * match counts for regexps registered with the same tag will be
 * returned.
 */
extern unsigned int np_syslog_count(int tag);

/* parameter support */
struct __np_param_dec
{
    char **var;
    const char *values;
};
/**
 * Statically define a test parameter and its values.
 *
 * @param nm	    C identifier of the variable to be declared
 * @param vals	    string literal with the set of values to apply
 *
 * Define a @c static @c char* variable called @a nm, and declare it as a
 * test parameter on the testnode corresponding to the source file in
 * which it appears, with a set of values defined by splitting up the
 * string literal @a vals on whitespace and commas.  For example:
 * @code
 * NP_PARAMETER(db_backend, "mysql,postgres");
 * @endcode
 * Declares a variable called @c db_backend in the current file, and at
 * runtime every test function in this file will be run twice, once with
 * the variable @c db_backend set to @c "mysql" and once with it set to
 * @c "postgres".
 */
#define NP_PARAMETER(nm, vals) \
    static char * nm ;\
    static const struct __np_param_dec *__np_parameter_##nm(void) __attribute__((used)); \
    static const struct __np_param_dec *__np_parameter_##nm(void) \
    { \
	static const struct __np_param_dec d = { & nm , vals }; \
	return &d; \
    }

/**
 * Install a dynamic mock by function pointer.
 *
 * @param fn the function to mock
 * @param to the function to call instead
 *
 * Installs a temporary dynamic function mock.  The mock can
 * be removed with @c np_unmock() or it can be left in place
 * to be automatically uninstalled when the test finishes.
 *
 * Note that if @c np_mock() may be called in a fixture setup
 * routine to install the mock for every test in a test source
 * file.
 */
#define np_mock(fn, to) __np_mock((void (*)(void))(fn), #fn, (void (*)(void))to)
extern void __np_mock(void (*from)(void), const char *name, void (*to)(void));

/**
 * Uninstall a dynamic mock by function pointer.
 *
 * @param fn the address of the function to mock
 *
 * Uninstall any dynamic mocks installed earlier by @c np_mock for
 * function @a fn. Note that dynamic mocks will be automatically
 * uninstalled at the end of the test, so calling @c np_unmock()
 * might not even be necessary in your tests.
 */
#define np_unmock(fn) __np_unmock((void (*)(void))(fn))
extern void __np_unmock(void (*from)(void));

/**
 * Install a dynamic mock by function name.
 *
 * @param fname the name of the function to mock
 * @param to the function to call instead
 *
 * Installs a temporary dynamic function mock.  The mock can
 * be removed with @c np_unmock_by_name() or it can be left in place
 * to be automatically uninstalled when the test finishes.
 *
 * Note that if @c np_mock_by_name() may be called in a fixture setup
 * routine to install the mock for every test in a test source
 * file.
 */
#define np_mock_by_name(fname, to) __np_mock_by_name(fname, (void (*)(void))to)
extern void __np_mock_by_name(const char *fname, void (*to)(void));

/**
 * Uninstall a dynamic mock by function name.
 *
 * @param fname the name of the function to mock
 *
 * Uninstall any dynamic mocks installed earlier by @c np_mock_by_name
 * for function @a fname. Note that dynamic mocks will be automatically
 * uninstalled at the end of the test, so calling @c np_unmock_by_name()
 * might not even be necessary in your tests.
 */
extern void np_unmock_by_name(const char *fname);

/**
 * Ensure a static test function is actually compiled
 *
 * Some compilers will not emit code for functions which have 'static'
 * linkage but are not called from within the compile unit.  If you find
 * it convenient to declare test functions that way, you should use this
 * macro when defining the function.  It provides a compiler-specific
 * hint which makes the compiler emit the function anyway.  For example
 *
 * static NP_USED void test_me_sometime(void)
 * { ... }
 */
#define NP_USED	    __attribute__((used))

#ifdef __cplusplus
};
#endif

#endif /* __NP_H__ */
