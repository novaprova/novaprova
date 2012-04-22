#ifndef __U4C_EXCEPT_H__
#define __U4C_EXCEPT_H__ 1

#include "u4c/common.hxx"
#include "u4c/event.hxx"
#include "spiegel/spiegel.hxx"
#include <setjmp.h>

struct __u4c_exceptstate_t
{
    jmp_buf jbuf;
    bool catching;
    int caught;
    u4c::event_t event;
};

/* in run.c */
extern __u4c_exceptstate_t __u4c_exceptstate;

#define u4c_try \
	__u4c_exceptstate.catching = true; \
	if (!(__u4c_exceptstate.caught = setjmp(__u4c_exceptstate.jbuf)))
#define u4c_catch(x) \
	__u4c_exceptstate.catching = false; \
	(x) = __u4c_exceptstate.caught ? &__u4c_exceptstate.event : 0; \
	if (__u4c_exceptstate.caught)

#define u4c_throw(ev) \
	do { \
	    if (__u4c_exceptstate.catching) \
	    { \
		__u4c_exceptstate.event = (ev); \
		longjmp(__u4c_exceptstate.jbuf, 1); \
	    } \
	    abort(); \
	} while(0)

#endif /* __U4C_EXCEPT_H__ */
