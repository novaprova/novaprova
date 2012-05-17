#include "spiegel/mapping.hxx"
#include <sys/mman.h>

namespace spiegel {
using namespace std;

int mapping_t::mmap(int fd, bool rw)
{
    expand_to_pages();

    int prot = (rw ? PROT_WRITE : PROT_READ);
    int flags = MAP_SHARED;

    void *map = ::mmap(NULL, size_, prot, flags, fd, offset_);
    if (map == MAP_FAILED)
	return -errno;
    map_ = map;
    return 0;
}

int mapping_t::munmap()
{
    if (map_)
    {
	::munmap(map_, size_);
	map_ = NULL;
    }
    return 0;
}

// close namespace
}
