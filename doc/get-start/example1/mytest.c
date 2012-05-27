#include <np.h>	    /* NovaProva library */
#include "mycode.h" /* declares the Code Under Test */

static void test_simple(void)
{
    int r;

    r = myatoi("42");
    NP_ASSERT_EQUAL(r, 42);
}

static void test_initial(void)
{
    int r;

    r = myatoi("4=2");
    NP_ASSERT_EQUAL(r, 4);
}
