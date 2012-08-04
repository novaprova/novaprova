/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2002-2003 Greg Banks <gnb@users.sourceforge.net>
 * copied from
 * CANT - A C implementation of the Apache/Tomcat ANT build system
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
