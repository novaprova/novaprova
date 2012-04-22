#include "u4c/event.hxx"

namespace u4c {
using namespace std;

string hex(unsigned long x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%lx", x);
    return string(buf);
}

string dec(unsigned int x)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", x);
    return string(buf);
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

    if (locflags & LT_CALLER)
    {
	unsigned long pc = (unsigned long)function;
	spiegel::location_t loc;

	if (spiegel::describe_address(pc, loc))
	{
	    filebuf = loc.compile_unit_->get_absolute_path();
	    norm.at_line(filebuf, loc.line_);

	    if (loc.class_)
		funcbuf = loc.class_->get_name() +
			  "::" +
			  loc.function_->get_name();
	    else
		funcbuf = loc.function_->get_name();
	    norm.in_function(funcbuf);
	}
	else
	{
	    funcbuf = "(" + hex(pc) + ")";
	    norm.in_function(funcbuf);
	}
    }
    else if (locflags & LT_SPIEGELFUNC)
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

    string s = string("EVENT ") + wstr + " " + description;

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

// close the namespace
};
