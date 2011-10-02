#include "common.h"
#include "except.h"
#include "u4c_priv.h"

typedef struct u4c_proxy_listener u4c_proxy_listener_t;
struct u4c_proxy_listener
{
    u4c_listener_t super;
    int fd;
};

enum u4c_proxy_call
{
    PROXY_EVENT = 1,
    PROXY_FINISHED = 2,
};

#define PASS 1
#define FAIL 0

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
serialise_uint(int fd, unsigned int i)
{
    write(fd, &i, sizeof(i));
}

static void
serialise_string(int fd, const char *s)
{
    unsigned int len = (s ? strlen(s) : 0);
    serialise_uint(fd, len);
    if (len)
    {
	write(fd, s, len);
	write(fd, "\0", 1);
    }
}

static void
serialise_event(int fd, const u4c_event_t *ev)
{
    serialise_uint(fd, ev->which);
    serialise_string(fd, ev->description);
    serialise_string(fd, ev->filename);
    serialise_uint(fd, ev->lineno);
    serialise_string(fd, ev->function);
}

static int
deserialise_bytes(int fd, char *p, unsigned int len)
{
    int r;

    while (len)
    {
	r = read(fd, p, len);
	if (r < 0)
	    return -errno;
	if (r == 0)
	    return EOF;
	len -= r;
	p += r;
    }
    return 0;
}

static int
deserialise_uint(int fd, unsigned int *ip)
{
    return deserialise_bytes(fd, (char *)ip, sizeof(*ip));
}

static int
deserialise_string(int fd, char *buf, unsigned int maxlen)
{
    unsigned int len;
    int r;
    if ((r = deserialise_uint(fd, &len)))
	return r;
    if (len >= maxlen)
	return -ENAMETOOLONG;
    if (!len)
	return 0;
    return deserialise_bytes(fd, buf, len+1);
}

static bool
deserialise_event(int fd, u4c_event_t *ev)
{
    int r;
    unsigned int which;
    unsigned int lineno;
    static char description_buf[1024];
    static char filename_buf[1024];
    static char function_buf[1024];

    if ((r = deserialise_uint(fd, &which)))
	return r;
    if ((r = deserialise_string(fd, description_buf,
				sizeof(description_buf))))
	return r;
    if ((r = deserialise_string(fd, filename_buf,
				sizeof(filename_buf))))
	return r;
    if ((r = deserialise_uint(fd, &lineno)))
	return r;
    if ((r = deserialise_string(fd, function_buf,
				sizeof(function_buf))))
	return r;
    ev->which = which;
    ev->description = description_buf;
    ev->lineno = lineno;
    ev->filename = filename_buf;
    ev->function = function_buf;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static void
proxy_add_event(u4c_listener_t *l, const u4c_event_t *ev, enum u4c_functype ft)
{
    u4c_proxy_listener_t *tl = container_of(l, u4c_proxy_listener_t, super);
    serialise_uint(tl->fd, PROXY_EVENT);
    serialise_event(tl->fd, ev);
    serialise_uint(tl->fd, ft);
}

static void
proxy_fail(u4c_listener_t *l)
{
    u4c_proxy_listener_t *tl = container_of(l, u4c_proxy_listener_t, super);
    serialise_uint(tl->fd, PROXY_FINISHED);
    serialise_uint(tl->fd, FAIL);
}

static void
proxy_pass(u4c_listener_t *l __attribute__((unused)))
{
    u4c_proxy_listener_t *tl = container_of(l, u4c_proxy_listener_t, super);
    serialise_uint(tl->fd, PROXY_FINISHED);
    serialise_uint(tl->fd, PASS);
}

static u4c_listener_ops_t proxy_ops =
{
    .add_event = proxy_add_event,
    .fail = proxy_fail,
    .pass = proxy_pass
};

u4c_listener_t *
__u4c_proxy_listener(int fd)
{
    static u4c_proxy_listener_t l = {
	    .super = { .next = NULL, .ops = &proxy_ops }};
    l.fd = fd;
    return &l.super;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Handles input on the read end of the event pipe.
 * Returns:
 *  0 - not finished
 *  1 - finished, passed
 *  2 - finished, failed
 *  <0	error decoding or EOF
 */
int
__u4c_handle_proxy_call(int fd)
{
    unsigned int which;
    u4c_event_t ev;
    unsigned int ft;
    unsigned int res;
    int r;

    r = deserialise_uint(fd, &which);
    if (r)
    {
	if (r != EOF)
	    fprintf(stderr, "u4c: can't read proxy call\n");
	return r;
    }
    switch (which)
    {
    case PROXY_EVENT:
	if ((r = deserialise_event(fd, &ev)) ||
	    (r = deserialise_uint(fd, &ft)))
	    return r;    /* failed to decode */
	__u4c_raise_event(&ev, ft);
	return 0;
    case PROXY_FINISHED:
	if ((r = deserialise_uint(fd, &res)))
	    return r;    /* failed to decode */
	return (res == PASS ? 1 : 2);
    default:
	fprintf(stderr,
		"u4c: can't decode proxy call (which=%u)\n",
		which);
	return -EINVAL;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
