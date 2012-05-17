/* iassert.c - intercept assert() failures in CUT */
#include "np_priv.h"
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
    np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
	      filename, lineno).in_function(function).with_stack());
}

void
__assert_perror_fail(int errnum,
		     const char *filename,
		     unsigned int lineno,
		     const char *function)
{
    np_throw(np::event_t(np::EV_ASSERT, strerror(errnum)).at_line(
	      filename, lineno).in_function(function).with_stack());
}

#endif

void
__assert(const char *condition,
	 const char *filename,
	 int lineno)
{
    np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
	      filename, lineno).with_stack());
}

