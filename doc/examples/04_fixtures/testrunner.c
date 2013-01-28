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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <np.h>	    /* NovaProva library */

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [test-spec...]\n", argv0);
    exit(1);
}

int
main(int argc, char **argv)
{
    int ec = 0;
    np_plan_t *plan = 0;
    const char *output_format = 0;
    int concurrency = -1;
    np_runner_t *runner;
    int c;

    while ((c = getopt(argc, argv, "f:j:")) >= 0)
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
	default:
	    usage(argv[0]);
	}
    }
    if (optind < argc)
    {
	plan = np_plan_new();
	np_plan_add_specs(plan, argc-optind, (const char **)argv+optind);
    }

    /* Initialise the NovaProva library */
    runner = np_init();

    /* Set the output format */
    if (output_format)
	np_set_output_format(runner, output_format);

    /* Set how many tests will be run in parallel */
    if (concurrency >= 0)
	np_set_concurrency(runner, concurrency);

    /* Run all the discovered tests */
    ec = np_run_tests(runner, plan);

    /* Shut down the NovaProva library */
    if (plan) np_plan_delete(plan);
    np_done(runner);

    exit(ec);
}

