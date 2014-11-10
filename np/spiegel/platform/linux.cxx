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
#include "np/spiegel/common.hxx"
#include "np/spiegel/intercept.hxx"
#include "np/util/tok.hxx"
#include "common.hxx"

#include <dlfcn.h>
#include <link.h>
#include <signal.h>
#include <memory.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>
#include <dirent.h>
#include <ctype.h>
#include <typeinfo>
#include <cxxabi.h>

#ifndef MIN
#define MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif

extern char **_dl_argv;

namespace np { namespace spiegel { namespace platform {
using namespace std;
using namespace np::util;

#if 0
/* This trick doesn't seem to work on 64b Linux, which
 * is confusing, but whatever */
extern char **environ;

bool
get_argv(int *argcp, char ***argvp)
{
    char **p;
    int n;

    /* This early, environ[] points at the area
     * above argv[], so walk down from there */
    for (p = environ-2, n = 1;
	 ((int *)p)[-1] != n ;
	 --p, ++n)
	;
    *argcp = n;
    *argvp = p;
    return true;
}
#else
bool
get_argv(int *argcp, char ***argvp)
{
    char **p;

    for (p = _dl_argv ; *p ; p++)
	;
    *argcp = (p - _dl_argv);
    *argvp = _dl_argv;
    return true;
}
#endif

char *
self_exe()
{
    // We could find the ELF auxv_t vector which is above the environ[]
    // passed to us by the kernel, there is usually a AT_EXECFN entry
    // there which points at the executable filename.  But, when running
    // under Valgrind, the aux vector is synthetically built by Valgrind
    // and doesn't contain AT_EXECFN.  However, Valgrind *does*
    // intercept a readlink() of the magical Linux /proc file and
    // substitute the answer we want (instead of what the real kernel
    // readlink() call would return, which is the Valgrind executable
    // itself).  So, we have a fallback that works in all cases, we may
    // as well use it.
    char buf[PATH_MAX];
    static const char filename[] = "/proc/self/exe";
    int r = readlink(filename, buf, sizeof(buf)-1);
    if (r < 0)
    {
	perror(filename);
	return 0;
    }
    // readlink() doesn't terminate the buffer
    buf[r] = '\0';
    return xstrdup(buf);
}

// Return the argv[0] of the given process id.  Isn't Linux' /proc
// wonderful?  Note that we choose cmdline because it's commonly
// readable even when the 'exe' symlink isn't.
static char *
get_exe_by_pid(pid_t pid)
{
    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "/proc/%u/cmdline", (unsigned)pid);

    FILE *fp = fopen(buf, "r");
    if (!fp)
    {
	perror(buf);
	return 0;
    }

    int r = fread(buf, 1, sizeof(buf)-1, fp);
    if (r <= 0)
    {
	perror("read");
	fclose(fp);
	return 0;
    }
    buf[r] = '\0';  /* JIC */

    fclose(fp);

    /* cmdline is \0 separated, so we can just treat the buffer
     * as a string describing argv[0] of the ptracer */
    return xstrdup(buf);
}

static int
add_one_linkobj(struct dl_phdr_info *info,
		size_t size __attribute__((unused)),	// sizeof(*info)
		void *closure)
{
    vector<linkobj_t> *vec = (vector<linkobj_t> *)closure;

    const char *name = info->dlpi_name;
    if (name && !*name)
	name = NULL;

    if (!name && info->dlpi_addr)
	return 0;
    if (!info->dlpi_phnum)
	return 0;

#if 0
    fprintf(stderr, "dl_phdr_info { addr=%p name=%s }\n",
	    (void *)info->dlpi_addr, info->dlpi_name);
#endif

    linkobj_t lo;
    lo.name = name;

    for (int i = 0 ; i < info->dlpi_phnum ; i++)
    {
	if (!info->dlpi_phdr[i].p_memsz)
	    continue;

	const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
	lo.mappings.push_back(mapping_t(
		(unsigned long)ph->p_offset, (unsigned long)ph->p_memsz,
		(void *)((unsigned long)info->dlpi_addr + ph->p_vaddr)));
    }
    vec->push_back(lo);

    return 0;
}

vector<linkobj_t> get_linkobjs()
{
    vector<linkobj_t> vec;
    dl_iterate_phdr(add_one_linkobj, &vec);
    return vec;
}

bool is_plt_section(const char *secname)
{
    return !strcmp(secname, ".plt");
}

np::spiegel::addr_t follow_plt(np::spiegel::addr_t addr)
{
    Dl_info info;
    memset(&info, 0, sizeof(info));
    int r = dladdr((void *)addr, &info);
    if (r)
	return (np::spiegel::addr_t)dlsym(RTLD_NEXT, info.dli_sname);
    return addr;
}

/* This trick doesn't work - Valgrind actively prevents
 * the simulated program from hijacking it's log fd */
#if 0
    if (RUNNING_ON_VALGRIND)
    {
	/* ok, this is cheating */
	int old_stderr = -1;
	int fd;
	int r;

	strncpy(buf, "/tmp/spiegel-stack-XXXXXX", maxlen);
	fd = mkstemp(buf);
	if (fd < 0)
	    return -errno;
	unlink(buf);

	old_stderr = dup(STDERR_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);

	VALGRIND_PRINTF_BACKTRACE("\n");

	lseek(STDERR_FILENO, 0, SEEK_SET);
	r = read(STDERR_FILENO, buf, sizeof(buf)-1);
	if (r < 0)
	{
	    r = -errno;
	    goto out;
	}
	buf[r] = '\0';
	r = 0;
out:
	if (old_stderr > 0)
	{
	    dup2(old_stderr, STDERR_FILENO);
	    close(old_stderr);
	}
	return r;
    }
#endif

vector<np::spiegel::addr_t> get_stacktrace()
{
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

/* Return the process id of any process which is ptrace()ing us, or 0 if
 * not being ptrace'd, or -1 on error. */
static pid_t
get_tracer_pid()
{
    static const char procfile[] = "/proc/self/status";
    FILE *fp;
    const char *p;
    pid_t pid = 0;
    char buf[1024];

    fp = fopen(procfile, "r");
    if (!fp)
    {
	/* most probable cause is that /proc is not mounted */
	perror(procfile);
	return -1;
    }

    while (fgets(buf, sizeof(buf), fp))
    {
	tok_t tok(buf);
	p = tok.next();
	if (!p || strcmp(p, "TracerPid:"))
	    continue;
	p = tok.next();
	if (p)
	    pid = strtoul(p, NULL, 10);
	break;
    }

    fclose(fp);
    return pid;
}

// Determine whether this process is running under a debugger
bool is_running_under_debugger()
{
    static const char * const debuggers[] = { "gdb", NULL };
    const char *tail;
    int i;
    pid_t tracer = 0;
    char *command;

    // Valgrind doesn't play well with debuggers
    if (RUNNING_ON_VALGRIND)
	return false;

    tracer = get_tracer_pid();
    if (tracer < 0)
	return false;	    /* no way to tell...so guess */
    if (!tracer)
	return false;	/* not being ptrace()d */

    // We know we're being ptrace()d, but that could mean either that
    // we're being debugged or strace()d.  In the latter case we don't
    // want to return true because we actually want our process to
    // behave as normally as possible.  One way of telling is to compare
    // the name of the program which is ptrace()ing us against a
    // whitelist of debugger names.  This may not be the best way, only
    // time will tell.  Note that we choose cmdline because it's
    // commonly readable even when the 'exe' symlink isn't.
    command = get_exe_by_pid(tracer);
    if (!command)
    {
	// can't tell for sure, but given we're definitely ptraced,
	// let's guess that it's a debugger doing it
	return true;
    }

    // find the filename
    tail = strrchr(command, '/');
    if (tail)
	tail++;
    else
	tail = command;

    // compare against the whitelist
    for (i = 0 ; debuggers[i] ; i++)
    {
	if (!strcmp(tail, debuggers[i]))
	{
	    fprintf(stderr, "np: being debugged by %s\n", command);
	    free(command);
	    return true;
	}
    }

    // didn't match the whitelist, probably strace or something strange

    free(command);
    return false;
}

vector<string> get_file_descriptors()
{
    struct dirent *de;
    DIR *dir;
    int r;
    int fd;
    vector<string> fds;
    string dirpath = string("/proc/self/fd");
    string filepath;
    char buf[PATH_MAX];

    dir = opendir(dirpath.c_str());
    if (dir)
    {
	while ((de = readdir(dir)))
	{
	    if (!isdigit(de->d_name[0]))
		continue;

	    fd = atoi(de->d_name);
	    if (fd == dirfd(dir))
		continue;

	    filepath = dirpath + string("/") + string(de->d_name);
	    r = readlink(filepath.c_str(), buf, sizeof(buf)-1);
	    if (r < 0)
	    {
		perror(filepath.c_str());
		continue;
	    }
	    buf[r] = '\0';

	    // Silently ignore named pipes to and from
	    // Valgrind's built-in gdbserver.
	    char *tail = strrchr(buf, '/');
	    if (tail && !strncmp(tail, "/vgdb-pipe", 10))
		continue;

	    // STL is just so screwed
	    if (fd >= (int)fds.size())
		fds.resize(fd+1);
	    fds[fd] = string(buf);
	}
	closedir(dir);
    }

    return fds;
}


int clock_gettime(int clk_id, struct timespec *res)
{
    return clock_gettime(clk_id, res);
}

bool symbol_filename(const char *filename __attribute__((unused)),
		     std::string &symfile __attribute__((unused)))
{
    return false;
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

/* #include <unwind-cxx.h> */
void cleanup_current_exception()
{
#if 0
    /*
     * I believe this is the correct code, but there's no way to
     * compile it because the unwind-cxx.h header is not shipped.
     */
    __cxa_eh_globals *globals = __cxa_get_globals();
    __cxa_exception *header = globals->caughtExceptions;
    if (header)
    {
	if (header->exceptionDestructor)
	    header->exceptionDestructor(header+1);
	__cxa_free_exception(header+1)
    }
#endif
}

// close namespaces
}; }; };

