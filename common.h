#ifndef __U4C_COMMON_H__
#define __U4C_COMMON_H__ 1

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <memory.h>
#include <malloc.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/poll.h>
#include <valgrind/memcheck.h>

extern void *__u4c_malloc(size_t sz);
extern void *__u4c_realloc(void *, size_t);
extern char *__u4c_strdup(const char *s);
extern const char *__u4c_argv0;

#define xmalloc(sz) __u4c_malloc(sz)
#define xrealloc(p, sz) __u4c_realloc(p, sz)
#define xstrdup(s) __u4c_strdup(s)
#define xfree(v) \
    do { free(v); (v) = NULL; } while(0)
#define xstr(x)  ((x) ? (x) : "")

/* copied from <linux/kernel.h> */
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


#endif /* __U4C_COMMON_H__ */
