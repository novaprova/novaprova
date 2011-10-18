/* uasserts.c - functions for graceful CuT failures */
#include "u4c.h"
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/valgrind.h>

void
__u4c_pass(const char *file, int line)
{
    u4c_throw(event(EV_EXPASS, "U4C_PASS called", file, line, NULL));
}

void
__u4c_fail(const char *file, int line)
{
    VALGRIND_PRINTF_BACKTRACE("U4C_FAIL failed at %s:%u\n",
			      file, line);
    u4c_throw(event(EV_EXFAIL, "U4C_FAIL called", file, line, NULL));
}

void
__u4c_notapplicable(const char *file, int line)
{
    u4c_throw(event(EV_EXNA, "U4C_NOTAPPLICABLE called", file, line, NULL));
}

void
__u4c_assert_failed(const char *file,
		    int line,
		    const char *fmt,
		    ...)
{
    va_list args;
    static char condition[1024];

    va_start(args, fmt);
    vsnprintf(condition, sizeof(condition), fmt, args);
    va_end(args);

    VALGRIND_PRINTF_BACKTRACE("Assert %s failed at %s:%u\n",
			      condition, file, line);
    u4c_throw(event(EV_ASSERT, condition, file, line, NULL));
}

