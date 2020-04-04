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
#include <sys/fcntl.h>
#include <bfd.h>
#include "state.hxx"
#include "reader.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"
#include "np/spiegel/platform/common.hxx"
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

static const char *get_partial_name(reference_t ref);

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
    address_index_.clear();
    linkobj_index_.clear();

    assert(instance_ == this);
    instance_ = 0;
}

bool
state_t::linkobj_t::is_in_plt(np::spiegel::addr_t addr) const
{
    vector<np::spiegel::mapping_t>::const_iterator i;
    for (i = plts_.begin() ; i != plts_.end() ; ++i)
    {
	if (i->contains((void *)addr))
	    return true;
    }
    return false;
}

/* See if the section can be satisfied out of
 * existing system mappings */
bool
state_t::linkobj_t::map_from_system(mapping_t &m) const
{
    vector<mapping_t>::const_iterator sm;
    for (sm = system_mappings_.begin() ; sm != system_mappings_.end() ; ++sm)
    {
	if (sm->contains(m))
	{
	    m.map_from(*sm);
	    return true;
	}
    }
    return false;
}

static void _np_bfd_error_handler(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (!strncmp(fmt, "%B", 2))
    {
        // The BFD library does this wacky thing where they pass
        // the struct bfd* as the printf argument of a fake %B
        // conversion string.  Really could they have made this
        // any less convenient?  FFS.  This is very difficult
        // to deal with properly given the stdarg interface.
        struct bfd *bfd = va_arg(args, struct bfd*);
        eprintf("In file %s:\n", bfd->filename);
        fmt += 2;
        if (*fmt == ':')
            fmt++;
        while (isspace(*fmt))
            fmt++;
    }

    _np_logvprintf(np::log::ERROR, fmt, args);
    va_end(args);
}

bool
state_t::linkobj_t::map_sections()
{
    int fd = -1;
    vector<section_t>::iterator m;
    bool r = true;
    int nsec = 0;   /* number of DWARF sections to be explicitly mapped herein */
    int ndwarf = 0; /* number of DWARF sections in the linkobj */

    bfd_init();
    bfd_set_error_handler(_np_bfd_error_handler);

    dprintf("opening bfd %s\n", filename_);
    /* Open a BFD */
    std::string path;
    bool is_separate = np::spiegel::platform::symbol_filename(filename_, path);
    if (!is_separate)
	path = filename_;

    dprintf("trying %s\n", path.c_str());
    bfd *b = bfd_openr(path.c_str(), NULL);
    if (!b)
    {
        eprintf("BFD library failed to open %s: %s\n",
                path.c_str(), bfd_errmsg(bfd_get_error()));
	return false;
    }
    if (!bfd_check_format(b, bfd_object))
    {
	eprintf("%s: not an object\n", path.c_str());
	goto error;
    }
    dprintf("Object %s\n", path.c_str());

    /* Extract the file shape of the DWARF sections */
    dprintf("sections:\n");
    asection *sec;
    for (sec = b->sections ; sec ; sec = sec->next)
    {
	if (np::spiegel::platform::is_plt_section(sec->name))
	{
	    mapping_t m((unsigned long)sec->filepos,
			(unsigned long)sec->size);
	    if (map_from_system(m))
		plts_.push_back(m);
	    continue;
	}

	int idx = secnames.to_index(sec->name);
	dprintf("section name %s size %lx filepos %lx index %d\n",
		sec->name, (unsigned long)sec->size,
		(unsigned long)sec->filepos, idx);
	if (idx == DW_sec_none)
	    continue;
	ndwarf++;
	sections_[idx].set_range((unsigned long)sec->filepos,
				 (unsigned long)sec->size);

	if (!is_separate)
	{
	    if (map_from_system(sections_[idx]))
		dprintf("found system mapping for section %s at %p\n",
			secnames.to_name(idx), sm->get_map());
	}
    }

    if (!ndwarf)
    {
	/* no DWARF sections at all */
	wprintf("no DWARF information found for %s, ignoring\n", filename_);
	r = true;
	goto out;
    }

    /* Coalesce unmapped sections into a minimal number of mappings */
    struct section_t *tsec[DW_sec_num];
    for (int idx = 0 ; idx < DW_sec_num ; idx++)
	if (!sections_[idx].is_mapped() &&
	    sections_[idx].get_size())
	    tsec[nsec++] = &sections_[idx];

    if (nsec)
    {
	/* some DWARF sections were not found in system mappings, need
	 * to mmap() them ourselves. */

	/* build a minimal set of mapping_t */
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

        if (is_enabled_for(np::log::DEBUG))
        {
            dprintf("mappings:\n");
            for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
            {
                dprintf("mapping offset 0x%lx size 0x%lx end 0x%lx\n",
                        m->get_offset(), m->get_size(), m->get_end());
            }
        }

	/* mmap() the mappings */
	fd = open(path.c_str(), O_RDONLY, 0);
	if (fd < 0)
	{
	    eprintf("Failed to open %s: %s", path.c_str(), strerror(errno));
	    goto error;
	}

	for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
	{
	    if (m->mmap(fd, /*read-only*/false) < 0)
	    {
                eprintf("Failed to mmap %s: %s", path.c_str(), strerror(errno));
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
    }

    if (is_enabled_for(np::log::DEBUG))
    {
        dprintf("new mappings after mmap()\n");
        for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
        {
            dprintf("    { offset=0x%lx size=0x%lx map=%p }\n",
                    m->get_offset(), m->get_size(), m->get_map());
        }
        dprintf("debug sections map:\n");
        for (int idx = 0 ; idx < DW_sec_num ; idx++)
        {
            dprintf("index %d name %s map 0x%lx size 0x%lx\n",
                    idx, secnames.to_name(idx),
                    (unsigned long)sections_[idx].get_map(),
                    sections_[idx].get_size());
        }
    }

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
    dprintf("reading compile units for linkobj %s\n", lo->filename_);
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

static const char *
filename_is_ignored(const char *filename)
{
    static const char * const prefixes[] =
    {
	"/bin/",
	"/lib/",
	"/lib64/",
	"/usr/bin/",
	"/usr/lib/",
	"/usr/lib64/",
	"/opt/",
	"linux-gate.so",
	"linux-vdso.so",
	NULL
    };
    const char * const *p;

    for (p = prefixes ; *p ; p++)
    {
	if (!strncmp(filename, *p, strlen(*p)))
	    return *p;
    }
    return 0;
}

bool
state_t::add_self()
{
    dprintf("Adding self executable\n");
    char *exe = np::spiegel::platform::self_exe();
    bool r = false;

    vector<np::spiegel::platform::linkobj_t> los = np::spiegel::platform::get_linkobjs();
    vector<np::spiegel::platform::linkobj_t>::iterator i;
    const char *filename;
    dprintf("converting platform linkobjs to spiegel linkobjs\n");
    for (i = los.begin() ; i != los.end() ; ++i)
    {
	dprintf("platform linkobj %s\n", i->name);
	filename = i->name;
	if (!filename)
	    filename = exe;

        const char *ignored_prefix = filename_is_ignored(filename);
	if (ignored_prefix)
        {
	    dprintf("matched prefix \"%s\", ignoring\n", ignored_prefix);
	    continue;
        }

	linkobj_t *lo = get_linkobj(filename);
	if (lo)
	{
	    dprintf("have spiegel linkobj\n");
	    lo->system_mappings_ = i->mappings;
	    lo->slide_ = i->slide;
	    vector<mapping_t>::iterator sm;
	    for (sm = lo->system_mappings_.begin() ; sm != lo->system_mappings_.end() ; ++sm)
		linkobj_index_.insert((np::spiegel::addr_t)sm->get_map(),
				      (np::spiegel::addr_t)sm->get_map() + sm->get_size(),
				      lo);
	}
    }

    r = read_linkobjs();
    if (r)
	prepare_address_index();
    free(exe);
    return r;
}

bool
state_t::add_executable(const char *filename)
{
    dprintf("Adding executable %s\n", filename);
    linkobj_t *lo = get_linkobj(filename);
    if (!lo)
	return false;
    bool r = read_linkobjs();
    if (r)
	prepare_address_index();
    return r;
}

state_t::linkobj_t *
state_t::get_linkobj(const char *filename)
{
    if (!filename)
	return 0;

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
	/* TODO: why isn't this in filename_is_ignored() ?? */
#if HAVE_VALGRIND
	/* Ignore Valgrind's preloaded dynamic objects.  No
	 * good will come of trying to poke into those.
	 */
	const char *tt = strrchr((*i)->filename_, '/');
	if (tt && !strncmp(tt, "/vgpreload_", 11))
	    continue;
#endif

	if (!(*i)->map_sections())
	    return false;
        /* note map_sections() can succeed but result in no sections */
        if ((*i)->has_sections() && !read_compile_units(*i))
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

    const char *name = get_partial_name(w.get_reference());
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

	    if (e->get_attribute(DW_AT_declaration) &&
		e->get_uint32_attribute(DW_AT_declaration))
		continue;

	    const char *structname = get_partial_name(w.get_reference());
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
		case DW_TAG_variable:	/* seen on some older toolchains, e.g. SLES11 */
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
	    if (!e->get_attribute(DW_AT_location))
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

compile_unit_t *
state_t::get_compile_unit_by_offset(uint32_t loindex, np::spiegel::offset_t off) const
{
    vector<compile_unit_t*>::const_iterator i;
    for (i = compile_units_.begin() ; i != compile_units_.end() ; ++i)
    {
        compile_unit_t *cu = *i;
        if (loindex == cu->get_link_object_index() &&
            cu->get_start_offset() <= off &&
            off < cu->get_end_offset())
            return cu;
    }
    return 0;
}

state_t::compile_unit_offset_tuple_t
state_t::resolve_reference(reference_t ref) const
{
    compile_unit_t *cu = 0;
    np::spiegel::offset_t off = 0;
    switch (ref.type)
    {
    case reference_t::REF_CU:
        cu = compile_units_[ref.cu];
        off = ref.offset;
        break;
    case reference_t::REF_ADDR:
        dprintf("Resolving REF_ADDR (cu=%u, offset=%llu)\n", ref.cu, ref.offset);
        cu = get_compile_unit_by_offset(ref.cu, ref.offset);
        if (cu)
            off = ref.offset - cu->get_start_offset();
        break;
    default:
        eprintf("WTF? trying to resolve reference of type %d, not implemented\n",
                (int)ref.type);
        break;
    }
    return compile_unit_offset_tuple_t(cu, off);
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
	/* In DWARF-4, DW_AT_high_pc can be absolute or relative
	 * depending on the form it was encoded in. */
	if (w.get_dwarf_version() == 4 &&
	    e->get_attribute_form(DW_AT_high_pc) != DW_FORM_addr)
	    hi += lo;
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
	    if (start == _NP_MAXADDR)
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
	w.set_filter_tag(DW_TAG_subprogram);
	while (const entry_t *e = w.move_preorder())
	{
	    assert(e->get_tag() == DW_TAG_subprogram);
	    if (e->get_attribute(DW_AT_specification))
		funcref = e->get_reference_attribute(DW_AT_specification);
	    else
		funcref = w.get_reference();
	    insert_ranges(w, funcref);
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
	    if (start == _NP_MAXADDR)
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

np::spiegel::addr_t
state_t::instrumentable_address(np::spiegel::addr_t addr) const
{
    vector<linkobj_t*>::const_iterator i;
    for (i = linkobjs_.begin() ; i != linkobjs_.end() ; ++i)
	if ((*i)->is_in_plt(addr))
	    return np::spiegel::platform::follow_plt(addr);
    return addr;
}

np::spiegel::addr_t
state_t::recorded_address(np::spiegel::addr_t addr) const
{
    np::util::rangetree<addr_t, linkobj_t*>::const_iterator i = linkobj_index_.find(addr);
    if (i != linkobj_index_.end())
	addr -= i->second->slide_;
    return addr;
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
	const char *name = e->get_string_attribute(DW_AT_name);
	if (name)
	{
	    if (full.length())
		full = string("::") + full;
	    full = string(name) + full;
	}
	e = w.move_up();
    } while (e);

    return full;
}

static const char *
get_partial_name(reference_t ref)
{
    walker_t w(ref);
    const entry_t *e = w.move_next();

    if (e->get_attribute(DW_AT_specification))
	e = w.move_to(e->get_reference_attribute(DW_AT_specification));
    return e->get_string_attribute(DW_AT_name);
}

// close namespaces
}; }; };
