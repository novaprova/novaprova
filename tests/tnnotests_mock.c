/*
 * Copyright 2011-2021 Gregory Banks
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
 * This testrunner has no tests, but it does have a static mock..
 * NP can of course run no tests, but it should do so gracefully
 * and without crashing.
 */
#include <stdio.h>

void mock_hard_with_a_vengeance(void)
{
    printf("Are you mocking me?\n");
}

void this_function_does_nothing(void)
{
    printf("Hello, I'm doing nothing\n");
}
