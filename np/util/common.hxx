/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __NP_COMMON_HXX__
#define __NP_COMMON_HXX__ 1

/* Include autoconf defines */
// #include <config.h>
#define HAVE_STDINT_H 1

#if HAVE_STDINT_H
/*
 * Glibc <stdint.h> says:
 * The ISO C 9X standard specifies that in C++ implementations these
 * macros [UINT64_MAX et al] should only be defined if explicitly requested.
 */
#define __STDC_LIMIT_MACROS 1
#endif

#include <stdio.h>
#include <limits.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <memory.h>
#include <malloc.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/poll.h>
#if HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif
#include <string>
#include <map>
#include <vector>
#include <exception>

#include "np/util/profile.hxx"

// Provide an empty definition of __attribute__ so
// we can just use it even on non-gcc compilers
#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

/*
 * Main NovaProva library namespace.
 *
 * Most of the NovaProva library appears in the `np` namespace.
 * For now the C++ API is undocumented.
 */
namespace np {
namespace util {

extern int u32cmp(uint32_t ul1, uint32_t ul2);
extern int u64cmp(uint64_t ull1, uint64_t ull2);

extern void fatal(const char *fmt, ...)
    __attribute__ (( noreturn ))
    __attribute__ (( format(printf,1,2) ));

extern void oom(void) __attribute__((noreturn));
extern void *xmalloc(size_t sz);
extern void *xrealloc(void *, size_t);
extern char *xstrdup(const char *s);
#define xfree(v) \
    do { free(v); (v) = NULL; } while(0)
#define xstr(x)  ((x) ? (x) : "")
extern const char *argv0;

class zalloc
{
public:
    void *operator new(size_t sz) { return xmalloc(sz); }
    void operator delete(void *x) { free(x); }
};

extern std::string hex(unsigned long x);
extern std::string HEX(unsigned long x);
extern std::string dec(unsigned int x);

#define NANOSEC_PER_SEC	    (1000000000LL)
extern int64_t rel_now();
extern int64_t abs_now();
extern std::string abs_format_iso8601(int64_t);
extern std::string rel_format(int64_t);
extern int64_t rel_time();
extern const char *rel_timestamp();

extern unsigned long page_size(void);
extern unsigned long page_round_up(unsigned long x);
extern unsigned long page_round_down(unsigned long x);

// close the namespaces
}; };

#endif /* __NP_COMMON_HXX__ */
