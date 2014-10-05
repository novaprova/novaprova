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
#include <np.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Test for dynamic mocking.  Demonstrates installing and
 * uninstalling a mock in a single test function.
 */

int called = 0;

int bird_tequila(int x)
{
    fprintf(stderr, "bird_tequila(%d)\n", x);
    called = 1;
    return x/2;
}

int not_bird_tequila(int x)
{
    fprintf(stderr, "mocked bird_tequila(%d)\n", x);
    called = 2;
    return x*2;
}

static NP_USED void test_dynamic_mocking(void)
{
    int x;

    fprintf(stderr, "before\n");
    called = 0;
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(x, 21);
    NP_ASSERT_EQUAL(called, 1);

    fprintf(stderr, "installing mock\n");
    np_mock(bird_tequila, not_bird_tequila);

    fprintf(stderr, "before\n");
    called = 0;
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(x, 84);
    NP_ASSERT_EQUAL(called, 2);

    fprintf(stderr, "removing mock\n");
    np_unmock(bird_tequila);

    fprintf(stderr, "before\n");
    called = 0;
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(x, 21);
    NP_ASSERT_EQUAL(called, 1);

    fprintf(stderr, "installing mock by name\n");
    np_mock_by_name("bird_tequila", not_bird_tequila);

    fprintf(stderr, "before\n");
    called = 0;
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(x, 84);
    NP_ASSERT_EQUAL(called, 2);

    fprintf(stderr, "removing mock by name\n");
    np_unmock_by_name("bird_tequila");

    fprintf(stderr, "before\n");
    called = 0;
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(x, 21);
    NP_ASSERT_EQUAL(called, 1);
}

