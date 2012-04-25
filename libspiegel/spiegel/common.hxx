/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2005 Greg Banks <gnb@users.sourceforge.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __spiegel_common_hxx__
#define __spiegel_common_hxx__ 1

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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif

#include <string>
#include <map>
#include <vector>
#include <exception>

// Provide an empty definition of __attribute__ so
// we can just use it even on non-gcc compilers
#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

// These are macros so we can #if on them
#define SPIEGEL_ADDRSIZE    4
#define SPIEGEL_MAXADDR	    (0xffffffffUL)
// #define SPIEGEL_ADDRSIZE    8
// #define SPIEGEL_MAXADDR	    (0xffffffffffffffffULL)

namespace spiegel {

#if SPIEGEL_ADDRSIZE == 4
typedef uint32_t addr_t;
#elif SPIEGEL_ADDRSIZE == 8
typedef uint64_t addr_t;
#else
#error "Unknown address size"
#endif

extern int safe_strcmp(const char *a, const char *b);

extern int u32cmp(uint32_t ul1, uint32_t ul2);
extern int u64cmp(uint64_t ull1, uint64_t ull2);

extern const char *argv0;
extern void fatal(const char *fmt, ...)
    __attribute__ (( noreturn ))
    __attribute__ (( format(printf,1,2) ));

extern void *xmalloc(size_t sz);
extern char *xstrdup(const char *s);

extern unsigned long page_size(void);
extern unsigned long page_round_up(unsigned long x);
extern unsigned long page_round_down(unsigned long x);

extern std::string hex(unsigned long x);
extern std::string dec(unsigned int x);

// close the namespace
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* __spiegel_common_hxx__ */
