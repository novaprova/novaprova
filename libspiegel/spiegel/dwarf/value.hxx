#ifndef __libspiegel_dwarf_value_hxx__
#define __libspiegel_dwarf_value_hxx__ 1

#include "spiegel/commonp.hxx"
#include "reference.hxx"

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
    static value_t make_ref(uint32_t cu, uint32_t off)
    {
	value_t val;
	val.type = T_REF;
	val.val.ref.cu = cu;
	val.val.ref.offset = off;
	return val;
    }

    void dump() const;
};

// close namespaces
} }

#endif // __libspiegel_dwarf_value_hxx__
