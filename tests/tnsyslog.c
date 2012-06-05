#include <np.h>
#include <syslog.h>

static void test_syslog(void)
{
    syslog(LOG_ERR, "Hello world!\n");
}

