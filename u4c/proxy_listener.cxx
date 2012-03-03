#include "u4c/proxy_listener.hxx"
#include "except.h"
#include "u4c_priv.h"

namespace u4c {

enum proxy_call
{
    PROXY_EVENT = 1,
    PROXY_FINISHED = 2,
};

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
	if (r < 0) {
	    perror("u4c: error reading from proxy");
	    return -errno;
	}
	if (r == 0) {
	    fprintf(stderr, "u4c: unexpected EOF deserialising from proxy\n");
	    return -EINVAL;
	}
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
    ev->which = (enum u4c_events)which;
    ev->description = description_buf;
    ev->lineno = lineno;
    ev->filename = filename_buf;
    ev->function = function_buf;
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

proxy_listener_t::proxy_listener_t(int fd)
 :  fd_(fd)
{
}

proxy_listener_t::~proxy_listener_t()
{
}

void
proxy_listener_t::begin()
{
}

void
proxy_listener_t::end()
{
}

void
proxy_listener_t::begin_node(const testnode_t *)
{
}

void
proxy_listener_t::end_node(const testnode_t *)
{
}

void
proxy_listener_t::add_event(const u4c_event_t *ev, functype_t ft)
{
    serialise_uint(fd_, PROXY_EVENT);
    serialise_event(fd_, ev);
    serialise_uint(fd_, ft);
}

void
proxy_listener_t::finished(result_t res)
{
    serialise_uint(fd_, PROXY_FINISHED);
    serialise_uint(fd_, res);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Handles input on the read end of the event pipe.  Returns false
 * when we should stop calling it, which might be due to a normal
 * end of test condition (FINISHED proxy call) or to some error.
 * Updates *@resp if necessary.
 */
bool
proxy_listener_t::handle_call(int fd, result_t *resp)
{
    unsigned int which;
    u4c_event_t ev;
    unsigned int ft;
    unsigned int res;
    int r;

    r = deserialise_uint(fd, &which);
    if (r)
	return false;
    switch (which)
    {
    case PROXY_EVENT:
	if ((r = deserialise_event(fd, &ev)) ||
	    (r = deserialise_uint(fd, &ft)))
	    return false;    /* failed to decode */
	__u4c_merge(*resp,
	u4c_globalstate_t::running()->raise_event(&ev, (functype_t)ft));
	return true;	    /* call me again */
    case PROXY_FINISHED:
	if ((r = deserialise_uint(fd, &res)))
	    return false;    /* failed to decode */
	__u4c_merge(*resp, res);
	return false;	      /* end of test, expect no more calls */
    default:
	fprintf(stderr,
		"u4c: can't decode proxy call (which=%u)\n",
		which);
	return false;
    }
}

// close the namespace
};
