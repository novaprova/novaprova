#include <np.h>

static void test_segv(void)
{
    *(char *)0 = 0;
}

