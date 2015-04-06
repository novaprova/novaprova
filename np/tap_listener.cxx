/*
 * Copyright 2011-2015 Gregory Banks
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
#include "np/tap_listener.hxx"
#include "np/job.hxx"
#include "np/util/tok.hxx"
#include "except.h"

namespace np {
using namespace std;
using namespace np::util;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
tap_listener_t::needs_stdout() const
{
    return true;
}

tap_listener_t::run_t *
tap_listener_t::find_run(const job_t *j)
{
    return &runs_[j->as_string()];
}

void
tap_listener_t::begin()
{
}

void
tap_listener_t::end()
{
    // emit the plan line
    printf("1..%d\n", last_number_);
}

void
tap_listener_t::begin_job(const job_t *j __attribute__((unused)))
{
}

void
tap_listener_t::emit_diagnostics(const string &s)
{
    tok_t tokenizer((const char *)s.c_str(), "\n");
    while (const char *line = tokenizer.next())
    {
	fputs("# ", stdout);
	fputs(line, stdout);
	fputs("\n", stdout);
    }
}

void
tap_listener_t::end_job(const job_t *j, result_t res)
{
    // the name should not contain any '#' characters
    string nm = j->as_string();
    unsigned number = ++last_number_;

    emit_diagnostics(j->get_stdout());
    emit_diagnostics(j->get_stderr());
    run_t *run = find_run(j);
    if (run->event_ != "")
	emit_diagnostics(run->event_);

    switch (res)
    {
    case R_PASS:
	printf("ok %d %s\n", number, nm.c_str());
	break;
    case R_NOTAPPLICABLE:
	printf("ok %d %s # SKIP skipped\n", number, nm.c_str());
	break;
    case R_FAIL:
	printf("not ok %d %s\n", number, nm.c_str());
	break;
    default:
	printf("not ok %d %s (unexpected result %d)\n", number, nm.c_str(), res);
	break;
    }
}

void
tap_listener_t::add_event(const job_t *j, const event_t *ev)
{
    find_run(j)->event_ +=
	string("EVENT ") +
	ev->as_string() +
	"\n" +
	ev->get_long_location() +
	"\n";
}

// close the namespace
};
