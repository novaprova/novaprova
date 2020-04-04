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
#include "value.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

string
value_t::to_string() const
{
    char buf[64];

    switch (type)
    {
    case T_UNDEF:
	return string("<undef>");
    case T_STRING:
        return string("\"") + string(val.string) + string("\"");
    case T_SINT32:
	snprintf(buf, sizeof(buf), "%d", val.sint32);
        return string(buf);
    case T_UINT32:
	snprintf(buf, sizeof(buf), "0x%x", val.uint32);
        return string(buf);
    case T_SINT64:
	snprintf(buf, sizeof(buf), "%lld", (long long)val.sint64);
        return string(buf);
    case T_UINT64:
	snprintf(buf, sizeof(buf), "0x%llx", (unsigned long long)val.uint64);
        return string(buf);
    case T_BYTES:
	{
            string s = "";
	    const unsigned char *u = val.bytes.buf;
	    unsigned len = val.bytes.len;
	    const char *sep = "";
	    while (len--)
	    {
		snprintf(buf, sizeof(buf), "%s0x%02x", sep, (unsigned)*u++);
                s += buf;
		sep = " ";
	    }
            return s;
	}
    case T_REF:
        return val.ref.as_string();
    }
}

// close namespaces
}; }; };
