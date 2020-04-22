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
#include "np/spiegel/spiegel.hxx"
#include "np/spiegel/dwarf/state.hxx"
#include "np/util/trace.h"
#include "np/util/log.hxx"
#include <libintl.h>
#include "fw.h"

using namespace std;
using namespace np::util;

#if _NP_ENABLE_TRACE
#define STACKME(ff) \
    { \
	unsigned long rsp; \
	unsigned long rbp; \
	__asm__ volatile("movq %%rsp, %0" : "=r"(rsp)); \
	__asm__ volatile("movq %%rbp, %0" : "=r"(rbp)); \
	np_trace(ff ": started %rsp="); \
	np_trace_hex(rsp); \
	np_trace(" %rbp="); \
	np_trace_hex(rbp); \
	np_trace(" ra="); \
	np_trace_hex((unsigned long)__builtin_return_address(0)); \
	np_trace("\n"); \
    }
#else
#define STACKME(ff)
#endif

#if 0
static unsigned long top_of_stack;

static void
__dump_stack(unsigned long ebp, unsigned long eip)
{
    unsigned int n = 0;
    while (ebp < top_of_stack)
    {
	printf("%u EBP 0x%08lx EIP 0x%08lx", n, ebp, eip);
	np::spiegel::location_t loc;
	if (np::spiegel::describe_address(eip, loc))
	    printf(" <%s+0x%x> at %s:%u",
		  loc.function_ ? loc.function_->get_name().c_str() : "???",
		  loc.offset_,
		  loc.compile_unit_->get_absolute_path().basename().c_str(),
		  loc.line_);
	printf("\n");
	eip = ((unsigned long *)ebp)[1];
	ebp = ((unsigned long *)ebp)[0];
	n++;
    }
}

#define DUMP_STACK \
    { \
	register unsigned long ebp; \
	register unsigned long eip; \
	__asm__ volatile("mov %%ebp,%0" : "=r"(ebp)); \
	__asm__ volatile("call .Lxx; .Lxx: popl %0" : "=r"(eip)); \
	__dump_stack(ebp, eip); \
    }
#endif

static int the_function_count = 0;

int
the_function(int x, int y)
{
    int i;

    STACKME("the_function");
    dprintf("Start of the_function, x=%d y=%d\n", x, y);
    the_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    dprintf("End of the_function, returning %d\n", y);
    return y;
}

static int another_function_count = 0;

int
another_function(int x, int y)
{
    int i;

    dprintf("Start of another_function, x=%d y=%d\n", x, y);
    another_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 2;
	y++;
    }
    dprintf("End of another_function, returning %d\n", y);
    return y;
}

class intercept_tester_t : public np::spiegel::intercept_t
{
public:
    intercept_tester_t();
    ~intercept_tester_t();

    void before(np::spiegel::call_t &);
    void after(np::spiegel::call_t &);
    void reset_counters();

    unsigned int after_count;
    unsigned int before_count;
    int x, y, r, nr;
    bool test_skip;
    bool test_redirect;
    bool test_set_retval;
};

intercept_tester_t::intercept_tester_t()
 :  intercept_t((np::spiegel::addr_t)&the_function)
    // we don't initialise the counts to 0 because the
    // inherited operator new does that.
{
}

intercept_tester_t::~intercept_tester_t()
{
}

void
intercept_tester_t::before(np::spiegel::call_t &call)
{
    x = call.get_arg(0);
    y = call.get_arg(1);
    before_count++;
    dprintf("BEFORE x=%d y=%d\n", x, y);
    if (test_skip)
    {
	dprintf("SKIPPING r=%d\n", nr);
	call.skip(nr);
    }
    if (test_redirect)
    {
	dprintf("REDIRECTING to another_function\n");
	call.redirect((np::spiegel::addr_t)&another_function);
    }
}

void
intercept_tester_t::after(np::spiegel::call_t &call)
{
    r = call.get_retval();
    after_count++;
    dprintf("AFTER, returned %d\n", r);
    if (test_set_retval)
    {
	dprintf("SETTING RETVAL to %d\n", nr);
	call.set_retval((unsigned long)nr);
    }
}

void
intercept_tester_t::reset_counters()
{
    x = 0;
    y = 0;
    r = 0;
    nr = 0;
    before_count = 0;
    after_count = 0;
    the_function_count = 0;
    another_function_count = 0;
    test_skip = false;
    test_redirect = false;
    test_set_retval = false;
}

int moe(int x, int y)
{
    STACKME("moe");
    dprintf("Start of moe, x=%d y=%d\n", x, y);
    int r = the_function(x+1, y+1);
    dprintf("End of moe, r=%d\n", r);
    return r-1;
}

int curly(int x, int y)
{
    STACKME("curly");
    dprintf("Start of curly, x=%d y=%d\n", x, y);
    int r = moe(x+1, y+1);
    dprintf("End of curly, r=%d\n", r);
    return r-1;
}

int larry(int x, int y)
{
    STACKME("larry");
    dprintf("Start of larry, x=%d y=%d\n", x, y);
    int r = curly(x+1, y+1);
    dprintf("End of larry, r=%d\n", r);
    return r-1;
}

int
wide_call(int a1, int a2, int a3,
	  int a4, int a5, int a6,
	  int a7, int a8, int a9,
	  int a10, int a11, int a12)
{
    return a1+a2+a3+a4+a5+a6+a7+a8+a9+a10+a11+a12;
}

class wide_intercept_tester_t : public np::spiegel::intercept_t
{
public:
    wide_intercept_tester_t()
     :  intercept_t((np::spiegel::addr_t)&wide_call)
    {
    }
    ~wide_intercept_tester_t()
    {
    }

    int x[12], r;

    void before(np::spiegel::call_t &call)
    {
	int i;
	for (i = 0 ; i < 12 ; i++)
	    x[i] = call.get_arg(i);
	dprintf("BEFORE\n");
    }
    void after(np::spiegel::call_t &call)
    {
	r = call.get_retval();
	dprintf("AFTER, returning %d\n", r);
    }
};

typedef void (*fn_t)(void);

class libc_intercept_tester_t : public np::spiegel::intercept_t
{
public:
    libc_intercept_tester_t(fn_t addr, const char *name)
     :  intercept_t((np::spiegel::addr_t)addr, name)
    {
    }
    ~libc_intercept_tester_t()
    {
    }

    unsigned int after_count;
    unsigned int before_count;

    void before(np::spiegel::call_t &call)
    {
	dprintf("BEFORE %s\n", get_name());
	before_count++;
    }
    void after(np::spiegel::call_t &call)
    {
	dprintf("AFTER %s\n", get_name());
	after_count++;
    }
};

int
main(int argc, char **argv __attribute__((unused)))
{
#if 0
    top_of_stack = (unsigned long)&argc;
#endif

    bool debug = false;
    if (argc == 2 && !strcmp(argv[1], "--debug"))
    {
        debug = true;
    }
    else if (argc > 1)
    {
	fatal("Usage: testrunner intercept\n");
    }
    np::log::basic_config(debug ? np::log::DEBUG : np::log::INFO, 0);
    if (is_verbose()) np_trace_init();
    STACKME("main");
    dprintf("Address of the_function is %p\n", the_function);

    dprintf("main, about to create state_t\n");
    np::spiegel::dwarf::state_t state;
    dprintf("main, about to call add_self\n");
    if (!state.add_self())
	return 1;

    intercept_tester_t *it;
    int r;

    BEGIN("self-check");
    CHECK(the_function(3, 42) == 5219);
    END;

    BEGIN("install");
    it = new intercept_tester_t();
    it->install();
    END;

    BEGIN("basic");
    it->reset_counters();
    r = larry(0, 39);

    CHECK(r == 5216);
    CHECK(it->x == 3);
    CHECK(it->y == 42);
    CHECK(it->r == 5219);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;

    BEGIN("basic2");
    it->reset_counters();
    r = larry(4, 24);

    CHECK(r == 2089841);
    CHECK(it->x == 7);
    CHECK(it->y == 27);
    CHECK(it->r == 2089844);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;

    BEGIN("skip");
    it->reset_counters();
    it->test_skip = true;
    it->nr = 327;
    r = larry(1, 53);

    CHECK(r == 324);
    CHECK(it->x == 4);
    CHECK(it->y == 56);
    CHECK(it->r == 0);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 0);
    CHECK(the_function_count == 0);
    CHECK(another_function_count == 0);
    END;

    BEGIN("redirect");
    it->reset_counters();
    it->test_redirect = true;
    r = larry(0, 39);

    CHECK(r == 340);
    CHECK(it->x == 3);
    CHECK(it->y == 42);
    CHECK(it->r == 343);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 0);
    CHECK(another_function_count == 1);
    END;

    BEGIN("set retval");
    it->reset_counters();
    it->test_set_retval = true;
    it->nr = 271828;
    r = larry(0, 39);

    CHECK(r == 271825);
    CHECK(it->x == 3);
    CHECK(it->y == 42);
    CHECK(it->r == 5219);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;

    intercept_tester_t *it2;
    BEGIN("two intercepts");
    it2 = new intercept_tester_t();
    it2->install();
    it->reset_counters();
    it2->reset_counters();

    r = larry(5, 23);

    CHECK(r == 10058591);
    CHECK(it->x == 8);
    CHECK(it->y == 26);
    CHECK(it->r == 10058594);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(it2->x == 8);
    CHECK(it2->y == 26);
    CHECK(it2->r == 10058594);
    CHECK(it2->before_count == 1);
    CHECK(it2->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;

    BEGIN("uninstall second");
    it2->uninstall();
    delete it2;

    it->reset_counters();
    r = larry(7, 11);

    CHECK(r == 134277341);
    CHECK(it->x == 10);
    CHECK(it->y == 14);
    CHECK(it->r == 134277344);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;

    BEGIN("uninstall last");
    it->reset_counters();
    it->uninstall();
    delete it;

    r = larry(3, 7);

    CHECK(r == 152341);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);
    END;


    BEGIN("larger stack frame");
    wide_intercept_tester_t *it3 = new wide_intercept_tester_t;
    it3->install();
    r = wide_call(1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144);
    CHECK(it3->x[0] == 1);
    CHECK(it3->x[1] == 4);
    CHECK(it3->x[2] == 9);
    CHECK(it3->x[3] == 16);
    CHECK(it3->x[4] == 25);
    CHECK(it3->x[5] == 36);
    CHECK(it3->x[6] == 49);
    CHECK(it3->x[7] == 64);
    CHECK(it3->x[8] == 81);
    CHECK(it3->x[9] == 100);
    CHECK(it3->x[10] == 121);
    CHECK(it3->x[11] == 144);
    CHECK(it3->r == 650);
    it3->uninstall();
    delete it3;
    END;

    /*
     * Test interception of functions in libc.  There are several issues:
     *
     * - the functions are in a shared library and are accessed via the PLT
     * - the code is heavily optimised which affects function prologues
     * - function prologues are more predictable on some ABIs
     *
     * So we try to cover all of these by testing specific functions
     * known to exhibit specific properties in one libc or another.
     *
     * On Darwin these functions are in the libintl shared library from
     * the "gettext" Homebrew keg.  It's not libc but it's a case that
     * needs to work.
     */
    const char *s;

    /* textdomain uses the 0x55 prologue on x86 and x86_64 */
    BEGIN("libc textdomain");
    libc_intercept_tester_t *it4 = new libc_intercept_tester_t((fn_t)textdomain, "textdomain");
    it4->install();
    CHECK(it4->before_count == 0);
    CHECK(it4->after_count == 0);
    const char *s = textdomain("locavore");
    CHECK(!strcmp(s, "locavore"));
    CHECK(it4->before_count == 1);
    CHECK(it4->after_count == 1);
    it4->uninstall();
    END;

    /* gettext doesn't use the 0x55 prologue on x86_64 */
    BEGIN("libc gettext");
    libc_intercept_tester_t *it5 = new libc_intercept_tester_t((fn_t)gettext, "gettext");
    it5->install();
    CHECK(it5->before_count == 0);
    CHECK(it5->after_count == 0);
    s = gettext("hello");
    CHECK(!strcmp(s, "hello"));
    CHECK(it5->before_count == 1);
    CHECK(it5->after_count == 1);
    it5->uninstall();
    END;

    /*
     * strncpy() starts with "mov %rdi,$r8" on 64b and with "push %edi"
     * on 32b.  The Darwin libc contains very few functions with oddball
     * prologs...maybe they don't optimise their libc?
     */
    BEGIN("libc strncpy");
    libc_intercept_tester_t *it6 = new libc_intercept_tester_t((fn_t)strncpy, "strncpy");
    it6->install();
    CHECK(it6->before_count == 0);
    CHECK(it6->after_count == 0);
    static char buf1[32] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    strncpy(buf1, "abc", sizeof(buf1));
    CHECK(!strcmp(buf1, "abc"));
    CHECK(it6->before_count == 1);
    CHECK(it6->after_count == 1);
    it6->uninstall();
    END;

    return 0;
}

