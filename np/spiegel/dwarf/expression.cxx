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
#include "expression.hxx"
#include "enumerations.hxx"
#include "np/util/log.hxx"

namespace np { namespace spiegel { namespace dwarf {
using namespace std;

expression_t::status_t
expression_t::execute(/* TODO: pass registers */addr_t *resultp)
{
    ops_.rewind();
    stack_.reserve(STACK_RESERVE);

    addr_t a;
    uint32_t nexecuted = 0;
    uint8_t opcode;
    while (ops_.read_u8(opcode))
    {
        dprintf("Fetched opcode 0x%02x (%s) at offset 0x%x",
                (unsigned)opcode, opvals.to_name(opcode), ops_.get_offset()-1);
        if (++nexecuted >= MAX_EXECUTED_OPS)
            return ERR_OP_LIMIT_EXCEEDED;
        switch (opcode)
        {
            /* Values defined in the DWARF3 standard */
            case DW_OP_addr:
                /* The DW_OP_addr operation has a single operand that
                 * encodes a machine address and whose size is the size
                 * of an address on the target machine */
                if (!ops_.read_addr(a))
                    return ERR_OPERAND_FETCH;
                if (!push(a))
                    return ERR_STACK_LIMIT_EXCEEDED;
                break;
            case DW_OP_deref:
                break;
            case DW_OP_const1u:
                /* The single operand of the DW_OP_const1u operation
                 * provides a 1-byte unsigned integer constant */
                {
                    uint8_t v;
                    if (!ops_.read_u8(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const1s:
                /* The single operand of the DW_OP_const1s operation
                 * provides a 1-byte signed integer constant. */
                {
                    uint8_t v;
                    if (!ops_.read_u8(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)((int)v)))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const2u:
                /* The single operand of the DW_OP_const2u operation
                 * provides a 2-byte unsigned integer constant */
                {
                    uint16_t v;
                    if (!ops_.read_u16(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const2s:
                /* The single operand of the DW_OP_const2s operation
                 * provides a 2-byte signed integer constant. */
                {
                    uint16_t v;
                    if (!ops_.read_u16(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)((int)v)))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const4u:
                /* The single operand of the DW_OP_const4u operation
                 * provides a 4-byte unsigned integer constant. */
                {
                    uint32_t v;
                    if (!ops_.read_u32(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const4s:
                /* The single operand of the DW_OP_const4s operation
                 * provides a 4-byte signed integer constant. */
                {
                    uint32_t v;
                    if (!ops_.read_u32(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)((long)v)))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const8u:
                /* The single operand of the DW_OP_const8u operation
                 * provides an 8-byte unsigned integer constant. */
                {
                    uint64_t v;
                    if (!ops_.read_u64(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_const8s:
                /* The single operand of the DW_OP_const8s operation
                 * provides an 8-byte signed integer constant.  Wait
                 * this makes no sense??? */
                {
                    uint64_t v;
                    if (!ops_.read_u64(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)((long)v)))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_constu:
                /* The single operand of the DW_OP_constu operation
                 * provides an unsigned LEB128 integer constant. */
                {
                    uint32_t v;
                    if (!ops_.read_uleb128(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_consts:
                /* The single operand of the DW_OP_consts operation
                 * provides a signed LEB128 integer constant. */
                {
                    int32_t v;
                    if (!ops_.read_sleb128(v))
                        return ERR_OPERAND_FETCH;
                    if (!push((addr_t)v))
                        return ERR_STACK_LIMIT_EXCEEDED;
                }
                break;
            case DW_OP_dup:
            case DW_OP_drop:
            case DW_OP_over:
            case DW_OP_pick:
            case DW_OP_swap:
            case DW_OP_rot:
            case DW_OP_xderef:
            case DW_OP_abs:
            case DW_OP_and:
            case DW_OP_div:
            case DW_OP_minus:
            case DW_OP_mod:
            case DW_OP_mul:
            case DW_OP_neg:
            case DW_OP_not:
            case DW_OP_or:
            case DW_OP_plus:
            case DW_OP_plus_uconst:
            case DW_OP_shl:
            case DW_OP_shr:
            case DW_OP_shra:
            case DW_OP_xor:
            case DW_OP_bra:
            case DW_OP_eq:
            case DW_OP_ge:
            case DW_OP_gt:
            case DW_OP_le:
            case DW_OP_lt:
            case DW_OP_ne:
            case DW_OP_skip:
            case DW_OP_regx:
            case DW_OP_fbreg:
            case DW_OP_bregx:
            case DW_OP_piece:
            case DW_OP_deref_size:
            case DW_OP_xderef_size:
            case DW_OP_nop:
            case DW_OP_push_object_address:
            case DW_OP_call2:
            case DW_OP_call4:
            case DW_OP_call_ref:
            case DW_OP_form_tls_address:
            case DW_OP_call_frame_cfa:
            case DW_OP_bit_piece:
                goto unimplemented_opcode;
            default:
                switch (opcode & DW_OP_LIT_MASK)
                {
                    case DW_OP_lit0:
                    case DW_OP_lit16:
                        /* The DW_OP_litn operations encode the unsigned literal values from 0 through 31, inclusive.  */
                        if (!push((addr_t)(opcode - DW_OP_lit0)))
                            return ERR_STACK_LIMIT_EXCEEDED;
                        break;
                    case DW_OP_reg0:
                    case DW_OP_reg16:
                    case DW_OP_breg0:
                    case DW_OP_breg16:
                        goto unimplemented_opcode;
                    default:
                        eprintf("Bad opcode 0x%02x at offset 0x%x",
                                (unsigned)opcode, ops_.get_offset()-1);
                        return ERR_BAD_OPCODE;
                }
                break;
        }
    }

    if (stack_.size() != 1)
        return ERR_NO_RESULT;
    *resultp = stack_[0];
    return SUCCESS;

unimplemented_opcode:
    eprintf("Valid but unimplemented opcode 0x%02x (%s) at offset 0x%x",
            (unsigned)opcode, opvals.to_name(opcode), ops_.get_offset()-1);
    return ERR_BAD_OPCODE;
}

// close namespaces
}; }; };
