#ifndef __libspiegel_platform_common_hxx__
#define __libspiegel_platform_common_hxx__ 1

#include <vector>

namespace spiegel { namespace platform {

extern char *self_exe();

struct linkobj_t
{
    const char *name;
    unsigned long addr;
    unsigned long size;
};
extern std::vector<linkobj_t> self_linkobjs();

// extern spiegel::value_t invoke(void *fnaddr, vector<spiegel::value_t> args);

extern int text_map_writable(addr_t addr, size_t len);
extern int text_restore(addr_t addr, size_t len);

struct intercept_t
{
    static void *operator new(size_t sz) { return xmalloc(sz); }
    static void operator delete(void *x) { free(x); }

    intercept_t(addr_t a) : addr_(a), runtime_(0) {}
    virtual ~intercept_t() {}

    virtual void before() = 0;
    virtual void after() = 0;

    addr_t get_address() const { return addr_; }

    int install();
    int uninstall();
    static intercept_t *find_installed(addr_t);

    struct runtime_t
    {
	unsigned long *args;
	addr_t redirect;
	unsigned long retval;
	bool skip;
    };
    void call_before(runtime_t *rt)
    {
	runtime_ = rt;
	before();
    }
    void call_after(runtime_t *rt)
    {
	runtime_ = rt;
	after();
    }

private:
    static std::vector<intercept_t*> installed_;

    /* saved parameters */
    addr_t addr_;

    /* Pointer to data which exists only for the lifetime of the
     * before and after methods */
    runtime_t *runtime_;

protected:
    /*
     * The before() and after() methods can examine the arguments to
     * the original function with get_arg().
     */
    unsigned long get_arg(unsigned int i) const { return runtime_->args[i]; }
    /*
     * The before() method can modify the arguments to the original
     * function, before the original function sees them, with set_arg().
     */
    void set_arg(unsigned int i, unsigned long v) { runtime_->args[i] = v; }
    /*
     * The before() method can call skip() to prevent executing the
     * original function.  The given @rv is used as the return value and
     * the after() method is not called.
     */
    void skip(unsigned long rv)
    {
	runtime_->retval = rv;
	runtime_->skip = true;
    }
    /*
     * The before() method can cause a given function to be called
     * instead of the original function with redirect().
     */
    void redirect(addr_t f) { runtime_->redirect = f; }
    /*
     * The after() method can examine the return value from the
     * original function with get_retval().
     */
    unsigned long get_retval() const { return runtime_->retval; }
    /*
     * The after() function can modify the return value from the
     * original function, before the caller sees it, with set_retval().
     */
    void set_retval(unsigned long rv) { runtime_->retval = rv; }
};


// close namespaces
} }

#endif // __libspiegel_platform_common_hxx__
