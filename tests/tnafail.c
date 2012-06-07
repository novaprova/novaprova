#include <np.h>

static void test_assert_fail(void)
{
    int x = 42;
    NP_ASSERT(x == 43);
}
