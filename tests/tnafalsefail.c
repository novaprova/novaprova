#include <np.h>

static void test_assert_false_fail(void)
{
    bool x = true;
    NP_ASSERT_FALSE(x);
}
