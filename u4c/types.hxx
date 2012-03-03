#ifndef __U4C_TYPES_H__
#define __U4C_TYPES_H__ 1

namespace u4c {

enum result_t
{
    /* Note ordinal values: we use MAX() to combine
     * multiple results for a given test */
    R_UNKNOWN=0,
    R_PASS,
    R_NOTAPPLICABLE,
    R_FAIL
};

enum functype_t
{
    FT_UNKNOWN,
    FT_BEFORE,
    FT_TEST,
    FT_AFTER,

    FT_NUM
};

extern const char *as_string(functype_t);

// close the namespace
};

#endif /* __U4C_TYPES_H__ */
