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
/* uasserts.c - functions for graceful CuT failures */
#include "np.h"
#include "np_priv.h"
#include "except.h"

void
__np_pass(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXPASS, "NP_PASS called").at_line(file, line));
}

void
__np_fail(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXFAIL, "NP_FAIL called")
		.at_line(file, line).with_stack());
}

void
__np_notapplicable(const char *file, int line)
{
    np_throw(np::event_t(np::EV_EXNA, "NP_NOTAPPLICABLE called")
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

