#include <np.h>

#define FOO 45
static void test_assert_equal_fail(void)
{
    int r = -32;
    NP_ASSERT_EQUAL(r, FOO);
}
