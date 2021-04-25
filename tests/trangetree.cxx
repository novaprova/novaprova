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
#include "np/util/rangetree.hxx"
#include "np/util/log.hxx"
#include "fw.h"

using namespace std;
using namespace np::util;

int
main(int argc, char **argv)
{
    np::util::argv0 = argv[0];
    if (argc != 1)
	fatal("Usage: trangetree\n");
    np::log::basic_config(np::log::DEBUG, 0);

    BEGIN("rangetree basic");
    np::util::rangetree<int, int> rt;
    CHECK(rt.size() == 0);
    CHECK(rt.begin() == rt.end());
    CHECK(rt.find(40) == rt.end());
    CHECK(rt.find(41) == rt.end());
    CHECK(rt.find(42) == rt.end());
    CHECK(rt.find(43) == rt.end());
    CHECK(rt.find(44) == rt.end());
    END;

    BEGIN("rangetree one");
    np::util::rangetree<int, int> rt;
    rt.insert(41, 43, 100041);
    CHECK(rt.size() == 1);

    auto ii = rt.begin();
    CHECK(ii != rt.end());
    CHECK(ii->first.lo == 41);
    CHECK(ii->first.hi == 43);
    CHECK(ii->second == 100041);
    ++ii;
    CHECK(ii == rt.end());

    auto ci = rt.find(40);
    CHECK(ci == rt.end());
    ci = rt.find(41);
    CHECK(ci != rt.end());
    CHECK(ci->second == 100041);
    ci = rt.find(42);
    CHECK(ci != rt.end());
    CHECK(ci->second == 100041);
    ci = rt.find(43);
    CHECK(ci == rt.end());
    ci = rt.find(44);
    CHECK(ci == rt.end());
    END;

    return 0;
}

