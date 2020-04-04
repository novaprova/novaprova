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
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "np/job.hxx"
#include "np/util/log.hxx"

namespace np {
using namespace std;
using namespace np::util;

unsigned int job_t::next_id_ = 1;

job_t::job_t(const plan_t::iterator &i)
 :  id_(next_id_++),
    node_(i.get_node()),
    assigns_(i.get_assignments())
{
}

job_t::~job_t()
{
    if (stdout_path_ != "")
	unlink(stdout_path_.c_str());
    if (stderr_path_ != "")
	unlink(stderr_path_.c_str());
}

string job_t::as_string() const
{
    string s = node_->get_fullname();
    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	s += string("[") + i->as_string() + "]";
    return s;
}

void
job_t::pre_run(bool in_parent)
{
    if (in_parent)
    {
	start_ = rel_now();
	return;
    }

    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	i->apply();

    node_->pre_run();
}

void
job_t::post_run(bool in_parent)
{
    if (in_parent)
    {
	end_ = rel_now();
	return;
    }

    node_->post_run();

    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	i->unapply();
}

int64_t
job_t::get_elapsed() const
{
    int64_t end = end_;
    if (!end)
	end = rel_now();
    return end - start_;
}

static string
get_file_contents(const string &path)
{
    struct stat sb;
    int r;
    int fd;
    int remain;
    char *b;
    string buf;

    fd = open(path.c_str(), O_RDONLY, 0);
    if (fd < 0)
    {
        eprintf("Failed to open file %s: %s\n", path.c_str(), strerror(errno));
	return string("");
    }

    r = fstat(fd, &sb);
    if (r < 0)
    {
        eprintf("Failed to fstat file %s: %s\n", path.c_str(), strerror(errno));
	return string("");
    }
    buf.resize(sb.st_size);

    remain = sb.st_size;
    b = (char *)buf.c_str();
    while (remain > 0 && (r = read(fd, b, remain)) > 0)
    {
	remain -= r;
	b += r;
    }

    close(fd);
    return buf;
}

string
job_t::get_stdout() const
{
    return get_file_contents(stdout_path_);
}

string
job_t::get_stderr() const
{
    return get_file_contents(stderr_path_);
}

// close the namespace
};
