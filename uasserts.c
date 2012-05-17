/* uasserts.c - functions for graceful CuT failures */
#include "np.h"
#include "np_priv.h"
#include "except.h"

void
__np_pass(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXPASS, "U4C_PASS called").at_line(file, line));
}

void
__np_fail(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXFAIL, "U4C_FAIL called")
		.at_line(file, line).with_stack());
}

void
__np_notapplicable(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXNA, "U4C_NOTAPPLICABLE called")
		.at_line(file, line).with_stack());
}

void
__np_assert_failed(const char *file,
		    int line,
		    const char *fmt,
		    ...)
{
    va_list args;
    static char condition[1024];

    va_start(args, fmt);
    vsnprintf(condition, sizeof(condition), fmt, args);
    va_end(args);

    np_throw(np::event_t(np::EV_ASSERT, condition)
	    .at_line(file, line).with_stack());
}

