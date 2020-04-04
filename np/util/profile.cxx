/*
 * Copyright 2011-2014 Gregory Banks
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
#include "np/util/common.hxx"
#include "np/util/profile.hxx"
#include <fcntl.h>
#include "np/util/log.hxx"

namespace np {
namespace profile {

int probe_t::depth_ = 0;
int probe_t::log_fd_ = -2;

probe_t::probe_t(const char *function)
 :  function_(function)
{
    log("begin");
    depth_++;
}

probe_t::~probe_t()
{
    depth_--;
    log("end");
}

void probe_t::log(const char *which)
{
    if (log_fd_ == -2)
    {
	static const char filename[] = "novaprova.profile.dat";
	log_fd_ = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (log_fd_ < 0)
	{
	    eprintf("Cannot open %s for writing: %s\n",
		    filename, strerror(errno));
	}
    }

    if (log_fd_ >= 0)
    {
	char line[512];
	snprintf(line, sizeof(line), "%lld %d %d %s %s\n",
		(long long)np::util::rel_time(),
		(int)getpid(), depth_, which, function_);
	write(log_fd_, line, strlen(line));
    }
}

// close the namespaces
}; };
