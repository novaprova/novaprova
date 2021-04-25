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
#include "np/util/common.hxx"
#include <sys/stat.h>
#include <libxml/tree.h>
#include "np/junit_listener.hxx"
#include "np/job.hxx"
#include "except.h"
#include "np/util/log.hxx"

namespace np {
using namespace std;
using namespace np::util;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
junit_listener_t::needs_stdout() const
{
    return true;
}

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

// We're using the C libxml2 library because the C++ wrapper for
// it is not available on some platforms we want to support,
// such as RHEL6.
//
// For some insane reason of its own, the libxml2 library takes
// all constant string arguments as const xmlChar *, which is
// not, repeat not, the same as const char *.  So we have to
// cast EVERY SINGLE STRING ARGUMENT to libxml library calls.
#define s(x) ((const xmlChar *)(const char *)(x))
#define ss(x) ((const xmlChar *)(x).c_str())

void
junit_listener_t::end()
{
    string hostname = get_hostname();
    string timestamp = abs_format_iso8601(abs_now(), NP_SECONDS);

    // TODO: mkdir_p
    string directory = "reports";
    int r = mkdir(directory.c_str(), 0777);
    if (r < 0 && errno != EEXIST)
    {
	eprintf("cannot make directory %s: %s\n",
		directory.c_str(), strerror(errno));
	return;
    }

    map<string, suite_t>::iterator sitr;
    for (sitr = suites_.begin() ; sitr != suites_.end() ; ++sitr)
    {
	const string &suitename = sitr->first;
	suite_t *suite = &sitr->second;

	xmlDoc *xdoc = xmlNewDoc(s("1.0"));
	// If only there were a standard DTD URL...
	// instead we have a Schema from
	// http://windyroad.org/dl/OpenSource/JUnit.xsd

	xmlNode *xsuite = xmlNewNode(NULL, s("testsuite"));
	xmlDocSetRootElement(xdoc, xsuite);
	xmlNewProp(xsuite, s("name"), ss(suitename));
	xmlNewProp(xsuite, s("failures"), s("0"));
	xmlNewProp(xsuite, s("tests"), ss(dec(suite->cases_.size())));
	xmlNewProp(xsuite, s("hostname"), ss(hostname));
	xmlNewProp(xsuite, s("timestamp"), ss(timestamp));

	xmlAddChild(xsuite, xmlNewNode(NULL, s("properties")));

	unsigned int nerrs = 0;
	int64_t sns = 0;
	map<string, case_t>::iterator citr;
	string all_stdout;
	string all_stderr;
	for (citr = suite->cases_.begin() ; citr != suite->cases_.end() ; ++citr)
	{
	    const string &casename = citr->first;
	    case_t *c = &citr->second;

	    xmlNode *xcase = xmlAddChild(xsuite, xmlNewNode(NULL, s("testcase")));
	    xmlNewProp(xcase, s("name"), ss(casename));
	    // TODO: this is wrong
	    xmlNewProp(xcase, s("classname"), ss(casename));

	    sns += c->elapsed_;
	    xmlNewProp(xcase, s("time"), ss(rel_format(c->elapsed_)));

	    if (c->event_)
	    {
		event_t *e = c->event_;
		xmlNode *xerror = xmlAddChild(xcase, xmlNewNode(NULL, s("error")));
		xmlNewProp(xerror, s("type"), ss(e->which_as_string()));
		xmlNewProp(xerror, s("message"), s(e->description));
		xmlAddChild(xerror, xmlNewText(ss(e->as_string() +
				       "\n" +
				       e->get_long_location())));
	    }
	    if (c->result_ == R_FAIL)
		nerrs++;

	    if (c->stdout_ != "")
	    {
		all_stdout += string("===") + casename + string("===\n");
		all_stdout += c->stdout_;
	    }
	    if (c->stderr_ != "")
	    {
		all_stderr += string("===") + casename + string("===\n");
		all_stderr += c->stderr_;
	    }
	}
	xmlNewProp(xsuite, s("errors"), ss(dec(nerrs)));
	xmlNewProp(xsuite, s("time"), ss(rel_format(sns)));

	xmlAddChild(xmlAddChild(xsuite, xmlNewNode(NULL, s("system-out"))), xmlNewText(ss(all_stdout)));
	xmlAddChild(xmlAddChild(xsuite, xmlNewNode(NULL, s("system-err"))), xmlNewText(ss(all_stderr)));

	string filename = directory + string("/TEST-") + suitename + ".xml";

	int r = xmlSaveFileEnc(filename.c_str(), xdoc, "UTF-8");
	if (r < 0)
	{
	    eprintf("failed to write JUnit XML file %s: %s\n",
		    filename.c_str(), strerror(errno));
	}
	xmlFreeDoc(xdoc);
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
    c->elapsed_ = j->get_elapsed();
    c->stdout_ = j->get_stdout();
    c->stderr_ = j->get_stderr();
}

void
junit_listener_t::add_event(const job_t *j, const event_t *ev)
{
    case_t *c = find_case(j);
    if (ev->get_result() == R_FAIL && !c->event_)
	c->event_ = ev->clone();
}

junit_listener_t::case_t::~case_t()
{
    delete event_;
}

// close the namespace
};
