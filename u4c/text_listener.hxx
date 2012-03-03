#ifndef __U4C_TEXT_LISTENER_H__
#define __U4C_TEXT_LISTENER_H__ 1

#include "u4c/listener.hxx"

namespace u4c {

class text_listener_t : public listener_t
{
public:
    text_listener_t() {}
    ~text_listener_t() {}

    void begin();
    void end();
    void begin_node(const testnode_t *tn);
    void end_node(const testnode_t *tn);
    void add_event(const u4c_event_t *ev, functype_t ft);
    void finished(result_t res);

private:
    unsigned int nrun_;
    unsigned int nfailed_;
    result_t result_; /* for the current test */
};

// close the namespace
};

#endif /* __U4C_TEXT_LISTENER_H__ */
