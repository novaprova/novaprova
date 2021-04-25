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
/*
 * Copied from code (all by the same author) and relicenced.
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2003 Greg Banks <gnb@users.sourceforge.net>
 * copied from
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 */

#include "np/util/tok.hxx"

namespace np { namespace util {

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
tok_t::init(char *str, const char *sep)
{
    first_ = true;
    buf_ = str;
    state_ = 0;
    sep_ = (sep == 0 ? " \t\r\n" : sep);
}

tok_t::tok_t(const char *str, const char *sep)
{
    init((str == 0 ? 0 : xstrdup(str)), sep);
    buf_is_ours_ = true;
}

tok_t::tok_t(char *str, const char *sep)
{
    init(str, sep);
    buf_is_ours_ = false;
}

tok_t::~tok_t()
{
    if (buf_is_ours_)
	free(buf_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

const char *
tok_t::next()
{
    if (!buf_)
	return 0;

    if (first_)
    {
	first_ = false;
	return strtok_r(buf_, sep_, &state_);
    }

    return strtok_r(0, sep_, &state_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
// close the namespaces
}; };
/*END*/
