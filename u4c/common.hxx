#ifndef __U4C_COMMON_HXX__
#define __U4C_COMMON_HXX__ 1

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
#include <string>

// Provide an empty definition of __attribute__ so
// we can just use it even on non-gcc compilers
#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

/**
 * Main NovaProva library namespace.
 *
 * Most of the NovaProva library appears in the `u4c` namespace.
 * For now the C++ API is undocumented.
 */
namespace u4c {

extern void *xmalloc(size_t sz);
extern void *xrealloc(void *, size_t);
extern char *xstrdup(const char *s);
#define xfree(v) \
    do { free(v); (v) = NULL; } while(0)
#define xstr(x)  ((x) ? (x) : "")
extern const char *argv0;

extern std::string hex(unsigned long x);
extern std::string dec(unsigned int x);

#define NANOSEC_PER_SEC	    (1000000000LL)
extern int64_t rel_now();
extern int64_t abs_now();
extern std::string abs_format_iso8601(int64_t);
extern std::string rel_format(int64_t);
extern std::string rel_timestamp();

// close the namespace
};

#endif /* __U4C_COMMON_HXX__ */
