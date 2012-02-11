#include "spiegel/commonp.hxx"
#include "compile_unit.hxx"
#include "walker.hxx"
#include "enumerations.hxx"

namespace spiegel { namespace dwarf {
using namespace std;

bool
compile_unit_t::read_header(reader_t &r)
{
    reader_ = r;	    // sample offset of start of header

    uint32_t length;
    uint16_t version;
    if (!r.read_u32(length))
	return false;
    if (length > r.get_remains())
	fatal("Bad DWARF compilation unit length %u", length);

    if (!r.read_u16(version))
	return false;
    if (version != 2)
	fatal("Bad DWARF version %u, expecting 2", version);
    if (length < header_length-6/*read so far*/)
	fatal("Bad DWARF compilation unit length %u", length);

    uint8_t addrsize;
    if (!r.read_u32(abbrevs_offset_) ||
	!r.read_u8(addrsize))
	return false;
    if (addrsize != sizeof(void*))
	fatal("Bad DWARF addrsize %u, expecting %u",
	      addrsize, sizeof(void*));

    printf("compilation unit\n");
    printf("    length %u\n", (unsigned)length);
    printf("    version %u\n", (unsigned)version);
    printf("    abbrevs %u\n", (unsigned)abbrevs_offset_);
    printf("    addrsize %u\n", (unsigned)addrsize);

    length += 4;	// account for the `length' field of the header

    // setup reader_ to point to the whole compile
    // unit but not any bytes of the next one
    reader_ = reader_.initial_subset(length);

    // skip the outer reader over the body
    r.skip(length - header_length);

    return true;
}

bool
compile_unit_t::read_compile_unit_entry(walker_t &w)
{
    const entry_t *e = w.move_to_sibling();
    if (!e)
	return false;

    name_ = e->get_string_attribute(DW_AT_name);
    comp_dir_ = e->get_string_attribute(DW_AT_comp_dir);
    low_pc_ = e->get_uint64_attribute(DW_AT_low_pc);
    high_pc_ = e->get_uint64_attribute(DW_AT_high_pc);
    language_ = e->get_uint32_attribute(DW_AT_language);

    printf("    name %s\n", name_);
    printf("    comp_dir %s\n", comp_dir_);
    printf("    low_pc 0x%llx\n", (unsigned long long)low_pc_);
    printf("    high_pc 0x%llx\n", (unsigned long long)high_pc_);
    printf("    language %u\n", (unsigned)language_);

    return true;
}


void
compile_unit_t::read_abbrevs(reader_t &r)
{
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
	abbrevs_[a->code] = a;
    }
}


void
compile_unit_t::dump_abbrevs() const
{
    printf("Abbrevs {\n");

    map<uint32_t, abbrev_t*>::const_iterator itr;
    for (itr = abbrevs_.begin() ; itr != abbrevs_.end() ; ++itr)
    {
	abbrev_t *a = itr->second;
	printf("Code %u\n", a->code);
	printf("    tag 0x%x (%s)\n", a->tag, tagnames.to_name(a->tag));
	printf("    children %u (%s)\n",
		(unsigned)a->children,
		childvals.to_name(a->children));
	printf("    attribute specifications {\n");

	vector<abbrev_t::attr_spec_t>::iterator i;
	for (i = a->attr_specs.begin() ; i != a->attr_specs.end() ; ++i)
	{
	    printf("        name 0x%x (%s)",
		    i->name, attrnames.to_name(i->name));
	    printf(" form 0x%x (%s)\n",
		    i->form, formvals.to_name(i->form));
	}
	printf("    }\n");
    }
    printf("}\n");
}

// close namespace
} }
