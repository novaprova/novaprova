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
#ifndef __NP_EVENT_H__
#define __NP_EVENT_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include "np/spiegel/spiegel.hxx"
#include <string>

namespace np {

enum events_t
{
    EV_ASSERT = 1,	/* CuT failed an assert() */
    EV_EXIT,		/* CuT called exit() */
    EV_SIGNAL,		/* CuT caused a fatal signal */
    EV_SYSLOG,		/* CuT did a syslog() */
    EV_FIXTURE,		/* fixture code returned an error */
    EV_EXPASS,		/* CuT explicitly called NP_PASS */
    EV_EXFAIL,		/* ... */
    EV_EXNA,		/* ... */
    EV_VALGRIND,	/* Valgrind spotted a memleak or error */
    EV_SLMATCH,		/* syslog matching */
    EV_TIMEOUT,		/* child took too long */
    EV_FDLEAK,		/* file descriptor leak */
    EV_EXCEPTION,	/* C++ exception thrown */
};

class event_t
{
public:
    enum locflags_t
    {
	LT_NONE		= 0,
	LT_FILENAME	= (1<<0),   /* filename */
	LT_LINENO	= (1<<1),   /* lineno */
	LT_FUNCNAME	= (1<<2),   /* function */
	LT_SPIEGELFUNC	= (1<<3),   /* function */
	LT_STACK	= (1<<4),   /* function */
	LT_FUNCTYPE	= (1<<5),   /* functype */

	LT__function	= (LT_FUNCNAME|LT_SPIEGELFUNC|LT_STACK)
    };

    // default c'tor
    event_t()
     :  which((enum events_t)0),
	description(0),
	locflags(0),
	filename(0),
	lineno(0),
	function(0),
	functype(FT_UNKNOWN),
	freeme_(0)
    {}
    event_t(enum events_t w,
	    const char *d)
     :  which(w),
	description(d),
	locflags(0),
	filename(0),
	lineno(0),
	function(0),
	functype(FT_UNKNOWN),
	freeme_(0)
    {}
    ~event_t()
    {
	xfree(freeme_);
    }

    // auxiliary functions designed to enable complex
    // initialisation sequences without a profusion
    // of c'tors.
    event_t &at_line(const char *f, unsigned int l)
    {
	locflags |= LT_FILENAME|LT_LINENO;
	filename = f;
	lineno = l;
	return *this;
    }
    event_t &at_line(std::string f, unsigned int l)
    {
	return at_line(f.c_str(), l);
    }
    event_t &in_file(const char *f)
    {
	locflags |= LT_FILENAME;
	filename = f;
	return *this;
    }
    event_t &in_file(std::string f)
    {
	return in_file(f.c_str());
    }
    event_t &in_function(const char *fn)
    {
	locflags &= ~LT__function;
	locflags |= LT_FUNCNAME;
	function = fn;
	return *this;
    }
    event_t &in_function(std::string fn)
    {
	return in_function(fn.c_str());
    }
    event_t &in_function(const spiegel::function_t *fn)
    {
	locflags &= ~LT__function;
	locflags |= LT_SPIEGELFUNC;
	function = (const char *)fn;
	return *this;
    }
    event_t &in_functype(functype_t ft)
    {
	locflags |= LT_FUNCTYPE;
	functype = ft;
	return *this;
    }
    event_t &with_stack();

    event_t *clone() const;
    void normalise(const event_t *orig);
    result_t get_result() const;
    std::string which_as_string() const;
    std::string as_string() const;
    std::string get_short_location() const;
    std::string get_long_location() const;

// private:
// This needs to be public for {de,}serialise_event
    events_t which;
    const char *description;
    unsigned int locflags;
    const char *filename;
    unsigned int lineno;
    const char *function;
    functype_t functype;

private:

    void save_strings();
    char *freeme_;
};

// close the namespace
};

#endif /* __NP_EVENT_H__ */
