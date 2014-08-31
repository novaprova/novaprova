#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <mach-o/dyld.h>

/*
 * See the dyld(3) manpage and
 * https://developer.apple.com/library/mac/documentation/DeveloperTools/Conceptual/MachORuntime/Reference/reference.html#//apple_ref/doc/uid/TP40000895
 */

static const char *
cputype_as_string(uint32_t cputype)
{
    switch (cputype)
    {
    case CPU_TYPE_POWERPC: return "PowerPC";
    case CPU_TYPE_I386: return "x86";
    case CPU_TYPE_X86_64: return "x86_64";
    default: return "unknown";
    }
}

static const char *
magic_as_string(uint32_t magic)
{
    switch (magic)
    {
    case MH_MAGIC: return "MAGIC";
    case MH_CIGAM: return "CIGAM";
    case MH_MAGIC_64: return "MAGIC_64";
    case MH_CIGAM_64: return "CIGAM_64";
    default: return "unknown";
    }
}

static const char *
filetype_as_string(uint32_t filetype)
{
    switch (filetype)
    {
    case MH_OBJECT: return "object";
    case MH_EXECUTE: return "executable";
    case MH_BUNDLE: return "bundle";
    case MH_DYLIB: return "dylib";
    case MH_PRELOAD: return "preload";
    case MH_CORE: return "core";
    case MH_DYLINKER: return "dylinker";
    case MH_DSYM: return "dsym";
    default: return "unknown";
    }
}

static const char *
flags_as_string(uint32_t flags)
{
    uint32_t flag;
    static char buf[1024];
    buf[0] = '\0';

    for (flag = 1 ; flag ; flag <<= 1)
    {
	if (!(flags & flag))
	    continue;
	if (buf[0])
	    strcat(buf, "|");
	switch (flag)
	{
	case MH_NOUNDEFS: strcat(buf, "NOUNDEFS"); break;
	case MH_INCRLINK: strcat(buf, "INCRLINK"); break;
	case MH_DYLDLINK: strcat(buf, "DYLDLINK"); break;
	case MH_TWOLEVEL: strcat(buf, "TWOLEVEL"); break;
	case MH_BINDATLOAD: strcat(buf, "BINDATLOAD"); break;
	case MH_PREBOUND: strcat(buf, "PREBOUND"); break;
	case MH_PREBINDABLE: strcat(buf, "PREBINDABLE"); break;
	case MH_NOFIXPREBINDING: strcat(buf, "NOFIXPREBINDING"); break;
	case MH_ALLMODSBOUND: strcat(buf, "ALLMODSBOUND"); break;
	case MH_CANONICAL: strcat(buf, "CANONICAL"); break;
	case MH_SPLIT_SEGS: strcat(buf, "SPLIT_SEGS"); break;
	case MH_FORCE_FLAT: strcat(buf, "FORCE_FLAT"); break;
	case MH_SUBSECTIONS_VIA_SYMBOLS: strcat(buf, "SUBSECTIONS_VIA_SYMBOLS"); break;
	case MH_NOMULTIDEFS: strcat(buf, "NOMULTIDEFS"); break;
	case MH_BINDS_TO_WEAK: strcat(buf, "BINDS_TO_WEAK"); break;
	case MH_NO_REEXPORTED_DYLIBS: strcat(buf, "NO_REEXPORTED_DYLIBS"); break;
	case MH_WEAK_DEFINES: strcat(buf, "WEAK_DEFINES"); break;
	default: sprintf(buf+strlen(buf), "0x%x", flag); break;
	}
    }

    return buf;
}

static const char *
load_command_as_string(uint32_t cmd)
{
    switch (cmd)
    {
    case LC_SEGMENT: return "SEGMENT"; break;
    case LC_SYMTAB: return "SYMTAB"; break;
    case LC_SYMSEG: return "SYMSEG"; break;
    case LC_THREAD: return "THREAD"; break;
    case LC_UNIXTHREAD: return "UNIXTHREAD"; break;
    case LC_LOADFVMLIB: return "LOADFVMLIB"; break;
    case LC_IDFVMLIB: return "IDFVMLIB"; break;
    case LC_IDENT: return "IDENT"; break;
    case LC_FVMFILE: return "FVMFILE"; break;
    case LC_PREPAGE: return "PREPAGE"; break;
    case LC_DYSYMTAB: return "DYSYMTAB"; break;
    case LC_LOAD_DYLIB: return "LOAD_DYLIB"; break;
    case LC_ID_DYLIB: return "ID_DYLIB"; break;
    case LC_LOAD_DYLINKER: return "LOAD_DYLINKER"; break;
    case LC_ID_DYLINKER: return "ID_DYLINKER"; break;
    case LC_PREBOUND_DYLIB: return "PREBOUND_DYLIB"; break;
    case LC_ROUTINES: return "ROUTINES"; break;
    case LC_SUB_FRAMEWORK: return "SUB_FRAMEWORK"; break;
    case LC_SUB_UMBRELLA: return "SUB_UMBRELLA"; break;
    case LC_SUB_CLIENT: return "SUB_CLIENT"; break;
    case LC_SUB_LIBRARY: return "SUB_LIBRARY"; break;
    case LC_TWOLEVEL_HINTS: return "TWOLEVEL_HINTS"; break;
    case LC_PREBIND_CKSUM: return "PREBIND_CKSUM"; break;
    case LC_LOAD_WEAK_DYLIB: return "LOAD_WEAK_DYLIB"; break;
    case LC_SEGMENT_64: return "SEGMENT_64"; break;
    case LC_ROUTINES_64: return "ROUTINES_64"; break;
    case LC_UUID: return "UUID"; break;
    case LC_RPATH: return "RPATH"; break;
    case LC_CODE_SIGNATURE: return "CODE_SIGNATURE"; break;
    case LC_SEGMENT_SPLIT_INFO: return "SEGMENT_SPLIT_INFO"; break;
    case LC_REEXPORT_DYLIB: return "REEXPORT_DYLIB"; break;
    case LC_LAZY_LOAD_DYLIB: return "LAZY_LOAD_DYLIB"; break;
    case LC_ENCRYPTION_INFO: return "ENCRYPTION_INFO"; break;
    case LC_DYLD_INFO: return "DYLD_INFO"; break;
    case LC_DYLD_INFO_ONLY: return "DYLD_INFO_ONLY"; break;
    case LC_LOAD_UPWARD_DYLIB: return "LOAD_UPWARD_DYLIB"; break;
    case LC_VERSION_MIN_MACOSX: return "VERSION_MIN_MACOSX"; break;
    case LC_VERSION_MIN_IPHONEOS: return "VERSION_MIN_IPHONEOS"; break;
    case LC_FUNCTION_STARTS: return "FUNCTION_STARTS"; break;
    case LC_DYLD_ENVIRONMENT: return "DYLD_ENVIRONMENT"; break;
    case LC_MAIN: return "MAIN"; break;
    case LC_DATA_IN_CODE: return "DATA_IN_CODE"; break;
    case LC_SOURCE_VERSION: return "SOURCE_VERSION"; break;
    case LC_DYLIB_CODE_SIGN_DRS: return "DYLIB_CODE_SIGN_DRS"; break;
    case LC_ENCRYPTION_INFO_64: return "ENCRYPTION_INFO_64"; break;
    case LC_LINKER_OPTION: return "LINKER_OPTION"; break;
    default: return "unknown";
    }
}

void
describe_load_command(const struct load_command *cmd)
{
    switch (cmd->cmd)
    {
    case LC_SEGMENT_64:
	{
	    const struct segment_command_64 *seg = (const struct segment_command_64 *)cmd;
	    char name[17];
	    memcpy(name, seg->segname, 16);
	    name[16] = '\0';
	    printf("        name \"%s\"\n", name);
	    printf("        vmaddr 0x%016llx\n", seg->vmaddr);
	    printf("        vmsize 0x%016llx\n", seg->vmsize);
	    printf("        fileoff 0x%016llx\n", seg->fileoff);
	    printf("        filesize 0x%016llx\n", seg->filesize);
	}
	break;
    }
}

int main(int argc, char **argv)
{
    uint32_t count;
    uint32_t i;

    count = _dyld_image_count();
    for (i = 0 ; i < count ; i++)
    {
	printf("image %u\n", i);
	printf("    name \"%s\"\n", _dyld_get_image_name(i));
	const struct mach_header *hdr = _dyld_get_image_header(i);
	if (hdr)
	{
	    printf("    magic 0x%08x (%s)\n", hdr->magic, magic_as_string(hdr->magic));
	    printf("    cputype %u (%s)\n", hdr->cputype, cputype_as_string(hdr->cputype));
	    printf("    cpusubtype %u\n", hdr->cpusubtype);
	    printf("    filetype %u (%s)\n", hdr->filetype, filetype_as_string(hdr->filetype));
	    printf("    ncmds %u\n", hdr->ncmds);
	    printf("    sizeofcmds %u\n", hdr->sizeofcmds);
	    printf("    flags %u (%s)\n", hdr->flags, flags_as_string(hdr->flags));

	    // load commands immediately follow the header, but there's
	    // an extra reserved field in the header on 64b
	    const struct load_command *cmds =
		    (hdr->magic == MH_MAGIC_64 || hdr->magic == MH_CIGAM_64 ?
			(const struct load_command *)(((struct mach_header_64 *)hdr)+1) :
			(const struct load_command *)(hdr+1));
	    uint32_t j = 0;
	    const struct load_command *cmd = cmds;
	    while (j < hdr->ncmds &&
		   ((unsigned long)cmd - (unsigned long)cmds) < hdr->sizeofcmds)
	    {
		printf("    command %u\n", j);
		printf("        cmd %u (%s)\n", cmd->cmd, load_command_as_string(cmd->cmd));
		describe_load_command(cmd);
		printf("        cmdsize %u\n", cmd->cmdsize);

		j++;
		cmd = (const struct load_command *)((unsigned long)cmd + cmd->cmdsize);
	    }
	}
    }

    return 0;
}
