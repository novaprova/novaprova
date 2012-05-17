#ifndef __NP_TYPES_H__
#define __NP_TYPES_H__ 1

namespace np {

enum result_t
{
    /* Note ordinal values: we use MAX() to combine
     * multiple results for a given test */
    R_UNKNOWN=0,
    R_PASS,
    R_NOTAPPLICABLE,
    R_FAIL
};

inline result_t merge(result_t r1, result_t r2)
{
    return (r1 > r2 ? r1 : r2);
}

enum functype_t
{
    FT_UNKNOWN,
    FT_BEFORE,
    FT_TEST,
    FT_AFTER,
#define FT_NUM_SINGULAR	(FT_AFTER+1)
    FT_MOCK,
    FT_PARAM,
#define FT_NUM		(FT_PARAM+1)
};

extern const char *as_string(functype_t);

// close the namespace
};

#endif /* __NP_TYPES_H__ */
