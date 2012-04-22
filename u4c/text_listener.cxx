#include "u4c/text_listener.hxx"
#include "u4c/testnode.hxx"
#include "except.h"

namespace u4c {
using namespace std;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
text_listener_t::begin()
{
    nrun_ = 0;
    nfailed_ = 0;
    fprintf(stderr, "u4c: running\n");
}

void
text_listener_t::end()
{
    fprintf(stderr, "u4c: %u run %u failed\n",
	    nrun_, nfailed_);
}

void
text_listener_t::begin_node(const testnode_t *tn)
{
    fprintf(stderr, "u4c: running: \"%s\"\n", tn->get_fullname().c_str());
    result_ = u4c::R_UNKNOWN;
}

void
text_listener_t::end_node(const testnode_t *tn)
{
    string fullname = tn->get_fullname();

    nrun_++;
    switch (result_)
    {
    case R_PASS:
	fprintf(stderr, "PASS %s\n", fullname.c_str());
	break;
    case R_NOTAPPLICABLE:
	fprintf(stderr, "N/A %s\n", fullname.c_str());
	break;
    case R_FAIL:
	nfailed_++;
	fprintf(stderr, "FAIL %s\n", fullname.c_str());
	break;
    default:
	fprintf(stderr, "??? (result %d) %s\n", result_, fullname.c_str());
	break;
    }
}

void
text_listener_t::add_event(const event_t *ev)
{
    fputs(ev->as_string().c_str(), stderr);
    fputc('\n', stderr);
}

void
text_listener_t::finished(result_t res)
{
    result_ = res;
}

// close the namespace
};
