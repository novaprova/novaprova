#include <u4c.h>
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
    u4c_plan_t *plan = 0;
    if (argc > 1)
    {
	plan = u4c_plan_new();
	u4c_plan_add_specs(plan, argc-1, (const char **)argv+1);
    }
    u4c_runner_t *runner = u4c_init();
    ec = u4c_run_tests(runner, plan);
    if (plan) u4c_plan_delete(plan);
    u4c_done(runner);
    exit(ec);
}
