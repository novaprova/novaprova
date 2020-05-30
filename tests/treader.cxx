/*
 * Copyright 2011-2020 Gregory Banks
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
