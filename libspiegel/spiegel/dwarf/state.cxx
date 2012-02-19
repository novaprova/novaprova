#include "spiegel/common.hxx"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <bfd.h>
#include "state.hxx"
#include "reader.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"
#include "spiegel/platform/common.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

state_t *state_t::instance_ = 0;

state_t::state_t()
{
    assert(!instance_);
    instance_ = this;
}

state_t::~state_t()
{
    vector<linkobj_t*>::iterator i;
    for (i = linkobjs_.begin() ; i != linkobjs_.end() ; ++i)
	delete *i;

    assert(instance_ == this);
    instance_ = 0;
}

bool
state_t::linkobj_t::map_sections()
{
    int fd = -1;
    vector<section_t>::iterator m;
    bool r = true;

    bfd_init();

    /* Open a BFD */
    bfd *b = bfd_openr(filename_, NULL);
    if (!b)
    {
	bfd_perror(filename_);
	return false;
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
#if 0
	printf("Section [%d], name=%s size=%lx filepos=%lx\n",
	    idx, sec->name, (unsigned long)sec->size,
	    (unsigned long)sec->filepos);
#endif
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

#if 0
    printf("Mappings:\n");
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	printf("offset=%lx size=%lx end=%lx\n",
	       m->offset, m->size, m->get_end());
    }
#endif

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

#if 0
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=.debug_%s map=0x%lx size=0x%lx\n",
		idx, secnames.to_name(idx),
		(unsigned long)sections_[idx].map,
		sections_[idx].size);
    }
#endif

    goto out;
error:
    unmap_sections();
    r = false;
out:
    if (fd >= 0)
	close(fd);
    bfd_close(b);
    return r;
}

void
state_t::linkobj_t::unmap_sections()
{
    vector<section_t>::iterator m;
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	if (m->map)
	{
	    munmap(m->map, m->size);
	    m->map = NULL;
	}
    }
}

bool
state_t::read_compile_units(linkobj_t *lo)
{
    reader_t infor = lo->sections_[DW_sec_info].get_contents();
    reader_t abbrevr = lo->sections_[DW_sec_abbrev].get_contents();

    compile_unit_t *cu = 0;
    for (;;)
    {
	cu = new compile_unit_t(compile_units_.size(), lo->index_);
	if (!cu->read_header(infor))
	    break;

	cu->read_abbrevs(abbrevr);

	compile_units_.push_back(cu);
    }
    delete cu;
    return true;
}

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

bool
state_t::add_self()
{
    char *exe = spiegel::platform::self_exe();
    bool r = false;

    vector<spiegel::platform::linkobj_t> los = spiegel::platform::self_linkobjs();
    vector<spiegel::platform::linkobj_t>::iterator i;
    const char *filename;
    for (i = los.begin() ; i != los.end() ; ++i)
    {
	filename = i->name;
	if (!filename)
	    filename = "";

	if (!i->addr && !*filename)
	    filename = exe;
	else if (!i->addr || !*filename)
	    continue;

	if (filename_is_ignored(filename))
	    continue;

	if (!add_executable(filename))
	    goto out;
    }

    r = true;
out:
    free(exe);
    return r;
}

bool
state_t::add_executable(const char *filename)
{
    if (!filename)
	return false;

    linkobj_t *lo = new linkobj_t(filename,
				  linkobjs_.size());

    if (!lo->map_sections() ||
        !read_compile_units(lo))
    {
	delete lo;
	return false;
    }

    linkobjs_.push_back(lo);
    return true;
}

static void
describe_type(const walker_t &ow, reference_t ref)
{
    if (ref == reference_t::null)
    {
	printf("void ");
	return;
    }
    walker_t w = ow;
    const entry_t *e = w.move_to(ref);
    if (!e)
    {
	printf("??? ");
	return;
    }

    const char *name = e->get_string_attribute(DW_AT_name);
    switch (e->get_tag())
    {
    case DW_TAG_base_type:
    case DW_TAG_typedef:
	printf("%s ", name);
	break;
    case DW_TAG_pointer_type:
	describe_type(w, e->get_reference_attribute(DW_AT_type));
	printf("* ");
	break;
    case DW_TAG_volatile_type:
	describe_type(w, e->get_reference_attribute(DW_AT_type));
	printf("volatile ");
	break;
    case DW_TAG_const_type:
	describe_type(w, e->get_reference_attribute(DW_AT_type));
	printf("const ");
	break;
    case DW_TAG_structure_type:
	printf("struct %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_union_type:
	printf("union %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_class_type:
	printf("class %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_enumeration_type:
	printf("enum %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_array_type:
	describe_type(w, e->get_reference_attribute(DW_AT_type));
	for (e = w.move_down() ; e ; e = w.move_next())
	{
	    uint32_t count;
	    if (e->get_tag() == DW_TAG_subrange_type &&
		((count = e->get_uint32_attribute(DW_AT_count)) ||
		 (count = e->get_uint32_attribute(DW_AT_upper_bound))))
		printf("[%u]", count);
	}
	printf(" ");
	break;
    default:
	printf("%s ", tagnames.to_name(e->get_tag()));
	break;
    }
}

static void
describe_function_parameters(walker_t &w)
{
    printf("(");
    int nparams = 0;
    bool got_ellipsis = false;
    for (const entry_t *e = w.move_down() ; e ; e = w.move_next())
    {
	if (got_ellipsis)
	    continue;
	if (e->get_tag() == DW_TAG_formal_parameter)
	{
	    if (nparams++)
		printf(", ");
	    describe_type(w, e->get_reference_attribute(DW_AT_type));
	}
	else if (e->get_tag() == DW_TAG_unspecified_parameters)
	{
	    if (nparams++)
		printf(", ");
	    printf("...");
	    got_ellipsis = true;
	}
    }
    printf(")");
}

void
state_t::dump_structs()
{
    const char *keyword;

    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	walker_t w(*i);
	w.move_next();	// at the DW_TAG_compile_unit
	for (const entry_t *e = w.move_down() ; e ; e = w.move_next())
	{
	    switch (e->get_tag())
	    {
	    case DW_TAG_structure_type: keyword = "struct"; break;
	    case DW_TAG_union_type: keyword = "union"; break;
	    case DW_TAG_class_type: keyword = "class"; break;
	    default: continue;
	    }

	    const char *structname = e->get_string_attribute(DW_AT_name);
	    if (!structname)
		continue;
	    printf("ZZZ %s %s;\n", keyword, structname);

	    // print members
	    for (e = w.move_down() ; e ; e = w.move_next())
	    {
		const char *name = e->get_string_attribute(DW_AT_name);
		if (!name)
		    continue;

		switch (e->get_tag())
		{
		case DW_TAG_member:
		    printf("    /*member*/ ");
		    describe_type(w, e->get_reference_attribute(DW_AT_type));
		    printf(" %s;\n", name);
		    break;
		case DW_TAG_subprogram:
		    {
			if (!strcmp(name, structname))
			{
			    printf("    /*constructor*/ ");
			}
			else if (name[0] == '~' && !strcmp(name+1, structname))
			{
			    printf("    /*destructor*/ ");
			}
			else
			{
			    printf("    /*function*/ ");
			    describe_type(w, e->get_reference_attribute(DW_AT_type));
			}
			printf("%s", name);
			describe_function_parameters(w);
			printf("\n");
		    }
		    break;
		default:
		    printf("    /* %s */\n", tagnames.to_name(e->get_tag()));
		    continue;
		}
	    }
	}

	printf("} compile_unit\n");
    }
    printf("\n\n");
}

void
state_t::dump_functions()
{
    printf("Functions\n");
    printf("=========\n");

    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	walker_t w(*i);
	w.move_next();	// at the DW_TAG_compile_unit
	for (const entry_t *e = w.move_down() ; e ; e = w.move_next())
	{
	    if (e->get_tag() != DW_TAG_subprogram)
		continue;

	    const char *name = e->get_string_attribute(DW_AT_name);
	    if (!name)
		continue;

	    describe_type(w, e->get_reference_attribute(DW_AT_type));
	    printf("%s", name);
	    describe_function_parameters(w);
	    printf("\n");
	}

	printf("} compile_unit\n");
    }
    printf("\n\n");
}

void
state_t::dump_variables()
{
    printf("Variables\n");
    printf("=========\n");

    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");

	walker_t w(*i);
	w.move_next();	// at the DW_TAG_compile_unit
	for (const entry_t *e = w.move_down() ; e ; e = w.move_next())
	{
	    if (e->get_tag() != DW_TAG_variable)
		continue;

	    const char *name = e->get_string_attribute(DW_AT_name);
	    if (!name)
		continue;

	    describe_type(w, e->get_reference_attribute(DW_AT_type));
	    printf("%s;\n", name);
	}

	printf("} compile_unit\n");
    }
    printf("\n\n");
}


static void
recursive_dump(const entry_t *e, walker_t &w, unsigned depth)
{
    assert(e->get_level() == depth);
    e->dump();
    for (e = w.move_down() ; e ; e = w.move_next())
	recursive_dump(e, w, depth+1);
}

void
state_t::dump_info(bool preorder)
{
    printf("Info\n");
    printf("====\n");
    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");
	walker_t w(*i);
	if (preorder)
	{
	    while (const entry_t *e = w.move_preorder())
		e->dump();
	}
	else
	{
	    recursive_dump(w.move_next(), w, 0);
	}
	printf("} compile_unit\n");
    }
    printf("\n\n");
}

void
state_t::dump_abbrevs()
{
    printf("Abbrevs\n");
    printf("=======\n");
    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	printf("compile_unit {\n");
	(*i)->dump_abbrevs();
	printf("} compile_unit\n");
    }
    printf("\n\n");
}

// close namespace
} }
