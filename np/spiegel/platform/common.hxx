#ifndef __np_spiegel_platform_common_hxx__
#define __np_spiegel_platform_common_hxx__ 1

#include "np/spiegel/mapping.hxx"
#include <vector>

namespace np { namespace spiegel { namespace platform {

extern char *self_exe();

struct linkobj_t
{
    const char *name;
    std::vector<np::spiegel::mapping_t> mappings;
};
extern std::vector<linkobj_t> get_linkobjs();

void add_plt(const np::spiegel::mapping_t &);
np::spiegel::addr_t normalise_address(np::spiegel::addr_t);

// extern np::spiegel::value_t invoke(void *fnaddr, vector<np::spiegel::value_t> args);

extern int text_map_writable(addr_t addr, size_t len);
extern int text_restore(addr_t addr, size_t len);
extern int install_intercept(np::spiegel::addr_t);
extern int uninstall_intercept(np::spiegel::addr_t);

extern std::vector<np::spiegel::addr_t> get_stacktrace();

extern bool is_running_under_debugger();

// close namespaces
}; }; };

#endif // __np_spiegel_platform_common_hxx__
