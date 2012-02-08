#include "commonp.H"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <bfd.h>

enum dwarf_sections
{
    /* These names are not defined in the DWARF standard
     * but use a similar naming style for consistency */
    DW_sec_aranges,
    DW_sec_pubnames,
    DW_sec_info,
    DW_sec_abbrev,
    DW_sec_line,
    DW_sec_frame,
    DW_sec_str,
    DW_sec_loc,
    DW_sec_ranges,

    DW_sec_num
};

static const char * const secnames[DW_sec_num] = {
    "aranges", "pubnames", "info",
    "abbrev", "line", "frame",
    "str", "loc", "ranges"
};

static int
secname_to_index(const char *name)
{
    if (strncmp(name, ".debug_", 7))
	return -1;
    name += 7;
    for (int i = 0 ; i < DW_sec_num ; i++)
    {
	if (!strcmp(secnames[i], name))
	    return i;
    }
    return -1;
}

static unsigned long
pagesize(void)
{
    static unsigned long ps;
    if (!ps)
	ps = sysconf(_SC_PAGESIZE);
    return ps;
}

static unsigned long
page_round_up(unsigned long x)
{
    unsigned long ps = pagesize();
    return ((x + ps - 1) / ps) * ps;
}

static unsigned long
page_round_down(unsigned long x)
{
    unsigned long ps = pagesize();
    return (x / ps) * ps;
}

struct section
{
    unsigned long offset;
    unsigned long size;
    void *map;

    unsigned long get_end() const { return offset + size; }
    void set_end(unsigned long e) { size = e - offset; }
    bool contains(const section &o)
    {
	return (offset <= o.offset &&
		get_end() >= o.get_end());
    }

    void expand_to_pages()
    {
	unsigned long end = page_round_up(offset + size);
	offset = page_round_down(offset);
	size = end - offset;
    }
};

static struct section sections[DW_sec_num];
static struct section mappings[DW_sec_num];
static int nmappings;

static int
compare_section_ptrs(const void *v1, const void *v2)
{
    const struct section **s1 = (const struct section **)v1;
    const struct section **s2 = (const struct section **)v2;
    return u64cmp((*s1)->offset, (*s2)->offset);
}

static void
dump_dwarf(const char *filename)
{
    int fd = -1;

    bfd_init();

    /* Open a BFD */
    bfd *b = bfd_openr(filename, NULL);
    if (!b)
    {
	bfd_perror(filename);
	return;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	fprintf(stderr, "spiegel: %s: not an object\n", filename);
	goto out;
    }

    /* Extract the file shape of the DWARF sections */
    asection *sec;
    for (sec = b->sections ; sec ; sec = sec->next)
    {
	int idx = secname_to_index(sec->name);
	if (idx < 0)
	    continue;
	printf("Section [%d], name=%s size=%lx filepos=%lx\n",
	    idx, sec->name, (unsigned long)sec->size,
	    (unsigned long)sec->filepos);
	sections[idx].offset = (unsigned long)sec->filepos;
	sections[idx].size = (unsigned long)sec->size;
    }

    /* Coalesce sections into a minimal number of mappings */
    struct section *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	tsec[idx] = &sections[idx];
    qsort(tsec, DW_sec_num, sizeof(struct section *), compare_section_ptrs);
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	if (nmappings &&
	    page_round_up(mappings[nmappings-1].get_end()) >= tsec[idx]->offset)
	{
	    /* section abuts the last mapping, extend it */
	    mappings[nmappings-1].set_end(tsec[idx]->get_end());
	}
	else
	{
	    /* create a new mapping */
	    mappings[nmappings++] = *tsec[idx];
	}
    }

    printf("Mappings:\n");
    for (int m = 0 ; m < nmappings ; m++)
    {
	printf("[%d] offset=%lx size=%lx end=%lx\n",
	    m, mappings[m].offset, mappings[m].size,
	    mappings[m].get_end());
    }

    /* mmap() the mappings */
    fd = open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(filename);
	goto out;
    }

    for (int m = 0 ; m < nmappings ; m++)
    {
	mappings[m].expand_to_pages();
	void *map = mmap(NULL, mappings[m].size,
			 PROT_READ, MAP_SHARED, fd,
			 mappings[m].offset);
	if (map == MAP_FAILED)
	{
	    perror("mmap");
	    goto out;
	}
	mappings[m].map = map;
    }

    /* setup sections[].map */
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	for (int m = 0 ; m < nmappings ; m++)
	{
	    if (mappings[m].contains(sections[idx]))
	    {
		sections[idx].map = (void *)(
		    (char *)mappings[m].map +
		    sections[idx].offset -
		    mappings[m].offset);
		break;
	    }
	}
	assert(sections[idx].map);
    }

    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=.debug_%s map=0x%lx size=0x%lx\n",
		idx, secnames[idx],
		(unsigned long)sections[idx].map,
		sections[idx].size);
    }

out:
    for (int m = 0 ; m < nmappings ; m++)
    {
	if (mappings[m].map)
	{
	    munmap(mappings[m].map, mappings[m].size);
	    mappings[m].map = NULL;
	}
    }
    if (fd >= 0)
	close(fd);
    /* Cleanup the BFD */
    bfd_close(b);
}

int
main(int argc, char **argv)
{
    if (argc != 2)
	fatal("Usage: spiegel EXE\n");
    dump_dwarf(argv[1]);
    return 0;
}

