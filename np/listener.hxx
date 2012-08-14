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
#ifndef __NP_LISTENER_H__
#define __NP_LISTENER_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"


namespace np {

class event_t;
class job_t;

class listener_t
{
public:
    listener_t() {}
    ~listener_t() {}

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void begin_job(const job_t *) = 0;
    virtual void end_job(const job_t *, result_t) = 0;
    virtual void add_event(const job_t *, const event_t *) = 0;
};

// close the namespace
};

#endif /* __NP_LISTENER_H__ */
