#include "spiegel.hxx"
#include "spiegel/dwarf/state.hxx"
#include "spiegel/dwarf/walker.hxx"
#include "spiegel/dwarf/entry.hxx"
#include "spiegel/dwarf/enumerations.hxx"

namespace spiegel {
using namespace std;

vector<compile_unit_t *>
compile_unit_t::get_compile_units()
{
    vector<spiegel::compile_unit_t *> res;

    const vector<spiegel::dwarf::compile_unit_t*> &units =
	spiegel::dwarf::state_t::instance()->get_compile_units();
    vector<spiegel::dwarf::compile_unit_t*>::const_iterator i;
    for (i = units.begin() ; i != units.end() ; ++i)
    {
	compile_unit_t *cu = new compile_unit_t((*i)->make_root_reference());
	if (!cu->populate())
	    delete cu;
	else
	    res.push_back(cu);
    }
    return res;
}

bool
compile_unit_t::populate()
{
    spiegel::dwarf::walker_t w(ref_);
    // move to DW_TAG_compile_unit
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
