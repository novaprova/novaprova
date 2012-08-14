/* iexit.c - intercept exit() calls from CUT */
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
#include "np/util/common.hxx"
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


