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
#ifndef __np_spiegel_dwarf_expression_hxx__
#define __np_spiegel_dwarf_expression_hxx__

#include "np/spiegel/common.hxx"
#include "np/spiegel/dwarf/reader.hxx"
#include <vector>

namespace np {
namespace spiegel {
namespace dwarf {

class expression_t
{
public:
    expression_t(const void *base, size_t len, bool is64 = false)
      : ops_(base, len, is64) {}
    ~expression_t() {}

    enum status_t
    {
        SUCCESS = 0,
        ERR_STACK_UNDERFLOW = -1,
        ERR_STACK_LIMIT_EXCEEDED = -2,
        ERR_NO_RESULT = -3,
        ERR_OP_LIMIT_EXCEEDED = -4,
        ERR_BAD_OPCODE = -5,
        ERR_OPERAND_FETCH = -6,
    };

    enum
    {
        STACK_RESERVE = 32,
        MAX_STACK = 1024,
        MAX_EXECUTED_OPS = 1024
    };

    status_t execute(/* TODO: pass registers */addr_t *resultp);

private:
    bool push(addr_t a)
    {
        if (stack_.size() >= MAX_STACK)
            return false;
        stack_.push_back(a);
        return true;
    }

    reader_t ops_;
    std::vector<addr_t> stack_;
};

// close namespaces
}; }; };

#endif // __np_spiegel_dwarf_expression_hxx__
