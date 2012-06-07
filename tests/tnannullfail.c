#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_not_null_fail(void)
{
    void *x = NULL;
    NP_ASSERT_NOT_NULL(x);
}
