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
#include "np/job.hxx"
#include "np/event.hxx"
#include "np/runner.hxx"
#include "np/decorator.hxx"

namespace np {
using namespace std;
using namespace np::util;

class fdleak_decorator_t : public decorator_t
{
public:
    fdleak_decorator_t();
    ~fdleak_decorator_t();

    result_t child_pre(job_t *);
    result_t child_post(job_t *);

private:
    vector<string> prefds_;
};

fdleak_decorator_t::fdleak_decorator_t()
{
}

fdleak_decorator_t::~fdleak_decorator_t()
{
    prefds_.clear();
}

result_t
fdleak_decorator_t::child_pre(job_t *j __attribute__((unused)))
{
    prefds_ = np::spiegel::platform::get_file_descriptors();
    return R_UNKNOWN;
}

result_t
fdleak_decorator_t::child_post(job_t *j)
{
    result_t res = R_UNKNOWN;
    vector<string> postfds = np::spiegel::platform::get_file_descriptors();
    unsigned int fd = 0;
    string none;
    char msg[1024];

    unsigned maxfd = max(prefds_.size(), postfds.size());
    for (fd = 0 ; fd < maxfd ; fd++)
    {
	const string &pre = (fd < prefds_.size() ? prefds_[fd] : none);
	const string &post = (fd < postfds.size() ? postfds[fd] : none);

	if (pre == post)
	    continue;

	if (post == "")
	    snprintf(msg, sizeof(msg),
		     "test closed file descriptor %d -> %s\n",
		    fd, pre.c_str());
	else if (pre == "")
	    snprintf(msg, sizeof(msg),
		     "test leaked file descriptor %d -> %s\n",
		     fd, post.c_str());
	else
	    snprintf(msg, sizeof(msg),
		     "test reopened file descriptor %d -> %s as %s\n",
		     fd, pre.c_str(), post.c_str());
	event_t ev(EV_FDLEAK, msg);
	res = merge(res, runner_t::running()->raise_event(j, &ev));
    }

    return res;
}

_NP_DECORATOR(fdleak_decorator_t, 50)

// hack to ensure we're linked into the executable
extern "C" void __np_linkme1(void) { }

// close the namespace
};
