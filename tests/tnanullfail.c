#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_null_fail(void)
{
    void *x = DEFACED;
    NP_ASSERT_NULL(x);
}
