/* iexcept.c - intercept C++ exceptions in CUT */
/*
 * Copyright 2011-2014 Gregory Banks
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
#include "np/event.hxx"
#include <exception>

/*
 * Verbose C++ terminate handler.  Installed with std::set_terminate().
 * Provides a usefully verbose message on stderr, including a stack
 * trace, and FAILs any running test, when an uncaught exception is
 * thrown, an attempt is made to re-throw when there is no current
 * exception, and a number of other C++ specific fatal error conditions
 */
void __np_terminate_handler(void)
{
    char *name = np::spiegel::platform::current_exception_type();
    if (!name)
    {
	np_throw(np::event_t(np::EV_EXCEPTION,
		 "terminate called without an active exception").with_stack());
    }

    static char desc[2048];
    snprintf(desc, sizeof(desc), "terminate called with exception %s", name);
    free(name);

    /*
     * STL std::exception has a useful method for generating a human
     * readable message from the exception, call that if the exception
     * derives from std::exception.
     */
    try
    {
	throw;	/* re-throw */
    }
    catch (std::exception &ex)
    {
	/* append to the static buffer */
	int l = strlen(desc);
	snprintf(desc+l, sizeof(desc)-l, ": %s", ex.what());
    }
    catch (...)
    {
	/* nope, it wasn't derived from std::exception.  Nevermind */
    }

    /*
     * Unlike the library's verbose terminate handler, we need to
     * actually clean up the exception object, because we know we're
     * running under Valgrind and the memory leak will mess up our
     * results.  There is no portable way to do this.
     */
    np::spiegel::platform::cleanup_current_exception();

    np_throw(np::event_t(np::EV_EXCEPTION, desc).with_stack());
}
