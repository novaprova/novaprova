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
#include "fw.h"
#include <sys/stat.h>

using namespace std;
using namespace np::util;

#define BASEDIR	"/tmp"
#define TESTDIR	BASEDIR"/ggcov.filename.test"

static char oldcwd[PATH_MAX];

int
setup(void)
{
    struct stat sb;
    int r;

    r = system("rm -rf " TESTDIR);
    if (r)
	return -1;
    r = stat(TESTDIR, &sb);
    if (r == 0 || errno != ENOENT)
    {
	perror(TESTDIR);
	return -1;
    }
    mkdir(TESTDIR, 0777);
    mkdir(TESTDIR"/dir3", 0777);
    mkdir(TESTDIR"/dir3/dir4", 0777);
    r = stat(TESTDIR, &sb);
    if (r != 0)
    {
	perror(TESTDIR);
	return -1;
    }

    if (getcwd(oldcwd, sizeof(oldcwd)) == NULL)
    {
	perror("getcwd");
	return -1;
    }
    if (chdir(TESTDIR"/dir3/dir4") < 0)
    {
	perror(TESTDIR"/dir3/dir4");
	return -1;
    }

    return 0;
}

int
teardown()
{
    int r;

    if (oldcwd[0])
	chdir(oldcwd);

    r = system("rm -rf " TESTDIR);
    if (r)
	return -1;
    return 0;
}

int
main(int argc, char **argv __attribute__((unused)))
{
    argv0 = argv[0];
    if (argc != 1)
	fatal("Usage: %s filenames\n", argv0);

#define TESTCASE(in, expected) \
{ \
    np::util::filename_t _in(in); \
    BEGIN("absolute(\"%s\")", _in.c_str()); \
    np::util::filename_t _out = _in.make_absolute(); \
    CHECK(!strcmp(_out.c_str(), expected)); \
    END; \
}
    TESTCASE("/foo/bar", "/foo/bar");
    TESTCASE("/foo", "/foo");
    TESTCASE("/", "/");
    TESTCASE("foo", TESTDIR"/dir3/dir4/foo");
    TESTCASE("foo/bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE(".", TESTDIR"/dir3/dir4");
    TESTCASE("./foo", TESTDIR"/dir3/dir4/foo");
    TESTCASE("./foo/bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("./foo/./bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("./././foo/./bar", TESTDIR"/dir3/dir4/foo/bar");
    TESTCASE("..", TESTDIR"/dir3");
    TESTCASE("../foo", TESTDIR"/dir3/foo");
    TESTCASE("../foo/bar", TESTDIR"/dir3/foo/bar");
    TESTCASE("../../foo", TESTDIR"/foo");
    TESTCASE("../../../foo/bar", BASEDIR"/foo/bar");
    TESTCASE("./../.././../foo/bar", BASEDIR"/foo/bar");
#undef TESTCASE

#define TESTCASE(in, expected) \
{ \
    np::util::filename_t _in(in); \
    BEGIN("normalise(\"%s\")", _in.c_str()); \
    np::util::filename_t _out = _in.normalise(); \
    CHECK(!strcmp(_out.c_str(), expected)); \
    END; \
}
    TESTCASE("/foo/bar", "/foo/bar");
    TESTCASE("//foo////bar", "/foo/bar");
    TESTCASE("/", "/");
    TESTCASE("foo", "foo");
    TESTCASE("./foo", "foo");
    TESTCASE("./././foo", "foo");
    TESTCASE("./foo/./", "foo");
    TESTCASE("foo/bar", "foo/bar");
    TESTCASE("foo////bar", "foo/bar");
    TESTCASE("./foo", "foo");
    TESTCASE("./foo/bar", "foo/bar");
    TESTCASE("./foo///bar", "foo/bar");
    TESTCASE(".//foo/bar", "foo/bar");
    TESTCASE("././././foo/bar", "foo/bar");
    TESTCASE(".", ".");
    TESTCASE("../foo", "../foo");
    TESTCASE("../foo/bar", "../foo/bar");
    TESTCASE("../foo//bar", "../foo/bar");
    TESTCASE("foo/..", ".");
    TESTCASE("foo///..", ".");
    TESTCASE("foo/../bar/..", ".");
    TESTCASE("foo//.././bar/.//..", ".");
    TESTCASE("../../../foo/bar", "../../../foo/bar");
    TESTCASE("./../.././../foo/bar", "../../../foo/bar");
#undef TESTCASE

    // Check that passing a null pointer when creating
    // a filename_t object doesn't cause a crash.
    char *null_string = NULL;
    BEGIN("null pointer passed to constructor");
    np::util::filename_t _in(null_string);
    CHECK(!strcmp(_in.c_str(), ""));
    END;

    {
	BEGIN("pop_back");
	filename_t f = "/foo/bar/baz";
	CHECK(!strcmp(f.c_str(), "/foo/bar/baz"));
	f.pop_back();
	CHECK(!strcmp(f.c_str(), "/foo/bar"));
	f.pop_back();
	CHECK(!strcmp(f.c_str(), "/foo"));
	f.pop_back();
	CHECK(!strcmp(f.c_str(), "/"));
	f.pop_back();
	CHECK(!strcmp(f.c_str(), "/"));
	END;
    }

    return 0;
}

