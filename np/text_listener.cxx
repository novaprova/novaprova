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
#include "np/text_listener.hxx"
#include "np/job.hxx"
#include "except.h"
#include <sstream>

namespace np {
using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
text_listener_t::begin()
{
    nrun_ = 0;
    nfailed_ = 0;
    fprintf(stderr, "np: running\n");
}

void
text_listener_t::end()
{
    fprintf(stderr, "np: %u run %u failed\n",
	    nrun_, nfailed_);
}

void
text_listener_t::begin_job(const job_t *j)
{
    fprintf(stderr, "np: running: \"%s\"\n", j->as_string().c_str());
}

void
text_listener_t::end_job(const job_t *j, result_t res)
{
    string nm = j->as_string();

    nrun_++;
    switch (res)
    {
    case R_PASS:
	fprintf(stderr, "PASS %s\n", nm.c_str());
	break;
    case R_NOTAPPLICABLE:
	fprintf(stderr, "N/A %s\n", nm.c_str());
	break;
    case R_FAIL:
	nfailed_++;
	fprintf(stderr, "FAIL %s\n", nm.c_str());
	break;
    default:
	fprintf(stderr, "??? (result %d) %s\n", res, nm.c_str());
	break;
    }
}

void
text_listener_t::add_event(const job_t *j, const event_t *ev)
{
    ostringstream ss;

    // generate a message useful for programs which scrape make
    // output for compiler messages, like vi's "make" command.
    string loc = ev->get_make_location();
    if (loc != "")
	ss << loc << ": " << ev->as_string() << " (" << j->as_string() << ")\n";
    else
	ss << "EVENT " << ev->as_string() << "\n";

    // append the long location information including stack trace
    ss << ev->get_long_location() << "\n";

    // find and annotate some source around the failure
    unsigned int ncontext = 5;
    const char *filename = ev->get_filename();
    unsigned int lineno = ev->get_lineno();
    if (filename && lineno)
    {
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	{
	    if (errno != ENOENT)
		ss << "Cannot open \"" << filename << "\" for reading: " <<strerror(errno) << "\n";
	}
	else
	{
	    unsigned int lno = 0;
	    char lnbuf[8];
	    char line[1024];
	    while (fgets(line, sizeof(line), fp))
	    {
		++lno;
		if (lno + ncontext < lineno || lno > lineno + ncontext)
		    continue;

		// trim trailing whitespace
		char *p = line+strlen(line)-1;
		while (p >= line && isspace(*p))
		    *p-- = '\0';

		snprintf(lnbuf, sizeof(lnbuf), "%4u ", lno);
		ss << "np: " << lnbuf << line;
		if (lno == lineno)
		    ss << " <-----";
		ss << "\n";
	    }
	    fclose(fp);
	}
    }

    fputs(ss.str().c_str(), stderr);
}

// close the namespace
};
