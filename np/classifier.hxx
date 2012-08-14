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
#ifndef __NP_CLASSIFIER_H__
#define __NP_CLASSIFIER_H__ 1

#include "np/util/common.hxx"
#include <regex.h>

namespace np {

class classifier_t : public np::util::zalloc
{
public:
    classifier_t()
    {
	results_[0] = false;
	results_[1] = true;
    }
    ~classifier_t()
    {
	xfree(re_);
	regfree(&compiled_re_);
    }

    bool set_regexp(const char *, bool);
    void set_results(int failed, int matched)
    {
	results_[0] = failed;
	results_[1] = matched;
    }
    int classify(const char *, char *, size_t) const;
    const char *error_string() const;

private:
    char *re_;
    regex_t compiled_re_;
    int results_[2];
    int error_;
};

// close the namespace
};

#endif /* __NP_CLASSIFIER_H__ */

