#ifndef __U4C_EXCEPT_H__
#define __U4C_EXCEPT_H__ 1

#include "common.h"
#include <setjmp.h>

struct __u4c_exceptstate_t;

enum u4c_events
{
    EV_ASSERT = 1,	/* CuT failed an assert() */
    EV_EXIT,		/* CuT called exit() */
    EV_SIGNAL,		/* CuT caused a fatal signal */
    EV_SYSLOG,		/* CuT did a syslog() */
    EV_FIXTURE,		/* fixture code returned an error */
    EV_EXPASS,		/* CuT explicitly called U4C_PASS */
    EV_EXFAIL,		/* ... */
    EV_EXNA,		/* ... */
    EV_VALGRIND,	/* Valgrind spotted a memleak or error */
    EV_SLMATCH,		/* syslog matching */
};

struct u4c_event_t
{
    enum u4c_events which;
    const char *description;
    const char *filename;
    unsigned int lineno;
    const char *function;

    u4c_event_t() {}
    u4c_event_t(enum u4c_events w,
		const char *d,
		const char *f,
		unsigned int l,
		const char *fn)
     :  which(w),
        description(d),
        filename(f),
        lineno(l),
        function(fn)
    {}
    u4c_event_t(enum u4c_events w,
		const char *d)
     :  which(w),
        description(d),
        filename((const char *)__builtin_return_address(0)),
        lineno(~0U),
        function((const char *)__builtin_frame_address(0))
    {}
};

struct __u4c_exceptstate_t
{
    jmp_buf jbuf;
    bool catching;
    int caught;
    u4c_event_t event;
};

/* in run.c */
extern __u4c_exceptstate_t __u4c_exceptstate;

#define event(w, d, f, l, fn) \
	u4c_event_t(w, d, f, l, fn)
#define eventc(w, d) \
	u4c_event_t(w, d)

#define u4c_try \
	__u4c_exceptstate.catching = true; \
	if (!(__u4c_exceptstate.caught = setjmp(__u4c_exceptstate.jbuf)))
#define u4c_catch(x) \
	__u4c_exceptstate.catching = false; \
	(x) = __u4c_exceptstate.caught ? &__u4c_exceptstate.event : 0; \
	if (__u4c_exceptstate.caught)

#define u4c_throw(...) \
	do { \
	    if (__u4c_exceptstate.catching) \
	    { \
		u4c_event_t _ev(__VA_ARGS__); \
		__u4c_exceptstate.event = _ev; \
		longjmp(__u4c_exceptstate.jbuf, 1); \
	    } \
	    abort(); \
	} while(0)

#endif /* __U4C_EXCEPT_H__ */
