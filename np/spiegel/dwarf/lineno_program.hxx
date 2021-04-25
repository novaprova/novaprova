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
        MIN_LINENO_VERSION = 2,
        MAX_LINENO_VERSION = 4
    };

    struct file_table_entry_t
    {
        const char *filename;
        uint32_t directory_index;
        uint32_t mtime;     /* may be 0 = don't know */
        uint32_t length;    /* may be 0 = don't know */
    };

    class range_t;
    class delegate_t
    {
    public:
        virtual bool receive_range(const range_t &) { return true; }
    };


    struct registers_t
    {
        // Present in DWARF2
        addr_t address;
        uint32_t file;
        uint32_t line;
        uint32_t column;
        bool is_stmt;
        bool basic_block;
        bool end_sequence;
        // Added in DWARF3
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
         :  program_(p), have_start_(false), running_(true)
        {
            memset(&registers_, 0, sizeof(registers_));
            memset(&start_, 0, sizeof(start_));
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
        void reset_registers();
        void copy(delegate_t *delegate)
        {
#if 0
            dprintf("registers: addr 0x%lx file %s line %u %s",
                    registers_.address,
                    get_absolute_filename(registers_.file).c_str(),
                    registers_.line,
                    registers_.end_sequence ? "EOS" : "");
#endif
            if (have_start_ && !delegate->receive_range(range_t(*this)))
            {
                running_ = false;
                return;
            }

            if (registers_.end_sequence)
            {
                have_start_ = false;
                memset(&start_, 0, sizeof(start_));
            }
            else
            {
                have_start_ = true;
                start_ = registers_;
            }

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
        lineno_program_t &program_;
        registers_t registers_;
        registers_t start_;
        bool have_start_;
        std::vector<file_table_entry_t> additional_files_;
        bool running_;

        friend lineno_program_t;
        friend range_t;
    };

    class range_t
    {
        // This class is used to provide a facade to report information
        // about ranges of operations described by a Line Number
        // Program.  As a facade, it only reports information stored in
        // the run_state_t and other places, and doesn't contain any
        // state.  It's valid only for the lifetime of the
        // receive_range() callback in delegate_t.
    public:
        /* Range of operations.  "start" is the address of the first
         * operation in the range, "end" is one operation past the last
         * operation in the range. */
        addr_t get_start_address() const { return run_state_.start_.address; }
        uint32_t get_start_op_index() const { return run_state_.start_.op_index; }
        addr_t get_end_address() const { return run_state_.registers_.address; }
        uint32_t get_end_op_index() const { return run_state_.registers_.op_index; }

        bool contains(addr_t a, uint32_t op_index=0) const
        {
            if (a < get_start_address() ||
                (a == get_start_address() && op_index < get_start_op_index()))
                return false;
            if (a >= get_end_address() ||
                (a == get_end_address() && op_index >= get_end_op_index()))
                return false;
            return true;
        }

        np::util::filename_t get_absolute_filename() const
        {
            return run_state_.get_absolute_filename(run_state_.start_.file);
        }
        uint32_t get_file_mtime() const
        {
            const file_table_entry_t *e = run_state_.get_file_entry(run_state_.start_.file);
            return e ? e->mtime : 0;
        }
        uint32_t get_file_length() const
        {
            const file_table_entry_t *e = run_state_.get_file_entry(run_state_.start_.file);
            return e ? e->length : 0;
        }
        uint32_t get_line() const { return run_state_.start_.line; }
        uint32_t get_column() const { return run_state_.start_.column; }
        bool get_is_stmt() const { return run_state_.start_.is_stmt; }
        bool get_is_basic_block() const { return run_state_.start_.basic_block; }
        bool get_is_prologue_end() const { return run_state_.start_.prologue_end; }
        bool get_is_epilogue_begin() const { return run_state_.start_.epilogue_begin; }
        uint32_t get_isa() const { return run_state_.start_.isa; }
        uint32_t get_discriminator() const { return run_state_.start_.discriminator; }

    private:
        range_t(run_state_t &rs) : run_state_(rs) {}
        run_state_t &run_state_;
        friend run_state_t;
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
                         /*out*/np::util::filename_t *filenamep,
                         /*out*/uint32_t *linep,
                         /*out*/uint32_t *columnp);

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
