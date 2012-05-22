#ifndef __NP_LISTENER_H__
#define __NP_LISTENER_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"


namespace np {

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
    virtual void end_job(const job_t *, result_t) = 0;
    virtual void add_event(const job_t *, const event_t *) = 0;
};

// close the namespace
};

#endif /* __NP_LISTENER_H__ */
