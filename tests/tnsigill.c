#include <np.h>
#include <unistd.h>
#include <signal.h>

static void test_sigill(void)
{
    kill(getpid(), SIGILL);
}

