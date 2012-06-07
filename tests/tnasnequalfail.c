#include <np.h>

#define GREET	"Hello"
static void test_assert_str_not_equal_fail(void)
{
    const char *x = "Hello";
    NP_ASSERT_STR_NOT_EQUAL(x, GREET);
}
