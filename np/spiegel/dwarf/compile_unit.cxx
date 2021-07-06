/*
 * Copyright 2011-2021 Gregory Banks
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
#include "state.hxx"
#include "compile_unit.hxx"
#include "link_object.hxx"
#include "walker.hxx"
#include "lineno_program.hxx"
#include "enumerations.hxx"
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

compile_unit_t::read_result_t
compile_unit_t::read_header(reader_t &r)
{
    reader_ = r;
    offset_ = r.get_offset(); // sample offset of start of header

    string context = string("compile unit header at section offset ") + HEX(offset_);
    dprintf("DWARF debug_info %s\n", context.c_str());

    // Per [DWARF4] section 7.5.1.1 point 1, "representing the length
    // of the .debug_info contribution for that compilation unit, not
    // including the length field itself"
    np::spiegel::offset_t length;
    bool is64;
    if (!r.read_initial_length(length, is64))
    {
        // no error message here, because this is normal
        // behavior at the end of a .info section.
	return READ_FAILED;
    }
    size_t initial_length_length = r.get_offset() - offset_;    // 12 in 64b mode, 4 in 32b mode
    if (length > r.get_remains() || length < MIN_HEADER_LENGTH)
    {
	eprintf("%s: bad DWARF compile unit length %llu, failing\n",
                context.c_str(), (unsigned long long)length);
        return READ_FAILED;
    }
    size_t next_header_offset = r.get_offset() + length;        // how to get to the next CU header

    uint16_t version;
    if (!r.read_u16(version))
    {
        eprintf("%s: short DWARF compile unit header, cannot read version\n",
                context.c_str());
        r.seek(next_header_offset);
	return READ_SKIPPED;
    }
    if (version < MIN_DWARF_VERSION || version > MAX_DWARF_VERSION)
    {
	wprintf("%s: unexpected DWARF version %u, expecting %u-%u\n",
	        context.c_str(), version, MIN_DWARF_VERSION, MAX_DWARF_VERSION);
        r.seek(next_header_offset);
	return READ_SKIPPED;
    }

    uint8_t addrsize;
    if (!r.read_u32(abbrevs_offset_) ||
	!r.read_u8(addrsize))
    {
        eprintf("%s: bad DWARF header: unable to read abbrevs_offset or addrsize\n",
	        context.c_str());
        r.seek(next_header_offset);
	return READ_SKIPPED;
    }
    if (addrsize != _NP_ADDRSIZE) {
        eprintf("%s: bad DWARF addrsize %u, expecting %u\n",
	        context.c_str(), addrsize, _NP_ADDRSIZE);
        r.seek(next_header_offset);
	return READ_SKIPPED;
    }

    dprintf("length %u version %u is64 %s abbrevs_offset %u addrsize %u\n",
	    (unsigned)length,
	    (unsigned)version,
	    is64 ? "true" : "false",
	    (unsigned)abbrevs_offset_,
	    (unsigned)addrsize);

    header_length_ = r.get_offset() - offset_;
    version_ = version;
    is64_ = is64;

    // setup reader_ to point to the whole compile
    // unit but not any bytes of the next one
    reader_.set_is64(is64);
    reader_ = reader_.initial_subset(length+initial_length_length);

    // skip the outer reader over the body
    r.seek(next_header_offset);

    return READ_SUCCEEDED;
}

void
compile_unit_t::read_abbrevs(reader_t &r)
{
    dprintf("reading abbrevs\n");
    r.seek(abbrevs_offset_);

    uint32_t code;
    unsigned int nread = 0;
    /* code 0 indicates end of compile unit */
    while (r.read_uleb128(code) && code)
    {
	abbrev_t *a = new abbrev_t(code);
	if (!a->read(r))
	{
            eprintf("Failed to read abbrev code %u\n", code);
	    delete a;
	    break;
	}
        nread++;
	if (a->code >= abbrevs_.size())
	    abbrevs_.resize(a->code+1, 0);
	abbrevs_[a->code] = a;
    }
    dprintf("Read %u abbrevs, largest code %u\n", nread, abbrevs_.size()-1);
}

bool
compile_unit_t::read_lineno_program(reader_t &r)
{
    lineno_program_t *lp = new lineno_program_t(compilation_directory_);
    if (!lp->read_header(r))
        return false;
    lineno_program_ = lp;
    return true;
}

void
compile_unit_t::dump_abbrevs() const
{
    fprintf(stderr, "np: Abbrevs {\n");

    vector<abbrev_t*>::const_iterator itr;
    for (itr = abbrevs_.begin() ; itr != abbrevs_.end() ; ++itr)
    {
	abbrev_t *a = *itr;
	if (!a) continue;
	fprintf(stderr, "np: Code %u\n", a->code);
	fprintf(stderr, "np:     tag 0x%x (%s)\n", a->tag, tagnames.to_name(a->tag));
	fprintf(stderr, "np:     children %u (%s)\n",
		(unsigned)a->children,
		childvals.to_name(a->children));
	fprintf(stderr, "np:     attribute specifications {\n");

	vector<abbrev_t::attr_spec_t>::iterator i;
	for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
	{
	    fprintf(stderr, "np:         name 0x%x (%s)",
		    i->name, attrnames.to_name(i->name));
	    fprintf(stderr, "np:  form 0x%x (%s)\n",
		    i->form, formvals.to_name(i->form));
	}
	fprintf(stderr, "np:     }\n");
    }
    fprintf(stderr, "np: }\n");
}

bool
compile_unit_t::read_attributes()
{
    np::spiegel::dwarf::walker_t w(make_root_reference());
    // move to DW_TAG_compile_unit
    const np::spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return false;

    filename_ = e->get_string_attribute(DW_AT_name);
    compilation_directory_ = e->get_string_attribute(DW_AT_comp_dir);
    low_pc_ = e->get_uint64_attribute(DW_AT_low_pc);
    high_pc_ = e->get_uint64_attribute(DW_AT_high_pc);
    language_ = e->get_uint32_attribute(DW_AT_language);

    dprintf("populated spiegel compile unit %s comp_dir %s "
            "low_pc 0x%llx high_pc 0x%llx language %u\n",
            filename_,
            compilation_directory_,
            (unsigned long long)low_pc_,
            (unsigned long long)high_pc_,
            (unsigned)language_);

    return true;
}

filename_t
compile_unit_t::get_absolute_path() const
{
    return filename_t(filename_).make_absolute_to_dir(filename_t(compilation_directory_));
}

const char *
compile_unit_t::get_executable() const
{
    return link_object_->get_filename();
}

const section_t *
compile_unit_t::get_section(uint32_t i) const
{
    return link_object_->get_section(i);
}

np::spiegel::addr_t
compile_unit_t::live_address(np::spiegel::addr_t addr) const
{
    return link_object_->live_address(addr);
}

compile_unit_offset_tuple_t
compile_unit_t::resolve_reference(const reference_t &ref) const
{
    return compile_unit_offset_tuple_t(const_cast<compile_unit_t*>(this), ref.offset);
}

string
compile_unit_t::describe_resolver() const
{
    char offbuf[32];
    snprintf(offbuf, sizeof(offbuf), ":%u)", index_);
    return string("compile_unit(") + string(link_object_->get_filename()) + string(offbuf);
}

bool
compile_unit_t::get_source_line(
    np::spiegel::addr_t addr,
    /*out*/np::util::filename_t *filenamep,
    /*out*/uint32_t *linep, /*out*/uint32_t *columnp)
{
    return lineno_program_->get_source_line(addr, filenamep, linep, columnp);
}

// close namespaces
}; }; };
