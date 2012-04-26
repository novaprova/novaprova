#ifndef __U4C_PROXY_LISTENER_H__
#define __U4C_PROXY_LISTENER_H__ 1

#include "u4c/common.hxx"
#include "u4c/listener.hxx"

namespace u4c {

class proxy_listener_t : public listener_t
{
public:
    proxy_listener_t(int);
    ~proxy_listener_t();

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *);
    void add_event(const event_t *ev);
    void finished(result_t res);

    /* proxyl.c */
    static bool handle_call(int fd, result_t *resp);

private:
    int fd_;
};

// close the namespace
};

#endif /* __U4C_PROXY_LISTENER_H__ */
