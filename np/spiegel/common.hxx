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

#include "np/util/common.hxx"

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

#if _NP_ADDRSIZE == 4
typedef uint32_t addr_t;
#elif _NP_ADDRSIZE == 8
typedef uint64_t addr_t;
#else
#error "Unknown address size"
#endif

// close the namespaces
}; };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* __spiegel_common_hxx__ */
