#include <np.h>
#include <syslog.h>

static void test_pass(void)
{
    NP_PASS;
    syslog(LOG_ERR, "Should never be seen\n");
}

