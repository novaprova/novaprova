/*
 * Copyright 2011-2021 Gregory Banks
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
#include <np.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include "np/util/profile.hxx"
#include "np/util/tok.hxx"
#include "np/util/log.hxx"

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [--debug] [-f output-format] [test-spec...]\n", argv0);
    fprintf(stderr, "       %s [--debug] --list [test-spec...]\n", argv0);
    exit(1);
}

// Codes returned from getopt_long() for options which have *only* long
// forms.  These are integers outside of the 7bit ASCII range so they do
// not clash with the characters used for short options.
enum opt_codes_t
{
    OPT_HELP=256,
    OPT_DEBUG
};

int
main(int argc, char **argv)
{
    int ec = 0;
    np_plan_t *plan = 0;
    np_runner_t *runner = 0;
    const char *output_formats = 0;
    enum { UNKNOWN, RUN, LIST } mode = UNKNOWN;
    int concurrency = -1;
    bool debug = false;
    int c;
    static const struct option opts[] =
    {
	{ "format", required_argument, NULL, 'f' },
	{ "jobs", required_argument, NULL, 'j' },
	{ "list", no_argument, NULL, 'l' },
	{ "debug", no_argument, NULL, OPT_DEBUG },
	{ "help", no_argument, NULL, OPT_HELP },
	{ NULL, 0, NULL, 0 },
    };

    /* Parse arguments */
    while ((c = getopt_long(argc, argv, "f:j:l", opts, NULL)) >= 0)
    {
	switch (c)
	{
	case 'f':
	    output_formats = optarg;
	    break;
	case 'j':
	    if (!strcasecmp(optarg, "max"))
		concurrency = 0;
	    else if ((concurrency = atoi(optarg)) <= 0)
		usage(argv[0]);
	    break;
	case 'l':
	    mode = LIST;
	    break;
        case OPT_DEBUG:
            debug = true;
            break;
        case OPT_HELP:
        default:
            // note, for unknown options getopt_long() has already
            // printed a message about the option being unknown..
	    usage(argv[0]);
	}
    }
    if (!debug)
    {
        const char *env = getenv("NOVAPROVA_DEBUG");
        if (env && !strcmp(env, "yes"))
            debug = true;
    }
    np::log::basic_config(debug ? np::log::DEBUG : np::log::INFO, 0);

    if (optind < argc)
    {
	/* Some tests were specified on the commandline */
	plan = np_plan_new();
	np_plan_add_specs(plan, argc-optind, (const char **)argv+optind);
    }

    /* Initialise the NovaProva library */
    runner = np_init();

    switch (mode)
    {
    case LIST:	    /* List the specified (or all the discovered) tests */
	np_list_tests(runner, plan);
	break;

    case UNKNOWN:
    case RUN:	    /* Run the specified (or all the discovered) tests */
	/* Set the output format */
	if (output_formats)
	{
		const char *format;
		np::util::tok_t tok(output_formats, ",");
		while ((format = tok.next()))
		{
			if (!np_set_output_format(runner, format))
			{
				fprintf(stderr, "np: unknown output format '%s'\n", output_formats);
				exit(1);
			}
		}
	}

	/* Set how many tests will be run in parallel */
	if (concurrency >= 0)
	    np_set_concurrency(runner, concurrency);

	/* Run the specified tests */
	ec = np_run_tests(runner, plan);
	break;
    }

    /* Shut down the NovaProva library */
    if (plan) np_plan_delete(plan);
    np_done(runner);

    exit(ec);
}
