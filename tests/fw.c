#include "fw.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int is_verbose(void)
{
    static int v = -1;
    if (v < 0)
    {
	const char *e = getenv("VERBOSE");
	if (e && !strcmp(e, "yes"))
	    v = 1;
	else
	    v = 0;
    }
    return v;
}

char __testname[1024];

void setup(void)
{
}

void teardown(void)
{
}
