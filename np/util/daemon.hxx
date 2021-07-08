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
#ifndef __np_util_daemon_hxx__
#define __np_util_daemon_hxx__ 1

#include <string>
#include "np/util/filename.hxx"

namespace np { namespace util {

class daemon_t
{
private:
    bool foreground_;
    np::util::filename_t logfile_;
    np::util::filename_t pidfile_;
    static volatile bool running_;

    static void signal_handler(int sig);
    bool write_pid_file();
    bool cleanup_pid_file();
    bool close_file_descriptors_and_open_logfile();
    bool daemonize();
    void setup_signal_handlers();

public:
    daemon_t() : foreground_(false) {}

    daemon_t &set_logfile(const np::util::filename_t &filename);
    daemon_t &set_pidfile(const np::util::filename_t &filename);
    daemon_t &set_foreground(bool foreground);

    bool running() const { return running_; }

    bool start();

    virtual bool mainloop() = 0;
};

// close the namespaces
}; };

#endif // __np_util_daemon_hxx__
