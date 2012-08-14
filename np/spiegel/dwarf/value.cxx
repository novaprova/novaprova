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

void
value_t::dump() const
{
    switch (type)
    {
    case T_UNDEF:
	printf("<undef>");
	break;
    case T_STRING:
	printf("\"%s\"", val.string);
	break;
    case T_SINT32:
	printf("%d", val.sint32);
	break;
    case T_UINT32:
	printf("0x%x", val.uint32);
	break;
    case T_SINT64:
	printf("%lld", (long long)val.sint64);
	break;
    case T_UINT64:
	printf("0x%llx", (unsigned long long)val.uint64);
	break;
    case T_BYTES:
	{
	    const unsigned char *u = val.bytes.buf;
	    unsigned len = val.bytes.len;
	    const char *sep = "";
	    while (len--)
	    {
		printf("%s0x%02x", sep, (unsigned)*u++);
		sep = " ";
	    }
	}
	break;
    case T_REF:
	printf("%s", val.ref.as_string().c_str());
	break;
    }
}

// close namespaces
}; }; };
