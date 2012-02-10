#ifndef __libspiegel_dwarf_walker_hxx__
#define __libspiegel_dwarf_walker_hxx__ 1

#include "spiegel/commonp.hxx"
#include "reader.hxx"
#include "entry.hxx"
#include "compile_unit.hxx"

namespace spiegel {
namespace dwarf {

class state_t;

class walker_t
{
public:
    walker_t(state_t &s, compile_unit_t &cu)
     :  state_(s),
	compile_unit_(cu),
	reader_(cu.get_contents()),
	level_(0)
    {
    }

    const entry_t &get_entry() const { return entry_; }

    bool move_to_sibling();
    bool move_to_children();
    bool move_preorder();
//     bool move_to(reference_t);

private:
    int read_entry();

    state_t &state_;
    compile_unit_t &compile_unit_;
    reader_t reader_;
    entry_t entry_;
    unsigned level_;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_walker_hxx__
