/*
 * Copyright 2011-2021 Gregory Banks
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
#ifndef __np_spiegel_dwarf_link_object_hxx__
#define __np_spiegel_dwarf_link_object_hxx__ 1

#include "np/spiegel/common.hxx"
#include "np/spiegel/mapping.hxx"
#include "section.hxx"
#include "reference.hxx"

namespace np {
namespace spiegel {
namespace dwarf {

class state_t;

class link_object_t : public reference_resolver_t
{
public:
    link_object_t(const char *n, state_t *state)
     :  filename_(np::util::xstrdup(n)),
        state_(state)
    {
    }
    ~link_object_t()
    {
        unmap_sections();
        free(filename_);
    }

    const char *get_filename() const { return filename_; }
    const section_t *get_section(uint32_t i) const { return i < DW_sec_num ? &sections_[i] : 0; }
    np::spiegel::addr_t live_address(np::spiegel::addr_t addr) const { return addr ? addr + slide_ : addr; }
    np::spiegel::addr_t recorded_address(np::spiegel::addr_t addr) const { return addr ? addr - slide_ : addr; }
    reference_t make_reference(np::spiegel::offset_t off) const
    {
        return reference_t::make(this, off);
    }
    compile_unit_offset_tuple_t resolve_reference(const reference_t &ref) const override;
    std::string describe_resolver() const override;

    /* interface for state_t */
    void set_system_mappings(const std::vector<np::spiegel::mapping_t> &mappings) { system_mappings_ = mappings; }
    void set_slide(unsigned long s) { slide_ = s; }
    bool is_in_plt(np::spiegel::addr_t addr) const;
    bool has_sections() const { return sections_[DW_sec_info].get_size() > 0; }
    bool map_from_system(mapping_t &m) const;
    bool map_sections();
    void unmap_sections();

private:

    char *filename_;
    state_t *state_;
    unsigned long slide_;
    section_t sections_[DW_sec_num];
    std::vector<section_t> mappings_;
    std::vector<np::spiegel::mapping_t> system_mappings_;
    std::vector<np::spiegel::mapping_t> plts_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_link_object_hxx__
