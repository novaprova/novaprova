#include "common.h"
#include "u4c_priv.h"
#include <elf.h>
#include <dlfcn.h>
#include <link.h>

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

extern char **environ;

/*
 * Find and return the filename of the currently running executable.
 *
 * Uses the aux vector given us by the kernel, on the primary stack
 * above the argv[] and environ[] arrays.
 */
static const char *
exe_filename(void)
{
    char **p;
    ElfW(auxv_t) *a;

    /* skip environ[] */
    for (p = environ ; *p ; p++)
	;
    p++;

    /* p now points to the aux array.  probably */
    for (a = (ElfW(auxv_t) *)p ; a->a_type != AT_NULL ; a++)
    {
	if (a->a_type == AT_EXECFN)
	    return (const char *)a->a_un.a_val;
    }

    /* This basically never happens...unless we're running under Valgrind */
    {
	static char buf[PATH_MAX];
	int r = readlink("/proc/self/exe", buf, sizeof(buf));
	if (r > 0)
	    return buf;
    }

    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
__u4c_describe_address(u4c_globalstate_t *state,
		       unsigned long addr,
		       const char **filenamep,
		       unsigned int *linenop,
		       const char **functionp)
{
    u4c_object_t *o;
    struct bfd_section *sec;

#ifdef __i386__
    /* addr is a return address; adjust it to the start of
     * the call instruction */
    addr -= 5;
#endif

    for (o = state->objects ; o ; o = o->next)
    {
	if (!o->bfd)
	    continue;
	for (sec = o->bfd->sections ;
	     sec ;
	     sec = sec->next)
	{
	    if ((bfd_vma)addr >= sec->vma &&
		(bfd_vma)addr < sec->vma + sec->size)
		return bfd_find_nearest_line(o->bfd, sec,
			o->syms, (bfd_vma)addr - sec->vma,
			filenamep, functionp, linenop);
	}
    }
    return false;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static bool
filename_is_ignored(const char *filename)
{
    static const char * const prefixes[] =
    {
	"/bin/",
	"/lib/",
	"/usr/bin/",
	"/usr/lib/",
	"/opt/",
	NULL
    };
    const char * const *p;

    for (p = prefixes ; *p ; p++)
    {
	if (!strncmp(filename, *p, strlen(*p)))
	    return true;
    }
    return false;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#if 0
static const char *sym_flags_as_string(unsigned int flags)
{
    static char buf[2048];
    char *p = buf;
    int remain = sizeof(buf)-1;
    unsigned int i;
    static const struct
    {
	unsigned int flag;
	const char *name;
    }
    flagdesc[] =
    {
	{ BSF_LOCAL, "local" },
	{ BSF_GLOBAL, "global" },
	{ BSF_DEBUGGING, "debugging" },
	{ BSF_FUNCTION, "function" },
	{ BSF_KEEP, "keep" },
	{ BSF_KEEP_G, "keep_g" },
	{ BSF_WEAK, "weak" },
	{ BSF_SECTION_SYM, "section_sym" },
	{ BSF_OLD_COMMON, "old_common" },
	{ BFD_FORT_COMM_DEFAULT_VALUE, "fortran_common" },
	{ BSF_NOT_AT_END, "not_at_end" },
	{ BSF_CONSTRUCTOR, "constructor" },
	{ BSF_WARNING, "warning" },
	{ BSF_INDIRECT, "indirect" },
	{ BSF_FILE, "file" },
	{ BSF_DYNAMIC, "dynamic" },
	{ BSF_OBJECT, "object" },
	{ BSF_DEBUGGING_RELOC, "debugging_reloc" },
	{ BSF_THREAD_LOCAL, "thread_local" },
	{ BSF_RELC, "relc" },
	{ BSF_SRELC, "srelc" },
	{ BSF_SYNTHETIC, "synthetic" },
	{ 0, NULL },
    };

    buf[0] = '\0';
    for (i = 0 ; flagdesc[i].name && remain > 0 ; i++)
    {
	if (!(flags & flagdesc[i].flag))
	    continue;
	flags &= ~flagdesc[i].flag;
	if (p != buf) {
	    *p++ = '|';
	    remain--;
	}
	strcpy(p, flagdesc[i].name);
	remain -= strlen(p);
	p += strlen(p);
    }

    return buf;
}
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
add_one_object(struct dl_phdr_info *info,
	       size_t size __attribute__((unused)),
	       void *closure)
{
    u4c_globalstate_t *state = closure;

    __u4c_add_object(state, 
		     info->dlpi_name,
		     (unsigned long)info->dlpi_addr);
    return 0;
}

void
__u4c_discover_objects(u4c_globalstate_t *state)
{
    dl_iterate_phdr(add_one_object, state);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static int
compare_syms_by_value(const void *v1, const void *v2)
{
    const asymbol *s1 = *(const asymbol **)v1;
    const asymbol *s2 = *(const asymbol **)v2;

    if (s1->value > s2->value)
	return 1;
    if (s1->value < s2->value)
	return -1;
    return 0;
}

void
__u4c_discover_functions(u4c_globalstate_t *state)
{
    u4c_object_t *o;
    bfd *b;
    const char *exe = exe_filename();

    bfd_init();

    fprintf(stderr, "u4c: exe_filename=\"%s\"\n", exe);

    for (o = state->objects ; o ; o = o->next)
    {
	const char *filename;
	int nsyms;
	asymbol **syms;
	asymbol *s;
	int i;

	if (!o->base && !*o->name)
	    filename = exe;
	else if (o->base && *o->name)
	    filename = o->name;
	else
	    continue;

	if (filename_is_ignored(filename))
	    continue;

	b = bfd_openr(filename, NULL);
	if (!b)
	{
	    bfd_perror(filename);
	    continue;
	}
	if (!bfd_check_format(b, bfd_object))
	{
	    fprintf(stderr, "u4c: %s: not an object\n", filename);
	    bfd_close(b);
	    continue;
	}
// 	fprintf(stderr, "u4c: filename=\"%s\" b=%p\n", filename, b);

	nsyms = bfd_get_symtab_upper_bound(b);
// 	fprintf(stderr, "u4c: nsyms=%d\n", nsyms);
	syms = xmalloc(sizeof(asymbol*) * nsyms);
	nsyms = bfd_canonicalize_symtab(b, syms);
// 	fprintf(stderr, "u4c: nsyms=%d\n", nsyms);

	qsort(syms, nsyms, sizeof(asymbol *), compare_syms_by_value);

#if 0
	for (i = 0 ; i < nsyms ; i++)
	{
	    s = syms[i];
	    fprintf(stderr, "%08lx %s %s %s\n",
		    (unsigned long)s->value,
		    sym_flags_as_string(s->flags),
		    s->section->name,
		    s->name);
	}
#endif
	for (i = 0 ; i < nsyms ; i++)
	{
	    enum u4c_functype type;
	    const char *filename = NULL;
	    unsigned int lineno = 0;
	    unsigned long addr;
	    char submatch[512];

	    s = syms[i];

	    if (!(s->flags & BSF_FUNCTION))
		continue;
	    if (strcmp(s->section->name, ".text"))
		continue;
	    type = __u4c_classify_function(state, s->name,
					   submatch, sizeof(submatch));
	    if (type == FT_UNKNOWN)
		continue;
	    if (type == FT_TEST && !submatch[0])
		continue;
	    if (!bfd_find_line(b, syms, s, &filename, &lineno))
		continue;
	    addr = (unsigned long)bfd_asymbol_value(s);

// 	    fprintf(stderr, "u4c: function \"%s\" type %d location \"%s\":%u\n",
// 			    s->name, type, filename, lineno);

	    __u4c_add_function(state, type, s->name, filename, submatch,
			       (void(*)(void))addr, o);
	}
	o->bfd = b;
	o->nsyms = nsyms;
	o->syms = syms;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
