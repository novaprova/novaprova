#ifndef __libspiegel_dwarf_walker_hxx__
#define __libspiegel_dwarf_walker_hxx__ 1

#include "spiegel/common.hxx"
#include "reader.hxx"
#include "entry.hxx"
#include "compile_unit.hxx"

namespace spiegel {
namespace dwarf {

class walker_t
{
public:
    walker_t(compile_unit_t *cu)
     :  id_(next_id_++),
	compile_unit_(cu),
	reader_(cu->get_contents()),
	level_(0)
    {
    }

    walker_t(const walker_t &o)
     :  id_(next_id_++),
	compile_unit_(o.compile_unit_),
	reader_(o.reader_),
	// Note: we don't clone the entry, on the assumption
	// that it's about to be clobbered anyway
	level_(o.level_)
    {
	// but we need the entry's level for the move
	// operations to work correctly
	entry_.partial_setup(o.entry_);
    }

    walker_t(reference_t ref)
     :  id_(next_id_++)
    {
	seek(ref);
    }

    const entry_t *get_entry() const { return &entry_; }
    reference_t get_reference() const
    {
	return compile_unit_->make_reference(entry_.get_offset());
    }
    std::vector<reference_t> get_path() const;

    // move in preorder: to next sibling or to
    // next available ancestor's sibling
    const entry_t *move_preorder();
    // move to next sibling and return it, or
    // return 0 and move back up
    const entry_t *move_next();
    // move to first child and return it, or
    // return 0 if has no children
    const entry_t *move_down();
    const entry_t *move_to(reference_t);
    const entry_t *move_up();

private:
    void seek(reference_t ref);
    int read_entry();

    // for debugging only
    static uint32_t next_id_;
    uint32_t id_;

    compile_unit_t *compile_unit_;
    reader_t reader_;
    entry_t entry_;
    unsigned level_;
};


// close namespaces
} }

#endif // __libspiegel_dwarf_walker_hxx__
