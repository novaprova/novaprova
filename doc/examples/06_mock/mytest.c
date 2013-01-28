/*
 * Copyright 2011-2013 Gregory Banks
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

#include <stdio.h>
#include <np.h>	    /* NovaProva library */
#include "mycode.h" /* declares the Code Under Test */

static void test_simple(void)
{
    int r;

    r = myatoi("42");
    NP_ASSERT_EQUAL(r, 42);
}

int mock_mychar2int(int c)
{
    fprintf(stderr, "Mocked mychar2int('%c') called\n", c);
    return (c - '0');
}
