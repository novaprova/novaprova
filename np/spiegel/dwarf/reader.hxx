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
#ifndef __np_spiegel_dwarf_reader_hxx__
#define __np_spiegel_dwarf_reader_hxx__ 1

#include "np/spiegel/common.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class reader_t
{
public:
    reader_t()
     :  p_(0), end_(0), base_(0) {}

    reader_t(const void *base, size_t len)
     :  p_((unsigned char *)base),
	end_(((unsigned char *)base) + len),
	base_((unsigned char *)base)
    {}

    reader_t initial_subset(size_t len) const
    {
	size_t remain = (end_ - p_);
	if (len > remain)
	    len = remain;
	return reader_t((void *)p_, len);
    }

    unsigned long get_offset() const
    {
	return p_ - base_;
    }
    unsigned long get_remains() const
    {
	return end_ - p_;
    }

    bool seek(size_t n)
    {
	if (base_ + n > end_)
	    return false;
	p_ = base_ + n;
	return true;
    }

    bool skip(size_t n)
    {
	if (p_ + n > end_)
	    return false;
	p_ += n;
	return true;
    }

    bool read_u32(uint32_t &v)
    {
	if (p_ + 4 > end_)
	    return false;
	v = ((uint32_t)p_[0]) |
	    ((uint32_t)p_[1] << 8) |
	    ((uint32_t)p_[2] << 16) |
	    ((uint32_t)p_[3] << 24);
	p_ += 4;
	return true;
    }

    bool read_u64(uint64_t &v)
    {
	if (p_ + 8 > end_)
	    return false;
	v = ((uint64_t)p_[0]) |
	    ((uint64_t)p_[1] << 8) |
	    ((uint64_t)p_[2] << 16) |
	    ((uint64_t)p_[3] << 24) |
	    ((uint64_t)p_[4] << 32) |
	    ((uint64_t)p_[5] << 40) |
	    ((uint64_t)p_[6] << 48) |
	    ((uint64_t)p_[7] << 56);
	p_ += 8;
	return true;
    }

    bool read_addr(np::spiegel::addr_t &v)
    {
#if _NP_ADDRSIZE == 4
	return read_u32(v);
#elif _NP_ADDRSIZE == 8
	return read_u64(v);
#else
#error "Unknown address size"
#endif
    }

    bool read_uleb128(uint32_t &v)
    {
	const unsigned char *pp = p_;
	uint32_t vv = 0;
	unsigned shift = 0;
	do
	{
	    if (pp == end_)
		return false;
	    vv |= ((*pp) & 0x7f) << shift;
	    shift += 7;
	} while ((*pp++) & 0x80);
	p_ = pp;
	v = vv;
	return true;
    }

    bool read_sleb128(int32_t &v)
    {
	const unsigned char *pp = p_;
	uint32_t vv = 0;
	unsigned shift = 0;

	do
	{
	    if (pp == end_)
		return false;
	    vv |= ((*pp) & 0x7f) << shift;
	    shift += 7;
	} while ((*pp++) & 0x80);

	// sign extend the result, from the last encoded bit
	if (pp[-1] & 0x40)
	    v = vv | (~0U << shift);
	else
	    v = vv;
	p_ = pp;
	return true;
    }

    bool read_u16(uint16_t &v)
    {
	if (p_ + 2 > end_)
	    return false;
	v = ((uint16_t)p_[0]) |
	    ((uint16_t)p_[1] << 8);
	p_ += 2;
	return true;
    }

    bool read_u8(uint8_t &v)
    {
	if (p_ >= end_)
	    return false;
	v = *p_++;
	return true;
    }

    bool read_string(const char *&v)
    {
	const unsigned char *e =
	    (const unsigned char *)memchr(p_, '\0', (end_ - p_));
	if (!e)
	    return false;
	v = (const char *)p_;
	p_ = e + 1;
	return true;
    }

    bool read_bytes(const unsigned char *&v, size_t len)
    {
	const unsigned char *e = p_ + len;
	if (e >= end_)
	    return false;
	v = (const unsigned char *)p_;
	p_ = e;
	return true;
    }

private:
    const unsigned char *p_;
    const unsigned char *end_;
    const unsigned char *base_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_reader_hxx__
