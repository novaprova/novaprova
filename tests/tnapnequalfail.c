#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_ptr_not_equal_fail(void)
{
    void *x = DEFACED;
    NP_ASSERT_PTR_NOT_EQUAL(x, DEFACED);
}
