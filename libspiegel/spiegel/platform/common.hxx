#ifndef __libspiegel_platform_common_hxx__
#define __libspiegel_platform_common_hxx__ 1

#include <spiegel/mapping.hxx>
#include <vector>

namespace spiegel { class intercept_t; }

namespace spiegel { namespace platform {

extern char *self_exe();

struct linkobj_t
{
    const char *name;
    std::vector<spiegel::mapping_t> mappings;
};
extern std::vector<linkobj_t> get_linkobjs();

spiegel::addr_t follow_plt(spiegel::addr_t);

// extern spiegel::value_t invoke(void *fnaddr, vector<spiegel::value_t> args);

extern int text_map_writable(addr_t addr, size_t len);
extern int text_restore(addr_t addr, size_t len);
extern int install_intercept(spiegel::addr_t);
extern int uninstall_intercept(spiegel::addr_t);

extern std::vector<spiegel::addr_t> get_stacktrace();

// close namespaces
} }

#endif // __libspiegel_platform_common_hxx__
