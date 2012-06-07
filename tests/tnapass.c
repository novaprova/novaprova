#include <np.h>

static void test_assert_pass(void)
{
    int x = 42;
    NP_ASSERT(x == 42);
}
