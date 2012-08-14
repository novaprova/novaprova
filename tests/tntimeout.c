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
#include <unistd.h>
#include <stdlib.h>

static void test_notimeout(void)
{
    int timeout = np_get_timeout();
    if (!timeout) return;
    fprintf(stderr, "MSG Sleeping for less than timeout\n");
    sleep(timeout-1);
    fprintf(stderr, "MSG Awoke!\n");
}

static void test_timeout(void)
{
    int timeout = np_get_timeout();
    if (!timeout) return;
    fprintf(stderr, "MSG Sleeping for more than timeout\n");
    sleep(timeout+1);
    fprintf(stderr, "MSG Awoke! - shouldn't happen\n");
}

