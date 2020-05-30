/*
 * Copyright 2011-2020 Gregory Banks
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
 *
 * This testrunner has no tests, but it does have a parameter.
 * NP can of course run no tests, but it should do so gracefully
 * and without crashing.
 */
#include <np.h>
#include <stdio.h>

NP_PARAMETER(mustache, "brooklyn,shoreditch");

void this_function_does_nothing(void)
{
    printf("Hello, I'm doing nothing\n");
}
