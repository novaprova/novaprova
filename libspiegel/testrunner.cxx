#include "spiegel/spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
using namespace std;

#define BASEDIR	"/tmp"
#define TESTDIR	BASEDIR"/ggcov.filename.test"

static char oldcwd[PATH_MAX];

static int
filenames_setup()
{
    struct stat sb;
    int r;

    r = system("rm -rf "TESTDIR);
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

static int
filenames_teardown()
{
    int r;

    if (oldcwd[0])
	chdir(oldcwd);

    r = system("rm -rf "TESTDIR);
    if (r)
	return -1;
    return 0;
}

static int
test_filenames(int argc, char **argv __attribute__((unused)))
{
    if (argc != 1)
	spiegel::fatal("Usage: testrunner filenames\n");

    filenames_setup();

#define TESTCASE(in, expected) \
{ \
    spiegel::filename_t _in(in); \
    spiegel::filename_t _out = _in.make_absolute(); \
    spiegel::filename_t _exp(expected); \
    fprintf(stderr, "absolute(\"%s\") = \"%s\", expecting \"%s\"\n", \
	_in.c_str(), _out.c_str(), _exp.c_str()); \
    assert(!strcmp(_out.c_str(), _exp.c_str())); \
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
    filenames_teardown();
    filenames_setup();
#define TESTCASE(in, expected) \
{ \
    spiegel::filename_t _in(in); \
    spiegel::filename_t _out = _in.normalise(); \
    spiegel::filename_t _exp(expected); \
    fprintf(stderr, "absolute(\"%s\") = \"%s\", expecting \"%s\"\n", \
	_in.c_str(), _out.c_str(), _exp.c_str()); \
    assert(!strcmp(_out.c_str(), _exp.c_str())); \
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
    filenames_teardown();
    return 0;
}

static int
test_info(int argc, char **argv)
{
    bool preorder = true;
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (!strcmp(argv[i], "--preorder"))
	{
	    preorder = true;
	}
	else if (!strcmp(argv[i], "--recursive"))
	{
	    preorder = false;
	}
	else if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner info [--preorder|--recursive] [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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
    state.dump_info(preorder);

    return 0;
}

static int
test_abbrevs(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner abbrevs [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    state.dump_abbrevs();

    return 0;
}

static int
test_functions_dwarf(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner functions_dwarf [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    state.dump_functions();

    return 0;
}

static int
test_variables(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner variables [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    state.dump_variables();

    return 0;
}

static int
test_structs(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner structs [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    state.dump_structs();

    return 0;
}

static int
test_read_uleb128(int argc __attribute__((unused)),
		  char **argv __attribute__((unused)))
{
#define TESTCASE(in, out) \
    { \
	printf(". read_uleb128(%u) ", out); fflush(stdout); \
	spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	uint32_t v; \
	if (!r.read_uleb128(v) || v != out) \
	{ \
	    printf("FAIL\n"); \
	    return 1; \
	} \
	printf("PASS\n"); \
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

static int
test_read_sleb128(int argc __attribute__((unused)),
		  char **argv __attribute__((unused)))
{
#define TESTCASE(in, out) \
    { \
	printf(". read_sleb128(%d) ", out); fflush(stdout); \
	spiegel::dwarf::reader_t r(in, sizeof(in)-1); \
	int32_t v; \
	if (!r.read_sleb128(v) || v != out) \
	{ \
	    printf("FAIL\n"); \
	    return 1; \
	} \
	printf("PASS\n"); \
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
    return 0;
}

static int
test_compile_units(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner compile_units [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    printf("Compile Units\n");
    printf("=============\n");

    vector<spiegel::compile_unit_t *> units = spiegel::compile_unit_t::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s/%s [%s]\n",
	       (*i)->get_compile_dir(), (*i)->get_name(),
	       (*i)->get_executable());
    }

    printf("\n\n");

    return 0;
}

static int
test_functions(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner functions [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    printf("Functions\n");
    printf("=========\n");

    vector<spiegel::compile_unit_t *> units = spiegel::compile_unit_t::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s/%s\n", (*i)->get_compile_dir(), (*i)->get_name());

	vector<spiegel::function_t *> fns = (*i)->get_functions();
	vector<spiegel::function_t *>::iterator j;
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

    return 0;
}

static int
test_types(int argc, char **argv)
{
    const char *filename = 0;
    for (int i = 1 ; i < argc ; i++)
    {
	if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner types [executable]\n");
	}
	else
	{
	    if (filename)
		goto usage;
	    filename = argv[i];
	}
    }

    spiegel::dwarf::state_t state;
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

    printf("Types\n");
    printf("=====\n");

    vector<spiegel::compile_unit_t *> units = spiegel::compile_unit_t::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s/%s\n", (*i)->get_compile_dir(), (*i)->get_name());
	(*i)->dump_types();
    }

    printf("\n\n");

    return 0;
}


int
main(int argc, char **argv)
{
    spiegel::argv0 = argv[0];
    if (!strcmp(argv[1], "filenames"))
	return test_filenames(argc-1, argv+1);
    if (!strcmp(argv[1], "info"))
	return test_info(argc-1, argv+1);
    if (!strcmp(argv[1], "abbrevs"))
	return test_abbrevs(argc-1, argv+1);
    if (!strcmp(argv[1], "functions"))
	return test_functions(argc-1, argv+1);
    if (!strcmp(argv[1], "functions_dwarf"))
	return test_functions_dwarf(argc-1, argv+1);
    if (!strcmp(argv[1], "variables"))
	return test_variables(argc-1, argv+1);
    if (!strcmp(argv[1], "structs"))
	return test_structs(argc-1, argv+1);
    if (!strcmp(argv[1], "read_uleb128"))
	return test_read_uleb128(argc-1, argv+1);
    if (!strcmp(argv[1], "read_sleb128"))
	return test_read_sleb128(argc-1, argv+1);
    if (!strcmp(argv[1], "compile_units"))
	return test_compile_units(argc-1, argv+1);
    if (!strcmp(argv[1], "types"))
	return test_types(argc-1, argv+1);
    spiegel::fatal("Usage: testrunner command args...\n");
    return 1;
}

