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
#ifndef __np_spiegel_dwarf_reference_hxx__
#define __np_spiegel_dwarf_reference_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct reference_t
{
    uint32_t cu;	// index into compile unit table
    uint32_t offset;	// byte offset from start of compile unit

    static const reference_t null;

    int operator==(const reference_t &o) const
    {
	return (o.cu == cu && o.offset == offset);
    }
    int operator<(const reference_t &o) const
    {
	if (cu < o.cu)
	    return true;
	if (cu == o.cu && offset < o.offset)
	    return true;
	return false;
    }
    std::string as_string() const;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_reference_hxx__
