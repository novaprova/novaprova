#include "np/spiegel/intercept.hxx"
#include "np/spiegel/platform/common.hxx"
#include "np/spiegel/dwarf/state.hxx"

namespace np {
namespace spiegel {
using namespace std;

map<addr_t, vector<intercept_t*> > intercept_t::installed_;

intercept_t::intercept_t(addr_t a)
{
    addr_ = np::spiegel::platform::normalise_address(a);
}

intercept_t::~intercept_t()
{
}

int
intercept_t::install()
{
    vector<intercept_t*> *v = &installed_[addr_];
    v->push_back(this);
    if (v->size() == 1)
	return np::spiegel::platform::install_intercept(addr_);
    return 0;
}

int
intercept_t::uninstall()
{
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
	return np::spiegel::platform::uninstall_intercept(addr_);
    }
    return 0;
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
