#include "u4c/common.hxx"

namespace u4c {

const char *argv0 = "u4c";

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

    if (do_write(2, argv0, strlen(argv0)) < 0)
	exit(1);
    if (do_write(2, message, sizeof(message)-1) < 0)
	exit(1);
    exit(1);
}

void *
xmalloc(size_t sz)
{
    void *x;

    x = malloc(sz);
    if (!x)
	oom();
    memset(x, 0, sz);
    return x;
}

char *
xstrdup(const char *s)
{
    char *x;
    size_t len;

    if (!s)
	return 0;
    len = strlen(s);
    x = (char *)malloc(len+1);
    if (!x)
	oom();
    return strcpy(x, s);
}

void *
xrealloc(void *p, size_t sz)
{
    void *x;

    x = realloc(p, sz);
    if (!x)
	oom();
    return x;
}

// close the namespace
};
