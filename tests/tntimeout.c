#include <np.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void test_notimeout(void)
{
    fprintf(stderr, "MSG Sleeping for 29 sec\n");
    sleep(29);
    fprintf(stderr, "MSG Awoke!\n");
}

static void test_timeout(void)
{
    fprintf(stderr, "MSG Sleeping for 31 sec\n");
    sleep(31);
    fprintf(stderr, "MSG Awoke!\n");
}

