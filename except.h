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
#ifndef __NP_EXCEPT_H__
#define __NP_EXCEPT_H__ 1

#include "np/util/common.hxx"
#include "np/event.hxx"
#include "np/spiegel/spiegel.hxx"
#include <setjmp.h>

struct __np_exceptstate_t
{
    jmp_buf jbuf;
    bool catching;
    int caught;
    np::event_t event;
};

/* in run.c */
extern __np_exceptstate_t __np_exceptstate;

#define np_try \
	__np_exceptstate.catching = true; \
	if (!(__np_exceptstate.caught = setjmp(__np_exceptstate.jbuf)))
#define np_catch(x) \
	__np_exceptstate.catching = false; \
	(x) = __np_exceptstate.caught ? &__np_exceptstate.event : 0; \
	if (__np_exceptstate.caught)

#define np_throw(ev) \
	do { \
	    if (__np_exceptstate.catching) \
	    { \
		__np_exceptstate.event = (ev); \
		longjmp(__np_exceptstate.jbuf, 1); \
	    } \
	    abort(); \
	} while(0)

#endif /* __NP_EXCEPT_H__ */
