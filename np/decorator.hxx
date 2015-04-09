/*
 * Copyright 2011-2015 Gregory Banks
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
#ifndef __NP_DECORATOR_H__
#define __NP_DECORATOR_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"

namespace np {

class job_t;

/*
 * decorator_t is an abstract base class for small objects
 * attached to a job which represent some additional per-job
 * state.  Decorators can be used to add new per-test checks
 * to NovaProva without changing the main test running code.
 */
class decorator_t : public np::util::zalloc
{
public:
    decorator_t() {}
    virtual ~decorator_t() {}

    virtual result_t child_pre(job_t *) { return R_UNKNOWN; }
    virtual result_t child_post(job_t *) { return R_UNKNOWN; }
    virtual result_t parent_pre(job_t *) { return R_UNKNOWN; }
    virtual result_t parent_post(job_t *) { return R_UNKNOWN; }
};

/* TODO: replace this with Reflection also */
#define _NP_DECORATOR(nm, pri) \
    extern "C" { \
    static np::decorator_t *__np_decorator_##pri##_##nm() __attribute__((used)); \
    static np::decorator_t *__np_decorator_##pri##_##nm() { return new nm(); } \
    };

// close the namespace
};

#endif /* __NP_DECORATOR_H__ */
