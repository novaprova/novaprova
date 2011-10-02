/* iassert.c - intercept assert() failures in CUT */
#include "u4c_priv.h"
#include "except.h"
#include <assert.h>
#include <unistd.h>

#if defined(__GLIBC__)

void
__assert_fail(const char *condition,
	      const char *filename,
	      unsigned int lineno,
	      const char *function)
{
    u4c_throw(event(EV_ASSERT, condition,
	      filename, lineno, function));
}

void
__assert_perror_fail(int errnum,
		     const char *filename,
		     unsigned int lineno,
		     const char *function)
{
    u4c_throw(event(EV_ASSERT, strerror(errnum),
	      filename, lineno, function));
}

#endif

void
__assert(const char *condition,
	 const char *filename,
	 int lineno)
{
    u4c_throw(event(EV_ASSERT, condition,
	      filename, lineno, 0));
}

