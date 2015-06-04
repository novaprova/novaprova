/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "np/proxy_listener.hxx"
#include "except.h"
#include "np_priv.h"

namespace np {

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
serialise_event(int fd, const event_t *ev)
{
    serialise_uint(fd, ev->which);
    serialise_string(fd, ev->description);
    serialise_uint(fd, ev->locflags);
    serialise_string(fd, ev->filename);
    serialise_uint(fd, ev->lineno);
    serialise_string(fd, ev->function);
    serialise_uint(fd, ev->functype);
}

static int
deserialise_bytes(int fd, char *p, unsigned int len)
{
    int r;

    while (len)
    {
	r = read(fd, p, len);
	if (r < 0) {
	    perror("np: error reading from proxy");
	    return -errno;
	}
	if (r == 0) {
	    fprintf(stderr, "np: unexpected EOF deserialising from proxy\n");
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
    {
	buf[0] = '\0';
	return 0;
    }
    return deserialise_bytes(fd, buf, len+1);
}

static bool
deserialise_event(int fd, event_t *ev)
{
    int r;
    unsigned int which;
    unsigned int locflags;
    unsigned int lineno;
    unsigned int ft;
    static char description_buf[1024];
    static char filename_buf[1024];
    static char function_buf[1024];

    if ((r = deserialise_uint(fd, &which)))
	return r;
    if ((r = deserialise_string(fd, description_buf,
				sizeof(description_buf))))
	return r;
    if ((r = deserialise_uint(fd, &locflags)))
	return r;
    if ((r = deserialise_string(fd, filename_buf,
				sizeof(filename_buf))))
	return r;
    if ((r = deserialise_uint(fd, &lineno)))
	return r;
    if ((r = deserialise_string(fd, function_buf,
				sizeof(function_buf))))
	return r;
    if ((r = deserialise_uint(fd, &ft)))
	return r;
    ev->which = (enum events_t)which;
    ev->description = description_buf;
    ev->locflags = locflags;
    ev->lineno = lineno;
    ev->filename = filename_buf;
    ev->function = function_buf;
    ev->functype = (functype_t)ft;
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
proxy_listener_t::begin_job(const job_t *)
{
}

void
proxy_listener_t::end_job(const job_t *j __attribute__((unused)),
			  result_t res)
{
    serialise_uint(fd_, PROXY_FINISHED);
    serialise_uint(fd_, res);
}

void
proxy_listener_t::add_event(const job_t *j __attribute__((unused)),
			    const event_t *ev)
{
    serialise_uint(fd_, PROXY_EVENT);
    serialise_event(fd_, ev);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Handles input on the read end of the event pipe.  Returns false
 * when we should stop calling it, which might be due to a normal
 * end of test condition (FINISHED proxy call) or to some error.
 * Updates *@resp if necessary.
 */
bool
proxy_listener_t::handle_call(int fd, job_t *j, result_t *resp)
{
    unsigned int which;
    event_t ev;
    unsigned int res;
    int r;

#if _NP_DEBUG
    fprintf(stderr, "np: proxy_listener_t::handle_call()\n");
#endif
    r = deserialise_uint(fd, &which);
    if (r)
	return false;
    switch (which)
    {
    case PROXY_EVENT:
#if _NP_DEBUG
	fprintf(stderr, "np: deserializing EVENT\n");
#endif
	if ((r = deserialise_event(fd, &ev)))
	    return false;    /* failed to decode */
	*resp = merge(*resp, np::runner_t::running()->raise_event(j, &ev));
	return true;	    /* call me again */
    case PROXY_FINISHED:
#if _NP_DEBUG
	fprintf(stderr, "np: deserializing FINISHED\n");
#endif
	if ((r = deserialise_uint(fd, &res)))
	    return false;    /* failed to decode */
	*resp = merge(*resp, (result_t)res);
	return false;	      /* end of test, expect no more calls */
    default:
	fprintf(stderr,
		"np: can't decode proxy call (which=%u)\n",
		which);
	return false;
    }
}

// close the namespace
};
