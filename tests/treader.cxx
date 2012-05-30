#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"
#include "fw.h"

using namespace std;
using namespace np::util;

int
main(int argc __attribute__((unused)),
     char **argv __attribute__((unused)))
{
#define TESTCASE(in, out) \
    { \
	BEGIN("read_sleb128(%d)", out); \
	np::spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	int32_t v = 0; \
	CHECK(r.read_sleb128(v)); \
	CHECK(v == out); \
	END; \
    }
    TESTCASE("\x02", 2);
    TESTCASE("\x7e", -2);
    TESTCASE("\xff\x00", 127);
    TESTCASE("\x81\x7f", -127);
    TESTCASE("\x80\x01", 128);
    TESTCASE("\x80\x7f", -128);
    TESTCASE("\x81\x01", 129);
    TESTCASE("\xff\x7e", -129);

#undef TESTCASE
#define TESTCASE(in, out) \
    { \
	BEGIN("read_uleb128(%u)", out); \
	np::spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	uint32_t v = 0; \
	CHECK(r.read_uleb128(v)); \
	CHECK(v == out); \
	END; \
    }

    TESTCASE("\x02", 2);
    TESTCASE("\x7f", 127);
    TESTCASE("\x80\x01", 128);
    TESTCASE("\x81\x01", 129);
    TESTCASE("\x82\x01", 130);
    TESTCASE("\xb9\x64", 12857);

#undef TESTCASE
    return 0;
}
