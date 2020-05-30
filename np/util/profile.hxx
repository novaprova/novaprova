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
#ifndef __NP_PROFILE_H__
#define __NP_PROFILE_H__ 1

namespace np {
namespace profile {

class probe_t
{
public:
    probe_t(const char *function);
    ~probe_t();

private:
    void log(const char *which);

    const char *function_;
    static int depth_;
    static int log_fd_;
};

#if 1
#define PROFILE
#else
#define PROFILE np::profile::probe_t __np_profile_probe(__FUNCTION__);
#endif

// close the namespace
}; };

#endif /* __NP_PROFILE_H__ */
