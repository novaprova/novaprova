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
#include <stdio.h>
#include <stdlib.h>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

void do_a_small_overrun(char *buf)
{
    strcpy(buf, "012345678901234567890123456789012");
}

#if 0
/* This generates a vgcore, which is inconvenient */
static void test_stack_overrun_small(void)
{
    char buf[32];

    fprintf(stderr, "MSG about to overrun a buffer by a small amount\n");
    do_a_small_overrun(buf);
    fprintf(stderr, "MSG overran\n");
}
#endif

static void test_heap_overrun_small(void)
{
    char *buf = malloc(32);

    fprintf(stderr, "MSG about to overrun a buffer by a small amount\n");
    do_a_small_overrun(buf);
    fprintf(stderr, "MSG overran\n");
    free(buf);
}

#if 0
/* Valgrind doesn't detect overruns of static buffers */
static void test_static_overrun_small(void)
{
    static char buf[32];

    fprintf(stderr, "MSG about to overrun a buffer by a small amount\n");
    do_a_small_overrun(buf);
    fprintf(stderr, "MSG overran\n");
}
#endif
