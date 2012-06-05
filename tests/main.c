#include <np.h>
#include <unistd.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
    int ec = 0;
    np_plan_t *plan = 0;
    if (argc > 1)
    {
	plan = np_plan_new();
	np_plan_add_specs(plan, argc-1, (const char **)argv+1);
    }
    np_runner_t *runner = np_init();
    ec = np_run_tests(runner, plan);
    if (plan) np_plan_delete(plan);
    np_done(runner);
    exit(ec);
}
