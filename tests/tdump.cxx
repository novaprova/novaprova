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
#include <string.h>

using namespace std;
using namespace np::util;

static void
dump_types(np::spiegel::state_t &state)
{
    printf("Types\n");
    printf("=====\n");

    vector<np::spiegel::compile_unit_t *> units = state.get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s\n", (*i)->get_absolute_path().c_str());
	(*i)->dump_types();
    }

    printf("\n\n");
}

static void
dump_functions(np::spiegel::state_t &state)
{
    printf("Functions\n");
    printf("=========\n");

    vector<np::spiegel::compile_unit_t *> units = state.get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s\n", (*i)->get_absolute_path().c_str());

	vector<np::spiegel::function_t *> fns = (*i)->get_functions();
	vector<np::spiegel::function_t *>::iterator j;
	for (j = fns.begin() ; j != fns.end() ; ++j)
	{
	    unsigned long addr = (unsigned long)(*j)->get_address();
	    printf("    ");
	    if (!addr)
		printf("extern");
	    else
		printf("/*0x%lx*/", addr);
	    printf(" %s\n", (*j)->to_string().c_str());
	}
    }

    printf("\n\n");
}

static void
dump_compile_units(np::spiegel::state_t &state)
{
    printf("Compile Units\n");
    printf("=============\n");

    vector<np::spiegel::compile_unit_t *> units = state.get_compile_units();
    vector<np::spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("compile_unit\n");
	printf("    name: %s\n", (*i)->get_filename().c_str());
	printf("    compile_dir: %s\n", (*i)->get_compilation_directory().c_str());
	printf("    absolute_path: %s\n", (*i)->get_absolute_path().c_str());
	printf("    executable: %s\n", (*i)->get_executable());
    }

    printf("\n\n");
}

extern void __np_terminate_handler();

int
main(int argc, char **argv)
{
    argv0 = argv[0];
    const char *a0 = 0;
    const char *filename = 0;
    std::set_terminate(__np_terminate_handler);
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    fatal("Usage: %s [executable]\n", argv0);
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
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

    if ((a0 = strrchr(argv0, '/')))
	a0++;
    else
	a0 = argv0;

    if (!strcmp(a0, "tdumpdvar"))
	state.dump_variables();
    else if (!strcmp(a0, "tdumpdabbr"))
	state.dump_abbrevs();
    else if (!strcmp(a0, "tdumpdfn"))
	state.dump_functions();
    else if (!strcmp(a0, "tdumpdstr"))
	state.dump_structs();
    else if (!strcmp(a0, "tdumpatype"))
	dump_types(state);
    else if (!strcmp(a0, "tdumpafn"))
	dump_functions(state);
    else if (!strcmp(a0, "tdumpacu"))
	dump_compile_units(state);
    else
	fatal("Don't know which dumper to run");

    return 0;
}

