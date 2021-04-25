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
#ifndef __NP_REDIRECT_H__
#define __NP_REDIRECT_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include "np/spiegel/spiegel.hxx"

namespace np {

class redirect_t : public spiegel::intercept_t
{
public:
    redirect_t(spiegel::addr_t from, const char *fromname, spiegel::addr_t to)
     :  intercept_t(from, fromname),
	to_(to)
    {}
    ~redirect_t() {}

    void before(spiegel::call_t &call)
    {
	call.redirect(to_);
    }
    void after(spiegel::call_t &call __attribute__((unused))) {}

private:
    spiegel::addr_t to_;
};

// close the namespace
};

#endif /* __NP_REDIRECT_H__ */

