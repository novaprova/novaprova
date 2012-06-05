#include <np.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

static void test_memleak(void)
{
    malloc(32);
}


