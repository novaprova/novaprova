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
