#include <np.h>

#define FOO 45
static void test_assert_not_equal_pass(void)
{
    int r = -33;
    NP_ASSERT_NOT_EQUAL(r, FOO);
}
