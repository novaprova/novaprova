#include <np.h>

#define GREET	"Hello"
static void test_assert_str_not_equal_pass(void)
{
    const char *x = "Bye";
    NP_ASSERT_STR_NOT_EQUAL(x, GREET);
}
