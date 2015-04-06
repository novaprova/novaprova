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
#ifndef __NP_TAP_LISTENER_H__
#define __NP_TAP_LISTENER_H__ 1

#include "np/listener.hxx"

namespace np {

class tap_listener_t : public listener_t
{
public:
    tap_listener_t() : last_number_(0) {}
    ~tap_listener_t() {}

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *ev);

private:
    struct run_t
    {
	run_t() {}
	~run_t() {}

	std::string event_;
    };

    bool needs_stdout() const;
    run_t *find_run(const job_t *j);
    void emit_diagnostics(const std::string &s);

    unsigned int last_number_;
    std::map<std::string, run_t> runs_;
};

// close the namespace
};

#endif /* __NP_TAP_LISTENER_H__ */
