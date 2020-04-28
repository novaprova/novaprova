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
#include "np/util/log.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class reader_t
{
public:
    reader_t()
     :  p_(0), end_(0), base_(0), is64_(false) {}

    reader_t(const void *base, size_t len, bool is64 = false)
     :  p_((unsigned char *)base),
	end_(((unsigned char *)base) + len),
	base_((unsigned char *)base),
	is64_(is64)
    {}

    reader_t initial_subset(size_t len) const
    {
	size_t remain = (end_ - p_);
	if (len > remain)
	    len = remain;
	return reader_t((void *)p_, len, is64_);
    }
    void set_is64(bool b)
    {
	is64_ = b;
    }

    void rewind()
    {
        p_ = base_;
    }
    unsigned long get_offset() const
    {
	return p_ - base_;
    }
    unsigned long get_remains() const
    {
	return end_ - p_;
    }
    unsigned long get_length() const
    {
	return end_ - base_;
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
    bool read_u32(uint64_t &v)
    {
	uint32_t v32;
	if (!read_u32(v32))
	    return false;
	v = v32;	    /* zero extend */
	return true;
    }
    bool skip_u32()
    {
	return skip(4);
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
    bool skip_u64()
    {
	return skip(8);
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
    bool skip_addr()
    {
#if _NP_ADDRSIZE == 4
	return skip_u32();
#elif _NP_ADDRSIZE == 8
	return skip_u64();
#else
#error "Unknown address size"
#endif
    }

    bool read_offset(np::spiegel::offset_t &v)
    {
#if _NP_ADDRSIZE == 4
	return read_u32(v);
#elif _NP_ADDRSIZE == 8
	return is64_ ? read_u64(v) : read_u32(v);
#else
#error "Unknown offset size"
#endif
    }
    bool skip_offset()
    {
#if _NP_ADDRSIZE == 4
	return skip_u32();
#elif _NP_ADDRSIZE == 8
	return is64_ ? skip_u64() : skip_u32();
#else
#error "Unknown offset size"
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
    bool skip_uleb128()
    {
	const unsigned char *pp = p_;
	do
	{
	    if (pp == end_)
		return false;
	} while ((*pp++) & 0x80);
	p_ = pp;
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
    bool skip_sleb128()
    {
	const unsigned char *pp = p_;
	do
	{
	    if (pp == end_)
		return false;
	} while ((*pp++) & 0x80);
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
    bool skip_u16()
    {
	return skip(2);
    }

    bool read_u8(uint8_t &v)
    {
	if (p_ >= end_)
	    return false;
	v = *p_++;
	return true;
    }
    bool skip_u8()
    {
	return skip(1);
    }

    bool read_s8(int8_t &v)
    {
	if (p_ >= end_)
	    return false;
	v = (int8_t)(*p_++);
	return true;
    }
    bool skip_s8()
    {
	return skip(1);
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
    bool skip_string()
    {
	const unsigned char *e =
	    (const unsigned char *)memchr(p_, '\0', (end_ - p_));
	if (!e)
	    return false;
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
    bool skip_bytes(size_t len)
    {
	return skip(len);
    }

    bool read_initial_length(offset_t &length, bool &is64)
    {
        is64 = false;
        if (!read_u32(length))
            return false;
        if (length == 0xffffffff)
        {
            /* An all-1 length marks the 64-bit format
             * introduced in the DWARF3 standard */
#if _NP_ADDRSIZE == 4
            fatal("The 64-bit DWARF format is not supported on 32-bit architectures");
#elif _NP_ADDRSIZE == 8
            is64 = true;
            if (!read_u64(length))
                return false;
#else
#error "Unknown address size"
#endif
        }
        else if (length >= 0xfffffff0)
        {
            eprintf("Invalid initial length 0x%lx", (unsigned long)length);
            return false;
        }
        return true;
    }

private:
    const unsigned char *p_;
    const unsigned char *end_;
    const unsigned char *base_;
    bool is64_;			// new 64-bit format introduced in DWARF3
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_reader_hxx__
