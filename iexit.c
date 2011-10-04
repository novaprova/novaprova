/* iexit.c - intercept exit() calls from CUT */
#include "common.h"
#include "except.h"
#include <stdlib.h>
#include <unistd.h>

void exit(int status)
{
    if (__u4c_exceptstate.catching)
    {
	static char cond[64];
	snprintf(cond, sizeof(cond), "exit(%d)", status);
	u4c_throw(eventc(EV_EXIT, cond));
    }
    _exit(status);
}


