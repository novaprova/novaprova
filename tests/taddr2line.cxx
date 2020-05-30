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

using namespace std;
using namespace np::util;

void
some_function(void)
{
    printf("This function exists\n");
    printf("Solely to have its address\n");
    printf("Used to lookup DWARF info.\n");
}

static void addr2line(np::spiegel::state_t &state, unsigned long addr)
{
    np::spiegel::location_t loc;

    if (!state.describe_address(addr, loc))
    {
	printf("address 0x%lx filename - line - function - offset -\n", addr);
	return;
    }

    printf("address 0x%lx filename %s line %u function %s offset 0x%x\n",
	  addr,
	  loc.compile_unit_->get_absolute_path().c_str(),
	  loc.line_,
	  loc.function_ ? loc.function_->get_full_name().c_str() : "-",
	  loc.offset_);
}

int
main(int argc, char **argv __attribute__((unused)))
{
    const char *filename = 0;
    unsigned long addr = (unsigned long)&some_function + 2;
    bool stdin_flag = false;
    if (argc > 1)
    {
	if (!strcmp(argv[1], "-"))
	    stdin_flag = true;
	else
	    addr = strtoul(argv[1], 0, 0);
    }
    if (argc > 2)
    {
	filename = argv[2];
    }
    if (argc > 3)
    {
	fatal("Usage: taddr2line [addr [executable]]\n");
    }

    np::spiegel::state_t state;
    if (filename)
    {
	if (!state.add_executable(filename))
	    return 1;
    }
    else
    {
	if (!state.add_self())
	    return 1;
    }

    if (stdin_flag)
    {
	char buf[128];
	while (fgets(buf, sizeof(buf), stdin))
	{
	    char *p = strchr(buf, '\n');
	    if (p)
		*p = '\0';
	    addr = strtoul(buf, 0, 0);
	    addr2line(state, addr);
	}
    }
    else
    {
	addr2line(state, addr);
    }
    return 0;
}

