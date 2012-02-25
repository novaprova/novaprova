#include "value.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

void
value_t::dump() const
{
    switch (type)
    {
    case T_UNDEF:
	printf("<undef>");
	break;
    case T_STRING:
	printf("\"%s\"", val.string);
	break;
    case T_SINT32:
	printf("%d", val.sint32);
	break;
    case T_UINT32:
	printf("0x%x", val.uint32);
	break;
    case T_SINT64:
	printf("%lld", (long long)val.sint64);
	break;
    case T_UINT64:
	printf("0x%llx", (unsigned long long)val.uint64);
	break;
    case T_BYTES:
	{
	    const unsigned char *u = val.bytes.buf;
	    unsigned len = val.bytes.len;
	    const char *sep = "";
	    while (len--)
	    {
		printf("%s0x%02x", sep, (unsigned)*u++);
		sep = " ";
	    }
	}
	break;
    case T_REF:
	printf("%s", val.ref.as_string().c_str());
	break;
    }
}

// close namespace
} }
