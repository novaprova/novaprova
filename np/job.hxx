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
#ifndef __NP_JOB_H__
#define __NP_JOB_H__ 1

#include "np/util/common.hxx"
#include "np/testnode.hxx"
#include "np/plan.hxx"

namespace np {

class job_t : public np::util::zalloc
{
public:
    job_t(const plan_t::iterator &);
    ~job_t();

    std::string as_string() const;
    testnode_t *get_node() const { return node_; }
    void pre_run(bool in_parent);
    void post_run(bool in_parent);

    int64_t get_start() const { return start_; }
    int64_t get_elapsed() const;

    void set_stdout_path(const char *path) { stdout_path_ = std::string(path); }
    void set_stderr_path(const char *path) { stderr_path_ = std::string(path); }
    std::string get_stdout() const;
    std::string get_stderr() const;

private:
    static unsigned int next_id_;

    unsigned int id_;
    testnode_t *node_;
    std::vector<testnode_t::assignment_t> assigns_;
    int64_t start_;
    int64_t end_;
    std::string stdout_path_;
    std::string stderr_path_;
};

// close the namespace
};

#endif /* __NP_JOB_H__ */
