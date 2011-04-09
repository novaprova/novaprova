#ifndef __U4C_EXCEPT_H__
#define __U4C_EXCEPT_H__ 1

#include "common.h"
#include <setjmp.h>

typedef struct __u4c_exceptstate __u4c_exceptstate_t;
struct __u4c_exceptstate
{
    jmp_buf jbuf;
    bool catching;
    int caught;
};
/* in run.c */
extern __u4c_exceptstate_t __u4c_exceptstate;

#define u4c_try \
	__u4c_exceptstate.catching = true; \
	if (!(__u4c_exceptstate.caught = setjmp(__u4c_exceptstate.jbuf)))
#define u4c_catch \
	__u4c_exceptstate.catching = false; \
	if (__u4c_exceptstate.caught)

#define __u4c_throw \
	if (__u4c_exceptstate.catching) \
	    longjmp(__u4c_exceptstate.jbuf, 1);
#define u4c_throw \
    do { \
	__u4c_throw; \
	abort(); \
    } while(0)


#endif /* __U4C_EXCEPT_H__ */
