#ifndef __U4C_REDIRECT_H__
#define __U4C_REDIRECT_H__ 1

#include "u4c/common.hxx"
#include "u4c/types.hxx"
#include "spiegel/spiegel.hxx"

namespace u4c {

class redirect_t : public spiegel::intercept_t
{
public:
    redirect_t(spiegel::addr_t from, spiegel::addr_t to)
     :  intercept_t(from),
        to_(to)
    {}
    ~redirect_t() {}

    void before(spiegel::call_t &call)
    {
	call.redirect(to_);
    }
    void after(spiegel::call_t &call __attribute__((unused))) {}

private:
    spiegel::addr_t to_;
};

// close the namespace
};

#endif /* __U4C_REDIRECT_H__ */

