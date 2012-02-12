#ifndef __libspiegel_dwarf_state_hxx__
#define __libspiegel_dwarf_state_hxx__ 1

#include "spiegel/commonp.hxx"
#include <vector>
#include "section.hxx"
#include "enumerations.hxx"

namespace spiegel {
namespace dwarf {

class walker_t;
class compile_unit_t;

class state_t
{
public:
    state_t(const char *filename);
    ~state_t();

    void map_sections();
    void read_compile_units();
    void dump_abbrevs();
    void dump_info(bool preorder);
    void dump_structs();

private:
    char *filename_;
    section_t sections_[DW_sec_num];
    std::vector<section_t> mappings_;
    std::vector<compile_unit_t*> compile_units_;

    friend class walker_t;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_state_hxx__
