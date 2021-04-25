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
 */
#include <np.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Test for dynamic mocking derived from an example suggested by
 * Greig Hamilton on the mailing list.  Demonstrates using a
 * single mock function for every test in a .c file by installing
 * it during the setup function.
 */

int get_value(void)
{
    return 0;
}

int find_answer(void)
{
    return get_value() + 42;
}

static int return_100(void)
{
    return 100;
}

static NP_USED int setup(void)
{
    np_mock(get_value, return_100);
    return 0;
}

static NP_USED void test_one(void)
{
    int answer;
    answer = find_answer();
    NP_ASSERT_EQUAL(142, answer);
}

static NP_USED void test_two(void)
{
    int answer;
    answer = find_answer();
    NP_ASSERT_EQUAL(142, answer);
}

static NP_USED void test_three(void)
{
    int answer;
    answer = find_answer();
    NP_ASSERT_EQUAL(142, answer);
}

