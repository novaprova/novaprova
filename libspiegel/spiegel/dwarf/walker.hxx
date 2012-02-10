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
    walker_t(state_t &s, compile_unit_t *cu)
     :  state_(s),
	compile_unit_(cu),
	reader_(cu->get_contents()),
	level_(0)
    {
    }

    walker_t(const walker_t &o)
     :  state_(o.state_),
	compile_unit_(o.compile_unit_),
	reader_(o.reader_),
	// Note: we don't clone the entry, on the assumption
	// that it's about to be clobbered anyway
	level_(o.level_)
    {
	// but we need the entry's level for the move
	// operations to work correctly
	entry_.setup(o.entry_.get_offset(),
		     o.entry_.get_level(),
		     o.entry_.get_abbrev());
    }

    const entry_t &get_entry() const { return entry_; }

    bool move_to_sibling();
    bool move_to_children();
    bool move_preorder();
    bool move_to(reference_t);

private:
    int read_entry();

    state_t &state_;
    compile_unit_t *compile_unit_;
    reader_t reader_;
    entry_t entry_;
    unsigned level_;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_walker_hxx__
