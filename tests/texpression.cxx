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
#include "np/spiegel/common.hxx"
#include "np/spiegel/dwarf/expression.hxx"
#include "np/spiegel/dwarf/enumerations.hxx"
#include "np/util/log.hxx"
#include "fw.h"

using namespace std;
using namespace np::spiegel::dwarf;

int
main(int argc, char **argv)
{
    np::util::argv0 = argv[0];
    if (argc != 1)
        np::util::fatal("Usage: tstack\n");
    np::log::basic_config(is_verbose() ? np::log::DEBUG : np::log::INFO, NULL);

    BEGIN("DW_OP_lit0");
    static const uint8_t prog[] = { DW_OP_lit0 };
    np::spiegel::addr_t result = ~0UL;
    expression_t expression(prog, sizeof(prog));
    CHECK(expression.execute(&result) == expression_t::SUCCESS);
    CHECK(result == 0x0);
    END;

    BEGIN("DW_OP_lit17");
    static const uint8_t prog[] = { DW_OP_lit0+17 };
    np::spiegel::addr_t result = ~0UL;
    expression_t expression(prog, sizeof(prog));
    CHECK(expression.execute(&result) == expression_t::SUCCESS);
    CHECK(result == 17);
    END;

    BEGIN("DW_OP_addr");
#if _NP_ADDRSIZE == 4
    static const uint8_t prog[] = { DW_OP_addr, 0x12, 0x34, 0x56, 0x78 };
#elif _NP_ADDRSIZE == 8
    static const uint8_t prog[] = { DW_OP_addr, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
#endif
    np::spiegel::addr_t result = ~0UL;
    expression_t expression(prog, sizeof(prog));
    CHECK(expression.execute(&result) == expression_t::SUCCESS);
#if _NP_ADDRSIZE == 4
    CHECK(result == 0x78563412);
#elif _NP_ADDRSIZE == 8
    CHECK(result == 0xf0debc9a78563412);
#endif
    END;

    BEGIN("Bad opcode");
    static const uint8_t prog[] = { 0xf0 };
    np::spiegel::addr_t result = ~0UL;
    expression_t expression(prog, sizeof(prog));
    CHECK(expression.execute(&result) == expression_t::ERR_BAD_OPCODE);
    END;

    return 0;
}

