/*
 * Copyright 2011-2015 Gregory Banks
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
#include <np.h>

int function_one(int *ip)
{
    return 42 + *ip;
}

static void test_uninitialized_int(void)
{
    int never_initted;
    int x = function_one(&never_initted);
    NP_ASSERT_EQUAL(x, 43);
}

