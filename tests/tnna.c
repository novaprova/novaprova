#include <np.h>
#include <syslog.h>

static void test_notapplicable(void)
{
    NP_NOTAPPLICABLE;
    syslog(LOG_ERR, "Should never be seen\n");
}

