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
#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"

using namespace std;
using namespace np::util;

int vegan(int x)
{
    string trace = np::spiegel::describe_stacktrace();
    printf("Stacktrace: \n%s\n", trace.c_str());
    return x/2;
}

namespace umami
{
    namespace pickled
    {
	int irony(int x)
	{
	    return vegan(x-3);
	}
    };
};

class leggings
{
public:
    static int dreamcatcher(int x)
    {
	return umami::pickled::irony(x+3);
    }
};

int
main(int argc, char **argv)
{
    np::util::argv0 = argv[0];
    if (argc != 1)
	fatal("Usage: tstack\n");

    np::spiegel::dwarf::state_t state;
    if (!state.add_self())
	return 1;

    leggings::dreamcatcher(42);

    return 0;
}

