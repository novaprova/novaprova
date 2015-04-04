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
#ifndef __spiegel_common_hxx__
#define __spiegel_common_hxx__ 1

#include "np/util/config.h"
#include "np/util/common.hxx"

namespace np {
/*
 * NovaProva reflection library namespace.
 *
 * The `spiegel` namespace is a set of utilities which implement true runtime
 * reflection for C and C++ programs, using the DWARF debug information
 * standard.
 *
 * For now the spiegel C++ interface is undocumented.
 */
namespace spiegel {

/*
 * Offsets into an ELF file can be 32-bit or 64-bit depending on the file
 * format, a concept introduced in the DWARF-3 standard.  The file format has
 * absolutely nothing to do with the address size and is determined by a
 * special encoding of the compile unit header.  This means that a 64-bit
 * format DWARF section on a 32bit address size is legal DWARF, albeit pretty
 * damn silly.  Note that as NovaProva mmap()s the executable to read the DWARF
 * information, a compile unit actually large enough to need the 64-bit format
 * cannot be mmap()ed and thus cannot be read on a 32-bit architecture.  So on
 * a 32-bit architecture we define np::spiegel::offset_t as a 32-bit quantity
 * and reject the 64-bit DWARF format as soon as we see it; on a 64-bit
 * architecture we handle both file formats at runtime.
 */
#if _NP_ADDRSIZE == 4
typedef uint32_t addr_t;		/* holds a machine address */
typedef uint32_t offset_t;		/* holds an ELF file offset */
#elif _NP_ADDRSIZE == 8
typedef uint64_t addr_t;		/* holds a machine address */
typedef uint64_t offset_t;		/* holds an ELF file offset */
#else
#error "Unknown address size"
#endif

// close the namespaces
}; };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* __spiegel_common_hxx__ */
