/*
 * Copyright 2014 David Arnold
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
#include "np/spiegel/intercept.hxx"
#include "np/util/tok.hxx"
#include "common.hxx"

#include <string>
#include <vector>


namespace np {
    namespace spiegel {
        namespace platform {
#if 0
        }
    }
}
#endif

using namespace std;


bool get_argv(int *argcp, char ***argvp) {
    return false;
}

char *self_exe() {
    return NULL;
}

vector<linkobj_t> get_linkobjs() {
    vector<linkobj_t>* res = new vector<linkobj_t>();
    return *res;
}

np::spiegel::addr_t normalise_address(np::spiegel::addr_t addr) {
    return addr;
}

void add_plt(const np::spiegel::mapping_t &m) {
}

int text_map_writable(addr_t addr, size_t len) {
    return -1;
}

int text_restore(addr_t addr, size_t len) {
    return -1;
}

vector<np::spiegel::addr_t> get_stacktrace() {
    vector<np::spiegel::addr_t>* res = new vector<np::spiegel::addr_t>();
    return *res;
}

bool is_running_under_debugger() {
    return false;
}

vector<string> get_file_descriptors() {
    vector<string>* res = new vector<string>();
    return *res;
}

int install_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err) {
    err = "Not yet implemented";
    return -1;
}

int uninstall_intercept(np::spiegel::addr_t addr, intstate_t &state, std::string &err) {
    err = "Not yet implemented";
    return -1;
}

int clock_gettime(clockid_t clk_id, struct timespec *res) {
    errno = ENOSYS;
    return -1;
}



// Close namespaces
#if 0
{
    {
        {
#endif
        };
    };
};
