/* isyslog.c - intercept syslog() calls from CUT */
#define SYSLOG_NAMES 1
#include "u4c_priv.h"
#include "except.h"
#include <syslog.h>
#include <unistd.h>
#include <valgrind/valgrind.h>

/*
 * Includes code copied from Cyrus IMAPD, which is
 *
 * Copyright (c) 1994-2010 Carnegie Mellon University.  All rights reserved.
 *
 * This product includes software developed by Computing Services
 * at Carnegie Mellon University (http://www.cmu.edu/computing/).
 */

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
void __syslog_chk(int prio,
		  int whatever __attribute__((unused)),
		  const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    VALGRIND_PRINTF_BACKTRACE("syslog %s\n", msg);

    {
	struct u4c_event ev = eventc(EV_SYSLOG, msg);
	__u4c_raise_event(&ev, FT_UNKNOWN);
    }
}
#endif

void syslog(int prio, const char *fmt, ...)
{
    const char *msg;
    va_list args;

    va_start(args, fmt);
    msg = vlogmsg(prio, fmt, args);
    va_end(args);

    VALGRIND_PRINTF_BACKTRACE("syslog %s\n", msg);

    {
	struct u4c_event ev = eventc(EV_SYSLOG, msg);
	__u4c_raise_event(&ev, FT_UNKNOWN);
    }
}

