#include <np.h>
#include <stdio.h>
#include <stdlib.h>

int called = 0;

int bird_tequila(int x)
{
    fprintf(stderr, "bird_tequila(%d)\n", x);
    called = 1;
    return x/2;
}

static void test_mocking(void)
{
    int x;
    fprintf(stderr, "before\n");
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(called, 2);
}

int mock_bird_tequila(int x)
{
    fprintf(stderr, "mocked bird_tequila(%d)\n", x);
    called = 2;
    return x*2;
}

