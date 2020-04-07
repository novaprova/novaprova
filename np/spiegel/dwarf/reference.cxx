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
#include "reference.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

const reference_t reference_t::null = { 0, 0 };

string reference_t::as_string() const
{
    if (!resolver)
        return string("(ref){0,0}");
    char offbuf[32];
    snprintf(offbuf, sizeof(offbuf), ",0x%llx}", offset);
    return string("(ref){") + resolver->describe_resolver() + string(offbuf);
}

// close the namespaces
}; }; };
