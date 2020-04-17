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
#ifndef __np_spiegel_platform_common_hxx__
#define __np_spiegel_platform_common_hxx__ 1

#include "np/util/config.h"
#include "np/spiegel/mapping.hxx"
#include <vector>

namespace np { namespace spiegel { namespace platform {

extern bool get_argv(int *argcp, char ***argvp);
extern char *self_exe();


#ifdef CLOCK_REALTIME
#define NP_CLOCK_REALTIME   CLOCK_REALTIME
#define NP_CLOCK_MONOTONIC  CLOCK_MONOTONIC
#else

/// Standard real-time clock (vs. NP_CLOCK_MONOTONIC).
#define NP_CLOCK_REALTIME (0)

/// Monotonically-increasing clock.  Never goes backwards; cannot be
/// set (vs. NP_CLOCK_REALTIME).
#define NP_CLOCK_MONOTONIC (1)
#endif

/// Return the time from the specified clock.
extern int clock_gettime(int clk_id, struct timespec *res);


struct linkobj_t
{
    const char *name;
    unsigned long slide;
    std::vector<np::spiegel::mapping_t> mappings;
};
extern std::vector<linkobj_t> get_linkobjs();

bool is_plt_section(const char *secname);
np::spiegel::addr_t follow_plt(np::spiegel::addr_t);

// extern np::spiegel::value_t invoke(void *fnaddr, vector<np::spiegel::value_t> args);

/*
 * Functions to ensure some .text space is writable so we can insert
 * breakpoint insns, and to undo the effect.
 *
 * Originally NovaProva worked by mapping the page RWX as long as the
 * intercept was installed.  However MacOS Catalina has a behavior
 * change where the kernel refuses to give pages X and W permissions
 * at the same time, except for some very narrow and unhelpful exceptions
 * e.g. mmap(MAP_ANON|MAP_JIT) from a codesigned binary with a special
 * entitlement.  From an OS security standpoint this is pretty sensible,
 * so we're just going to cope with it.  Now NovaProva will map the
 * page RW- long enough to install the intercept, then map it back to
 * R-X while the test runs, repeating the two maps to uninstall the
 * intercept.  NovaProva does this on all platforms now, for simplicity
 * and in the expectation that the restrictive kernel behavior will
 * eventually catch on in the Linux world.
 *
 * Unexplored ramifications:
 *
 * - the page reference counting behavior here needs re-assessing
 * - intercepts are now even less thread-safe
 * - there is now a short period when a text page will not be
 *   executable, which might cause problems if a thread is executing
 *   code in that page
 * - worse, there's a possibility that the part of NovaProva which
 *   installs intercepts might accidentally end up on the same page
 *   as a function being intercepted.
 *
 * Uses per-page reference counting so that multiple calls can be
 * nested.  This is for tidiness so that we can re-map pages back to
 * their default state after we've finished, and still handle the case
 * of two intercepts in separate functions which are located in the
 * same page.
 *
 * These functions are more general than they need to be, as we only
 * ever need the len=1 case.
 */
extern int text_map_writable(addr_t addr, size_t len);
extern int text_restore(addr_t addr, size_t len);

struct intstate_t
{
#if defined(_NP_x86) || defined(_NP_x86_64)
    enum { UNKNOWN, PUSHBP, OTHER } type_;
    unsigned char orig_;	    /* first byte of original insn */

    intstate_t()
     : type_(UNKNOWN), orig_(0)  {}
#endif
};
extern int install_intercept(np::spiegel::addr_t,
			     intstate_t &state,
			     /*return*/std::string &err);
extern int uninstall_intercept(np::spiegel::addr_t,
			     intstate_t &state,
			     /*return*/std::string &err);

extern std::vector<np::spiegel::addr_t> get_stacktrace();

extern bool is_running_under_debugger();

extern std::vector<std::string> get_file_descriptors();

extern char *current_exception_type();
extern void cleanup_current_exception();

extern bool symbol_filename(const char *, std::string &);

extern char *current_exception_type();
extern void cleanup_current_exception();

// close namespaces
}; }; };

#endif // __np_spiegel_platform_common_hxx__
