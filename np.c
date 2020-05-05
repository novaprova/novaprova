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
#include "np_priv.h"
#include "except.h"
#include <sys/time.h>
#include "np/util/valgrind.h"
#include "np/util/trace.h"
#include "np/util/log.hxx"

using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
be_valground(void)
{
#if HAVE_VALGRIND
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
	iprintf("disabling Valgrind under debugger\n");
	return;
    }

    if (!np::spiegel::platform::get_argv(&argc, &argv))
	return;

    iprintf("starting valgrind\n");

    p = newargv = (const char **)np::util::xmalloc(sizeof(char *) * (argc+8));
    *p++ = VALGRIND_BINARY;
    *p++ = "-q";
    *p++ = "--tool=memcheck";
    *p++ = "--sym-offsets=yes";
#ifdef HAVE_VALGRIND_SHOW_LEAK_KINDS
    *p++ = "--show-leak-kinds=definite,indirect";
#endif
#ifdef _NP_VALGRIND_SUPPRESSION_FILE
    *p++ = "--gen-suppressions=all";
    *p++ = "--suppressions=" _NP_VALGRIND_SUPPRESSION_FILE;
#endif
    while (*argv)
	*p++ = *argv++;

    execv(newargv[0], (char * const *)newargv);
    eprintf("Failed to execv(\"%s\"): %s\n", newargv[0], strerror(errno));
    exit(1);
#endif
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/**
 * Initialise the NovaProva library.
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
 *
 * The function also sets a C++ terminate handler using
 * @c std::set_terminate() which handles any uncaught C++ exceptions,
 * generates a useful error message, and fails the running test.
 *
 * @return	a new runner object
 *
 * \ingroup main
 */
extern "C" np_runner_t *
np_init(void)
{
    be_valground();
    std::set_terminate(__np_terminate_handler);
    np::util::rel_time();
    np_trace_init();
    np::testmanager_t::instance();
    return new np::runner_t;
}

/**
 * Shut down the NovaProva library.
 *
 * @param runner    The runner object to destroy
 *
 * Destroys the given runner object and shuts down the library.
 *
 * \ingroup main
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
