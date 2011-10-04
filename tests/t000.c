#include <u4c.h>
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

int
main(int argc, char **argv)
{
    int ec = 0;
    u4c_globalstate_t *state = u4c_init();
    ec = u4c_run_tests(state);
    u4c_done(state);
    exit(ec);
}
