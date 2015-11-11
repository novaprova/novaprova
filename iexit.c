/* iexit.c - intercept exit() calls from CUT */
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
#include "np_priv.h"
#include "except.h"
#include <stdlib.h>
#include <unistd.h>

namespace np {
using namespace std;

static void mock_exit(int status)
{
    static char cond[64];
    snprintf(cond, sizeof(cond), "exit(%d)", status);
    np_throw(np::event_t(np::EV_EXIT, cond).with_stack());
}

void init_exit_intercepts(testnode_t *tn)
{
    tn->add_mock((np::spiegel::addr_t)&exit,
		 "exit",
		 (np::spiegel::addr_t)&mock_exit);
}

// close the namespace
};
