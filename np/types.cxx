#include "np/types.hxx"

namespace np {

const char *
as_string(functype_t type)
{
    switch (type)
    {
    case FT_UNKNOWN: return "unknown";
    case FT_BEFORE: return "before";
    case FT_TEST: return "test";
    case FT_AFTER: return "after";
    case FT_MOCK: return "mock";
    default: return "INTERNAL ERROR!";
    }
}

// close the namespace
};
