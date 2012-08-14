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
    char buf[64];
    snprintf(buf, sizeof(buf), "(ref){0x%x,0x%x}", cu, offset);
    return buf;
}

// close the namespaces
}; }; };
