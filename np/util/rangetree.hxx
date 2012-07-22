#ifndef __spiegel_rangetree_hxx__
#define __spiegel_rangetree_hxx__ 1

#include <map>

namespace np { namespace util {

// This pattern for implementing a ranged container on top
// of STL map is from David Rodriguez, see
// http://stackoverflow.com/questions/1089192/c-stl-range-container

template <typename T> class range
{
public:
    range(T x) : lo(x), hi(x) {}
    range(T l, T h) : lo(l), hi(h) {}

    T lo, hi;
};

template <typename T> struct left_of_range
    : public std::binary_function< range<T>, range<T>, bool>
{
    bool operator()(const range<T> &a, const range<T> &b) const
    {
	return (a.lo < b.lo && a.hi <= b.lo);
    }
};

template <typename K, typename V> class rangetree
{
public:
    typedef K key_type;
    typedef V value_type;
    typedef typename std::map<range<K>, V, left_of_range<K> >::iterator iterator;
    typedef typename std::map<range<K>, V, left_of_range<K> >::const_iterator const_iterator;

private:
    std::map<range<K>, V, left_of_range<K> > map_;

public:
    // no c'tor

    unsigned size() const { return map_.size(); }

    void insert(K x, const V &val) { map_[range<K>(x)] = val; }
    void insert(K lo, K hi, const V &val) { map_[range<K>(lo, hi)] = val; }

    const_iterator find(K x) const { return map_.find(range<K>(x)); }
    const_iterator find(K lo, K hi) const { return map_.find(range<K>(lo, hi)); }

    iterator begin() { return map_.begin(); }
    iterator end() { return map_.end(); }
    const_iterator begin() const { return map_.begin(); }
    const_iterator end() const { return map_.end(); }
};

// close the namespaces
}; };

#endif // __spiegel_rangetree_hxx__
