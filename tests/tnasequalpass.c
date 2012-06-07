#include <np.h>

#define GREET	"Hello"
static void test_assert_str_equal_pass(void)
{
    const char *x = "Hello";
    NP_ASSERT_STR_EQUAL(x, GREET);
}
