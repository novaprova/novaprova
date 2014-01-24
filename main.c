/*
 * Copyright 2011-2013 Gregory Banks
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

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [-f output-format] [test-spec...]\n", argv0);
    exit(1);
}

int
main(int argc, char **argv)
{
    int ec = 0;
    np_plan_t *plan = 0;
    np_runner_t *runner = 0;
    const char *output_format = 0;
    enum { UNKNOWN, RUN, LIST } mode = UNKNOWN;
    int concurrency = -1;
    int c;
    static const struct option opts[] =
    {
	{ "format", required_argument, NULL, 'f' },
	{ "jobs", required_argument, NULL, 'j' },
	{ "list", no_argument, NULL, 'l' },
	{ NULL, 0, NULL, 0 },
    };

    /* Parse arguments */
    while ((c = getopt_long(argc, argv, "f:j:l", opts, NULL)) >= 0)
    {
	switch (c)
	{
	case 'f':
	    output_format = optarg;
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
	default:
	    usage(argv[0]);
	}
    }
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
	if (output_format)
	{
	    if (!np_set_output_format(runner, output_format))
	    {
		fprintf(stderr, "np: unknown output format '%s'\n", output_format);
		exit(1);
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
