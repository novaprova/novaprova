/*
 * Copyright 2011-2020 Gregory Banks
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
#ifndef __NP_TEXT_LISTENER_H__
#define __NP_TEXT_LISTENER_H__ 1

#include "np/listener.hxx"

namespace np {

class text_listener_t : public listener_t
{
public:
    text_listener_t() {}
    ~text_listener_t() {}

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *ev);

private:
    unsigned int nrun_;
    unsigned int nfailed_;
};

// close the namespace
};

#endif /* __NP_TEXT_LISTENER_H__ */
