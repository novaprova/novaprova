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
#include "np/util/filename.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

struct abbrev_t;
class walker_t;
struct section_t;
class link_object_t;
class lineno_program_t;

class compile_unit_t : public reference_resolver_t, public np::util::zalloc
{
private:
    enum {
	header_length = 11,	// this might depend on version

	MIN_DWARF_VERSION = 2,
	MAX_DWARF_VERSION = 4
    };
public:
    compile_unit_t(uint32_t idx, link_object_t *lo)
     :  index_(idx),
        link_object_(lo)
    {}

    ~compile_unit_t()
    {}

    bool read_header(reader_t &r);
    void read_abbrevs(reader_t &r);
    bool read_lineno_program(reader_t &r);
    void dump_abbrevs() const;
    bool read_attributes();

    uint32_t get_index() const { return index_; }
    void *get_upper() const { return upper_; }
    void set_upper(void *u) { upper_ = u; }
    link_object_t *get_link_object() const { return link_object_; }
    // return the file offset of the first byte of the compile unit on the disk
    np::spiegel::offset_t get_start_offset() const { return offset_; }
    // return the file offset one byte beyond the last byte of the compile unit on the disk
    np::spiegel::offset_t get_end_offset() const { return offset_ + reader_.get_length(); }
    const char *get_executable() const;
    const section_t *get_section(uint32_t) const;
    uint16_t get_version() const { return version_; }
    np::spiegel::addr_t live_address(np::spiegel::addr_t addr) const;

    reference_t make_reference(uint32_t off) const
    {
        return reference_t::make(this, off);
    }
    reference_t make_root_reference() const
    {
        return reference_t::make(this, header_length);
    }

    compile_unit_offset_tuple_t resolve_reference(const reference_t &ref) const override;
    std::string describe_resolver() const override;

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

    bool get_source_line(np::spiegel::addr_t addr,
                         /*out*/np::util::filename_t *filenamep,
                         /*out*/uint32_t *linep, /*out*/uint32_t *columnp);

    np::util::filename_t get_filename() const { return filename_; }
    np::util::filename_t get_compilation_directory() const { return compilation_directory_; }
    np::util::filename_t get_absolute_path() const;
    uint32_t get_language() const { return language_; }
private:
    uint32_t index_;
    link_object_t *link_object_;
    void *upper_;
    uint16_t version_;
    bool is64_;		    // new 64b format introduced in DWARF3
    reader_t reader_;	    // for whole including header
    np::spiegel::offset_t offset_;
    uint32_t abbrevs_offset_;
    std::vector<abbrev_t*> abbrevs_;
    // from attributes of DW_TAG_compile_unit
    const char *filename_;
    const char *compilation_directory_;
    uint64_t low_pc_;	    // TODO: should be an addr_t
    uint64_t high_pc_;
    uint32_t language_;
    // from .debug_line section
    lineno_program_t *lineno_program_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_compile_unit_hxx__
