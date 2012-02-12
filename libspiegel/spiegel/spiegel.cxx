#include "spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/dwarf/walker.hxx"
#include "spiegel/dwarf/entry.hxx"
#include "spiegel/dwarf/enumerations.hxx"

namespace spiegel {
using namespace std;

bool
compile_unit_t::populate(spiegel::dwarf::walker_t &w)
{
    const spiegel::dwarf::entry_t *e = w.move_next();
    if (!e)
	return false;

    name_ = e->get_string_attribute(DW_AT_name);
    comp_dir_ = e->get_string_attribute(DW_AT_comp_dir);
    low_pc_ = e->get_uint64_attribute(DW_AT_low_pc);
    high_pc_ = e->get_uint64_attribute(DW_AT_high_pc);
    language_ = e->get_uint32_attribute(DW_AT_language);

#if 0
    printf("    name %s\n", name_);
    printf("    comp_dir %s\n", comp_dir_);
    printf("    low_pc 0x%llx\n", (unsigned long long)low_pc_);
    printf("    high_pc 0x%llx\n", (unsigned long long)high_pc_);
    printf("    language %u\n", (unsigned)language_);
#endif

    return true;
}

// close namespace
};
