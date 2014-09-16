/*
 * Copyright 2014 David Arnold, Greg Banks
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
#include "np/spiegel/common.hxx"
#include "np/spiegel/intercept.hxx"
#include "np/util/tok.hxx"
#include "np/util/filename.hxx"
#include "common.hxx"
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include <string>
#include <vector>


namespace np {
    namespace spiegel {
        namespace platform {

using namespace std;
using namespace np::util;

/*
 * In OSX, the argc and argv are not stashed anywhere convenient, but
 * constructors are called before main() with the same arguments as
 * main(), which gives us a chance to sample them for later.
 */
static int _argc;
static char **_argv;
static void __attribute__((constructor)) stash_argv(int argc, char **argv)
{
    _argc = argc;
    _argv = argv;
}

bool get_argv(int *argcp, char ***argvp)
{
    *argcp = _argc;
    *argvp = _argv;
    return true;
}

char *self_exe()
{
    // This convenient function does exactly what we want.
    // See the dyld(3) manpage.
    char buf[PATH_MAX];
    uint32_t len = sizeof(buf);
    int r = _NSGetExecutablePath(buf, &len);
    if (r != 0)
	return NULL;	// buffer too small is the only documented
			// error case, and can't happen here
    return xstrdup(buf);
}

vector<linkobj_t> get_linkobjs()
{
    vector<linkobj_t> vec;

    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0 ; i < count ; i++)
    {
	const struct mach_header *hdr = _dyld_get_image_header(i);
	if (!hdr)
	    continue;
	if (hdr->filetype != MH_EXECUTE && hdr->filetype != MH_DYLIB)
	    continue;

	linkobj_t lo;
	lo.name = _dyld_get_image_name(i);
	lo.slide = (unsigned long) _dyld_get_image_vmaddr_slide(i);

	const struct load_command *cmds =
		(hdr->magic == MH_MAGIC_64 || hdr->magic == MH_CIGAM_64 ?
		    (const struct load_command *)(((struct mach_header_64 *)hdr)+1) :
		    (const struct load_command *)(hdr+1));
	uint32_t j = 0;
	const struct load_command *cmd = cmds;
	while (j < hdr->ncmds &&
	       ((unsigned long)cmd - (unsigned long)cmds) < hdr->sizeofcmds)
	{
	    switch (cmd->cmd)
	    {
#if _NP_ADDRSIZE == 4
	    case LC_SEGMENT:
		{
		    const struct segment_command *seg = (const struct segment_command *)cmd;
		    if (!seg->vmsize)
			continue;
		    lo.mappings.push_back(mapping_t(
			    (unsigned long)seg->fileoff,
			    (unsigned long)seg->vmsize,
			    (void *)(seg->vmaddr + lo.slide)));
		}
		break;
#endif
#if _NP_ADDRSIZE == 8
	    case LC_SEGMENT_64:
		{
		    const struct segment_command_64 *seg = (const struct segment_command_64 *)cmd;
		    if (!seg->vmsize)
			continue;
		    lo.mappings.push_back(mapping_t(
			    (unsigned long)seg->fileoff,
			    (unsigned long)seg->vmsize,
			    (void *)(seg->vmaddr + lo.slide)));
		}
		break;
#endif
	    }
	    j++;
	    cmd = (const struct load_command *)((unsigned long)cmd + cmd->cmdsize);
	}
	vec.push_back(lo);
    }
    return vec;
}

bool is_plt_section(const char *secname)
{
    return (!strcmp(secname, "__DATA.__nl_symbol_ptr") ||
	    !strcmp(secname, "__DATA.__la_symbol_ptr"));
}

np::spiegel::addr_t follow_plt(np::spiegel::addr_t addr)
{
    /* Note: this is identical to the Linux implementation,
     * should share the code somehow */
    Dl_info info;
    memset(&info, 0, sizeof(info));
    int r = dladdr((void *)addr, &info);
    if (r)
	return (np::spiegel::addr_t)dlsym(RTLD_NEXT, info.dli_sname);
    return addr;
}

vector<np::spiegel::addr_t> get_stacktrace()
{
    /* Note: this is identical to the linux-x86 and
     * linux-x86_64 implementation, because it depends
     * only on the processor ABI which both those platforms obey.
     * Should merge the two.
     */

    /* This only works if a frame pointer is used, i.e. it breaks
     * with -fomit-frame-pointer.
     *
     * TODO: use DWARF2 unwind info and handle -fomit-frame-pointer
     *
     * TODO: terminating the unwind loop is tricky to do properly,
     *       we need to estimate the stack boundaries.  Instead
     *       we approximate
     *
     * TODO: should return a vector of {ip=%eip,fp=%ebp,sp=%esp}
     */
    unsigned long bp;
    vector<np::spiegel::addr_t> stack;

#if _NP_ADDRSIZE == 4
    __asm__ volatile("movl %%ebp, %0" : "=r"(bp));
#else
    __asm__ volatile("movq %%rbp, %0" : "=r"(bp));
#endif
    for (;;)
    {
	stack.push_back(((unsigned long *)bp)[1]-_NP_ADDRSIZE-1);
	unsigned long nextbp = ((unsigned long *)bp)[0];
	if (!nextbp)
	    break;
	if (nextbp < bp)
	    break;	// moving in the wrong direction
	if ((nextbp - bp) > 16384)
	    break;	// moving a heuristic "too far"
	bp = nextbp;
    };
    return stack;
}

bool is_running_under_debugger()
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    return false;
}

vector<string> get_file_descriptors()
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    vector<string>* res = new vector<string>();
    return *res;
}

int install_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    err = "Not yet implemented";
    return -1;
}

int uninstall_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    err = "Not yet implemented";
    return -1;
}

int clock_gettime(int clk_id, struct timespec *res)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    errno = ENOSYS;
    return -1;
}

bool symbol_filename(const char *filename, std::string &symfile)
{
    filename_t path = filename;
    filename_t base = path.basename();
    path += ".dSYM";
    path.push_back("Contents/Resources/DWARF");
    path.push_back(base);

    /* The debug symbol file might not exist due to there being no
     * symbols, or possibly due to the symbols being in the original
     * file.  Our caller can handle missing DWARF sections in the
     * original file, so in either case we just return false and let the
     * caller look in the origin file.
     */
    struct stat sb;
    if (stat(path.c_str(), &sb) < 0 && errno == ENOENT)
	return false;

    symfile = path;
    return true;
}

// Close namespaces
}; }; };
