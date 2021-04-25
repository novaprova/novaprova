/*
 * Copyright 2011-2021 Gregory Banks
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
#include "np/util/log.hxx"

namespace np {

enum proxy_call
{
    PROXY_INVALID = 0,
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

static bool
deserialise_bytes(int fd, char *p, unsigned int len)
{
    int r;

    while (len)
    {
	r = read(fd, p, len);
	if (r < 0) {
            eprintf("error reading from proxy: %s\n", strerror(errno));
	    return false;
	}
	if (r == 0) {
            /*
             * Not reporting this as an error, because it seems to
             * happen all the time on Darwin and never on Linux
             * (where we note the premature end of child processes
             * by seeing POLLHUP from poll(2)).
             */
	    dprintf("unexpected EOF deserialising from proxy\n");
	    return false;
	}
	len -= r;
	p += r;
    }
    return true;
}

static bool
deserialise_uint(int fd, unsigned int *ip)
{
    return deserialise_bytes(fd, (char *)ip, sizeof(*ip));
}

static bool
deserialise_string(int fd, char **buf)
{
    unsigned int len;

    if (!(deserialise_uint(fd, &len)))
	return false;
    *buf = (char *) malloc(sizeof(char) * (len + 1));
    if (!len)
    {
        *buf[0] = '\0';
        return true;
    }
    return deserialise_bytes(fd, *buf, len+1);
}

static bool
deserialise_event(int fd, event_t *ev)
{
    unsigned int which;
    unsigned int locflags;
    unsigned int lineno;
    unsigned int ft;
    char * description = NULL;
    char * filename = NULL;
    char * function = NULL;

    if (!(deserialise_uint(fd, &which)))
	return false;
    if (!(deserialise_string(fd, &description)))
	return false;
    if (!(deserialise_uint(fd, &locflags)))
	return false;
    if (!(deserialise_string(fd, &filename)))
	return false;
    if (!(deserialise_uint(fd, &lineno)))
	return false;
    if (!(deserialise_string(fd, &function)))
	return false;
    if (!(deserialise_uint(fd, &ft)))
	return false;
    ev->which = (enum events_t)which;
    ev->description = description;
    ev->locflags = locflags;
    ev->lineno = lineno;
    ev->filename = filename;
    ev->function = function;
    ev->functype = (functype_t)ft;
    return true;
}

static void
deserialise_event_cleanup(event_t *ev)
{
    if (ev->description)
        free((void *)ev->description);
    if (ev->filename)
        free((void *)ev->filename);
    if (ev->function)
        free((void *)ev->function);
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
    unsigned int which = PROXY_INVALID;
    event_t ev;
    unsigned int res;

    dprintf("starting to handle call\n");
    if (deserialise_uint(fd, &which))
    {
        switch (which)
        {
        case PROXY_EVENT:
            dprintf("deserializing EVENT\n");
            if (deserialise_event(fd, &ev))
            {
                *resp = merge(*resp, np::runner_t::running()->raise_event(j, &ev));
                deserialise_event_cleanup(&ev);
                return true;  /* call me again */
            }
            deserialise_event_cleanup(&ev);
            break;
        case PROXY_FINISHED:
            dprintf("deserializing FINISHED\n");
            if (deserialise_uint(fd, &res))
            {
                *resp = merge(*resp, (result_t)res);
                return false;	      /* end of test, expect no more calls */
            }
            break;
        default:
            break;
        }
    }

    /* Decoding failed somehow so fail the test and tell the user */
    *resp = merge(*resp, R_FAIL);
    /*
     * Not reporting this as an error, because it seems to
     * happen all the time on Darwin and never on Linux
     * (where we note the premature end of child processes
     * by seeing POLLHUP from poll(2)).
     */
    dprintf("can't decode proxy call (which=%u)\n", which);
    return false;
}

// close the namespace
};
