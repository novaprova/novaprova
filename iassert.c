/* iassert.c - intercept assert() failures in CUT */
/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

void
__assert(const char *condition,
	 const char *filename,
	 int lineno)
{
    np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
	      filename, lineno).with_stack());
}

#endif

#if defined(__GNUC__) && defined(__APPLE__)

void
__assert_rtn(const char *function,
	     const char *filename,
	     int lineno,
	     const char *condition)
{
    if (function == (const char *)-1)
	np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
		  filename, lineno).with_stack());
    else
	np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
		  filename, lineno).in_function(function).with_stack());
}

void
__eprintf(const char *format __attribute__((unused)),
	  const char *filename,
	  unsigned lineno,
	  const char *condition)
{
    np_throw(np::event_t(np::EV_ASSERT, condition).at_line(
	      filename, lineno).with_stack());
}

#endif
