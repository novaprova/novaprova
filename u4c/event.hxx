#ifndef __U4C_EVENT_H__
#define __U4C_EVENT_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"
#include "spiegel/spiegel.hxx"
#include <string>

namespace u4c {

enum events_t
{
    EV_ASSERT = 1,	/* CuT failed an assert() */
    EV_EXIT,		/* CuT called exit() */
    EV_SIGNAL,		/* CuT caused a fatal signal */
    EV_SYSLOG,		/* CuT did a syslog() */
    EV_FIXTURE,		/* fixture code returned an error */
    EV_EXPASS,		/* CuT explicitly called U4C_PASS */
    EV_EXFAIL,		/* ... */
    EV_EXNA,		/* ... */
    EV_VALGRIND,	/* Valgrind spotted a memleak or error */
    EV_SLMATCH,		/* syslog matching */
};

class event_t
{
public:
    enum locflags_t
    {
	LT_NONE		= 0,
	LT_FILENAME	= (1<<0),
	LT_LINENO	= (1<<1),
	LT_FUNCNAME	= (1<<2),
	LT_SPIEGELFUNC	= (1<<3),
	LT_CALLER	= (1<<4),
	LT_FUNCTYPE	= (1<<5),
    };

    // default c'tor
    event_t()
     :  which((enum events_t)0),
        description(0),
	locflags(0),
        filename(0),
        lineno(0),
        function(0),
	functype(FT_UNKNOWN)
    {}
    event_t(enum events_t w,
	    const char *d)
     :  which(w),
        description(d),
	locflags(0),
        filename(0),
        lineno(0),
        function(0),
	functype(FT_UNKNOWN)
    {}

    // auxiliary functions designed to enable complex
    // initialisation sequences without a profusion
    // of c'tors.
    event_t &at_line(const char *f, unsigned int l)
    {
	locflags &= ~(LT_CALLER);
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
	locflags &= ~(LT_CALLER);
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
	locflags &= ~(LT_SPIEGELFUNC|LT_CALLER);
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
	locflags &= ~(LT_FUNCNAME|LT_CALLER|LT_LINENO);
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
    inline event_t &from_caller()
    {
	locflags &= ~(LT_FUNCNAME|LT_SPIEGELFUNC|LT_LINENO);
	locflags |= LT_CALLER;
        function = (const char *)__builtin_return_address(0);
	return *this;
    }

    const event_t *normalise() const;
    result_t get_result() const;
    std::string as_string() const;

// private:
// This needs to be public for {de,}serialise_event
    events_t which;
    const char *description;
    unsigned int locflags;
    const char *filename;
    unsigned int lineno;
    const char *function;
    functype_t functype;
};

// close the namespace
};

#endif /* __U4C_EVENT_H__ */
