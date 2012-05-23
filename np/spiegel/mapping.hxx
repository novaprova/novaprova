#ifndef __np_spiegel_mapping_hxx__
#define __np_spiegel_mapping_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {

struct mapping_t
{
    static void *operator new(size_t sz) { return np::util::xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    mapping_t()
     :  offset_(0), size_(0), map_(0)
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
