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
#include "state.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"
#include "enumerations.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

bool
compile_unit_t::read_header(reader_t &r)
{
    reader_ = r;	    // sample offset of start of header

#if _NP_DEBUG
    fprintf(stderr, "np: DWARF compile unit header at "
		    "section offset 0x%lx\n",
		    r.get_offset());
#endif

    np::spiegel::offset_t length;
    uint16_t version;
    bool is64 = false;
    if (!r.read_u32(length))
	return false;
    if (length == 0xffffffff)
    {
	/* An all-1 length marks the 64-bit format
	 * introduced in the DWARF3 standard */
#if _NP_ADDRSIZE == 4
	fatal("The 64-bit DWARF format is not supported on 32-bit architectures");
#elif _NP_ADDRSIZE == 8
	is64 = true;
	if (!r.read_u64(length))
	    return false;
#else
#error "Unknown address size"
#endif
    }
    if (length > r.get_remains())
	fatal("Bad DWARF compile unit length %llu", (unsigned long long)length);

    if (!r.read_u16(version))
	return false;
    if (version < MIN_DWARF_VERSION || version > MAX_DWARF_VERSION)
	fatal("Bad DWARF version %u, expecting %u-%u",
	      version, MIN_DWARF_VERSION, MAX_DWARF_VERSION);
    if (length < (unsigned)(header_length-(is64 ? 14 : 6)/*read so far*/))
	fatal("Bad DWARF compile unit length %llu", (unsigned long long)length);

    uint8_t addrsize;
    if (!r.read_u32(abbrevs_offset_) ||
	!r.read_u8(addrsize))
	return false;
    if (addrsize != _NP_ADDRSIZE) fatal("Bad DWARF addrsize %u, expecting %u",
	      addrsize, _NP_ADDRSIZE);

#if _NP_DEBUG
    fprintf(stderr, "np: length %u version %u is64 %s abbrevs_offset %u addrsize %u\n",
	    (unsigned)length,
	    (unsigned)version,
	    is64 ? "true" : "false",
	    (unsigned)abbrevs_offset_,
	    (unsigned)addrsize);
#endif

    version_ = version;
    is64_ = is64;

    length += is64 ? 12 : 4;	// account for the `length' field of the header

    // setup reader_ to point to the whole compile
    // unit but not any bytes of the next one
    reader_.set_is64(is64);
    reader_ = reader_.initial_subset(length);

    // skip the outer reader over the body
    r.skip(length - header_length);

    return true;
}

void
compile_unit_t::read_abbrevs(reader_t &r)
{
#if _NP_DEBUG
    fprintf(stderr, "np: reading abbrevs\n");
#endif
    r.seek(abbrevs_offset_);

    uint32_t code;
    /* code 0 indicates end of compile unit */
    while (r.read_uleb128(code) && code)
    {
	abbrev_t *a = new abbrev_t(code);
	if (!a->read(r))
	{
	    delete a;
	    break;
	}
	if (a->code >= abbrevs_.size())
	    abbrevs_.resize(a->code+1, 0);
	abbrevs_[a->code] = a;
    }
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

const char *
compile_unit_t::get_executable() const
{
    const state_t::linkobj_t *lo = state_t::instance()->linkobjs_[loindex_];
    return lo->filename_;
}

const section_t *
compile_unit_t::get_section(uint32_t i) const
{
    const state_t::linkobj_t *lo = state_t::instance()->linkobjs_[loindex_];
    return &lo->sections_[i];
}

np::spiegel::addr_t
compile_unit_t::live_address(np::spiegel::addr_t addr) const
{
    const state_t::linkobj_t *lo = state_t::instance()->linkobjs_[loindex_];
    return addr ? addr + lo->slide_ : addr;
}

// close namespaces
}; }; };
