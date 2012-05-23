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

#include "np/util/common.hxx"

// These are macros so we can #if on them
#define SPIEGEL_ADDRSIZE    4
#define SPIEGEL_MAXADDR	    (0xffffffffUL)
// #define SPIEGEL_ADDRSIZE    8
// #define SPIEGEL_MAXADDR	    (0xffffffffffffffffULL)

namespace np {
/**
 * NovaProva reflection library namespace.
 *
 * The `spiegel` namespace is a set of utilities which implement true runtime
 * reflection for C and C++ programs, using the DWARF debug information
 * standard.
 *
 * For now the spiegel C++ interface is undocumented.
 */
namespace spiegel {

#if SPIEGEL_ADDRSIZE == 4
typedef uint32_t addr_t;
#elif SPIEGEL_ADDRSIZE == 8
typedef uint64_t addr_t;
#else
#error "Unknown address size"
#endif

// close the namespaces
}; };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* __spiegel_common_hxx__ */
