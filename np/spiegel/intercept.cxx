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
#include "np/spiegel/intercept.hxx"
#include "np/spiegel/platform/common.hxx"
#include "np/spiegel/dwarf/state.hxx"

namespace np {
namespace spiegel {
using namespace std;

map<addr_t, vector<intercept_t*> > intercept_t::installed_;

intercept_t::intercept_t(addr_t a, const char *name)
{
    addr_ = np::spiegel::platform::normalise_address(a);
    name_ = np::util::xstrdup(name);
}

intercept_t::~intercept_t()
{
    xfree(name_);
}

int
intercept_t::install()
{
    int r = 0;
    vector<intercept_t*> *v = &installed_[addr_];
    v->push_back(this);
    if (v->size() == 1)
    {
	string err;
	r = np::spiegel::platform::install_intercept(addr_, err);
	if (r < 0)
	    fprintf(stderr, "np: failed to install intercepted "
			    "function %s at 0x%lx: %s\n",
			    get_name(), (unsigned long)addr_, err.c_str());
    }
    return r;
}

int
intercept_t::uninstall()
{
    int r = 0;
    vector<intercept_t*> *v = &installed_[addr_];
    vector<intercept_t*>::iterator itr;
    for (itr = v->begin() ; itr != v->end() ; ++itr)
    {
	if (*itr == this)
	{
	    v->erase(itr);
	    break;
	}
    }
    if (v->size() == 0)
    {
	installed_.erase(addr_);
	string err;
	r = np::spiegel::platform::uninstall_intercept(addr_, err);
	if (r < 0)
	    fprintf(stderr, "np: failed to uninstall intercepted "
			    "function %s at 0x%lx: %s\n",
			    get_name(), (unsigned long)addr_, err.c_str());
    }
    return r;
}

bool
intercept_t::is_intercepted(addr_t addr)
{
    map<addr_t, vector<intercept_t*> >::iterator aitr = installed_.find(addr);
    return (aitr != installed_.end());
}

void
intercept_t::dispatch_before(addr_t addr, call_t &call)
{
    map<addr_t, vector<intercept_t*> >::iterator aitr = installed_.find(addr);
    if (aitr != installed_.end())
    {
	vector<intercept_t*>::iterator vitr;
	for (vitr = aitr->second.begin() ; vitr != aitr->second.end() ; ++vitr)
	    (*vitr)->before(call);
    }
}

void
intercept_t::dispatch_after(addr_t addr, call_t &call)
{
    map<addr_t, vector<intercept_t*> >::iterator aitr = installed_.find(addr);
    if (aitr != installed_.end())
    {
	vector<intercept_t*>::iterator vitr;
	for (vitr = aitr->second.begin() ; vitr != aitr->second.end() ; ++vitr)
	    (*vitr)->after(call);
    }
}

// close the namespaces
}; };
