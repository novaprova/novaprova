#include "np/spiegel/common.hxx"
#include <sys/fcntl.h>
#include <bfd.h>
#include "state.hxx"
#include "reader.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"
#include "np/spiegel/platform/common.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

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
    int nsec = 0;

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
#if 0
	printf("Section name=%s size=%lx filepos=%lx => %d\n",
	    sec->name, (unsigned long)sec->size,
	    (unsigned long)sec->filepos, idx);
#endif
	if (idx == DW_sec_none)
	    continue;
	sections_[idx].set_range((unsigned long)sec->filepos,
				 (unsigned long)sec->size);

	/* See if the section can be satisfied out of
	 * existing system mappings */
	vector<mapping_t>::iterator sm;
	for (sm = system_mappings_.begin() ; sm != system_mappings_.end() ; ++sm)
	{
	    if (sm->contains(sections_[idx]))
	    {
		sections_[idx].map_from(*sm);
		break;
	    }
	}
    }

    /* Coalesce unmapped sections into a minimal number of mappings */
    struct section_t *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	if (!sections_[idx].is_mapped() &&
	    sections_[idx].get_size())
	    tsec[nsec++] = &sections_[idx];
    qsort(tsec, nsec, sizeof(section_t *), mapping_t::compare_by_offset);
    for (int idx = 0 ; idx < nsec ; idx++)
    {
	if (mappings_.size() &&
	    page_round_up(mappings_.back().get_end()) >= tsec[idx]->get_offset())
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
	       m->get_offset(), m->get_size(), m->get_end());
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
	if (m->mmap(fd, /*read-only*/false) < 0)
	{
	    perror("mmap");
	    goto error;
	}
    }

    /* setup sections[].map */
    for (int idx = 0 ; idx < nsec ; idx++)
    {
	for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
	{
	    if (m->contains(*tsec[idx]))
	    {
		tsec[idx]->map_from(*m);
		break;
	    }
	}
	assert(tsec[idx]->is_mapped());
    }

#if 0
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
    {
	printf("[%d] name=%s map=0x%lx size=0x%lx\n",
		idx, secnames.to_name(idx),
		(unsigned long)sections_[idx].get_map(),
		sections_[idx].get_size());
    }
#endif

    if (sections_[DW_sec_plt].is_mapped())
	np::spiegel::platform::add_plt(sections_[DW_sec_plt]);

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
	m->munmap();
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
    char *exe = np::spiegel::platform::self_exe();
    bool r = false;

    vector<np::spiegel::platform::linkobj_t> los = np::spiegel::platform::get_linkobjs();
    vector<np::spiegel::platform::linkobj_t>::iterator i;
    const char *filename;
    for (i = los.begin() ; i != los.end() ; ++i)
    {
	filename = i->name;
	if (!filename)
	    filename = exe;

	if (filename_is_ignored(filename))
	    continue;

	linkobj_t *lo = get_linkobj(filename);
	if (lo)
	    lo->system_mappings_ = i->mappings;
    }

    r = read_linkobjs();
    free(exe);
    return r;
}

bool
state_t::add_executable(const char *filename)
{
    linkobj_t *lo = get_linkobj(filename);
    if (!lo)
	return false;
    return read_linkobjs();
}

state_t::linkobj_t *
state_t::get_linkobj(const char *filename)
{
    if (!filename)
	return false;

    vector<linkobj_t*>::iterator i;
    for (i = linkobjs_.begin() ; i != linkobjs_.end() ; ++i)
    {
	if (!strcmp((*i)->filename_, filename))
	    return (*i);
    }

    linkobj_t *lo = new linkobj_t(filename, linkobjs_.size());
    linkobjs_.push_back(lo);
    return lo;
}

bool
state_t::read_linkobjs()
{
    vector<linkobj_t*>::iterator i;
    for (i = linkobjs_.begin() ; i != linkobjs_.end() ; ++i)
    {
	if (!(*i)->map_sections() ||
	    !read_compile_units(*i))
	    return false;
    }
    return true;
}

static void
describe_type(const walker_t &ow)
{
    walker_t w = ow;
    const entry_t *e = ow.get_entry();
    reference_t ref;

    if (e->get_attribute(DW_AT_specification))
	e = w.move_to(e->get_reference_attribute(DW_AT_specification));

    ref = e->get_reference_attribute(DW_AT_type);
    if (ref == reference_t::null)
    {
	printf("void ");
	return;
    }

    e = w.move_to(ref);
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
	describe_type(w);
	printf("* ");
	break;
    case DW_TAG_volatile_type:
	describe_type(w);
	printf("volatile ");
	break;
    case DW_TAG_const_type:
	describe_type(w);
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
    case DW_TAG_namespace_type:
	printf("namespace %s ", (name ? name : "{...}"));
	break;
    case DW_TAG_array_type:
	describe_type(w);
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
	    describe_type(w);
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
	while (const entry_t *e = w.move_preorder())
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
	    printf("%s %s {\n", keyword, get_full_name(w.get_reference()).c_str());

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
		    describe_type(w);
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
			    describe_type(w);
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
	    printf("} %s\n", keyword);
	}

	printf("} compile_unit\n");
    }
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

	    describe_type(w);
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
    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	walker_t w(*i);
	const entry_t *e = w.move_next();	// at the DW_TAG_compile_unit

	printf("compile_unit %s {\n",
	       e->get_string_attribute(DW_AT_name));

	while ((e = w.move_preorder()))
	{
	    if (e->get_tag() != DW_TAG_variable)
		continue;

	    describe_type(w);
	    printf("%s;\n", get_full_name(w.get_reference()).c_str());
	}

	printf("} compile_unit\n");
    }
}

static void
dump_path(const walker_t &w)
{
    printf("Path:");
    vector<reference_t> path = w.get_path();
    vector<reference_t>::iterator i;
    for (i = path.begin() ; i != path.end() ; ++i)
	printf(" %s", i->as_string().c_str());
    printf("\n");
}

static void
recursive_dump(const entry_t *e, walker_t &w, unsigned depth, bool paths)
{
    assert(e->get_level() == depth);
    if (paths)
	dump_path(w);
    e->dump();
    for (e = w.move_down() ; e ; e = w.move_next())
	recursive_dump(e, w, depth+1, paths);
}

void
state_t::dump_info(bool preorder, bool paths)
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
	    {
		if (paths)
		    dump_path(w);
		e->dump();
	    }
	}
	else
	{
	    recursive_dump(w.move_next(), w, 0, paths);
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

void
state_t::insert_ranges(const walker_t &w, reference_t funcref)
{
    const entry_t *e = w.get_entry();
    bool has_lo = (e->get_attribute(DW_AT_low_pc) != 0);
    uint64_t lo = e->get_uint64_attribute(DW_AT_low_pc);
    bool has_hi = (e->get_attribute(DW_AT_high_pc) != 0);
    uint64_t hi = e->get_uint64_attribute(DW_AT_high_pc);
    // DW_AT_ranges is a DWARF3 attribute, but g++ generates
    // it (despite only claiming DWARF2 compliance).
    uint64_t ranges = e->get_uint64_attribute(DW_AT_ranges);

    if (has_lo && has_hi)
    {
	address_index_.insert(lo, hi, funcref);
    }
    else if (ranges)
    {
	reader_t r = w.get_section_contents(DW_sec_ranges);
	r.skip(ranges);
	np::spiegel::addr_t base = 0; // TODO: compile unit base
	np::spiegel::addr_t start, end;
	for (;;)
	{
	    if (!r.read_addr(start) || !r.read_addr(end))
		break;
	    /* (0,0) marks the end of the list */
	    if (!start && !end)
		break;
	    /* (~0,base) marks a new base address */
	    if (start == SPIEGEL_MAXADDR)
	    {
		base = end;
		continue;
	    }
	    start += base;
	    end += base;
	    address_index_.insert(start, end, funcref);
	}
    }
    else if (has_lo)
    {
	address_index_.insert(lo, funcref);
    }
}

void
state_t::prepare_address_index()
{
    reference_t funcref;

    vector<compile_unit_t*>::iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	walker_t w(*i);
	while (const entry_t *e = w.move_preorder())
	{
	    switch (e->get_tag())
	    {
	    case DW_TAG_subprogram:
		if (e->get_attribute(DW_AT_specification))
		    funcref = e->get_reference_attribute(DW_AT_specification);
		else
		    funcref = w.get_reference();
		insert_ranges(w, funcref);
		break;
	    }
	}
    }
}

bool
state_t::is_within(np::spiegel::addr_t addr, const walker_t &w,
		   unsigned int &offset) const
{
    const entry_t *e = w.get_entry();
    bool has_lo = (e->get_attribute(DW_AT_low_pc) != 0);
    uint64_t lo = e->get_uint64_attribute(DW_AT_low_pc);
    bool has_hi = (e->get_attribute(DW_AT_high_pc) != 0);
    uint64_t hi = e->get_uint64_attribute(DW_AT_high_pc);
    // DW_AT_ranges is a DWARF3 attribute, but g++ generates
    // it (despite only claiming DWARF2 compliance).
    uint64_t ranges = e->get_uint64_attribute(DW_AT_ranges);
    if (has_lo && has_hi)
    {
	if (addr >= lo && addr <= hi)
	{
	    offset = (addr - lo);
	    return true;
	}
	return false;
    }
    if (ranges)
    {
	reader_t r = w.get_section_contents(DW_sec_ranges);
	r.skip(ranges);
	np::spiegel::addr_t base = 0; // TODO: compile unit base
	np::spiegel::addr_t start, end;
	for (;;)
	{
	    if (!r.read_addr(start) || !r.read_addr(end))
		break;
	    /* (0,0) marks the end of the list */
	    if (!start && !end)
		break;
	    /* (~0,base) marks a new base address */
	    if (start == SPIEGEL_MAXADDR)
	    {
		base = end;
		continue;
	    }
	    start += base;
	    end += base;
	    if (addr >= start && addr < end)
	    {
		offset = addr - base;
		return true;
	    }
	}
    }
    if (has_lo)
    {
	if (addr == lo)
	{
	    offset = 0;
	    return true;
	}
	return false;
    }
    return false;
}

bool
state_t::describe_address(np::spiegel::addr_t addr,
			  reference_t &curef,
			  unsigned int &lineno,
			  reference_t &funcref,
			  unsigned int &offset) const
{
    // initialise all the results to the "dunno" case
    curef = reference_t::null;
    lineno = 0;
    funcref = reference_t::null;
    offset = 0;

    if (address_index_.size())
    {
	np::util::rangetree<addr_t, reference_t>::const_iterator i = address_index_.find(addr);
	if (i == address_index_.end())
	    return false;
	offset = addr - i->first.lo;
	funcref = i->second;
	return true;
    }

    vector<compile_unit_t*>::const_iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
	walker_t w((*i)->make_root_reference());
	const entry_t *e = w.move_next();
	for (; e ; e = w.move_next())
	{
	    if (is_within(addr, w, offset))
	    {
		switch (e->get_tag())
		{
		case DW_TAG_compile_unit:
		    curef = w.get_reference();
		    e = w.move_down();
		    break;
		case DW_TAG_subprogram:
		    if (e->get_attribute(DW_AT_specification))
			e = w.move_to(e->get_reference_attribute(DW_AT_specification));
		    funcref = w.get_reference();
		    return true;
		case DW_TAG_class_type:
		case DW_TAG_structure_type:
		case DW_TAG_union_type:
		case DW_TAG_namespace_type:
		    e = w.move_down();
		    break;
		}
	    }
	}
    }
    return false;
}

string
state_t::get_full_name(reference_t ref)
{
    string full;
    walker_t w(ref);
    const entry_t *e = w.move_next();

    do
    {
	if (e->get_attribute(DW_AT_specification))
	    e = w.move_to(e->get_reference_attribute(DW_AT_specification));
	if (e->get_tag() == DW_TAG_compile_unit)
	    break;
	if (full.length())
	    full = string("::") + full;
	full = string(e->get_string_attribute(DW_AT_name)) + full;
	e = w.move_up();
    } while (e);

    return full;
}

// close namespaces
}; }; };
