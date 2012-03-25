#include "spiegel/intercept.hxx"
#include "spiegel/platform/common.hxx"

namespace spiegel {
using namespace std;

vector<intercept_t*> intercept_t::installed_;

intercept_t *
intercept_t::_find_installed(addr_t addr)
{
    vector<intercept_t*>::iterator itr;
    for (itr = installed_.begin() ; itr != installed_.end() ; ++itr)
	if ((*itr)->addr_ == addr)
	    return *itr;
    return 0;
}

void
intercept_t::_add_installed()
{
    installed_.push_back(this);
}

void
intercept_t::_remove_installed()
{
//     installed_.erase(itr);
}

int
intercept_t::install()
{
    return spiegel::platform::install_intercept(this);
}

int
intercept_t::uninstall()
{
    return spiegel::platform::uninstall_intercept(this);
}

// close namespace
}
