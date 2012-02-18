#ifndef __libspiegel_platform_common_hxx__
#define __libspiegel_platform_common_hxx__ 1

#include <vector>

namespace spiegel { namespace platform {

extern char *self_exe();

struct linkobj_t
{
    const char *name;
    unsigned long addr;
    unsigned long size;
};
extern std::vector<linkobj_t> self_linkobjs();

// close namespaces
} }

#endif // __libspiegel_platform_common_hxx__
