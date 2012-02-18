#include "spiegel/commonp.hxx"
#include "common.hxx"

#include <dlfcn.h>
#include <link.h>

namespace spiegel { namespace platform {
using namespace std;

char *
self_exe()
{
    // We could find the ELF auxv_t vector which is above the environ[]
    // passed to us by the kernel, there is usually a AT_EXECFN entry
    // there which points at the executable filename.  But, when running
    // under Valgrind, the aux vector is synthetically built by Valgrind
    // and doesn't contain AT_EXECFN.  However, Valgrind *does*
    // intercept a readlink() of the magical Linux /proc file and
    // substitute the answer we want (instead of what the real kernel
    // readlink() call would return, which is the Valgrind executable
    // itself).  So, we have a fallback that works in all cases, we may
    // as well use it.
    char buf[PATH_MAX];
    static const char filename[] = "/proc/self/exe";
    int r = readlink(filename, buf, sizeof(buf)-1);
    if (r < 0)
    {
	perror(filename);
	return 0;
    }
    // readlink() doesn't terminate the buffer
    buf[r] = '\0';
    return xstrdup(buf);
}

static int
add_one_linkobj(struct dl_phdr_info *info,
		size_t size, void *closure)
{
    vector<linkobj_t> *vec = (vector<linkobj_t> *)closure;

    linkobj_t lo;
    lo.name = info->dlpi_name;
    lo.addr = (unsigned long)info->dlpi_addr;
    lo.size = (unsigned long)size;
    vec->push_back(lo);

    return 0;
}

vector<linkobj_t> self_linkobjs()
{
    vector<linkobj_t> vec;
    dl_iterate_phdr(add_one_linkobj, &vec);
    return vec;
}

// close namespace
} }
