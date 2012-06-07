#include <np.h>

static void test_assert_true_pass(void)
{
    bool x = true;
    NP_ASSERT_TRUE(x);
}
