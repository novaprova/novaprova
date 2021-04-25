/*
 * Copyright 2011-2021 Gregory Banks
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
#ifndef __np_spiegel_mapping_hxx__
#define __np_spiegel_mapping_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {

struct mapping_t : public np::util::zalloc
{
    mapping_t()
     :  offset_(0), size_(0), map_(0)
    {}
    mapping_t(unsigned long off, unsigned long sz)
     :  offset_(off), size_(sz), map_(0)
    {}
    mapping_t(unsigned long off, unsigned long sz, void *m)
     :  offset_(off), size_(sz), map_(m)
    {}
    ~mapping_t() {}

    unsigned long get_offset() const { return offset_; }
    void set_offset(unsigned long off) { offset_ = off; }

    unsigned long get_size() const { return size_; }
    void set_size(unsigned long sz) { size_ = sz; }

    void set_range(unsigned long off, unsigned long sz)
    {
	offset_ = off;
	size_ = sz;
    }

    bool is_mapped() const { return !!map_; }
    void set_map(void *m) { map_ = m; }
    void *get_map() const { return map_; }

    unsigned long get_end() const { return offset_ + size_; }
    void set_end(unsigned long e) { size_ = e - offset_; }

    bool contains(const mapping_t &m) const
    {
	return (offset_ <= m.offset_ &&
		get_end() >= m.get_end());
    }
    bool contains(void *p) const
    {
	return (map_ <= p &&
		(void *)((char *)map_ + size_) >= p);
    }
    void map_from(const mapping_t &m)
    {
	map_ = (void *)((char *)m.map_ + offset_ - m.offset_);
    }

    int mmap(int fd, bool rw);
    int munmap();

    void expand_to_pages();

    static int compare_by_offset(const void *v1, const void *v2);

protected:
    unsigned long offset_;
    unsigned long size_;
    void *map_;
};

// close namespaces
}; };

#endif // __np_spiegel_mapping_hxx__
