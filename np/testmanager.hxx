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
#ifndef __NP_TESTMANAGER_H__
#define __NP_TESTMANAGER_H__ 1

#include "np/util/common.hxx"
#include "np/types.hxx"
#include "np/testnode.hxx"
#include <string>
#include <vector>

namespace np { namespace spiegel { namespace dwarf { class state_t; } } }

namespace np {

class classifier_t;

class testmanager_t : public np::util::zalloc
{
public:
    /* testmanager is a singleton */
    static testmanager_t *instance();
    static testmanager_t *instance_for_executable(const char *exe);

    testnode_t *find_node(const char *nm) const
    {
	return root_ ? root_->find(nm) : 0;
    }
    testnode_t *get_root() { return root_; }
    np::spiegel::state_t *get_spiegel_state() { return spiegel_; };

    static void done() { delete instance_; }

    spiegel::function_t *find_mock_target(std::string name);

private:
    testmanager_t();
    ~testmanager_t();

    void print_banner();
    functype_t classify_function(const char *func, char *match_return, size_t maxmatch);
    void add_classifier(const char *re, bool case_sensitive, functype_t type);
    void setup_classifiers();
    void init_spiegel_state(const char *exe);
    void discover_functions();
    void setup_builtin_intercepts();

    static testmanager_t *instance_;

    std::vector<classifier_t*> classifiers_;
    spiegel::state_t *spiegel_;
    testnode_t *root_;
    testnode_t *common_;	// nodes from filesystem root down to root_
};

// close the namespaces
};

#endif /* __NP_TESTMANAGER_H__ */
