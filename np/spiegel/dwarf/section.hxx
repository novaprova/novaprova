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

#ifndef __np_spiegel_dwarf_section_hxx__
#define __np_spiegel_dwarf_section_hxx__ 1

#include "np/spiegel/common.hxx"
#include "np/spiegel/mapping.hxx"
#include "reader.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct section_t : public np::spiegel::mapping_t
{
public:
    reader_t get_contents() const
    {
	reader_t r(map_, size_);
	return r;
    }

    const char *
    offset_as_string(unsigned long off) const
    {
	if (off >= size_)
	    return 0;
	const char *v = (const char *)map_ + off;
	// check that there's a \0 terminator
	if (!memchr(v, '\0', size_-off))
	    return 0;
	return v;
    }
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_section_hxx__
