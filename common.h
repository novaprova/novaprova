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

extern void *__u4c_malloc(size_t sz);
extern char *__u4c_strdup(const char *s);
extern const char *__u4c_argv0;

#define xmalloc(sz) __u4c_malloc(sz)
#define xstrdup(s) __u4c_strdup(s)
#define xfree(v) \
    do { free(v); (v) = NULL; } while(0)
#define xstr(x)  ((x) ? (x) : "")

#endif /* __U4C_COMMON_H__ */
