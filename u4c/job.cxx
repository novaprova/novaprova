#include "u4c/job.hxx"

namespace u4c {
using namespace std;

unsigned int job_t::next_id_ = 1;

job_t::job_t(const plan_t::iterator &i)
 :  id_(next_id_++),
    node_(i.get_node()),
    assigns_(i.get_assignments())
{
}

job_t::~job_t()
{
}

string job_t::as_string() const
{
    string s = dec(id_) + ":" + node_->get_fullname();
    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	s += string("[") + i->as_string() + "]";
    return s;
}

void
job_t::pre_run(bool in_parent)
{
    if (in_parent)
    {
	start_ = rel_now();
	return;
    }

    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	i->apply();

    node_->pre_run();
}

void
job_t::post_run(bool in_parent)
{
    if (in_parent)
    {
	end_ = rel_now();
	return;
    }

    node_->post_run();

    vector<testnode_t::assignment_t>::const_iterator i;
    for (i = assigns_.begin() ; i != assigns_.end() ; ++i)
	i->unapply();
}

int64_t
job_t::get_elapsed() const
{
    int64_t end = end_;
    if (!end)
	end = rel_now();
    return end - start_;
}

// close the namespace
};
