#include <np.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

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
    const char *output_format = 0;
    int concurrency = -1;
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
    np_runner_t *runner = np_init();
    if (output_format)
	np_set_output_format(runner, output_format);
    if (concurrency >= 0)
	np_set_concurrency(runner, concurrency);
    ec = np_run_tests(runner, plan);
    if (plan) np_plan_delete(plan);
    np_done(runner);
    exit(ec);
}
