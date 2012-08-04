#include "np_priv.h"
#include "except.h"
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
    const char *env;
    int argc;
    char **argv;
    const char **newargv;
    const char **p;

    if (RUNNING_ON_VALGRIND)
	return;

    env = getenv("NOVAPROVA_VALGRIND");
    if (env && !strcmp(env, "no"))
	return;

    if (np::spiegel::platform::is_running_under_debugger())
    {
	fprintf(stderr, "spiegel: disabling Valgrind under debugger\n");
	return;
    }

    if (!discover_args(&argc, &argv))
	return;

    fprintf(stderr, "[%s] np: starting valgrind\n",
	    np::util::rel_timestamp());

    p = newargv = (const char **)np::util::xmalloc(sizeof(char *) * (argc+6));
    *p++ = "/usr/bin/valgrind";
    *p++ = "-q";
    *p++ = "--tool=memcheck";
//     *p++ = "--leak-check=full";
//     *p++ = "--suppressions=../../../np/valgrind.supp";
    while (*argv)
	*p++ = *argv++;

    execv(newargv[0], (char * const *)newargv);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/**
 * Initialise the NovaProva library.
 *
 * @return	a new runner object
 *
 * You should call @c np_init to initialise NovaProva before running
 * any tests.  It discovers tests in the current executable, and returns
 * a pointer to a @c np_runner_t object which you can pass to
 * @c np_run_tests to actually run the tests.
 *
 * The first thing the function does is to ensure that the calling
 * executable is running under Valgrind, which involves re-running
 * the process.  So be aware that any code between the start of @c main
 * and the call to @c np_init will be run twice in two different
 * processes, the second time under Valgrind.
 */
extern "C" np_runner_t *
np_init(void)
{
    be_valground();
    np::util::rel_timestamp();
    np::testmanager_t::instance();
    return new np::runner_t;
}

/**
 * Shut down the NovaProva library.
 *
 * @param runner    The runner object to destroy
 *
 * Destroys the given runner object and shuts down the library.
 */
extern "C" void
np_done(np_runner_t *runner)
{
    delete runner;
    np::testmanager_t::done();
}

extern "C" const char *
np_rel_timestamp(void)
{
    return np::util::rel_timestamp();
}
