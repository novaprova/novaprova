#include <np.h>
#include <assert.h>

static void test_assert(void)
{
    int white = 1;
    int black = 0;

    assert(white == black);
}

