#include <np.h>
#include <syslog.h>

static void test_fail(void)
{
    NP_FAIL;
    syslog(LOG_ERR, "Should never be seen\n");
}


