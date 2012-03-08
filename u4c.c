#include "common.h"
#include "u4c_priv.h"
#include "except.h"
#include "spiegel/tok.hxx"
#include <sys/time.h>
#include <valgrind/valgrind.h>

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
u4c_reltimestamp(void)
{
    static char buf[32];
    static struct timeval first;
    struct timeval now;
    struct timeval delta;
    gettimeofday(&now, NULL);
    if (!first.tv_sec)
	first = now;
    timersub(&now, &first, &delta);
    snprintf(buf, sizeof(buf), "%lu.%06lu",
	     (unsigned long)delta.tv_sec,
	     (unsigned long)delta.tv_usec);
    return buf;
}

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

    p = newargv = (const char **)xmalloc(sizeof(char *) * (argc+6));
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

extern "C" u4c_runner_t *
u4c_init(void)
{
    be_valground();
    u4c_reltimestamp();
    u4c::testmanager_t::instance();
    return new u4c::runner_t;
}

extern "C" void
u4c_done(u4c_runner_t *runner)
{
    delete runner;
    u4c::testmanager_t::done();
}

