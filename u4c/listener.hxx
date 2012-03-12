#ifndef __U4C_LISTENER_H__
#define __U4C_LISTENER_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"

class u4c_event_t;

namespace u4c {

class testnode_t;

class listener_t
{
public:
    listener_t() {}
    ~listener_t() {}

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void begin_node(const testnode_t *) = 0;
    virtual void end_node(const testnode_t *) = 0;
    virtual void add_event(const u4c_event_t *, functype_t ft) = 0;
    virtual void finished(result_t) = 0;
};

// close the namespace
};

#endif /* __U4C_LISTENER_H__ */
