#include "u4c/common.hxx"

namespace u4c {
using namespace std;

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

string hex(unsigned long x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%lx", x);
    return string(buf);
}

string dec(unsigned int x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", x);
    return string(buf);
}

#define NANOSEC_PER_SEC	    1000000000
static int64_t posix_now(int clock)
{
    struct timespec ts;
    memset(&ts, 0, sizeof(ts));
    clock_gettime(clock, &ts);
    return ts.tv_sec * NANOSEC_PER_SEC + ts.tv_nsec;
}

int64_t abs_now()
{
    return posix_now(CLOCK_REALTIME);
}

int64_t rel_now()
{
    return posix_now(CLOCK_MONOTONIC);
}

string abs_format_iso8601(int64_t abs)
{
    time_t clock = abs / NANOSEC_PER_SEC;
    struct tm tm;
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S",
	     localtime_r(&clock, &tm));
    return string(buf);
}

string rel_format(int64_t rel)
{
    const char *sign = "";
    if (rel < 0)
    {
	rel = -rel;
	sign = "-";
    }
    int sec = rel / NANOSEC_PER_SEC;
    int ns = rel % NANOSEC_PER_SEC;
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%u.%03u", sign, sec, ns);
    return string(buf);
}

string rel_timestamp()
{
    static int64_t first;
    int64_t now = rel_now();
    if (!first)
	first = now;
    return rel_format(now - first);
}

// close the namespace
};
