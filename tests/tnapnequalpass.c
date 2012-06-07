#include <np.h>

#define DEFACED	((void *)0xbdefaced)
static void test_assert_ptr_not_equal_pass(void)
{
    void *x = (void *)0x42;
    NP_ASSERT_PTR_NOT_EQUAL(x, DEFACED);
}
