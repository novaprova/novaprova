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
    bool paths = false;
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
	else if (!strcmp(argv[i], "--paths"))
	{
	    paths = true;
	}
	else if (argv[i][0] == '-')
	{
usage:
	    spiegel::fatal("Usage: testrunner info [--preorder|--recursive] [--paths] [executable]\n");
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
    state.dump_info(preorder, paths);

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

    vector<spiegel::compile_unit_t *> units = spiegel::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("compile_unit\n");
	printf("    name: %s\n", (*i)->get_name().c_str());
	printf("    compile_dir: %s\n", (*i)->get_compile_dir().c_str());
	printf("    absolute_path: %s\n", (*i)->get_absolute_path().c_str());
	printf("    executable: %s\n", (*i)->get_executable());
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

    vector<spiegel::compile_unit_t *> units = spiegel::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s\n", (*i)->get_absolute_path().c_str());

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

    vector<spiegel::compile_unit_t *> units = spiegel::get_compile_units();
    vector<spiegel::compile_unit_t *>::iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	printf("%s\n", (*i)->get_absolute_path().c_str());
	(*i)->dump_types();
    }

    printf("\n\n");

    return 0;
}

static void addr2line(unsigned long addr)
{
    spiegel::location_t loc;

    if (!spiegel::describe_address(addr, loc))
    {
	printf("address 0x%lx filename - line - class - function -\n", addr);
	return;
    }

    printf("address 0x%lx filename %s line %u class %s function %s offset 0x%x\n",
	  addr,
	  loc.compile_unit_->get_absolute_path().c_str(),
	  loc.line_,
	  loc.class_ ? loc.class_->get_name().c_str() : "-",
	  loc.function_ ? loc.function_->get_name().c_str() : "-",
	  loc.offset_);
}

static int
test_addr2line(int argc, char **argv __attribute__((unused)))
{
    const char *filename = 0;
    unsigned long addr = (unsigned long)&test_addr2line + 10;
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
	spiegel::fatal("Usage: testrunner addr2line [addr [executable]]\n");
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

    if (stdin_flag)
    {
	char buf[128];
	while (fgets(buf, sizeof(buf), stdin))
	{
	    char *p = strchr(buf, '\n');
	    if (p)
		*p = '\0';
	    addr = strtoul(buf, 0, 0);
	    addr2line(addr);
	}
    }
    else
    {
	addr2line(addr);
    }
    return 0;
}

#if 0
static unsigned long top_of_stack;

static void
__dump_stack(unsigned long ebp, unsigned long eip)
{
    unsigned int n = 0;
    while (ebp < top_of_stack)
    {
	printf("%u EBP 0x%08lx EIP 0x%08lx", n, ebp, eip);
	spiegel::location_t loc;
	if (spiegel::describe_address(eip, loc))
	    printf(" <%s+0x%x> at %s:%u",
		  loc.function_ ? loc.function_->get_name().c_str() : "???",
		  loc.offset_,
		  loc.compile_unit_->get_absolute_path().basename().c_str(),
		  loc.line_);
	printf("\n");
	eip = ((unsigned long *)ebp)[1];
	ebp = ((unsigned long *)ebp)[0];
	n++;
    }
}

#define DUMP_STACK \
    { \
	register unsigned long ebp; \
	register unsigned long eip; \
	__asm__ volatile("mov %%ebp,%0" : "=r"(ebp)); \
	__asm__ volatile("call .Lxx; .Lxx: popl %0" : "=r"(eip)); \
	__dump_stack(ebp, eip); \
    }
#endif

static int the_function_count = 0;

int
the_function(int x, int y)
{
    int i;

    printf("Start of the_function, x=%d y=%d\n", x, y);
    the_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    printf("End of the_function, returning %d\n", y);
    return y;
}

static int another_function_count = 0;

int
another_function(int x, int y)
{
    int i;

    printf("Start of another_function, x=%d y=%d\n", x, y);
    another_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 2;
	y++;
    }
    printf("End of another_function, returning %d\n", y);
    return y;
}

class intercept_tester_t : public spiegel::intercept_t
{
public:
    intercept_tester_t();
    ~intercept_tester_t();

    void before(spiegel::call_t &);
    void after(spiegel::call_t &);

    unsigned int after_count;
    unsigned int before_count;
    int x, y, r;
    bool test_skip;
    bool test_redirect;
};

intercept_tester_t::intercept_tester_t()
 :  intercept_t((spiegel::addr_t)&the_function)
    // we don't initialise the counts to 0 because the
    // inherited operator new does that.
{
}

intercept_tester_t::~intercept_tester_t()
{
}

void
intercept_tester_t::before(spiegel::call_t &call)
{
    x = call.get_arg(0);
    y = call.get_arg(1);
    before_count++;
    printf("BEFORE x=%d y=%d\n", x, y);
    if (test_skip)
    {
	printf("SKIPPING r=%d\n", r);
	call.skip(r);
    }
    if (test_redirect)
    {
	printf("REDIRECTING\n");
	call.redirect((spiegel::addr_t)&another_function);
    }
}

void
intercept_tester_t::after(spiegel::call_t &call)
{
    r = call.get_retval();
    after_count++;
    printf("AFTER, returning %d\n", r);
}

int moe(int x, int y)
{
    int r = the_function(x+1, y+1);
    return r-1;
}

int curly(int x, int y)
{
    int r = moe(x+1, y+1);
    return r-1;
}

int larry(int x, int y)
{
    int r = curly(x+1, y+1);
    return r-1;
}

int
wide_call(int a1, int a2, int a3,
	  int a4, int a5, int a6,
	  int a7, int a8, int a9,
	  int a10, int a11, int a12)
{
    return a1+a2+a3+a4+a5+a6+a7+a8+a9+a10+a11+a12;
}

class wide_intercept_tester_t : public spiegel::intercept_t
{
public:
    wide_intercept_tester_t()
     :  intercept_t((spiegel::addr_t)&wide_call)
    {
    }
    ~wide_intercept_tester_t()
    {
    }

    int x[12], r;

    void before(spiegel::call_t &call)
    {
	int i;
	for (i = 0 ; i < 12 ; i++)
	    x[i] = call.get_arg(i);
	printf("BEFORE\n");
    }
    void after(spiegel::call_t &call)
    {
	r = call.get_retval();
	printf("AFTER, returning %d\n", r);
    }
};

static int
test_intercept(int argc, char **argv __attribute__((unused)))
{
    if (argc > 1)
    {
	spiegel::fatal("Usage: testrunner intercept\n");
    }

    spiegel::dwarf::state_t state;
    if (!state.add_self())
	return 1;

    intercept_tester_t *it = new intercept_tester_t();
    it->install();

    assert(it->x == 0);
    assert(it->y == 0);
    assert(it->r == 0);
    assert(it->before_count == 0);
    assert(it->after_count == 0);
    assert(the_function_count == 0);
    assert(another_function_count == 0);

    int r = larry(0, 39);

    assert(r == 5216);
    assert(it->x == 3);
    assert(it->y == 42);
    assert(it->r == 5219);
    assert(it->before_count == 1);
    assert(it->after_count == 1);
    assert(the_function_count == 1);
    assert(another_function_count == 0);

    r = larry(4, 24);

    assert(r == 2089841);
    assert(it->x == 7);
    assert(it->y == 27);
    assert(it->r == 2089844);
    assert(it->before_count == 2);
    assert(it->after_count == 2);
    assert(the_function_count == 2);
    assert(another_function_count == 0);

    it->test_skip = true;
    it->r = 327;
    r = larry(1, 53);
    it->test_skip = false;

    assert(r == 324);
    assert(it->x == 4);
    assert(it->y == 56);
    assert(it->r == 327);
    assert(it->before_count == 3);
    assert(it->after_count == 2);
    assert(the_function_count == 2);
    assert(another_function_count == 0);

    it->test_redirect = true;
    r = larry(0, 39);
    it->test_redirect = false;

    assert(r == 340);
    assert(it->x == 3);
    assert(it->y == 42);
    assert(it->r == 343);
    assert(it->before_count == 4);
    assert(it->after_count == 3);
    assert(the_function_count == 2);
    assert(another_function_count == 1);

    intercept_tester_t *it2 = new intercept_tester_t();
    it2->install();

    r = larry(5, 23);

    assert(r == 10058591);
    assert(it->x == 8);
    assert(it->y == 26);
    assert(it->r == 10058594);
    assert(it->before_count == 5);
    assert(it->after_count == 4);
    assert(it2->x == 8);
    assert(it2->y == 26);
    assert(it2->r == 10058594);
    assert(it2->before_count == 1);
    assert(it2->after_count == 1);
    assert(the_function_count == 3);
    assert(another_function_count == 1);

    it2->uninstall();
    delete it2;

    it->uninstall();
    delete it;

    // Test a larger stack frame
    wide_intercept_tester_t *it3 = new wide_intercept_tester_t;
    it3->install();
    r = wide_call(1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144);
    assert(it3->x[0] == 1);
    assert(it3->x[1] == 4);
    assert(it3->x[2] == 9);
    assert(it3->x[3] == 16);
    assert(it3->x[4] == 25);
    assert(it3->x[5] == 36);
    assert(it3->x[6] == 49);
    assert(it3->x[7] == 64);
    assert(it3->x[8] == 81);
    assert(it3->x[9] == 100);
    assert(it3->x[10] == 121);
    assert(it3->x[11] == 144);
    assert(it3->r == 650);
    it3->uninstall();
    delete it3;

    printf("PASS\n");
    return 0;
}


int
main(int argc, char **argv)
{
#if 0
    top_of_stack = (unsigned long)&argc;
#endif

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
    if (!strcmp(argv[1], "addr2line"))
	return test_addr2line(argc-1, argv+1);
    if (!strcmp(argv[1], "intercept"))
	return test_intercept(argc-1, argv+1);
    spiegel::fatal("Usage: testrunner command args...\n");
    return 1;
}

