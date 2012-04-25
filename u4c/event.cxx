#include "u4c/event.hxx"

namespace u4c {
using namespace std;

event_t &event_t::with_stack()
{
    string trace = spiegel::describe_stacktrace();
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
	spiegel::function_t *f = (spiegel::function_t *)function;

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
event_t::as_string() const
{
    static const char * const whichstrs[] =
    {
	"NONE", "ASSERT", "EXIT", "SIGNAL",
	"SYSLOG", "FIXTURE", "EXPASS", "EXFAIL",
	"EXNA", "VALGRIND", "SLMATCH"
    };
    const char *wstr = ((unsigned)which < arraysize(whichstrs))
			? whichstrs[(unsigned)which] : "unknown";

    return string(wstr) + " " + description;
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
	    s += u4c::as_string((functype_t)functype);
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
