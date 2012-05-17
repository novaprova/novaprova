#include <np.h>
#include <stdio.h>
#include <stdlib.h>

int bird_tequila(int x)
{
    fprintf(stderr, "bird_tequila(%d)\n", x);
    return x/2;
}

static void test_mocking(void)
{
    int x;
    fprintf(stderr, "before\n");
    x = bird_tequila(42);
    fprintf(stderr, "after, returned %d\n", x);
}

int mock_bird_tequila(int x)
{
    fprintf(stderr, "mocked bird_tequila(%d)\n", x);
    return x*2;
}

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
