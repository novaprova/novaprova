/* iexit.c - intercept exit() calls from CUT */
#include "u4c/common.hxx"
#include "except.h"
#include <stdlib.h>
#include <unistd.h>

void exit(int status)
{
    if (__u4c_exceptstate.catching)
    {
	static char cond[64];
	snprintf(cond, sizeof(cond), "exit(%d)", status);
	u4c_throw(u4c::event_t(u4c::EV_EXIT, cond).with_stack());
    }
    _exit(status);
}


