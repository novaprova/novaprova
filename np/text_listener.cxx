#include "np/text_listener.hxx"
#include "np/job.hxx"
#include "except.h"

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
text_listener_t::add_event(const job_t *j __attribute__((unused)),
			   const event_t *ev)
{
    string s = string("EVENT ") +
		ev->as_string() +
	       "\nCalled from\n" +
	       ev->get_long_location() +
	       "\n";
    fputs(s.c_str(), stderr);
}

// close the namespace
};
