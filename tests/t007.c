#include <np.h>
#include <assert.h>
#include <syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

static void test_segv(void)
{
    *(char *)0 = 0;
}

static void test_sigill(void)
{
    kill(getpid(), SIGILL);
}

static void test_assert(void)
{
    int white = 1;
    int black = 0;

    assert(white == black);
}

static void test_exit(void)
{
    exit(1);
}

static void test_syslog(void)
{
    syslog(LOG_ERR, "Hello world!\n");
}

static void test_memleak(void)
{
    malloc(32);
}

static void test_pass(void)
{
    U4C_PASS;
    syslog(LOG_ERR, "Should never be seen\n");
}

static void test_fail(void)
{
    U4C_FAIL;
    syslog(LOG_ERR, "Should never be seen\n");
}

static void test_notapplicable(void)
{
    U4C_NOTAPPLICABLE;
    syslog(LOG_ERR, "Should never be seen\n");
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
    np_set_output_format(runner, "junit");
    ec = np_run_tests(runner, plan);
    if (plan) np_plan_delete(plan);
    np_done(runner);
    exit(ec);
}
