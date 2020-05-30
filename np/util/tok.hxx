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
/*
 * Copied from code (all by the same author) and relicenced.
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2003 Greg Banks <gnb@users.sourceforge.net>
 * copied from
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 */

#ifndef __spiegel_tok_hxx__
#define __spiegel_tok_hxx__ 1

#include "np/util/common.hxx"

namespace np { namespace util {

/*
 * A simple re-entrant string tokenizer, wraps strtok_r().
 */
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

class tok_t
{
public:
    /* ctors */
    tok_t(const char *str, const char *sep = 0);    /* const=>copy */
    tok_t(char *str, const char *sep = 0);  	    /* nonconst=>don't copy */
    /* dtor */
    ~tok_t();

    /*
     * Returns next token or 0 when last token reached.
     */
    const char *next();

private:
    void init(char *str, const char *sep);

    bool first_;
    char *buf_;
    char *state_;
    const char *sep_;
    bool buf_is_ours_;
};

// close the namespaces
}; };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
#endif /* __spiegel_tok_hxx__ */
