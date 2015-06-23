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
    FT_DECORATOR,
    FT_INTERNAL,
#define FT_NUM		(FT_INTERNAL+1)
};

extern const char *as_string(functype_t);

// close the namespace
};

#endif /* __NP_TYPES_H__ */
