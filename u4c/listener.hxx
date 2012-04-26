#ifndef __U4C_LISTENER_H__
#define __U4C_LISTENER_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"


namespace u4c {

class event_t;
class job_t;

class listener_t
{
public:
    listener_t() {}
    ~listener_t() {}

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void begin_job(const job_t *) = 0;
    virtual void end_job(const job_t *) = 0;
    virtual void add_event(const event_t *) = 0;
    virtual void finished(result_t) = 0;
};

// close the namespace
};

#endif /* __U4C_LISTENER_H__ */
