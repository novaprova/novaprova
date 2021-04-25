/*
 * Copyright 2011-2021 Gregory Banks
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
#ifndef __NP_JUNIT_LISTENER_H__
#define __NP_JUNIT_LISTENER_H__ 1

#include "np/listener.hxx"

namespace np {

class junit_listener_t : public listener_t
{
public:
    junit_listener_t() {}
    ~junit_listener_t() {}

    // TODO: methods to allow changing the base directory

    bool needs_stdout() const;
    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *);

private:
    struct case_t
    {
	case_t()
	 :  result_(R_UNKNOWN),
	    event_(0),
	    elapsed_(0)
	{ }
	~case_t();

	std::string name_;
	result_t result_;
	event_t *event_;
	int64_t elapsed_;
	std::string stdout_;
	std::string stderr_;
    };

    struct suite_t
    {
	std::map<std::string, case_t> cases_;
    };

    std::string get_hostname() const;
    std::string get_timestamp() const;
    case_t *find_case(const job_t *j);

    std::map<std::string, suite_t> suites_;
};

// close the namespace
};

#endif /* __NP_JUNIT_LISTENER_H__ */
