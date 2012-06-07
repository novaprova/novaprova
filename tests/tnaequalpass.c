#include <np.h>

#define FOO 45
static void test_assert_equal_pass(void)
{
    int r = FOO;
    NP_ASSERT_EQUAL(r, FOO);
}
