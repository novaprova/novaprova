/*
 * Copyright 2017 @bernieh-atlnz, Gregory Banks
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
 * Test mocking of functions which return 64b values.  This was
 * broken for 32b ABIs.
 * 
 * test code contributed by bernieh-atlnz https://github.com/novaprova/novaprova/issues/20
 */
#include <inttypes.h>
#include <np.h>
#include <stdio.h>


int called = 0;

uint64_t m_func(void)
{
    called = 2;
    return 0xa1a2a3a4a5a6a7a8LL;
}

uint64_t func(void)
{
    called = 1;
    return 0x0102030405060708LL;
}

void test_uint64_bug(void)
{
    np_mock(func, m_func);
    uint64_t asdf = func();
    NP_ASSERT_EQUAL(asdf, 0xa1a2a3a4a5a6a7a8LL);
    NP_ASSERT_EQUAL(called, 2);
}
