#include <unistd.h>
#include <stdlib.h>
#include <np.h>

static void test_assert_fail(void)
{
    int x = 42;
    U4C_ASSERT(x == 43);
}
static void test_assert_pass(void)
{
    int x = 42;
    U4C_ASSERT(x == 42);
}

static void test_assert_true_fail(void)
{
    bool x = false;
    U4C_ASSERT_TRUE(x);
}
static void test_assert_true_pass(void)
{
    bool x = true;
    U4C_ASSERT_TRUE(x);
}
static void test_assert_false_fail(void)
{
    bool x = true;
    U4C_ASSERT_FALSE(x);
}
static void test_assert_false_pass(void)
{
    bool x = false;
    U4C_ASSERT_FALSE(x);
}

#define FOO 45
static void test_assert_equal_fail(void)
{
    int r = -32;
    U4C_ASSERT_EQUAL(r, FOO);
}
static void test_assert_equal_pass(void)
{
    int r = FOO;
    U4C_ASSERT_EQUAL(r, FOO);
}
static void test_assert_not_equal_fail(void)
{
    int r = FOO;
    U4C_ASSERT_NOT_EQUAL(r, FOO);
}
static void test_assert_not_equal_pass(void)
{
    int r = -33;
    U4C_ASSERT_NOT_EQUAL(r, FOO);
}

#define DEFACED	((void *)0xbdefaced)
static void test_assert_ptr_equal_fail(void)
{
    void *x = (void *)0x42;
    U4C_ASSERT_PTR_EQUAL(x, DEFACED);
}
static void test_assert_ptr_equal_pass(void)
{
    void *x = DEFACED;
    U4C_ASSERT_PTR_EQUAL(x, DEFACED);
}
static void test_assert_ptr_not_equal_fail(void)
{
    void *x = DEFACED;
    U4C_ASSERT_PTR_NOT_EQUAL(x, DEFACED);
}
static void test_assert_ptr_not_equal_pass(void)
{
    void *x = (void *)0x42;
    U4C_ASSERT_PTR_NOT_EQUAL(x, DEFACED);
}

static void test_assert_null_fail(void)
{
    void *x = DEFACED;
    U4C_ASSERT_NULL(x);
}
static void test_assert_null_pass(void)
{
    void *x = NULL;
    U4C_ASSERT_NULL(x);
}
static void test_assert_not_null_fail(void)
{
    void *x = NULL;
    U4C_ASSERT_NOT_NULL(x);
}
static void test_assert_not_null_pass(void)
{
    void *x = DEFACED;
    U4C_ASSERT_NOT_NULL(x);
}

#define GREET	"Hello"
static void test_assert_str_equal_fail(void)
{
    const char *x = "Bye";
    U4C_ASSERT_STR_EQUAL(x, GREET);
}
static void test_assert_str_equal_pass(void)
{
    const char *x = "Hello";
    U4C_ASSERT_STR_EQUAL(x, GREET);
}
static void test_assert_str_not_equal_fail(void)
{
    const char *x = "Hello";
    U4C_ASSERT_STR_NOT_EQUAL(x, GREET);
}
static void test_assert_str_not_equal_pass(void)
{
    const char *x = "Bye";
    U4C_ASSERT_STR_NOT_EQUAL(x, GREET);
}

int
main(int argc, char **argv)
{
    int ec = 0;
    np_runner_t *runner = np_init();
    ec = np_run_tests(runner, 0);
    np_done(runner);
    exit(ec);
}
