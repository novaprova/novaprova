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
#ifndef __np_spiegel_dwarf_walker_hxx__
#define __np_spiegel_dwarf_walker_hxx__ 1

#include "np/spiegel/common.hxx"
#include "reader.hxx"
#include "entry.hxx"
#include "compile_unit.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class walker_t
{
public:
    walker_t(compile_unit_t *cu)
     :  id_(next_id_++),
	compile_unit_(cu),
	reader_(cu->get_contents()),
	level_(0),
	filter_tag_(0)
    {
    }

    walker_t(const walker_t &o)
     :  id_(next_id_++),
	compile_unit_(o.compile_unit_),
	reader_(o.reader_),
	// Note: we don't clone the entry, on the assumption
	// that it's about to be clobbered anyway
	level_(o.level_),
	filter_tag_(o.filter_tag_)
    {
	// but we need the entry's level for the move
	// operations to work correctly
	entry_.partial_setup(o.entry_);
    }

    walker_t(reference_t ref)
     :  id_(next_id_++),
	filter_tag_(0)
    {
	seek(ref);
    }

    const entry_t *get_entry() const { return &entry_; }
    reference_t get_reference() const
    {
	return compile_unit_->make_reference(entry_.get_offset());
    }
    std::vector<reference_t> get_path() const;

    reader_t get_section_contents(uint32_t sec) const;

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

    uint16_t get_dwarf_version() const { return compile_unit_->get_version(); }

    void set_filter_tag(unsigned tag) { filter_tag_ = tag; }

private:
    enum read_entry_results_t
    {
	RE_EOF = -1,
	RE_EOL = 0,
	RE_OK = 1,
	RE_FILTERED = 2,
    };

    void seek(reference_t ref);
    int read_entry();
    int read_attributes();
    int skip_attributes();

    // for debugging only
    static uint32_t next_id_;
    uint32_t id_;

    compile_unit_t *compile_unit_;
    reader_t reader_;
    entry_t entry_;
    unsigned level_;
    unsigned filter_tag_;
};


// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_walker_hxx__
