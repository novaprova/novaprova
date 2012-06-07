#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_not_null_pass(void)
{
    void *x = DEFACED;
    NP_ASSERT_NOT_NULL(x);
}
