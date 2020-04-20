/*
 * Copyright 2011-2020 Gregory Banks
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
#include "fw.h"
#include "succulents.hxx"

using namespace std;
using namespace np::util;

void
microdosing(void)
{
    fprintf(stderr, "This function exists\n");
    fprintf(stderr, "Solely to have its address\n");
    fprintf(stderr, "Used to lookup DWARF info.\n");
}

static void __attribute__((used))
biodiesel(void)
{
    fprintf(stderr, "This function exists\n");
    fprintf(stderr, "Solely to have its address\n");
    fprintf(stderr, "Used to lookup DWARF info.\n");
}

#if defined(__x86_64__)
#define capture_pc(_pc) \
    __asm__ volatile("call 1f; 1: popq %0" : "=r"(_pc))
#endif

class semiotics_t
{
public:

    unsigned long gentrify()
    {
        fprintf(stderr, "XXX called %s\n", __FUNCTION__);
        unsigned long pc;
        capture_pc(pc);
        fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
        return pc;
    }

    virtual unsigned long flannel()
    {
        unsigned long pc;
        capture_pc(pc);
        fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
        return pc;
    }

    static unsigned long chicharrones()
    {
        unsigned long pc;
        capture_pc(pc);
        fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
        return pc;
    }

    unsigned long gastropub();
    static unsigned long chambray();
};

unsigned long semiotics_t::gastropub()
{
    unsigned long pc;
    capture_pc(pc);
    fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
    return pc;
}

unsigned long semiotics_t::chambray()
{
    unsigned long pc;
    capture_pc(pc);
    fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
    return pc;
}

unsigned long succulents_t::normcore()
{
    unsigned long pc;
    capture_pc(pc);
    fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
    return pc;
}

unsigned long succulents_t::sartorial()
{
    unsigned long pc;
    capture_pc(pc);
    fprintf(stderr, "Got an address 0x%lx for %s\n", pc, __FUNCTION__);
    return pc;
}

namespace helvetica
{
    void scenester(void)
    {
        fprintf(stderr, "This function exists\n");
        fprintf(stderr, "Solely to have its address\n");
        fprintf(stderr, "Used to lookup DWARF info.\n");
    }
};

#define TESTCASE(addr, filename, function, offset) \
    { \
        np::spiegel::addr_t _addr = (np::spiegel::addr_t)(addr); \
        np::spiegel::location_t loc; \
        bool success = state.describe_address(_addr, loc); \
        CHECK(success); \
        CHECK(loc.compile_unit_ != 0); \
        CHECK(!strcmp(loc.compile_unit_->get_filename().basename().c_str(), (filename))); \
        CHECK(loc.line_ == 0); \
        CHECK(loc.function_ != 0); \
        fprintf(stderr, "XXX function=\"%s\"\n", loc.function_->get_full_name().c_str()); \
        CHECK(!strcmp(loc.function_->get_full_name().c_str(), (function))); \
        if ((offset) != ~0UL) { \
            CHECK(loc.offset_ == (offset)); \
        } \
    }

int
main(int argc, char **argv __attribute__((unused)))
{
    if (argc > 1)
    {
        fatal("Usage: testrunner intercept\n");
    }

    np::spiegel::state_t state;
    if (!state.add_self())
        return 1;

    BEGIN("extern-global-function-same-file");
    TESTCASE(microdosing, "tdescaddr.cxx", "microdosing", 0);
    END;

    BEGIN("extern-global-function-offset=same-file");
    TESTCASE((unsigned long)microdosing+3, "tdescaddr.cxx", "microdosing", 3);
    END;

    BEGIN("static-global-function-same-file");
    TESTCASE(biodiesel, "tdescaddr.cxx", "biodiesel", 0);
    END;

    BEGIN("static-global-function-same-file");
    TESTCASE((unsigned long)biodiesel+5, "tdescaddr.cxx", "biodiesel", 5);
    END;

    BEGIN("class-member-function-same-file");
    semiotics_t s;
    TESTCASE(s.gentrify(), "tdescaddr.cxx", "semiotics_t::gentrify", ~0UL);
    END;

    BEGIN("virtual-class-member-function-same-file");
    semiotics_t s;
    TESTCASE(s.flannel(), "tdescaddr.cxx", "semiotics_t::flannel", ~0UL);
    END;

    BEGIN("static-class-member-function-same-file");
    TESTCASE(semiotics_t::chicharrones(), "tdescaddr.cxx", "semiotics_t::chicharrones", ~0UL);
    END;

    BEGIN("declared-class-member-function-same-file");
    semiotics_t s;
    TESTCASE(s.gastropub(), "tdescaddr.cxx", "semiotics_t::gastropub", ~0UL);
    END;

    BEGIN("declared-static-class-member-function-same-file");
    TESTCASE(semiotics_t::chambray(), "tdescaddr.cxx", "semiotics_t::chambray", ~0UL);
    END;

    BEGIN("declared-class-member-function-header-file");
    succulents_t s;
    TESTCASE(s.normcore(), "tdescaddr.cxx", "succulents_t::normcore", ~0UL);
    END;

    BEGIN("declared-static-class-member-function-header-file");
    TESTCASE(succulents_t::sartorial(), "tdescaddr.cxx", "succulents_t::sartorial", ~0UL);
    END;

    BEGIN("namespace-member-function-same-file");
    TESTCASE(helvetica::scenester, "tdescaddr.cxx", "helvetica::scenester", 0);
    END;

    return 0;
}

