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
#include "common.hxx"
#include <mach-o/dyld.h>

#include <string>
#include <vector>


namespace np {
    namespace spiegel {
        namespace platform {

using namespace std;
using namespace np::util;


bool get_argv(int *argcp, char ***argvp)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    return false;
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
			    (void *)seg->vmaddr));
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
			    (void *)seg->vmaddr));
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

np::spiegel::addr_t normalise_address(np::spiegel::addr_t addr)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    return addr;
}

void add_plt(const np::spiegel::mapping_t &m)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
}

int text_map_writable(addr_t addr, size_t len)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    return -1;
}

int text_restore(addr_t addr, size_t len)
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    return -1;
}

vector<np::spiegel::addr_t> get_stacktrace()
{
    fprintf(stderr, "TODO: %s not implemented for this platform\n", __FUNCTION__);
    vector<np::spiegel::addr_t>* res = new vector<np::spiegel::addr_t>();
    return *res;
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



// Close namespaces
}; }; };
