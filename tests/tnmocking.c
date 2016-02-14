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

int called = 0;

int bird_tequila(int x)
{
    fprintf(stderr, "bird_tequila(%d)\n", x);
    called = 1;
    return x/2;
}

static NP_USED void test_mocking(void)
{
    int x;
    fprintf(stderr, "before\n");
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
    NP_ASSERT_EQUAL(called, 2);
}

int mock_bird_tequila(int x)
{
#ifdef _NP_x86_64
    /* check that the ABI-mandated stack alignment is correct */
    fprintf(stderr, "checking stack alignment\n");
    unsigned long rbp;
    __asm__ volatile("movq %%rbp, %0" : "=r"(rbp));
    NP_ASSERT((rbp & 0xf) == 0);
#endif
    fprintf(stderr, "mocked bird_tequila(%d)\n", x);
    called = 2;
    return x*2;
}

