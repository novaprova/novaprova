#include <np.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void test_notimeout(void)
{
    int timeout = np_get_timeout();
    if (!timeout) return;
    fprintf(stderr, "MSG Sleeping for less than timeout\n");
    sleep(timeout-1);
    fprintf(stderr, "MSG Awoke!\n");
}

static void test_timeout(void)
{
    int timeout = np_get_timeout();
    if (!timeout) return;
    fprintf(stderr, "MSG Sleeping for more than timeout\n");
    sleep(timeout+1);
    fprintf(stderr, "MSG Awoke! - shouldn't happen\n");
}

