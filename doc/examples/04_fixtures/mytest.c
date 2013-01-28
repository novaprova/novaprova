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
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <np.h>	    /* NovaProva library */

static char *str;

static int set_up(void)
{
    fprintf(stderr, "Setting up fixtures\n");
    str = strdup("Hello");
    return 0;
}

static void test_one(void)
{
    fprintf(stderr, "Running test_one\n");
    str[0] = tolower(str[0]);
    NP_ASSERT_STR_EQUAL(str, "hello");
    fprintf(stderr, "Finished test_one\n");
}

static void test_two(void)
{
    fprintf(stderr, "Running test_two\n");
    str[1] = toupper(str[1]);
    NP_ASSERT_STR_EQUAL(str, "HEllo");
    fprintf(stderr, "Finished test_two\n");
}

static int tear_down(void)
{
    fprintf(stderr, "Tearing down fixtures\n");
    free(str);
    str = NULL;
    return 0;
}

