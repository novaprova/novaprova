/*
 * Copyright 2011-2021 Gregory Banks
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
#include "np/spiegel/platform/abi.h"
#include "fw.h"
#include "succulents.hxx"

using namespace std;
using namespace np::util;

#define capture_line(nm) \
    (nm) = __LINE__

unsigned long microdosing_LINE;

void
microdosing(void)
{   capture_line(microdosing_LINE);
    fprintf(stderr, "This function exists\n");
    fprintf(stderr, "Solely to have its address\n");
    fprintf(stderr, "Used to lookup DWARF info.\n");
}

unsigned long biodiesel_LINE;

static void
biodiesel(void)
{   capture_line(biodiesel_LINE);
    fprintf(stderr, "This function exists\n");
    fprintf(stderr, "Solely to have its address\n");
    fprintf(stderr, "Used to lookup DWARF info.\n");
}

unsigned long gentrify_PC, gentrify_LINE;
unsigned long flannel_PC, flannel_LINE;
unsigned long chicharrones_PC, chicharrones_LINE;
unsigned long gastropub_PC, gastropub_LINE;
unsigned long chambray_PC, chambray_LINE;

class semiotics_t
{
public:

    void gentrify()
    {
        fprintf(stderr, "called %s\n", __FUNCTION__);
        capture_pc(gentrify_PC); capture_line(gentrify_LINE);
        fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", gentrify_PC, gentrify_LINE, __FUNCTION__);
    }

    virtual void flannel()
    {
        fprintf(stderr, "called %s\n", __FUNCTION__);
        capture_pc(flannel_PC); capture_line(flannel_LINE);
        fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", flannel_PC, flannel_LINE, __FUNCTION__);
    }

    static void chicharrones()
    {
        fprintf(stderr, "called %s\n", __FUNCTION__);
        capture_pc(chicharrones_PC); capture_line(chicharrones_LINE);
        fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", chicharrones_PC, chicharrones_LINE, __FUNCTION__);
    }

    void gastropub();
    static void chambray();
};

void semiotics_t::gastropub()
{
    fprintf(stderr, "called %s\n", __FUNCTION__);
    capture_pc(gastropub_PC); capture_line(gastropub_LINE);
    fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", gastropub_PC, gastropub_LINE, __FUNCTION__);
}

void semiotics_t::chambray()
{
    fprintf(stderr, "called %s\n", __FUNCTION__);
    capture_pc(chambray_PC); capture_line(chambray_LINE);
    fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", chambray_PC, chambray_LINE, __FUNCTION__);
}

unsigned long normcore_PC, normcore_LINE;
unsigned long sartorial_PC, sartorial_LINE;

void succulents_t::normcore()
{
    fprintf(stderr, "called %s\n", __FUNCTION__);
    capture_pc(normcore_PC); capture_line(normcore_LINE);
    fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", normcore_PC, normcore_LINE, __FUNCTION__);
}

void succulents_t::sartorial()
{
    fprintf(stderr, "called %s\n", __FUNCTION__);
    capture_pc(sartorial_PC); capture_line(sartorial_LINE);
    fprintf(stderr, "Got an address 0x%lx and line %lu for %s\n", sartorial_PC, sartorial_LINE, __FUNCTION__);
}

unsigned long scenester_LINE;

namespace helvetica
{
    void scenester(void)
    {   capture_line(scenester_LINE);
        fprintf(stderr, "This function exists\n");
        fprintf(stderr, "Solely to have its address\n");
        fprintf(stderr, "Used to lookup DWARF info.\n");
    }
};

#define OFFSET_DONTCARE (~0U)
#define TESTCASE(addr, filename, lineno, function, offset) \
    { \
        np::spiegel::addr_t _addr = (np::spiegel::addr_t)(addr); \
        np::spiegel::location_t loc; \
        bool success = state.describe_address(_addr, loc); \
        CHECK(success); \
        CHECK(loc.compile_unit_ != 0); \
        CHECK(!strcmp(loc.compile_unit_->get_filename().basename().c_str(), (filename))); \
        CHECK(loc.line_ == (lineno)); \
        CHECK(loc.function_ != 0); \
        CHECK(!strcmp(loc.function_->get_full_name().c_str(), (function))); \
        if ((offset) != OFFSET_DONTCARE) { \
            CHECK(loc.offset_ == (offset)); \
        } \
    }

int
main(int argc, char **argv __attribute__((unused)))
{
    bool debug = false;
    if (argc == 2 && !strcmp(argv[1], "--debug"))
    {
        debug = true;
    }
    else if (argc > 1)
    {
        fatal("Usage: tdescaddr [--debug]\n");
    }
    np::log::basic_config(debug ? np::log::DEBUG : np::log::INFO, 0);

    np::spiegel::state_t state;
    if (!state.add_self())
        return 1;

    BEGIN("extern-global-function-same-file");
    microdosing();
    TESTCASE(microdosing, "tdescaddr.cxx", microdosing_LINE, "microdosing", 0);
    END;

    BEGIN("extern-global-function-offset=same-file");
    TESTCASE((unsigned long)microdosing+3, "tdescaddr.cxx", microdosing_LINE, "microdosing", 3);
    END;

    BEGIN("static-global-function-same-file");
    biodiesel();
    TESTCASE(biodiesel, "tdescaddr.cxx", biodiesel_LINE, "biodiesel", 0);
    END;

    BEGIN("static-global-function-same-file");
    TESTCASE((unsigned long)biodiesel+5, "tdescaddr.cxx", biodiesel_LINE, "biodiesel", 5);
    END;

    BEGIN("class-member-function-same-file");
    semiotics_t s;
    s.gentrify();
    TESTCASE(gentrify_PC, "tdescaddr.cxx", gentrify_LINE, "semiotics_t::gentrify", OFFSET_DONTCARE);
    END;

    BEGIN("virtual-class-member-function-same-file");
    semiotics_t s;
    s.flannel();
    TESTCASE(flannel_PC, "tdescaddr.cxx", flannel_LINE, "semiotics_t::flannel", OFFSET_DONTCARE);
    END;

    BEGIN("static-class-member-function-same-file");
    semiotics_t::chicharrones();
    TESTCASE(chicharrones_PC, "tdescaddr.cxx", chicharrones_LINE, "semiotics_t::chicharrones", OFFSET_DONTCARE);
    END;

    BEGIN("declared-class-member-function-same-file");
    semiotics_t s;
    s.gastropub();
    TESTCASE(gastropub_PC, "tdescaddr.cxx", gastropub_LINE, "semiotics_t::gastropub", OFFSET_DONTCARE);
    END;

    BEGIN("declared-static-class-member-function-same-file");
    semiotics_t::chambray();
    TESTCASE(chambray_PC, "tdescaddr.cxx", chambray_LINE, "semiotics_t::chambray", OFFSET_DONTCARE);
    END;

    BEGIN("declared-class-member-function-header-file");
    succulents_t s;
    s.normcore();
    TESTCASE(normcore_PC, "tdescaddr.cxx", normcore_LINE, "succulents_t::normcore", OFFSET_DONTCARE);
    END;

    BEGIN("declared-static-class-member-function-header-file");
    succulents_t::sartorial();
    TESTCASE(sartorial_PC, "tdescaddr.cxx", sartorial_LINE, "succulents_t::sartorial", OFFSET_DONTCARE);
    END;

    BEGIN("namespace-member-function-same-file");
    helvetica::scenester();
    TESTCASE(helvetica::scenester, "tdescaddr.cxx", scenester_LINE, "helvetica::scenester", 0);
    END;

    return 0;
}

