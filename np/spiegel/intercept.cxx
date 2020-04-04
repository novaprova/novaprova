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
#include "np/util/log.hxx"

namespace np {
namespace spiegel {
using namespace std;
using namespace np::util;

map<addr_t, intercept_t::addrstate_t> intercept_t::installed_;

intercept_t::addrstate_t *
intercept_t::get_addrstate(addr_t addr, bool create)
{
    map<addr_t, addrstate_t>::iterator aitr = installed_.find(addr);
    addrstate_t *as = NULL;
    if (aitr != installed_.end())
	as = &aitr->second;
    else if (create)
	as = &installed_[addr];
    return as;
}

void
intercept_t::remove_addrstate(addr_t addr)
{
    installed_.erase(addr);
}

intercept_t::intercept_t(addr_t a, const char *name)
{
    addr_ = np::spiegel::dwarf::state_t::instance()->instrumentable_address(a);
    name_ = np::util::xstrdup(name);
}

intercept_t::~intercept_t()
{
    xfree(name_);
}

string
intercept_t::as_string() const
{
    string s;
    s += get_name();
    s += " at ";
    s += HEX((unsigned long)addr_);
    return s;
}

int
intercept_t::install()
{
    int r = 0;
    addrstate_t *as = get_addrstate(addr_, /*create*/true);
    as->intercepts_.push_back(this);
    if (as->intercepts_.size() == 1)
    {
	string err;
	r = np::spiegel::platform::install_intercept(addr_, as->state_, err);
	if (r < 0)
	    eprintf("failed to install intercepted function %s at 0x%lx: %s\n",
                    get_name(), (unsigned long)addr_, err.c_str());
    }
    return r;
}

int
intercept_t::uninstall()
{
    int r = 0;
    addrstate_t *as = get_addrstate(addr_, /*create*/true);
    vector<intercept_t*>::iterator itr;
    for (itr = as->intercepts_.begin() ; itr != as->intercepts_.end() ; ++itr)
    {
	if (*itr == this)
	{
	    as->intercepts_.erase(itr);
	    break;
	}
    }
    if (as->intercepts_.size() == 0)
    {
	string err;
	r = np::spiegel::platform::uninstall_intercept(addr_, as->state_, err);
	if (r < 0)
	    eprintf("failed to uninstall intercepted function %s at 0x%lx: %s\n",
                    get_name(), (unsigned long)addr_, err.c_str());
	remove_addrstate(addr_);
    }
    return r;
}

bool
intercept_t::is_intercepted(addr_t addr)
{
    return (get_addrstate(addr, /*create*/false) != NULL);
}

np::spiegel::platform::intstate_t *
intercept_t::get_intstate(addr_t addr)
{
    addrstate_t *as = get_addrstate(addr, /*create*/false);
    return (as ? &as->state_ : NULL);
}

void
intercept_t::dispatch_before(addr_t addr, call_t &call)
{
    addrstate_t *as = get_addrstate(addr, /*create*/false);
    if (as)
    {
	vector<intercept_t*>::iterator vitr;
	for (vitr = as->intercepts_.begin() ; vitr != as->intercepts_.end() ; ++vitr)
	    (*vitr)->before(call);
    }
}

void
intercept_t::dispatch_after(addr_t addr, call_t &call)
{
    addrstate_t *as = get_addrstate(addr, /*create*/false);
    if (as)
    {
	vector<intercept_t*>::iterator vitr;
	for (vitr = as->intercepts_.begin() ; vitr != as->intercepts_.end() ; ++vitr)
	    (*vitr)->after(call);
    }
}

// close the namespaces
}; };
