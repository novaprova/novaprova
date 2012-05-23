#ifndef __libspiegel_intercept_hxx__
#define __libspiegel_intercept_hxx__ 1

#include "spiegel/common.hxx"

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
    /*
     * The after() function can modify the return value from the
     * original function, before the caller sees it, with set_retval().
     */
    virtual void set_retval(unsigned long rv) { retval_ = rv; }
};

struct intercept_t
{
    static void *operator new(size_t sz) { return np::util::xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    intercept_t(addr_t a);
    virtual ~intercept_t();

    virtual void before(call_t &) = 0;
    virtual void after(call_t &) = 0;

    addr_t get_address() const { return addr_; }

    int install();
    int uninstall();

    // functions for the platform-specific intercept code
    static bool is_intercepted(addr_t);
    static void dispatch_before(addr_t, call_t &);
    static void dispatch_after(addr_t, call_t &);

private:
    static std::map<addr_t, std::vector<intercept_t*> > installed_;

    /* saved parameters */
    addr_t addr_;
};

// close namespaces
}

#endif // __libspiegel_intercept_hxx__
