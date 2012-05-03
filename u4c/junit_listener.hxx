#ifndef __U4C_JUNIT_LISTENER_H__
#define __U4C_JUNIT_LISTENER_H__ 1

#include "u4c/listener.hxx"

namespace u4c {

class junit_listener_t : public listener_t
{
public:
    junit_listener_t() {}
    ~junit_listener_t() {}

    // TODO: methods to allow changing the base directory
    // TODO: capture job stdout/stderr

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *);

private:
    struct case_t
    {
	std::string name_;
	result_t result_;
	event_t *event_;
	int64_t start_ns_;
	int64_t end_ns_;
    };

    struct suite_t
    {
	std::map<std::string, case_t> cases_;
    };

    std::string get_hostname() const;
    std::string get_timestamp() const;
    case_t *find_case(const job_t *j);

    std::map<std::string, suite_t> suites_;
};

// close the namespace
};

#endif /* __U4C_JUNIT_LISTENER_H__ */
