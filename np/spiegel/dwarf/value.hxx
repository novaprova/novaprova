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
#ifndef __np_spiegel_dwarf_value_hxx__
#define __np_spiegel_dwarf_value_hxx__ 1

#include "np/spiegel/common.hxx"
#include "reference.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct value_t
{
    enum
    {
	T_UNDEF,
	T_STRING,
	T_SINT32,
	T_UINT32,
	T_SINT64,
	T_UINT64,
	T_BYTES,
	T_REF
    } type;
    union
    {
	const char *string;
	int32_t sint32;
	uint32_t uint32;
	int64_t sint64;
	uint64_t uint64;
	struct {
	    const unsigned char *buf;
	    size_t len;
	} bytes;
	reference_t ref;
    } val;

    value_t() : type(T_UNDEF) {}

    static value_t make_string(const char *v)
    {
	value_t val;
	val.type = T_STRING;
	val.val.string = v;
	return val;
    }
    static value_t make_uint32(uint32_t v)
    {
	value_t val;
	val.type = T_UINT32;
	val.val.uint32 = v;
	return val;
    }
    static value_t make_sint32(int32_t v)
    {
	value_t val;
	val.type = T_SINT32;
	val.val.sint32 = v;
	return val;
    }
    static value_t make_uint64(uint64_t v)
    {
	value_t val;
	val.type = T_UINT64;
	val.val.uint64 = v;
	return val;
    }
    static value_t make_sint64(int64_t v)
    {
	value_t val;
	val.type = T_SINT64;
	val.val.sint64 = v;
	return val;
    }
    static value_t make_bytes(const unsigned char *b, size_t l)
    {
	value_t val;
	val.type = T_BYTES;
	val.val.bytes.buf = b;
	val.val.bytes.len = l;
	return val;
    }
    static value_t make_ref(reference_t ref)
    {
	value_t val;
	val.type = T_REF;
	val.val.ref = ref;
	return val;
    }

    void dump() const;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_value_hxx__
