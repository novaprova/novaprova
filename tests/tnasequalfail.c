#include <np.h>

#define GREET	"Hello"
static void test_assert_str_equal_fail(void)
{
    const char *x = "Bye";
    NP_ASSERT_STR_EQUAL(x, GREET);
}
