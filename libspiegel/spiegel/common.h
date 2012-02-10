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

#ifndef _common_h_
#define _common_h_ 1

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

// #include <glib.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define listdelete(v,type,dtor) \
    do { \
	while ((v) != 0) \
	{ \
    	    dtor((type *)(v)->data); \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define listclear(v) \
    do { \
	while ((v) != 0) \
	{ \
    	    (v) = g_list_remove_link((v), (v)); \
	} \
    } while(0)

#define boolassign(bv, s) \
    (bv) = strbool((s), (bv))

#define boolstr(b)  	((b) ? "true" : "false")

/* boolean, true, and false need to be defined always, and
 * unsigned so that struct fields of type boolean:1 do not
 * generate whiny gcc warnings */
#ifdef boolean
#undef boolean
#endif
#define boolean	    unsigned int

#ifdef false
#undef false
#endif
#define false	    (0U)

#ifdef false
#undef false
#endif
#define false	    (0U)

#define safestr(s)  ((s) == 0 ? "" : (s))
static inline int safe_strcmp(const char *a, const char *b)
{
    return strcmp(safestr(a), safestr(b));
}

int u32cmp(uint32_t ul1, uint32_t ul2);
int u64cmp(uint64_t ull1, uint64_t ull2);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#ifdef __cplusplus
extern "C" {
#endif

extern const char *argv0;
extern void fatal(const char *fmt, ...)
#ifdef __GNUC__
__attribute__ (( noreturn ))
__attribute__ (( format(printf,1,2) ))
#endif
;

extern void *xmalloc(size_t sz);
extern char *xstrdup(const char *s);
#ifdef __cplusplus
};
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern unsigned long page_size(void);
extern unsigned long page_round_up(unsigned long x);
extern unsigned long page_round_down(unsigned long x);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#ifdef _
#undef _
#endif
#define _(s)	s

#ifdef N_
#undef N_
#endif
#define N_(s)	s

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/


#endif /* _common_h_ */
