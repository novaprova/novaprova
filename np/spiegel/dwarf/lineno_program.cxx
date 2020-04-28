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
#include "np/util/common.hxx"
#include "lineno_program.hxx"
#include "enumerations.hxx"
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;
using namespace np::util;

bool
lineno_program_t::read_header(reader_t &r)
{
    dprintf("DWARF debug_line compile unit header at section offset 0x%lx\n",
            r.get_offset());

    np::spiegel::offset_t unit_length;
    bool is64;
    if (!r.read_initial_length(unit_length, is64))
	return false;
    if (unit_length > r.get_remains())
	fatal("Bad DWARF compile unit length %llu",
              (unsigned long long)unit_length);

    // Setup reader_ to point to the whole compile
    // unit except the initial unit length,
    // but not any bytes of the next one.
    reader_ = r.initial_subset(unit_length);
    reader_.set_is64(is64);
    r.skip(unit_length);

    if (!reader_.read_u16(version_))
        return false;
    if (version_ < MIN_LINENO_VERSION || version_ > MAX_LINENO_VERSION)
    {
        eprintf("Bad version number %u for line number program, expecting %u-%u",
                (unsigned)version_, MIN_LINENO_VERSION, MAX_LINENO_VERSION);
        return false;
    }

    if (!reader_.read_offset(header_length_))
        return false;
    header_length_ += reader_.get_offset();
    if (!reader_.read_u8(minimum_instruction_length_))
        return false;
    maximum_operations_per_instruction_ = 1;
    if (version_ >= 4)
    {
        if (!reader_.read_u8(maximum_operations_per_instruction_))
            return false;
    }
    if (!reader_.read_u8(default_is_stmt_))
        return false;
    default_is_stmt_ = !!default_is_stmt_;
    if (!reader_.read_s8(line_base_))
        return false;
    if (!reader_.read_u8(line_range_))
        return false;
    if (!reader_.read_u8(opcode_base_))
        return false;
    // standard_opcode_lengths_.reserve(opcode_base_-1);
    for (unsigned i = 1 ; i < opcode_base_ ; i++)
    {
        uint8_t v;
        if (!reader_.read_u8(v))
            return false;
        // standard_opcode_lengths_.push_back(v);
    }
    for (;;)
    {
        const char *v;
        if (!reader_.read_string(v))
            return false;
        if (!*v)
            break;
        include_directories_.push_back(v);
    }
    for (;;)
    {
        file_table_entry_t e = {0, 0, 0, 0};
        if (!reader_.read_string(e.filename))
            return false;
        if (!*e.filename)
            break;
        if (!reader_.read_uleb128(e.directory_index))
            return false;
        /* TODO: range-check directory_index */
        if (!reader_.read_uleb128(e.mtime))
            return false;
        if (!reader_.read_uleb128(e.length))
            return false;
        files_.push_back(e);
    }

    if (header_length_ != reader_.get_offset())
    {
        eprintf("Bad header length in .debug_line compile unit header, got 0x%lx expecting 0x%lx",
                reader_.get_offset(), header_length_);
        return false;
    }

    dprintf("debug_line compile unit version %u "
            "minimum_instruction_length %u "
            "maximum_operations_per_instruction %u "
            "default_is_stmt %s "
            "line_base %d "
            "line_range %u "
            "opcode_base %u",
            (unsigned)version_,
            (unsigned)minimum_instruction_length_,
            (unsigned)maximum_operations_per_instruction_,
            default_is_stmt_ ? "true" : "false",
            (int)line_base_,
            (unsigned)line_range_,
            (unsigned)opcode_base_);
    if (is_enabled_for(np::log::DEBUG))
    {
        dprintf("    compilation_directory \"%s\"", compilation_directory_);
        unsigned i = 0;
        for (const char *d : include_directories_)
            dprintf("    include_directories[%u] \"%s\"", i++, d);
        i = 0;
        for (const file_table_entry_t &e : files_)
        {
            const char *d = (e.directory_index ?
                             include_directories_[e.directory_index-1] :
                             compilation_directory_);
            dprintf("    files_[%u] \"%s\" directory [%u]=\"%s\" mtime %u length %u",
                    i++,
                    e.filename,
                    e.directory_index, d,
                    e.mtime,
                    e.length);
        }
    }

    return true;
}

void
lineno_program_t::run_state_t::reset_registers(bool default_is_stmt)
{
    registers_.address = 0;
    registers_.file = 1;
    registers_.line = 1;
    registers_.column = 0;
    registers_.is_stmt = default_is_stmt;
    registers_.basic_block = false;
    registers_.end_sequence = false;
    registers_.prologue_end = false;
    registers_.epilogue_begin = false;
    registers_.isa = 0;
    registers_.op_index = 0;
    registers_.discriminator = 0;
}

lineno_program_t::status_t
lineno_program_t::run(delegate_t *delegate)
{
    run_state_t rs(*this);
    rs.reset_registers(default_is_stmt_);
    reader_t r = reader_;
    r.seek(header_length_);
    rs.running_ = true;

    uint8_t opcode;
    while (rs.running_ && r.read_u8(opcode))
    {
        if (opcode == 0)
        {
            /* extended opcodes */
            uint32_t nbytes;
            if (!r.read_uleb128(nbytes))
                return ERR_OPCODE_FETCH;
            if (!r.read_u8(opcode))
                return ERR_OPCODE_FETCH;
            switch (opcode)
            {
            case DW_LNE_end_sequence:
                rs.registers_.end_sequence = true;
                rs.copy(delegate);
                rs.reset_registers(default_is_stmt_);
                break;
            case DW_LNE_set_address:
                {
                    np::spiegel::addr_t v;
                    if (!r.read_addr(v))
                        return ERR_OPERAND_FETCH;
                    rs.registers_.address = v;
                    rs.registers_.op_index = 0;
                }
                break;
            case DW_LNE_define_file:
                {
                    file_table_entry_t e = {0, 0, 0, 0};
                    if (!r.read_string(e.filename))
                        return ERR_OPERAND_FETCH;
                    if (!r.read_uleb128(e.directory_index))
                        return ERR_OPERAND_FETCH;
                    /* TODO: range-check directory_index */
                    if (!r.read_uleb128(e.mtime))
                        return ERR_OPERAND_FETCH;
                    if (!r.read_uleb128(e.length))
                        return ERR_OPERAND_FETCH;
                    rs.additional_files_.push_back(e);
                }
                break;
            // Added in DWARF4
            case DW_LNE_set_discriminator:
                {
                    uint32_t d;
                    if (!r.read_uleb128(d))
                        return ERR_OPCODE_FETCH;
                    rs.registers_.discriminator = d;
                }
                break;
            default:
                /* extended opcodes not known at compile time, skip */
                eprintf("Unknown extended opcode 0x%02x at offset 0x%x",
                        (unsigned int)opcode, r.get_offset());
                return ERR_BAD_OPCODE;
                /*
                if (!r.skip(nbytes-1))
                    return ERR_OPERAND_FETCH;
                */
                break;
            }
        }
        else if (opcode < opcode_base_)
        {
            /* standard opcodes */
            uint32_t u;
            int32_t s;
            switch (opcode)
            {
            case DW_LNS_copy:
                rs.copy(delegate);
                break;
            case DW_LNS_advance_pc:
                if (!r.read_uleb128(u))
                    return ERR_OPERAND_FETCH;
                rs.advance_pc(u);
                break;
            case DW_LNS_advance_line:
                if (!r.read_sleb128(s))
                    return ERR_OPERAND_FETCH;
                rs.advance_line(s);
                break;
            case DW_LNS_set_file:
                if (!r.read_uleb128(u))
                    return ERR_OPERAND_FETCH;
                /* TODO: bounds-check */
                rs.registers_.file = u;
                break;
            case DW_LNS_set_column:
                if (!r.read_uleb128(u))
                    return ERR_OPERAND_FETCH;
                /* TODO: bounds-check */
                rs.registers_.column = u;
                break;
            case DW_LNS_negate_stmt:
                rs.registers_.is_stmt = !rs.registers_.is_stmt;
                break;
            case DW_LNS_set_basic_block:
                rs.registers_.basic_block = true;
                break;
            case DW_LNS_const_add_pc:
                rs.advance_pc(special_opcode_operation_advance(255));
                break;
            case DW_LNS_fixed_advance_pc:
                {
                    uint16_t h;
                    if (!r.read_u16(h))
                        return ERR_OPERAND_FETCH;
                    /* "It also does not multiply the operand by the
                     * minimum_instruction_length field of the header" */
                    rs.registers_.address += h;
                }
                break;
            case DW_LNS_set_prologue_end:
                rs.registers_.prologue_end = true;
                break;
            case DW_LNS_set_epilogue_begin:
                rs.registers_.epilogue_begin = true;
                break;
            case DW_LNS_set_isa:
                if (!r.read_uleb128(u))
                    return ERR_OPERAND_FETCH;
                rs.registers_.isa = u;
                break;
            }
        }
        else
        {
            /* special opcodes, contains it's own operands */
            rs.advance_line(special_opcode_line_advance(opcode));
            rs.advance_pc(special_opcode_operation_advance(opcode));
            rs.copy(delegate);
        }
    }
    return rs.running_ ? SUCCESS : ERR_HALTED_BY_DELEGATE;
}

bool
lineno_program_t::get_source_line(
    np::spiegel::addr_t addr,
    np::util::filename_t &filename,
    unsigned &line, unsigned &column)
{
    class get_source_line_delegate_t : public lineno_program_t::delegate_t
    {
    public:
        get_source_line_delegate_t(addr_t a)
          : seeking_(a),
            have_last_(false),
            found_(false)
        {
            memset(&last_, 0, sizeof(last_));
        }

        bool receive_registers(const lineno_program_t::run_state_t *rs) override
        {
            auto reg = rs->registers();
            if (have_last_)
            {
#if 0
                dprintf("registers: addr 0x%lx file %s line %u %s",
                        reg.address,
                        rs->get_absolute_filename(reg.file).c_str(),
                        reg.line,
                        reg.end_sequence ? "EOS" : "");
#endif
                if (seeking_ >= last_.address && seeking_ < reg.address)
                {
                    found_ = true;
                    abs_filename_ = rs->get_absolute_filename(last_.file);
                    return false;
                }
            }
            if (reg.end_sequence)
            {
                have_last_ = false;
                memset(&last_, 0, sizeof(last_));
            }
            else
            {
                have_last_ = true;
                last_ = reg;
            }
            return true;
        }

        bool was_found() const { return found_; }
        np::util::filename_t get_absolute_filename() const { return abs_filename_; }
        uint32_t get_line() const { return last_.line; }
        uint32_t get_column() const { return last_.column; }

    private:
        addr_t seeking_;
        bool have_last_;
        lineno_program_t::registers_t last_;
        bool found_;
        np::util::filename_t abs_filename_;
    };
    get_source_line_delegate_t delegate(addr);
    status_t status = run(&delegate);
    if (status != ERR_HALTED_BY_DELEGATE || !delegate.was_found())
        return false;
    filename = delegate.get_absolute_filename();
    line = delegate.get_line();
    column = delegate.get_column();
    return true;
}

// close namespaces
}; }; };
