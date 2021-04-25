/*
 * Copyright 2011-2021 Gregory Banks
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
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

/* Sometimes, in DWARF-4, the form used to store an attribute
 * impacts its semantics.  For example, DW_AT_high_pc is either
 * an absolute address (if stored as DW_FORM_addr), or a length
 * (if stored as DW_FORM_{data[1248],[us]data}. */
enum form_values
entry_t::get_attribute_form(uint32_t name) const
{
    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = abbrev_->attr_specs.begin() ; i != abbrev_->attr_specs.end() ; ++i)
    {
	if (i->name == name)
	    return (enum form_values)i->form;
    }
    return (enum form_values)0;
}

string
entry_t::to_string() const
{
    if (!abbrev_)
	return string();

    string ret;
    char buf[1024];
    snprintf(buf, sizeof(buf), "Entry offset=0x%x level=%u tag=%s {",
	offset_,
	level_,
	tagnames.to_name(abbrev_->tag));
    ret += buf;

    const char *sep = "";
    vector<abbrev_t::attr_spec_t>::const_iterator i;
    for (i = abbrev_->attr_specs.begin() ; i != abbrev_->attr_specs.end() ; ++i)
    {
        ret += sep;
        sep = ", ";
        const char *attrname = attrnames.to_name(i->name);
        if (attrname)
            ret += attrname;
        else
            ret += "???";
        ret += "=";
	const value_t *v = get_attribute(i->name);
	if (v)
            ret += v->to_string();
	else
            ret += "<missing>";
    }
    ret += "}";
    return ret;
}

// close namespaces
}; }; };
