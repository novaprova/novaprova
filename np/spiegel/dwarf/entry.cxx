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
#include "entry.hxx"
#include "enumerations.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

void
entry_t::dump() const
{
    if (!abbrev_)
	return;

    printf("Entry 0x%x [%u] %s {\n",
	offset_,
	level_,
	tagnames.to_name(abbrev_->tag));

    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = abbrev_->attr_specs.begin() ; i != abbrev_->attr_specs.end() ; ++i)
    {
	printf("    %s = ", attrnames.to_name(i->name));
	const value_t *v = get_attribute(i->name);
	if (v)
	    v->dump();
	else
	    printf("<missing>");
	printf("\n");
    }
    printf("}\n");
}

// close namespaces
}; }; };
