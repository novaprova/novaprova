#include "np/spiegel/mapping.hxx"
#include <sys/mman.h>

namespace np {
namespace spiegel {
using namespace std;
using namespace np::util;

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

void mapping_t::expand_to_pages()
{
    unsigned long end = page_round_up(offset_ + size_);
    offset_ = page_round_down(offset_);
    size_ = end - offset_;
}

int mapping_t::compare_by_offset(const void *v1, const void *v2)
{
    const mapping_t *m1 = *(const mapping_t **)v1;
    const mapping_t *m2 = *(const mapping_t **)v2;
    return u64cmp(m1->offset_, m2->offset_);
}

// close namespaces
}; };
