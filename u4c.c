#include "u4c_priv.h"
#include "except.h"
#include "spiegel/tok.hxx"
#include <sys/time.h>
#include <valgrind/valgrind.h>

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char **environ;

static bool
discover_args(int *argcp, char ***argvp)
{
    char **p;
    int n;

    /* This early, environ[] points at the area
     * above argv[], so walk down from there */
    for (p = environ-2, n = 1;
	 ((int *)p)[-1] != n ;
	 --p, ++n)
	;
    *argcp = n;
    *argvp = p;
    return true;
}

static void
be_valground(void)
{
    int argc;
    char **argv;
    const char **newargv;
    const char **p;

    if (RUNNING_ON_VALGRIND)
	return;
    fprintf(stderr, "u4c: starting valgrind\n");

    if (!discover_args(&argc, &argv))
	return;

    p = newargv = (const char **)u4c::xmalloc(sizeof(char *) * (argc+6));
    *p++ = "/usr/bin/valgrind";
    *p++ = "-q";
    *p++ = "--tool=memcheck";
//     *p++ = "--leak-check=full";
//     *p++ = "--suppressions=../../../u4c/valgrind.supp";
    while (*argv)
	*p++ = *argv++;

    execv(newargv[0], (char * const *)newargv);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/**
 * Initialise the NovaProva library.
 *
 * You should call `u4c_init()` to initialise NovaProva before running
 * any tests.  It discovers tests in the current executable, and returns
 * a pointer to a `u4c_runner_t` object which you can pass to
 * `u4c_run_tests()` to actually run the tests.
 *
 * The first thing the function does is to ensure that the calling
 * executable is running under Valgrind, which involves re-running
 * the process.  So be aware that any code between the start of `main`
 * and the call to `u4c_init` will be run twice in two different
 * processes, the second time under Valgrind.
 */
extern "C" u4c_runner_t *
u4c_init(void)
{
    be_valground();
    u4c::rel_timestamp();
    u4c::testmanager_t::instance();
    return new u4c::runner_t;
}

/**
 * Shut down the NovaProva library.
 *
 * Destroys the given `u4c_runner_t` object and shuts down the library.
 */
extern "C" void
u4c_done(u4c_runner_t *runner)
{
    delete runner;
    u4c::testmanager_t::done();
}

