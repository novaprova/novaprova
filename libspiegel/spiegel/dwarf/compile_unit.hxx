#ifndef __libspiegel_dwarf_compile_unit_hxx__
#define __libspiegel_dwarf_compile_unit_hxx__ 1

#include "spiegel/commonp.hxx"
#include <map>
#include "reader.hxx"

namespace spiegel {
namespace dwarf {

class abbrev_t;
class walker_t;

class compile_unit_t
{
private:
    enum { header_length = 11 };	    // this might depend on version
public:
    compile_unit_t()
    {}

    ~compile_unit_t()
    {}

    bool read_header(reader_t &r);
    bool read_compile_unit_entry(walker_t &w);
    void read_abbrevs(reader_t &r);
    void dump_abbrevs() const;

    reader_t get_contents() const
    {
	reader_t r = reader_;
	r.skip(header_length);
	return r;
    }

    const abbrev_t *get_abbrev(uint32_t code) const
    {
	std::map<uint32_t, abbrev_t*>::const_iterator i = abbrevs_.find(code);
	if (i == abbrevs_.end())
	    return 0;
	return i->second;
    }

private:
    reader_t reader_;	    // for whole including header
    uint32_t abbrevs_offset_;
    std::map<uint32_t, abbrev_t*> abbrevs_;
    const char *name_;
    const char *comp_dir_;
    uint64_t low_pc_;	    // TODO: should be an addr_t
    uint64_t high_pc_;
    uint32_t language_;
};

// close namespaces
} }

#endif // __libspiegel_dwarf_compile_unit_hxx__
