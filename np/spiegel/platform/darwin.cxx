/*
 * Copyright 2014-2020 David Arnold, Greg Banks
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
#include "np/util/log.hxx"
#include "common.hxx"
#include <mach-o/dyld.h>
#include <mach/mach_time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cxxabi.h>
#include <libproc.h>

#include <string>
#include <iostream>
#include <sstream>
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

bool is_running_under_debugger()
{
    struct proc_bsdinfo info;
    memset(&info, 0, sizeof(info));

    int r = proc_pidinfo(getpid(), PROC_PIDTBSDINFO, 0,  &info, sizeof(info));
    if (r < 0)
    {
        eprintf("Failed to call proc_pidinfo(PROC_PIDTBSDINFO): %s\n", strerror(errno));
	return false;
    }

    return (info.pbi_flags & PROC_FLAG_TRACED ? true : false);
}

static void describe_sockaddr(const struct sockaddr *sa, ostream &o)
{
    switch (sa->sa_family)
    {
    case AF_INET:
	{
	    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	    char addrbuf[INET_ADDRSTRLEN];
	    o << "inet "
	      << inet_ntop(sin->sin_family,
			   &sin->sin_addr.s_addr,
			   addrbuf, sizeof(addrbuf))
	      << ":"
	      << ntohs(sin->sin_port);
	}
	break;
    case AF_INET6:
	{
	    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
	    char addrbuf[INET6_ADDRSTRLEN];
	    o << "inet6 "
	      << inet_ntop(sin6->sin6_family,
			   &sin6->sin6_addr,
			   addrbuf, sizeof(addrbuf))
	      << "."
	      << ntohs(sin6->sin6_port);
	}
	break;
    case AF_UNIX:
	{
	    struct sockaddr_un *sun = (struct sockaddr_un *)sa;
	    o << "unix " << sun->sun_path;
	}
	break;
    default:
	eprintf("unexpected socket family %d\n", sa->sa_family);
	break;
    }
}

static bool describe_fd(int fd, ostream &o)
{
    struct stat sb;
    int r;
    bool need_path = false;
    bool need_dev = false;
    bool need_sockname = false;

    r = fstat(fd, &sb);
    if (r < 0)
    {
	if (errno == EBADF)
	{
	    /* not a file descriptor */
	    return false;
	}
	if (errno == EACCES || errno == EPERM)
	{
	    /* permission denied */
	    return false;
	}
        eprintf("Failed to fstat fd %d: %s\n", fd, strerror(errno));
	return false;
    }
    switch (sb.st_mode & S_IFMT)
    {
    case S_IFIFO:   /* named pipe (fifo) or actual pipe */
	/* Experiment indicates named pipes have non-zero st_dev
	 * field but anonymous pipes don't */
	if (sb.st_dev == 0)
	{
	    o << "pipe:0x" << std::hex << sb.st_ino;
	}
	else
	{
	    need_path = true;
	}
	break;
    case S_IFCHR:   /* character special */
	need_dev = true;
	o << "char device ";
	break;
    case S_IFDIR:   /* directory */
	need_path = true;
	break;
    case S_IFBLK:   /* block special */
	need_dev = true;
	o << "block device ";
	break;
    case S_IFREG:   /* regular file */
	need_path = true;
	break;
    /* We won't get S_IFLNK from fstat(), any symlink
     * has already been followed at open time */
    case S_IFSOCK:  /* socket */
	need_sockname = true;
	o << "socket ";
	break;
    /* S_IFWHT is used as argument to mknod() to white-out
     * a name in a UnionFS directory, and doesn't appear
     * as the type of any actual inodes */
    default:
	eprintf("fd %d: unexpected stat mode %d\n",
		fd, sb.st_mode & S_IFMT);
	return false;
    }

    if (need_dev)
    {
	o << major(sb.st_rdev) << ":" << minor(sb.st_rdev);
    }
    if (need_path)
    {
	char path[PATH_MAX];
	r = fcntl(fd, F_GETPATH, path);
	if (r < 0)
	{
	    eprintf("fcntl(%d, F_GETPATH) failed: %s\n", fd, strerror(errno));
	    return false;
	}
	o << path;
    }
    if (need_sockname)
    {
	struct sockaddr_storage addr;
	socklen_t socklen = sizeof(addr);
	r = getsockname(fd, (struct sockaddr *)&addr, &socklen);
	if (r < 0)
	{
            /* EOPNOTSUPP happens mysteriously when running in Travis */
            if (errno != EOPNOTSUPP)
                eprintf("Failed to call getsockname(): %s\n", strerror(errno));
	    return false;
	}
	o << "local ";
	describe_sockaddr((struct sockaddr *)&addr, o);
	if (addr.ss_family != AF_UNIX)
	{
	    socklen = sizeof(addr);
	    r = getpeername(fd, (struct sockaddr *)&addr, &socklen);
	    if (r < 0)
	    {
                if (errno != EOPNOTSUPP)
                    eprintf("Failed to call getpeername(): %s\n", strerror(errno));
		return false;
	    }
	    o << " remote ";
	    describe_sockaddr((struct sockaddr *)&addr, o);
	}
    }
    return true;
}

vector<string> get_file_descriptors()
{
    vector<string> fds;
    int fd;
    int numfds = getdtablesize();

    for (fd = 0 ; fd < numfds ; fd++)
    {
	ostringstream o;
	if (describe_fd(fd, o))
	{
	    // STL is just so screwed
	    if (fd >= (int)fds.size())
		fds.resize(fd+1);
	    fds[fd] = o.str();
	}
    }

    return fds;
}

/*
 * Darwin doesn't have the POSIX clock_getttime().  This is from
 * http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
 * and https://developer.apple.com/library/mac/qa/qa1398/_index.html
 */
int clock_gettime(int clk_id, struct timespec *res)
{
    switch (clk_id)
    {
    case NP_CLOCK_MONOTONIC:
	{
	    // TODO: be more careful in a multithreaded environement
	    static mach_timebase_info_data_t tb;
	    static uint64_t epoch;
	    if (!epoch)
	    {
		mach_timebase_info(&tb);
		epoch = mach_absolute_time();
	    }
	    uint64_t elapsed_ns = (mach_absolute_time() - epoch) * tb.numer / tb.denom;
	    res->tv_sec = elapsed_ns / NANOSEC_PER_SEC;
	    res->tv_nsec = elapsed_ns - (res->tv_sec * NANOSEC_PER_SEC);
	}
	return 0;
    case NP_CLOCK_REALTIME:
	{
	    struct timeval tv;
	    gettimeofday(&tv, NULL);
	    res->tv_sec = tv.tv_sec;
	    res->tv_nsec = tv.tv_usec * 1000;
	}
	return 0;
    }
    errno = EINVAL;
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

/*
 * The parts of the cxxabi we use here are common between the Darwin
 * and GNU libc implementations.
 */
char *current_exception_type()
{
    /*
     * Logic copied from __gnu_cxx::__verbose_terminate_handler() in
     * gcc/libstdc++-v3/src/vterminate.cc.
     */
    type_info *tinfo = __cxxabiv1::__cxa_current_exception_type();
    if (!tinfo)
	return 0;

    /* 0 = success, -1 = failed allocating memory,
     * -2 = invalid name, -3 = invalid argument */
    int status = 0;
    char *demangled = __cxxabiv1::__cxa_demangle(tinfo->name(), NULL, NULL, &status);
    if (status == -1)
	oom(); /* failed allocating memory */

    return (status == 0 ? demangled : xstrdup(tinfo->name()));
}

void cleanup_current_exception()
{
    /*
     * Here we use a pair of functions which are Apple extensions.
     * These are declaredin the same header as the standard functions
     * but for some reason fail to link.
     */
#if 0
    __cxxabiv1::__cxa_decrement_exception_refcount(__cxxabiv1::__cxa_current_primary_exception());
#endif
}

// Close namespaces
}; }; };
