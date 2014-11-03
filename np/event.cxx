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
#include "np/event.hxx"

namespace np {
using namespace std;
using namespace np::util;

event_t &event_t::with_stack()
{
    string trace = np::spiegel::describe_stacktrace();
    if (trace.length())
    {
	/* only clobber `function' if we have something better */
	locflags &= ~LT__function;
	locflags |= LT_STACK;

	/* It's really really annoying that we have to do this,
	 * but it avoids Valgrind complaining about the stack
	 * trace string being potentially leaked.
	 * TODO: work out why the hell Valgrind complains about
	 * this string and not about the ones in normalise() */
	static char tracebuf[2048];
	strncpy(tracebuf, trace.c_str(), sizeof(tracebuf));
	memcpy(tracebuf+sizeof(tracebuf)-5, "...\0", 4);
	function = tracebuf;;
    }
    return *this;
}

event_t *
event_t::clone() const
{
    event_t *e = new event_t;
    *e = *this;
    e->save_strings();
    return e;
}

void
event_t::save_strings()
{
    size_t len = 0;
    if (description)
	len += strlen(description)+1;
    if (filename)
	len += strlen(filename)+1;
    if (function && !(locflags & LT_SPIEGELFUNC))
	len += strlen(function)+1;

    char *p = freeme_ = (char *)np::util::xmalloc(len);
    if (description)
    {
	strcpy(p, description);
	description = p;
	p += strlen(p)+1;
    }
    if (filename)
    {
	strcpy(p, filename);
	filename = p;
	p += strlen(p)+1;
    }
    if (function && !(locflags & LT_SPIEGELFUNC))
    {
	strcpy(p, function);
	function = p;
	p += strlen(p)+1;
    }
}

const event_t *
event_t::normalise() const
{
    static event_t norm;
    static string filebuf;
    static string funcbuf;

    memset(&norm, 0, sizeof(norm));
    norm.which = which;
    norm.description = xstr(description);

    if (locflags & LT_FUNCTYPE)
	norm.in_functype(functype);

    if (locflags & LT_SPIEGELFUNC)
    {
	np::spiegel::function_t *f = (np::spiegel::function_t *)function;

	filebuf = f->get_compile_unit()->get_absolute_path();
	norm.in_file(filebuf);

	funcbuf = f->get_name();
	norm.in_function(funcbuf);
    }
    else
    {
	if ((locflags & (LT_FILENAME|LT_LINENO)) == (LT_FILENAME|LT_LINENO))
	    norm.at_line(xstr(filename), lineno);
	else if (locflags & LT_FILENAME)
	    norm.in_file(xstr(filename));

	if (locflags & LT_FUNCNAME)
	    norm.in_function(xstr(function));
	if (locflags & LT_STACK)
	{
	    norm.locflags |= LT_STACK;
	    norm.function = xstr(function);
	}
    }

    return &norm;
}

result_t
event_t::get_result() const
{
    switch (which)
    {
    case EV_ASSERT:
    case EV_EXIT:
    case EV_SIGNAL:
    case EV_FIXTURE:
    case EV_VALGRIND:
    case EV_SLMATCH:
    case EV_TIMEOUT:
    case EV_FDLEAK:
    case EV_EXCEPTION:
	return R_FAIL;
    case EV_EXPASS:
	return R_PASS;
    case EV_EXFAIL:
	return R_FAIL;
    case EV_EXNA:
	return R_NOTAPPLICABLE;
    default:
	/* there was an event, but it makes no difference */
	return R_UNKNOWN;
    }
}

#define arraysize(x) sizeof(x)/sizeof(x[0])

string
event_t::which_as_string() const
{
    static const char * const whichstrs[] =
    {
	"NONE", "ASSERT", "EXIT", "SIGNAL",
	"SYSLOG", "FIXTURE", "EXPASS", "EXFAIL",
	"EXNA", "VALGRIND", "SLMATCH", "TIMEOUT",
	"FDLEAK", "EXCEPTION"
    };
    const char *wstr = ((unsigned)which < arraysize(whichstrs))
			? whichstrs[(unsigned)which] : "unknown";
    return string(wstr);
}

string
event_t::as_string() const
{
    return which_as_string() + " " + description;
}

string event_t::get_short_location() const
{
    string s = "";

    if (locflags & LT_FILENAME)
    {
	s += " at ";
	s += filename;
	s += ":";
	if (locflags & LT_LINENO)
	    s += dec(lineno);
	else
	    s += "???";
    }

    if (locflags & LT_FUNCNAME)
    {
	s += " in ";
	if (locflags & LT_FUNCTYPE)
	{
	    s += "(";
	    s += np::as_string((functype_t)functype);
	    s += ") ";
	}
	s += function;
    }

    return s;
}

string event_t::get_long_location() const
{
    if (locflags & LT_STACK)
	return string(function);
    return get_short_location() + "\n";
}


// close the namespace
};
