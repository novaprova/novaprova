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
#ifndef __np_spiegel_dwarf_compile_unit_hxx__
#define __np_spiegel_dwarf_compile_unit_hxx__ 1

#include "np/spiegel/common.hxx"
#include "reference.hxx"
#include "reader.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct abbrev_t;
class walker_t;
struct section_t;

class compile_unit_t
{
private:
    enum { header_length = 11 };	    // this might depend on version
public:
    compile_unit_t(uint32_t idx, uint32_t loidx)
     :  index_(idx),
        loindex_(loidx)
    {}

    ~compile_unit_t()
    {}

    bool read_header(reader_t &r);
    bool read_compile_unit_entry(walker_t &w);
    void read_abbrevs(reader_t &r);
    void dump_abbrevs() const;

    uint32_t get_index() const { return index_; }
    uint32_t get_link_object_index() const { return loindex_; }
    const char *get_executable() const;
    const section_t *get_section(uint32_t) const;
    np::spiegel::addr_t live_address(np::spiegel::addr_t addr) const;

    reference_t make_reference(uint32_t off) const
    {
	reference_t ref;
	ref.cu = index_;
	ref.offset = off;
	return ref;
    }
    reference_t make_root_reference() const
    {
	reference_t ref;
	ref.cu = index_;
	ref.offset = header_length;
	return ref;
    }

    reader_t get_contents() const
    {
	reader_t r = reader_;
	r.skip(header_length);
	return r;
    }

    const abbrev_t *get_abbrev(uint32_t code) const
    {
	return (code >= abbrevs_.size() ? 0 : abbrevs_[code]);
    }

private:
    uint32_t index_;
    uint32_t loindex_;
    reader_t reader_;	    // for whole including header
    uint32_t abbrevs_offset_;
    std::vector<abbrev_t*> abbrevs_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_compile_unit_hxx__
