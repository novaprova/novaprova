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
#include <libintl.h>
#include "fw.h"

using namespace std;
using namespace np::util;

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

    if (is_verbose()) printf("Start of the_function, x=%d y=%d\n", x, y);
    the_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 5;
	y--;
    }
    if (is_verbose()) printf("End of the_function, returning %d\n", y);
    return y;
}

static int another_function_count = 0;

int
another_function(int x, int y)
{
    int i;

    if (is_verbose()) printf("Start of another_function, x=%d y=%d\n", x, y);
    another_function_count++;
    for (i = 0 ; i < x ; i++)
    {
	y *= 2;
	y++;
    }
    if (is_verbose()) printf("End of another_function, returning %d\n", y);
    return y;
}

class intercept_tester_t : public np::spiegel::intercept_t
{
public:
    intercept_tester_t();
    ~intercept_tester_t();

    void before(np::spiegel::call_t &);
    void after(np::spiegel::call_t &);

    unsigned int after_count;
    unsigned int before_count;
    int x, y, r;
    bool test_skip;
    bool test_redirect;
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
    if (is_verbose()) printf("BEFORE x=%d y=%d\n", x, y);
    if (test_skip)
    {
	if (is_verbose()) printf("SKIPPING r=%d\n", r);
	call.skip(r);
    }
    if (test_redirect)
    {
	if (is_verbose()) printf("REDIRECTING\n");
	call.redirect((np::spiegel::addr_t)&another_function);
    }
}

void
intercept_tester_t::after(np::spiegel::call_t &call)
{
    r = call.get_retval();
    after_count++;
    if (is_verbose()) printf("AFTER, returning %d\n", r);
}

int moe(int x, int y)
{
    int r = the_function(x+1, y+1);
    return r-1;
}

int curly(int x, int y)
{
    int r = moe(x+1, y+1);
    return r-1;
}

int larry(int x, int y)
{
    int r = curly(x+1, y+1);
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
	if (is_verbose()) printf("BEFORE\n");
    }
    void after(np::spiegel::call_t &call)
    {
	r = call.get_retval();
	if (is_verbose()) printf("AFTER, returning %d\n", r);
    }
};

class plt_intercept_tester_t : public np::spiegel::intercept_t
{
public:
    plt_intercept_tester_t()
     :  intercept_t((np::spiegel::addr_t)&gettext)
    {
    }
    ~plt_intercept_tester_t()
    {
    }

    unsigned int after_count;
    unsigned int before_count;

    void before(np::spiegel::call_t &call)
    {
	if (is_verbose()) printf("BEFORE, arg0 \"%s\"\n", (const char *)call.get_arg(0));
	before_count++;
    }
    void after(np::spiegel::call_t &call)
    {
	const char *s = (const char *)call.get_retval();
	if (is_verbose()) printf("AFTER, returning \"%s\"\n", s);
	after_count++;
	call.set_retval((unsigned long)"world");
    }
};

int
main(int argc, char **argv __attribute__((unused)))
{
#if 0
    top_of_stack = (unsigned long)&argc;
#endif
    if (argc > 1)
    {
	fatal("Usage: testrunner intercept\n");
    }

    np::spiegel::dwarf::state_t state;
    if (!state.add_self())
	return 1;

    intercept_tester_t *it;
    int r;

    BEGIN("basic");
    it = new intercept_tester_t();
    it->install();

    CHECK(it->x == 0);
    CHECK(it->y == 0);
    CHECK(it->r == 0);
    CHECK(it->before_count == 0);
    CHECK(it->after_count == 0);
    CHECK(the_function_count == 0);
    CHECK(another_function_count == 0);

    r = larry(0, 39);

    CHECK(r == 5216);
    CHECK(it->x == 3);
    CHECK(it->y == 42);
    CHECK(it->r == 5219);
    CHECK(it->before_count == 1);
    CHECK(it->after_count == 1);
    CHECK(the_function_count == 1);
    CHECK(another_function_count == 0);

    r = larry(4, 24);

    CHECK(r == 2089841);
    CHECK(it->x == 7);
    CHECK(it->y == 27);
    CHECK(it->r == 2089844);
    CHECK(it->before_count == 2);
    CHECK(it->after_count == 2);
    CHECK(the_function_count == 2);
    CHECK(another_function_count == 0);
    END;

    BEGIN("skip");
    it->test_skip = true;
    it->r = 327;
    r = larry(1, 53);
    it->test_skip = false;

    CHECK(r == 324);
    CHECK(it->x == 4);
    CHECK(it->y == 56);
    CHECK(it->r == 327);
    CHECK(it->before_count == 3);
    CHECK(it->after_count == 2);
    CHECK(the_function_count == 2);
    CHECK(another_function_count == 0);
    END;

    BEGIN("redirect");
    it->test_redirect = true;
    r = larry(0, 39);
    it->test_redirect = false;

    CHECK(r == 340);
    CHECK(it->x == 3);
    CHECK(it->y == 42);
    CHECK(it->r == 343);
    CHECK(it->before_count == 4);
    CHECK(it->after_count == 3);
    CHECK(the_function_count == 2);
    CHECK(another_function_count == 1);
    END;

    intercept_tester_t *it2;
    BEGIN("two intercepts");
    it2 = new intercept_tester_t();
    it2->install();

    r = larry(5, 23);

    CHECK(r == 10058591);
    CHECK(it->x == 8);
    CHECK(it->y == 26);
    CHECK(it->r == 10058594);
    CHECK(it->before_count == 5);
    CHECK(it->after_count == 4);
    CHECK(it2->x == 8);
    CHECK(it2->y == 26);
    CHECK(it2->r == 10058594);
    CHECK(it2->before_count == 1);
    CHECK(it2->after_count == 1);
    CHECK(the_function_count == 3);
    CHECK(another_function_count == 1);
    END;

    BEGIN("uninstall");
    it2->uninstall();
    delete it2;

    r = larry(7, 11);

    CHECK(r == 134277341);
    CHECK(it->x == 10);
    CHECK(it->y == 14);
    CHECK(it->r == 134277344);
    CHECK(it->before_count == 6);
    CHECK(it->after_count == 5);
    CHECK(the_function_count == 4);
    CHECK(another_function_count == 1);
    END;

//     fprintf(stderr, "About to SEGV\n");
//     *(char *)0 = 0;
//     fprintf(stderr, "Done doing SEGV\n");

    BEGIN("uninstall last");
    it->uninstall();
    delete it;

    r = larry(3, 7);

    CHECK(r == 152341);
    CHECK(the_function_count == 5);
    CHECK(another_function_count == 1);
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

    BEGIN("plt");
    plt_intercept_tester_t *it4 = new plt_intercept_tester_t();
    it4->install();
    CHECK(it4->before_count == 0);
    CHECK(it4->after_count == 0);
    const char *s = gettext("hello");
    CHECK(!strcmp(s, "world"));
    CHECK(it4->before_count == 1);
    CHECK(it4->after_count == 1);
    it4->uninstall();
    END;

    return 0;
}

