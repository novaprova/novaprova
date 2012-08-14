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
#ifndef __np_spiegel_dwarf_abbrev_hxx__
#define __np_spiegel_dwarf_abbrev_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class reader_t;

struct abbrev_t
{
    struct attr_spec_t
    {
	uint32_t name;
	uint32_t form;
    };

    // default c'tor
    abbrev_t()
     :  code(0),
        tag(0),
        children(0)
    {}

    // c'tor with code
    abbrev_t(uint32_t c)
     :  code(c),
        tag(0),
	children(0)
    {}

    bool read(reader_t &r);

    uint32_t code;
    uint32_t tag;
    uint8_t children;
    std::vector<attr_spec_t> attr_specs;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_abbrev_hxx__
