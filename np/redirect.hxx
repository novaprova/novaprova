#ifndef __NP_REDIRECT_H__
#define __NP_REDIRECT_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include "np/spiegel/spiegel.hxx"

namespace np {

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

#endif /* __NP_REDIRECT_H__ */

