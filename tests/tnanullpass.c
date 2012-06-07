#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_null_pass(void)
{
    void *x = NULL;
    NP_ASSERT_NULL(x);
}
