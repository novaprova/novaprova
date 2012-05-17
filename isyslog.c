/* isyslog.c - intercept syslog() calls from CUT */
#define SYSLOG_NAMES 1
#include "np_priv.h"
#include "except.h"
#include <syslog.h>
#include <unistd.h>
#include <regex.h>

/*
 * Includes code copied from Cyrus IMAPD, which is
 *
 * Copyright (c) 1994-2010 Carnegie Mellon University.  All rights reserved.
 *
 * This product includes software developed by Computing Services
 * at Carnegie Mellon University (http://www.cmu.edu/computing/).
 */
namespace np {
using namespace std;

enum sldisposition_t
{
    SL_UNKNOWN,
    SL_IGNORE,
    SL_COUNT,
    SL_FAIL,
};

struct slmatch_t
{
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    classifier_t classifier_;
    unsigned int count_;
    int tag_;

    slmatch_t(sldisposition_t dis, int tag)
     :  tag_(tag)
    {
	classifier_.set_results(SL_UNKNOWN, dis);
    }
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static vector<slmatch_t*> slmatches;

static void
add_slmatch(const char *re, sldisposition_t dis,
	    int tag, const char *file, int line)
{
    slmatch_t *slm = new slmatch_t(dis, tag);
    if (!slm->classifier_.set_regexp(re, false))
    {
	const char *msg = slm->classifier_.error_string();
	delete slm;
	np_throw(event_t(EV_SLMATCH, msg).at_line(file, line));
    }

    /* order shouldn't matter due to the way we
     * resolve multiple matches, but let's be
     * careful to preserve caller order anyway by
     * always appending. */
    slmatches.push_back(slm);
}

extern "C" void
__np_syslog_fail(const char *re, const char *file, int line)
{
    add_slmatch(re, SL_FAIL, 0, file, line);
}

extern "C" void
__np_syslog_ignore(const char *re, const char *file, int line)
{
    add_slmatch(re, SL_IGNORE, 0, file, line);
}

extern "C" void
__np_syslog_match(const char *re, int tag, const char *file, int line)
{
    add_slmatch(re, SL_COUNT, tag, file, line);
}

extern "C" unsigned int
__np_syslog_count(int tag, const char *file, int line)
{
    unsigned int count = 0;
    int nmatches = 0;

    vector<slmatch_t*>::iterator i;
    for (i = slmatches.begin() ; i != slmatches.end() ; ++i)
    {
	slmatch_t *slm = *i;

	if (tag < 0 || slm->tag_ == tag)
	{
	    count += slm->count_;
	    nmatches++;
	}
    }

    if (!nmatches)
    {
	static char buf[64];
	snprintf(buf, sizeof(buf), "Unmatched syslog tag %d", tag);
	np_throw(event_t(EV_SLMATCH, buf).at_line(file, line));
    }
    return count;
}

static sldisposition_t
find_slmatch(const char **msgp)
{
    slmatch_t *most = NULL;
    sldisposition_t mostdis = SL_UNKNOWN;

    vector<slmatch_t*>::iterator i;
    for (i = slmatches.begin() ; i != slmatches.end() ; ++i)
    {
	slmatch_t *slm = *i;
	sldisposition_t dis = (sldisposition_t)slm->classifier_.classify(*msgp, 0, 0);
	if (dis != SL_UNKNOWN)
	{
	    /* found */
	    if (!most || dis > mostdis)
		most = slm;
	}
    }

    if (!most)
	return SL_FAIL;
    if (mostdis == SL_COUNT)
	most->count_++;
    return mostdis;
}

/*
 * We don't need a function to reset the syslog matches; thanks
 * to full fork() based isolation we know they're initialised to
 * empty when every test starts.
 */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


static const char *
syslog_priority_name(int prio)
{
    const CODE *c;
    static char buf[32];

    prio &= LOG_PRIMASK;
    for (c = prioritynames ; c->c_name ; c++)
    {
	if (prio == c->c_val)
	    return c->c_name;
    }

    snprintf(buf, sizeof(buf), "%d", prio);
    return buf;
}

static const char *
vlogmsg(int prio, const char *fmt, va_list args)
{
    /* glibc handles %m in vfprintf() so we don't need to do
     * anything special to simulate that feature of syslog() */
     /* TODO: find and expand %m on non-glibc platforms */
    char *p;
    static char buf[1024];

    strncpy(buf, syslog_priority_name(prio), sizeof(buf-3));
    buf[sizeof(buf)-3] = '\0';
    strcat(buf, ": ");
    vsnprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
	      fmt, args);
    for (p = buf+strlen(buf)-1 ; p > buf && isspace(*p) ; p--)
	*p = '\0';

    return buf;
}

#if defined(__GLIBC__)
/* Under some but not all combinations of options, glibc
 * defines syslog() as an inline that calls this function */
extern "C" void
__syslog_chk(int prio,
	     int whatever __attribute__((unused)),
	     const char *fmt, ...);
static void
mock___syslog_chk(int prio,
		  int whatever __attribute__((unused)),
		  const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    event_t ev(EV_SYSLOG, msg);
    ev.with_stack();
    runner_t::running()->raise_event(0, &ev);

    if (find_slmatch(&msg) == SL_FAIL)
	np_throw(event_t(EV_SLMATCH, msg).with_stack());
}
#endif

static void
mock_syslog(int prio, const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    event_t ev(EV_SYSLOG, msg);
    ev.with_stack();
    runner_t::running()->raise_event(0, &ev);

    if (find_slmatch(&msg) == SL_FAIL)
	np_throw(event_t(EV_SLMATCH, msg).with_stack());
}

void init_syslog_intercepts(testnode_t *tn)
{
    tn->add_mock((spiegel::addr_t)&syslog,
		 (spiegel::addr_t)&mock_syslog);
#if defined(__GLIBC__)
    tn->add_mock((spiegel::addr_t)&__syslog_chk,
		 (spiegel::addr_t)&mock___syslog_chk);
#endif
}

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
