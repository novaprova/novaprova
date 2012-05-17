/* iexit.c - intercept exit() calls from CUT */
#include "np/common.hxx"
#include "except.h"
#include <stdlib.h>
#include <unistd.h>

void exit(int status)
{
    if (__np_exceptstate.catching)
    {
	static char cond[64];
	snprintf(cond, sizeof(cond), "exit(%d)", status);
	np_throw(np::event_t(np::EV_EXIT, cond).with_stack());
    }
    _exit(status);
}


