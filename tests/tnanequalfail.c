#include <np.h>

#define FOO 45
static void test_assert_not_equal_fail(void)
{
    int r = FOO;
    NP_ASSERT_NOT_EQUAL(r, FOO);
}
