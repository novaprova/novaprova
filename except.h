#ifndef __U4C_EXCEPT_H__
#define __U4C_EXCEPT_H__ 1

#include "common.h"
#include <setjmp.h>

typedef struct __u4c_exceptstate __u4c_exceptstate_t;

enum u4c_events
{
    EV_ASSERT = 1,
    EV_EXIT,
    EV_SIGNAL,
    EV_SYSLOG,
    EV_FIXTURE,
    EV_VALGRIND,
};

struct u4c_event
{
    enum u4c_events which;
    const char *description;
    const char *filename;
    unsigned int lineno;
    const char *function;
};

struct __u4c_exceptstate
{
    jmp_buf jbuf;
    bool catching;
    int caught;
    struct u4c_event event;
};

/* in run.c */
extern __u4c_exceptstate_t __u4c_exceptstate;

#define event(w, d, f, l, fn) \
    { .which = (w), \
      .description = (d), \
      .filename = (f), \
      .lineno = (l), \
      .function = (fn) \
    }
#define eventc(w, d) \
    { .which = (w), \
      .description = (d), \
      .filename = __builtin_return_address(0), \
      .lineno = ~0U, \
      .function = __builtin_frame_address(0) \
    }

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
		struct u4c_event _ev = __VA_ARGS__; \
		__u4c_exceptstate.event = _ev; \
		longjmp(__u4c_exceptstate.jbuf, 1); \
	    } \
	    abort(); \
	} while(0)

#endif /* __U4C_EXCEPT_H__ */
