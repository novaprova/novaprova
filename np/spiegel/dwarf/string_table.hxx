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
#ifndef __np_spiegel_dwarf_string_table_hxx__
#define __np_spiegel_dwarf_string_table_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class string_table_t
{
public:
    string_table_t(const char *prefix, const char * const *names)
     :  prefix_(prefix),
        names_(names),
	prefix_len_(prefix ? strlen(prefix) : 0)
    {
	int i;
	for (i = 0 ; names_[i] ; i++)
	    ;
	names_count_ = i;
    }

    int to_index(const char *name) const;
    const char *to_name(int i) const;

private:
    const char *prefix_;
    const char * const * names_;
    unsigned prefix_len_;
    unsigned names_count_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_string_table_hxx__
