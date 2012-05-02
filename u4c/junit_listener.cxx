#include "u4c/common.hxx"
#include <libxml++/document.h>
#include "u4c/junit_listener.hxx"
#include "u4c/job.hxx"
#include "except.h"

namespace u4c {
using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
junit_listener_t::begin()
{
}

string
junit_listener_t::get_hostname() const
{
    char hostname[1024];
    int r = gethostname(hostname, sizeof(hostname));
    return (r < 0 ? string("localhost") : string(hostname));
}

string
junit_listener_t::get_timestamp() const
{
    time_t clock;
    struct tm tm;
    char buf[20];
    time(&clock);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S",
	     localtime_r(&clock, &tm));
    return string(buf);
}

void
junit_listener_t::end()
{
    string hostname = get_hostname();
    string timestamp = get_timestamp();

    map<string, suite_t>::iterator sitr;
    for (sitr = suites_.begin() ; sitr != suites_.end() ; ++sitr)
    {
	const string &suitename = sitr->first;
	suite_t *s = &sitr->second;

	xmlpp::Document *xdoc = new xmlpp::Document();
	// If only there were a standard DTD URL...
	// instead we have a Schema from
	// http://windyroad.org/dl/OpenSource/JUnit.xsd
	xmlpp::Element *xsuite = xdoc->create_root_node("testsuite");
	xsuite->set_attribute("name", suitename);
	xsuite->set_attribute("failures", "0");
	xsuite->set_attribute("time", "0.1");
	xsuite->set_attribute("tests", dec(s->cases_.size()));
	xsuite->set_attribute("hostname", hostname);
	xsuite->set_attribute("timestamp", timestamp);

	xsuite->add_child("properties");

	unsigned int nerrs = 0;
	map<string, case_t>::iterator citr;
	for (citr = s->cases_.begin() ; citr != s->cases_.end() ; ++citr)
	{
	    const string &casename = citr->first;
	    case_t *c = &citr->second;

	    xmlpp::Element *xcase = xsuite->add_child("testcase");
	    xcase->set_attribute("name", casename);
	    // TODO: this is wrong
	    xcase->set_attribute("classname", casename);
	    xcase->set_attribute("time", "0.1");

	    if (c->event_)
	    {
		event_t *e = c->event_;
		xmlpp::Element *xerror = xcase->add_child("error");
		xerror->set_attribute("type", e->which_as_string());
		xerror->set_attribute("message", e->description);
		xerror->add_child_text(e->as_string() +
				       "\n" +
				       e->get_long_location());
	    }
	    if (c->result_ == R_FAIL)
		nerrs++;
	}
	xsuite->set_attribute("errors", dec(nerrs));

	xsuite->add_child("system-out");
	xsuite->add_child("system-err");

	string filename = string("reports/TEST-") + suitename + ".xml";
	// TODO: mkdir 
	xdoc->write_to_file(filename, "UTF-8");
	delete xdoc;
    }
}

junit_listener_t::case_t *
junit_listener_t::find_case(const job_t *j)
{
    string suitename = j->get_node()->get_parent()->get_fullname();

    string jobname = j->as_string();
    int off = jobname.find(suitename);
    string casename(jobname, off+suitename.length()+1);

    return &suites_[suitename].cases_[casename];
}

void
junit_listener_t::begin_job(const job_t *j __attribute__((unused)))
{
}

void
junit_listener_t::end_job(const job_t *j, result_t res)
{
    case_t *c = find_case(j);
    c->result_ = res;
}

void
junit_listener_t::add_event(const job_t *j, const event_t *ev)
{
    case_t *c = find_case(j);
    if (ev->get_result() == R_FAIL && !c->event_)
	c->event_ = ev->clone();
}

// close the namespace
};
