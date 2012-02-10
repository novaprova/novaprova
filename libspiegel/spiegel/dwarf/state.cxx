#include "spiegel/commonp.hxx"
#include <sys/mman.h>
#include <vector>
#include <sys/fcntl.h>
#include <bfd.h>
#include "state.hxx"
#include "reader.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

state_t::state_t(const char *filename)
 :  filename_(xstrdup(filename))
{
    memset(sections_, 0, sizeof(sections_));
}

state_t::~state_t()
{
    free(filename_);
}

void
state_t::map_sections()
{
    int fd = -1;
    vector<section_t>::iterator m;

    bfd_init();

    /* Open a BFD */
    bfd *b = bfd_openr(filename_, NULL);
    if (!b)
    {
	bfd_perror(filename_);
	return;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	fprintf(stderr, "spiegel: %s: not an object\n", filename_);
	goto error;
    }

    /* Extract the file shape of the DWARF sections */
    asection *sec;
    for (sec = b->sections ; sec ; sec = sec->next)
    {
	int idx = secnames.to_index(sec->name);
	if (idx == DW_sec_none)
	    continue;
	printf("Section [%d], name=%s size=%lx filepos=%lx\n",
	    idx, sec->name, (unsigned long)sec->size,
	    (unsigned long)sec->filepos);
	sections_[idx].offset = (unsigned long)sec->filepos;
	sections_[idx].size = (unsigned long)sec->size;
    }

    /* Coalesce sections into a minimal number of mappings */
    struct section_t *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	tsec[idx] = &sections_[idx];
    qsort(tsec, DW_sec_num, sizeof(section_t *), section_t::compare_ptrs);
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	if (!tsec[idx]->size)
	    continue;
	if (mappings_.size() &&
	    page_round_up(mappings_.back().get_end()) >= tsec[idx]->offset)
	{
	    /* section abuts the last mapping, extend it */
	    mappings_.back().set_end(tsec[idx]->get_end());
	}
	else
	{
	    /* create a new mapping */
	    mappings_.push_back(*tsec[idx]);
	}
    }

    printf("Mappings:\n");
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	printf("offset=%lx size=%lx end=%lx\n",
	       m->offset, m->size, m->get_end());
    }

    /* mmap() the mappings */
    fd = open(filename_, O_RDONLY, 0);
    if (fd < 0)
    {
	perror(filename_);
	goto error;
    }

    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	m->expand_to_pages();
	void *map = mmap(NULL, m->size,
			 PROT_READ, MAP_SHARED, fd,
			 m->offset);
	if (map == MAP_FAILED)
	{
	    perror("mmap");
	    goto error;
	}
	m->map = map;
    }

    /* setup sections[].map */
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
	{
	    if (m->contains(sections_[idx]))
	    {
		sections_[idx].map = (void *)(
		    (char *)m->map +
		    sections_[idx].offset -
		    m->offset);
		break;
	    }
	}
	assert(sections_[idx].map);
    }

    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=.debug_%s map=0x%lx size=0x%lx\n",
		idx, secnames.to_name(idx),
		(unsigned long)sections_[idx].map,
		sections_[idx].size);
    }

    goto out;
error:
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	if (m->map)
	{
	    munmap(m->map, m->size);
	    m->map = NULL;
	}
    }
out:
    if (fd >= 0)
	close(fd);
    bfd_close(b);
}

static void
describe_type(const walker_t &ow, const reference_t *ref)
{
    if (!ref)
    {
	printf("??? ");
	return;
    }
    walker_t w = ow;
    if (!w.move_to(*ref))
    {
	printf("??? ");
	return;
    }
    const entry_t &e = w.get_entry();

    const char *name = e.get_string_attribute(DW_AT_name);
    switch (e.get_tag())
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
	printf("%s ", name);
	break;
    case DW_TAG_pointer_type:
	describe_type(w, e.get_reference_attribute(DW_AT_type));
	printf("* ");
	break;
    case DW_TAG_volatile_type:
	describe_type(w, e.get_reference_attribute(DW_AT_type));
	printf("volatile ");
	break;
    case DW_TAG_const_type:
	describe_type(w, e.get_reference_attribute(DW_AT_type));
	printf("const ");
	break;
    case DW_TAG_structure_type:
	printf("struct %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_union_type:
	printf("union %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_enumeration_type:
	printf("enum %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_array_type:
	describe_type(w, e.get_reference_attribute(DW_AT_type));
	if (w.move_to_children())
	{
	    do
	    {
		const entry_t &e = w.get_entry();
		uint32_t count;
		if (e.get_tag() == DW_TAG_subrange_type &&
		    ((count = e.get_uint32_attribute(DW_AT_count)) ||
		     (count = e.get_uint32_attribute(DW_AT_upper_bound))))
		    printf("[%u]", count);
	    } while (w.move_to_sibling());
	}
	printf(" ");
	break;
    default:
	printf("%s ", tagnames.to_name(e.get_tag()));
	break;
    }
}

void
state_t::dump_structs()
{
    const char *keyword;

    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	walker_t w(*this, *i);
	w.move_to_sibling();
	w.move_to_children();

	do
	{
	    const entry_t &e = w.get_entry();
	    switch (e.get_tag())
	    {
	    case DW_TAG_structure_type: keyword = "struct"; break;
	    case DW_TAG_union_type: keyword = "union"; break;
	    case DW_TAG_class_type: keyword = "class"; break;
	    default: continue;
	    }

	    const char *name = e.get_string_attribute(DW_AT_name);
	    if (!name)
		continue;
	    printf("ZZZ %s %s;\n", keyword, name);

	    // print members
	    if (!w.move_to_children())
		continue;
	    do
	    {
		const entry_t &e = w.get_entry();
		const char *name = e.get_string_attribute(DW_AT_name);
		if (!name)
		    continue;

		switch (e.get_tag())
		{
		case DW_TAG_member: keyword = "member"; break;
		case DW_TAG_subprogram: keyword = "function"; break;
		default: continue;
		}
		printf("    %s ", keyword);
		describe_type(w, e.get_reference_attribute(DW_AT_type));
		printf(" %s;\n", name);

	    } while (w.move_to_sibling());
	    w.move_preorder();

	} while (w.move_to_sibling());

	printf("} compile_unit\n");
    }
    printf("\n\n");
}

void
state_t::read_compile_units()
{
    reader_t infor = sections_[DW_sec_info].get_contents();
    reader_t abbrevr = sections_[DW_sec_abbrev].get_contents();

    compile_unit_t *cu = 0;
    for (;;)
    {
	cu = new compile_unit_t(compile_units_.size());
	if (!cu->read_header(infor))
	    break;

	cu->read_abbrevs(abbrevr);

	walker_t w(*this, cu);
	if (!cu->read_compile_unit_entry(w))
	    break;

	compile_units_.push_back(cu);
    }
    delete cu;
}

#if 1
static void
preorder_dump2(walker_t &w, unsigned depth)
{
    do
    {
	assert(w.get_entry().get_abbrev());
	assert(depth == w.get_entry().get_level());
	w.get_entry().dump();
	if (w.move_to_children())
	    preorder_dump2(w, depth+1);
    } while (w.move_to_sibling());
}

static void
preorder_dump(walker_t &w)
{
    w.move_to_sibling();
    preorder_dump2(w, 0);
}

#else
static void
preorder_dump(walker_t &w)
{
    while (w.move_preorder())
	w.get_entry().dump();
}

#endif

void
state_t::dump_info()
{
    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	(*i)->dump_abbrevs();

	walker_t w(*this, *i);
	preorder_dump(w);

	printf("} compile_unit\n");
    }
    printf("\n\n");
}

// close namespace
} }
