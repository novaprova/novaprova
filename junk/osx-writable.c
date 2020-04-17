/*
 * Junk code to explore what sequence of system calls are needed to
 * make a text page writable in order to install an intercept.
 * Needed because the mprotect() method written for Linux stopped
 * working in MacOS Catalina.
 *
 * Based on reading the LLDB code at
 * https://github.com/llvm-mirror/lldb/blob/731e9526215dcd044f28068f4c5b03e9ba9233ae/tools/debugserver/source/MacOSX/MachVMRegion.h
 *
 * and this presentation
 * https://papers.put.as/papers/ios/2011/syscan11_breaking_ios_code_signing.pdf
 *
 * For a little while I was afraid that installing intercepts
 * would require a codesigned binary with the entitlement
 * com.apple.security.cs.allow-unsigned-executable-memory
 * but experiment indicates that at least on Catalina this is
 * not necessary.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <libproc.h>
#include <sys/mman.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>

#define INSN_INT3          0xcc

static char *prot_to_string(uint32_t prot, char *buf)
{
    buf[0] = (prot & PROT_READ ? 'r' : '-');
    buf[1] = (prot & PROT_WRITE ? 'w' : '-');
    buf[2] = (prot & PROT_EXEC ? 'x' : '-');
    buf[3] = '\0';
    return buf;
}

static const char *share_mode_to_string(uint32_t sm)
{
    static const char * strs[] = {
        NULL,
        "COW",              // SM_COW
        "PRV",              // SM_PRIVATE
        "NUL",              // SM_EMPTY
        "SHM",              // SM_SHARED
        "TSH",              // SM_TRUESHARED
        "ALI",              // SM_PRIVATE_ALIASED
        "S/A",              // SM_SHARED_ALIASED
        "LG",               // SM_LARGE_PAGE
    };
    static const char unknown[] = "???";
    return (sm < sizeof(strs)/sizeof(strs[0]) ?
        (strs[sm] ? strs[sm] : unknown) :
        unknown);
}

static const char *region_type_to_string(uint32_t tag, uint32_t prot)
{
    static const struct
    {
        uint32_t tag;
        uint32_t prot;
        const char *str;
    } recs[] = {
        {0x0, PROT_READ|PROT_EXEC, "__TEXT"},
        {0x0, PROT_READ, "__DATA_CONST"},
        {0x1, PROT_READ, "MALLOC metadata"},
        {0x1, PROT_READ|PROT_WRITE, "MALLOC metadata"},
        {0x1, 0, "MALLOC guard page"},
        {0x2, PROT_READ|PROT_WRITE, "MALLOC_SMALL"},
        {0x7, PROT_READ|PROT_WRITE, "MALLOC_TINY"},
        {0x1e, PROT_READ|PROT_WRITE, "Stack"},
        {0x1e, 0, "STACK GUARD"},
        {0x49, PROT_READ|PROT_WRITE, "Kernel Alloc Once"},
    };
    for (int i = 0 ; i < sizeof(recs)/sizeof(recs[0]) ; i++)
    {
        if (recs[i].tag == tag && recs[i].prot == prot)
            return recs[i].str;
    }
    static char buf[16];
    snprintf(buf, sizeof(buf), "tag:%x", tag);
    return buf;
}

/*
 * Walk and printg the kernel virtual memory regions for
 * the self process, similar to what the vmmap program does
 * but quicker and simpler.  Does not handle submaps but
 * that's OK for our purposes as we care about the regions
 * which map the text segment and submaps seem to be only
 * used for common system libraries.
 */
static void dump_regions(void)
{
    uint64_t address = 0;
    int pid = getpid();
    fprintf(stderr, "VIRTUAL MEMORY REGIONS\n");
    fprintf(stderr, "submap       user-tag                  start-end                prot share offset  path\n");
    fprintf(stderr, "------ -------------------- --------------------------------- ------- --- -------- ----\n");
    for (int i = 0 ; i < 100 ; i++)
    {
        struct proc_regionwithpathinfo buf;
        memset(&buf, 0, sizeof(buf));
        int r = proc_pidinfo(pid, PROC_PIDREGIONPATHINFO, address, &buf, sizeof(buf));
        if (r <= 0)
            break;
        char prot_buf[4], max_prot_buf[4];
        fprintf(stderr,
            "%s %-20s %016llx-%016llx %s/%s %-3s %08llx %s\n",
            (buf.prp_prinfo.pri_flags & PROC_REGION_SUBMAP ? "submap" : "      "),
            region_type_to_string(buf.prp_prinfo.pri_user_tag, buf.prp_prinfo.pri_protection),
            (unsigned long long)buf.prp_prinfo.pri_address,
            (unsigned long long)(buf.prp_prinfo.pri_address+buf.prp_prinfo.pri_size),
            prot_to_string(buf.prp_prinfo.pri_protection, prot_buf),
            prot_to_string(buf.prp_prinfo.pri_max_protection, max_prot_buf),
            share_mode_to_string(buf.prp_prinfo.pri_share_mode),
            (unsigned long long)buf.prp_prinfo.pri_offset,
            buf.prp_vip.vip_path);
        address = buf.prp_prinfo.pri_address + buf.prp_prinfo.pri_size;
    }
}

#if 0
/* First cut was to run the vmmap program on ourselves */
static void dump_regions(void)
{
    fprintf(stderr, "Running vmmap\n");
    char buf[1024];
    snprintf(buf, sizeof(buf), "/usr/bin/vmmap %d", getpid());
    system(buf);
    fprintf(stderr, "Finished running vmmap\n");
}
#endif

/*
 * This function exists to provide padding in the text
 * segment, at least as long as a page, around the breakme()
 * function to ensure that we can change the mapping of
 * the page containing breakme() without accidentally
 * making any of the code around it unexecutable.
 */
#define nopfunc nopfunc_1
#include "nopfunc.h"
#undef nopfunc

void breakme(void)
{
    printf("This function is for setting a breakpoint on\n");
}

#define nopfunc nopfunc_1001
#include "nopfunc.h"
#undef nopfunc

static int install_trap_mach(void *addr, const void *trapops, size_t size)
{
    dump_regions();

    mach_port_t self = mach_task_self();
    fprintf(stderr, "mach_task_self() = %x\n", self);
    fprintf(stderr, "getpid() = %x\n", getpid());
    kern_return_t r = vm_protect(
        self,
        (vm_address_t)addr,
        (vm_size_t)size,
        /*set_maximum*/false,
        VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY/*|VM_PROT_EXECUTE*/);
    if (r != KERN_SUCCESS)
    {
        fprintf(stderr, "vm_protect(VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY) failed, error %d\n", r);
        return -1;
    }
    fprintf(stderr, "vm_protect(PROT_READ|VM_PROT_WRITE|VM_PROT_COPY) succeeded\n");
    dump_regions();

    fprintf(stderr, "About to copy trapops bytes\n");
    memcpy(addr, trapops, size);
    fprintf(stderr, "successfully copied trapops bytes\n");

    r = vm_protect(
        self,
        (vm_address_t)addr,
        (vm_size_t)size,
        /*set_maximum*/false,
        VM_PROT_READ|VM_PROT_EXECUTE);
    if (r != KERN_SUCCESS)
    {
        fprintf(stderr, "vm_protect(VM_PROT_READ|VM_PROT_EXECUTE) failed, error %d\n", r);
        return -1;
    }
    fprintf(stderr, "vm_protect(VM_PROT_READ|VM_PROT_EXECUTE) succeeded\n");
    dump_regions();

    return 0;
}

#define ROUND_DOWN(x, q) (((x) / (q)) * (q))
#define ROUND_UP(x, q) ((((x)+(q)-1) / (q)) * (q))

static int install_trap_bsd(void *addr, const void *trapops, size_t size)
{
    dump_regions();

    unsigned long page_size = sysconf(_SC_PAGE_SIZE);
    unsigned long start_addr = ROUND_DOWN((unsigned long)addr, page_size);
    unsigned long end_addr = ROUND_UP((unsigned long)(breakme+1), page_size);
    int r = mprotect(
        (void *)start_addr,
        end_addr-start_addr,
        PROT_READ|PROT_WRITE);
    if (r < 0)
    {
        fprintf(stderr, "mprotect(PROT_READ|PROT_WRITE) failed: %s\n", strerror(errno));
        return -1;
    }
    fprintf(stderr, "mprotect(PROT_READ|PROT_WRITE) succeeded\n");

    dump_regions();

    fprintf(stderr, "About to copy trapops bytes\n");
    memcpy(addr, trapops, size);
    fprintf(stderr, "successfully copied trapops bytes\n");

    r = mprotect(
        (void *)start_addr,
        end_addr-start_addr,
        PROT_READ|PROT_EXEC);
    if (r < 0)
    {
        fprintf(stderr, "mprotect(PROT_READ|PROT_EXEC) failed: %s\n", strerror(errno));
        return -1;
    }
    fprintf(stderr, "mprotect(PROT_READ|PROT_EXEC) succeeded\n");

    dump_regions();

    return 0;
}

int main(int argc, char **argv)
{
    const char *mode = (argc > 1 ? argv[1] : "mach");

    fprintf(stderr, "About to call breakme() at 0x%016lx\n", (unsigned long)breakme);
    breakme();

    fprintf(stderr, "About to insert a debug trap instruction on the breakme() function\n");
    int r = -1;
    static const unsigned char trapops[] = { INSN_INT3 };
    if (!strcmp(mode, "mach"))
        r = install_trap_mach(breakme, trapops, sizeof(trapops));
    else if (!strcmp(mode, "bsd"))
        r = install_trap_bsd(breakme, trapops, sizeof(trapops));
    else
        fprintf(stderr, "Unknown mode: %s\n", mode);
    if (r < 0)
        return 1;

    fprintf(stderr, "About to call breakme() with a trap inserted\n");
    breakme();

    return (r < 0 ? 1 : 0);
}
