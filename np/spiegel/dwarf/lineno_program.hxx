/*
 * Copyright 2011-2020 Gregory Banks
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
#ifndef __np_spiegel_dwarf_lineno_program_hxx__
#define __np_spiegel_dwarf_lineno_program_hxx__

#include "np/spiegel/common.hxx"
#include "np/spiegel/dwarf/reader.hxx"
#include "np/util/filename.hxx"
#include <vector>

namespace np {
namespace spiegel {
namespace dwarf {

class lineno_program_t : public np::util::zalloc
{
public:
    lineno_program_t(const char *compilation_directory)
      : compilation_directory_(compilation_directory)
    {}
    ~lineno_program_t() {}

    enum
    {
        MIN_LINENO_VERSION = 3,
        MAX_LINENO_VERSION = 4
    };

    class run_state_t;
    class delegate_t
    {
    public:
        virtual bool receive_registers(const run_state_t *) { return true; }
    };

    struct file_table_entry_t
    {
        const char *filename;
        uint32_t directory_index;
        uint32_t mtime;     /* may be 0 = don't know */
        uint32_t length;    /* may be 0 = don't know */
    };

    struct registers_t
    {
        addr_t address;
        uint32_t file;
        uint32_t line;
        uint32_t column;
        bool is_stmt;
        bool basic_block;
        bool end_sequence;
        bool prologue_end;
        bool epilogue_begin;
        uint32_t isa;
        // Added in DWARF4
        uint32_t op_index;
        uint32_t discriminator;
    };

    class run_state_t
    {
    private:
        run_state_t(lineno_program_t &p)
         :  program_(p), running_(true)
        {
            memset(&registers_, 0, sizeof(registers_));
        }
        ~run_state_t() {}

    public:
        const lineno_program_t::file_table_entry_t *get_file_entry(uint32_t i) const
        {
            if (i == 0)
                return 0;
            i--;
            if (i < program_.files_.size())
                return &program_.files_[i];
            i -= program_.files_.size();
            if (i < additional_files_.size())
                return &additional_files_[i];
            return 0;
        }
        np::util::filename_t get_absolute_filename(uint32_t i) const
        {
            const lineno_program_t::file_table_entry_t *e = get_file_entry(i);
            if (!e)
                return np::util::filename_t();
            np::util::filename_t f(e->filename);
            const char *cd = program_.compilation_directory_;
            const char *d = (e->directory_index ?
                             program_.include_directories_[e->directory_index-1] :
                             cd);
            np::util::filename_t f2 = f.make_absolute_to_dir(np::util::filename_t(d));
            return f2.make_absolute_to_dir(np::util::filename_t(cd));
        }

        const registers_t &registers() const { return registers_; }

    private:
        void reset_registers(bool default_is_stmt);
        void copy(delegate_t *delegate)
        {
            if (!delegate->receive_registers(this))
                running_ = false;
            registers_.basic_block = false;
            registers_.prologue_end = false;
            registers_.epilogue_begin = false;
            registers_.discriminator = 0;
        }
        void advance_pc(uint32_t operation_advance)
        {
            // This is the DWARF4 algorithm.  It's backwards compatible with
            // DWARF3 and every architecture that isn't ia64 because the default
            // value of maximum_operations_per_instruction is 1
            uint32_t new_op_index = registers_.op_index + operation_advance;
            registers_.address += (new_op_index / program_.maximum_operations_per_instruction_) * program_.minimum_instruction_length_;
            registers_.op_index = new_op_index % program_.maximum_operations_per_instruction_;
        }
        void advance_line(int32_t line_advance)
        {
            registers_.line += line_advance;
        }

    private:
        registers_t registers_;
        lineno_program_t &program_;
        std::vector<file_table_entry_t> additional_files_;
        bool running_;

        friend lineno_program_t;
    };

    enum status_t
    {
        SUCCESS = 0,
        ERR_BAD_OPCODE = -1,
        ERR_OPCODE_FETCH = -2,
        ERR_OPERAND_FETCH = -3,
        ERR_HALTED_BY_DELEGATE = -4
    };

    bool read_header(reader_t &);
    status_t run(delegate_t *);
    bool get_source_line(np::spiegel::addr_t addr,
                         np::util::filename_t &filename,
                         unsigned &line, unsigned &column);

private:
    int32_t special_opcode_line_advance(uint8_t opcode) const
    {
        uint8_t adjusted_opcode = opcode - opcode_base_;
        return line_base_ + (int32_t)(adjusted_opcode % line_range_);
    }
    uint32_t special_opcode_operation_advance(uint8_t opcode) const
    {
        uint8_t adjusted_opcode = opcode - opcode_base_;
        return adjusted_opcode / line_range_;
    }

    // read from the .debug_info attributes, passed in by compile_unit_t
    // used to make absolure filenames
    const char *compilation_directory_;

    // points to the whole .debug_line compile unit, header and all
    reader_t reader_;

    // fields read from the .debug_line section compile unit header

    // Unlike the DWARF standard, this is stored as the number of
    // bytes from the start of the .debug_line compile unit to the
    // first bytes of the program itself.
    np::spiegel::offset_t header_length_;
    uint16_t version_;
    uint8_t minimum_instruction_length_;
    uint8_t maximum_operations_per_instruction_;
    uint8_t default_is_stmt_;
    int8_t line_base_;
    uint8_t line_range_;
    uint8_t opcode_base_;
    // std::vector<uint8_t> standard_opcode_lengths_;
    std::vector<const char *> include_directories_;
    std::vector<file_table_entry_t> files_;

    friend run_state_t;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_lineno_program_hxx__
