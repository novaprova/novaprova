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
#include <typeinfo>

/*
 * Intercept all exceptions at the throw point and emit
 * a NovaProva event instead.  This is easy but imperfect.
 * Known problems:
 *
 * 1.  memory allocated for the exception will be leaked
 *
 * 2.  exceptions which are thrown and then caught normally
 *     inside the code under test result in spurious test
 *     failures.
 *
 * A better approach would be to overload
 * __cxa_allocate_exception() to allocate extra space,
 * __cxa_throw() to build a np::event_t with a stack
 * sample in that space, __cxa_free_exception() to
 * free the event, and add some special case code in
 * np_try/np_catch to catch real C++ exceptions too.
 */

extern "C" void __cxa_throw(void *thrown_exception __attribute__((unused)),
			    std::type_info *tinfo,
			    void (*dtor)(void *) __attribute__((unused)))
{
    np_throw(np::event_t(np::EV_EXCEPTION, tinfo->name()).with_stack());
}

