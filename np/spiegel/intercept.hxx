/*
 * Copyright 2011-2012 Gregory Banks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __np_spiegel_intercept_hxx__
#define __np_spiegel_intercept_hxx__ 1

#include "np/spiegel/common.hxx"
#include "np/spiegel/platform/common.hxx"

namespace np {
namespace spiegel {

struct call_t
{
protected:
    call_t()
     : redirect_(0),
       retval_(0),
       skip_(false)
    {}

    addr_t redirect_;
    unsigned long retval_;
    bool skip_;

public:
    /*
     * The before() and after() methods can examine the arguments to
     * the original function with get_arg().
     */
    virtual unsigned long get_arg(unsigned int i) const = 0;
    /*
     * The before() method can modify the arguments to the original
     * function, before the original function sees them, with set_arg().
     */
    virtual void set_arg(unsigned int i, unsigned long v) = 0;
    /*
     * The before() method can call skip() to prevent executing the
     * original function.  The given @rv is used as the return value and
     * the after() method is not called.
     */
    virtual void skip(unsigned long rv)
    {
	retval_ = rv;
	skip_ = true;
    }
    /*
     * The before() method can cause a given function to be called
     * instead of the original function with redirect().
     */
    virtual void redirect(addr_t f) { redirect_ = f; }
    /*
     * The after() method can examine the return value from the
     * original function with get_retval().
     */
    virtual unsigned long get_retval() const { return retval_; }
    virtual unsigned long get_retval32() const { return retval_; }
    virtual uint64_t get_retval64() const { return (uint64_t)retval_; }
    /*
     * The after() function can modify the return value from the
     * original function, before the caller sees it, with set_retval().
     */
    virtual void set_retval(unsigned long rv) { retval_ = rv; }
};

struct intercept_t : public np::util::zalloc
{
    intercept_t(addr_t a, const char *name = 0);
    virtual ~intercept_t();

    virtual void before(call_t &) = 0;
    virtual void after(call_t &) = 0;

    addr_t get_address() const { return addr_; }
    const char *get_name() const { return (name_ ? name_ : "(unknown)"); }

    int install();
    int uninstall();

    // functions for the platform-specific intercept code
    static bool is_intercepted(addr_t);
    static np::spiegel::platform::intstate_t *get_intstate(addr_t);
    static void dispatch_before(addr_t, call_t &);
    static void dispatch_after(addr_t, call_t &);

private:
    struct addrstate_t
    {
	np::spiegel::platform::intstate_t state_;
	std::vector<intercept_t*> intercepts_;
    };

    static std::map<addr_t, addrstate_t> installed_;
    static addrstate_t *get_addrstate(addr_t addr, bool create);
    static void remove_addrstate(addr_t addr);

    /* saved parameters */
    addr_t addr_;
    char *name_;	/* may be NULL */
};

// close the namespaces
}; };

#endif // __np_spiegel_intercept_hxx__
