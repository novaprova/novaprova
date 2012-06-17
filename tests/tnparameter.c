#include <np.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

NP_PARAMETER(pastry, "donut,bearclaw,danish");

static void test_param(void)
{
    fprintf(stderr, "MSG pastry=\"%s\"\n", pastry);
}
