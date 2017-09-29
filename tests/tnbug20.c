#include <inttypes.h>
#include <np.h>
#include <stdio.h>

/* test code contributed by bernieh-atlnz https://github.com/novaprova/novaprova/issues/20 */

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
