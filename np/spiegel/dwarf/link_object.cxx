/*
 * Copyright 2011-2020 Gregory Banks
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
#include "link_object.hxx"
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

bool
link_object_t::is_in_plt(np::spiegel::addr_t addr) const
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
link_object_t::map_from_system(mapping_t &m) const
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

static void _np_bfd_error_vhandler(const char *fmt, va_list args)
{
    // In MacOS Catalina, executables contain a load command
    // #define LC_BUILD_VERSION 0x32 /* build for platform min OS version */
    // which BFD doesn't recognise but which is totally harmless to
    // ignore.  We suppresses that error message here.
    if (strstr(fmt, "unknown load command"))
        return;

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
}

#if HAVE_BFD_ERROR_HANDLER_VPRINTFLIKE
#define _np_bfd_error_handler _np_bfd_error_vhandler
#else
static void _np_bfd_error_handler(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _np_bfd_error_vhandler(fmt, args);
    va_end(args);
}
#endif

bool
link_object_t::map_sections()
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
link_object_t::unmap_sections()
{
    vector<section_t>::iterator m;
    for (m = mappings_.begin() ; m != mappings_.end() ; ++m)
    {
	m->munmap();
    }
}

compile_unit_offset_tuple_t
link_object_t::resolve_reference(const reference_t &ref) const
{
    return state_->resolve_link_object_reference(ref);
}

string
link_object_t::describe_resolver() const
{
    return string("link_object(") + string(filename_) + string(")");
}


// close namespaces
}; }; };
