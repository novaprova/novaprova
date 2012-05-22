#ifndef __NP_PROXY_LISTENER_H__
#define __NP_PROXY_LISTENER_H__ 1

#include "np/util/common.hxx"
#include "np/listener.hxx"

namespace np {

class proxy_listener_t : public listener_t
{
public:
    proxy_listener_t(int);
    ~proxy_listener_t();

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *ev);

    /* proxyl.c */
    static bool handle_call(int fd, job_t *, result_t *resp);

private:
    int fd_;
};

// close the namespace
};

#endif /* __NP_PROXY_LISTENER_H__ */
