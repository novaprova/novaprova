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


// Represents a object (the main application or a shared library)
// reported by the platforms-specific runtime linker.
struct linkobj_t
{
    // The full pathname of the object, or NULL for the main
    // application.  The string points into the runtime linker's
    // data structures.
    const char *name;
    // The difference in bytes between the actual in-memory load
    // address of the object and the load address recorded in
    // the object.  The two differ when ASLR (Address Space Layout
    // Randomization) is enabled; it's a security technique
    // designed to make it harder for malware to turn bugs like
    // buffer overflows into running code.
    unsigned long slide;
    // Memory mappings extracted from platform specific executable
    // information, e.g. Program Headers on ELF systems.
    std::vector<np::spiegel::mapping_t> mappings;
};
extern std::vector<linkobj_t> get_linkobjs();

bool is_plt_section(const char *secname);
np::spiegel::addr_t follow_plt(np::spiegel::addr_t);

// extern np::spiegel::value_t invoke(void *fnaddr, vector<np::spiegel::value_t> args);

/*
 * Write the given bytes to the text segment, taking care of issues
 * like memory protection and Valgrind translations.  Used to insert
 * a software breakpoint instruction, and uninstall it again.  The
 * text segment remains executable and not writable after the function
 * returns.
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
 * With the new behavior we don't need two separate functions to make
 * the text pages writable and not-writable anymore, so API is now
 * a single function that does the prot/write/unprot cycle.  This also
 * lets us cope with some gnarly side effects of having rw- pages, like
 * accidentally making the PLT non-executable.  The page reference counting
 * is also gone, it was moot and can't work without an executable PLT.
 *
 * Unexplored ramifications:
 *
 * - intercepts are now even less thread-safe
 *
 * This function is more general than it needs to be, as we only
 * ever need the len=1 case.
 */
extern int text_write(addr_t addr, const uint8_t *trap_bytes, size_t len);

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
			     intstate_t &state);
extern int uninstall_intercept(np::spiegel::addr_t,
			     intstate_t &state);
extern bool is_intercept_installed(np::spiegel::addr_t,
			     const intstate_t &state);

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
