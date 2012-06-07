#include <np.h>

static void test_assert_true_fail(void)
{
    bool x = false;
    NP_ASSERT_TRUE(x);
}
