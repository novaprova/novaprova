/* isyslog.c - intercept syslog() calls from CUT */
#define SYSLOG_NAMES 1
#include "u4c_priv.h"
#include "except.h"
#include <syslog.h>
#include <unistd.h>
#include <regex.h>
#include <valgrind/valgrind.h>

/*
 * Includes code copied from Cyrus IMAPD, which is
 *
 * Copyright (c) 1994-2010 Carnegie Mellon University.  All rights reserved.
 *
 * This product includes software developed by Computing Services
 * at Carnegie Mellon University (http://www.cmu.edu/computing/).
 */
struct u4c_slmatch_t;

enum u4c_sldisposition_t
{
    SL_IGNORE,
    SL_COUNT,
    SL_FAIL,
};

struct u4c_slmatch_t
{
    u4c_slmatch_t *next;
    char *re;
    unsigned int count;
    int tag;
    u4c_sldisposition_t disposition;
    regex_t compiled_re;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static u4c_slmatch_t *slmatches;

static const char *
slmatch_error(const u4c_slmatch_t *slm, int r)
{
    static char buf[2048];
    int n;

    snprintf(buf, sizeof(buf)-100, "/%s/: ", slm->re);
    n = strlen(buf);
    regerror(r, &slm->compiled_re, buf+n, sizeof(buf)-n-1);

    return buf;
}

static void
add_slmatch(const char *re, u4c_sldisposition_t dis,
	    int tag, const char *file, int line)
{
    u4c_slmatch_t *slm, **prevp;
    int r;

    slm = (u4c_slmatch_t *)xmalloc(sizeof(*slm));
    slm->re = xstrdup(re);
    slm->disposition = dis;
    slm->tag = tag;

    r = regcomp(&slm->compiled_re, slm->re,
		REG_EXTENDED|REG_ICASE|REG_NOSUB);
    if (r)
    {
	const char *msg = slmatch_error(slm, r);
	free(slm->re);
	free(slm);
	u4c_throw(event(EV_SLMATCH, msg, file, line, 0));
    }

    /* order shouldn't matter due to the way we
     * resolve multiple matches, but let's be
     * careful to preserve caller order anyway by
     * always appending. */
    for (prevp = &slmatches ;
	 *prevp ;
	 prevp = &((*prevp)->next))
	;
    slm->next = NULL;
    *prevp = slm;
}

void
__u4c_syslog_fail(const char *re, const char *file, int line)
{
    add_slmatch(re, SL_FAIL, 0, file, line);
}

void
__u4c_syslog_ignore(const char *re, const char *file, int line)
{
    add_slmatch(re, SL_IGNORE, 0, file, line);
}

void
__u4c_syslog_match(const char *re, int tag, const char *file, int line)
{
    add_slmatch(re, SL_COUNT, tag, file, line);
}

unsigned int
__u4c_syslog_count(int tag, const char *file, int line)
{
    unsigned int count = 0;
    int nmatches = 0;
    u4c_slmatch_t *slm;

    for (slm = slmatches ; slm ; slm = slm->next)
    {
	if (tag < 0 || slm->tag == tag)
	{
	    count += slm->count;
	    nmatches++;
	}
    }

    if (!nmatches)
    {
	static char buf[64];
	snprintf(buf, sizeof(buf), "Unmatched syslog tag %d", tag);
	u4c_throw(event(EV_SLMATCH, buf, file, line, 0));
    }
    return count;
}

static u4c_sldisposition_t
find_slmatch(const char **msgp)
{
    int r;
    u4c_slmatch_t *slm;
    u4c_slmatch_t *most = NULL;

    for (slm = slmatches ; slm ; slm = slm->next)
    {
	r = regexec(&slm->compiled_re, *msgp, 0, NULL, 0);
	if (!r)
	{
	    /* found */
	    if (!most || slm->disposition > most->disposition)
		most = slm;
	}
	else if (r != REG_NOMATCH)
	{
	    /* runtime regex error */
	    *msgp = slmatch_error(slm, r);
	    return SL_FAIL;
	}
    }

    if (!most)
	return SL_FAIL;
    if (most->disposition == SL_COUNT)
	most->count++;
    return most->disposition;
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
	     const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    VALGRIND_PRINTF_BACKTRACE("syslog %s\n", msg);

    u4c_event_t ev(EV_SYSLOG, msg);
    u4c::runner_t::running()->raise_event(&ev, u4c::FT_UNKNOWN);

    if (find_slmatch(&msg) == SL_FAIL)
	u4c_throw(eventc(EV_SLMATCH, msg));
}
#endif

extern "C" void
syslog(int prio, const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    VALGRIND_PRINTF_BACKTRACE("syslog %s\n", msg);

    u4c_event_t ev(EV_SYSLOG, msg);
    u4c::runner_t::running()->raise_event(&ev, u4c::FT_UNKNOWN);

    if (find_slmatch(&msg) == SL_FAIL)
	u4c_throw(eventc(EV_SLMATCH, msg));
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
