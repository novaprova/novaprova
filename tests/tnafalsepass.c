#include <np.h>

static void test_assert_false_pass(void)
{
    bool x = false;
    NP_ASSERT_FALSE(x);
}

