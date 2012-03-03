#include "u4c/types.hxx"

namespace u4c {

const char *
as_string(functype_t type)
{
    switch (type)
    {
    case FT_UNKNOWN: return "unknown";
    case FT_BEFORE: return "before";
    case FT_TEST: return "test";
    case FT_AFTER: return "after";
    default: return "INTERNAL ERROR!";
    }
}

// close the namespace
};
