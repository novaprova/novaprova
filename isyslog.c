/* isyslog.c - intercept syslog() calls from CUT */
#include "common.h"
#include "except.h"
#include <syslog.h>
#include <unistd.h>

/*
 * Includes code copied from Cyrus IMAPD, which is
 *
 * Copyright (c) 1994-2010 Carnegie Mellon University.  All rights reserved.
 *
 * This product includes software developed by Computing Services
 * at Carnegie Mellon University (http://www.cmu.edu/computing/).
 */

static void vlog(int prio, const char *fmt, va_list args)
{
    /* glibc handles %m in vfprintf() so we don't need to do
     * anything special to simulate that feature of syslog() */
     /* TODO: find and expand %m on non-glibc platforms */

    fprintf(stderr, "\nSYSLOG %d[", prio & LOG_PRIMASK);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "]\n");
    fflush(stderr);
}

#if defined(__GLIBC__)
/* Under some but not all combinations of options, glibc
 * defines syslog() as an inline that calls this function */
void __syslog_chk(int prio,
		  int whatever __attribute__((unused)),
		  const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vlog(prio, fmt, args);
    va_end(args);
}
#endif

void syslog(int prio, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vlog(prio, fmt, args);
    va_end(args);
}

