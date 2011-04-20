#include "common.h"

const char *__u4c_argv0 = "u4c";

static void oom(void) __attribute__((noreturn));

/* Do write() but handle short writes */
static int
do_write(int fd, const char *buf, int len)
{
    int remain = len;

    while (remain > 0)
    {
	int n = write(fd, buf, remain);
	if (n < 0)
	    return -1;
	buf += n;
	remain -= n;
    }
    return len;
}

static void
oom(void)
{
    static const char message[] = ": no memory, exiting\n";

    if (do_write(2, __u4c_argv0, strlen(__u4c_argv0)) < 0)
	exit(1);
    if (do_write(2, message, sizeof(message)-1) < 0)
	exit(1);
    exit(1);
}

void *
__u4c_malloc(size_t sz)
{
    void *x;

    x = malloc(sz);
    if (!x)
	oom();
    memset(x, 0, sz);
    return x;
}

char *
__u4c_strdup(const char *s)
{
    char *x;
    size_t len;

    if (!s)
	return 0;
    len = strlen(s);
    x = malloc(len+1);
    if (!x)
	oom();
    return strcpy(x, s);
}

void *
__u4c_realloc(void *p, size_t sz)
{
    void *x;

    x = realloc(p, sz);
    if (!x)
	oom();
    return x;
}
