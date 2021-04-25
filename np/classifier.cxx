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

#include "classifier.hxx"
#include "np/util/log.hxx"

namespace np {

const char *
classifier_t::error_string() const
{
    static char buf[2048];

    if (!error_)
	return 0;

    int n = snprintf(buf, sizeof(buf)-100, "/%s/: ", re_);
    regerror(error_, &compiled_re_, buf+n, sizeof(buf)-n-1);

    return buf;
}

bool
classifier_t::set_regexp(const char *re, bool case_sensitive)
{
    re_ = np::util::xstrdup(re);
    error_ = regcomp(&compiled_re_, re,
		    REG_EXTENDED|(case_sensitive ? 0 : REG_ICASE));
    if (error_)
    {
	eprintf("bad classifier %s\n", error_string());
	return false;
    }
    return true;
}

int
classifier_t::classify(const char *func,
		       char *match_return,
		       size_t maxmatch) const
{
    regmatch_t match[2];
    int r = regexec(&compiled_re_, func, 2, match, 0);

    if (r == 0)
    {
	/* successful match */
	if (match_return && match[1].rm_so >= 0)
	{
	    size_t len = match[1].rm_eo - match[1].rm_so;
	    if (len >= maxmatch)
	    {
		eprintf("match for classifier %s too long\n", re_);
		return results_[0];
	    }
	    memcpy(match_return, func+match[1].rm_so, len);
	    match_return[len] = '\0';
	}
	return results_[1];
    }

    if (r != REG_NOMATCH)
    {
	/* some runtime error */
	eprintf("failed matching \"%s\": %s\n", func, error_string());
    }

    return results_[0];
}

// close the namespace
};
