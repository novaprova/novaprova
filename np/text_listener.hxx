#ifndef __NP_TEXT_LISTENER_H__
#define __NP_TEXT_LISTENER_H__ 1

#include "np/listener.hxx"

namespace np {

class text_listener_t : public listener_t
{
public:
    text_listener_t() {}
    ~text_listener_t() {}

    void begin();
    void end();
    void begin_job(const job_t *);
    void end_job(const job_t *, result_t);
    void add_event(const job_t *, const event_t *ev);

private:
    unsigned int nrun_;
    unsigned int nfailed_;
};

// close the namespace
};

#endif /* __NP_TEXT_LISTENER_H__ */
