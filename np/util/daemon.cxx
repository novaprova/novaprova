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
#include "np/util/common.hxx"
#include <sys/fcntl.h>
#include "np/util/daemon.hxx"
#include "np/util/log.hxx"

namespace np { namespace util {
using namespace std;

volatile bool daemon_t::running_ = true;

daemon_t &
daemon_t::set_logfile(const np::util::filename_t &filename)
{
    dprintf("Setting logfile=\"%s\"\n", filename.c_str());
    logfile_ = filename;
    return *this;
}

daemon_t &
daemon_t::set_pidfile(const np::util::filename_t &filename)
{
    dprintf("Setting pidfile=\"%s\"\n", filename.c_str());
    pidfile_ = filename;
    return *this;
}

daemon_t &
daemon_t::set_foreground(bool foreground)
{
    dprintf("Setting foreground=%s\n", foreground ? "true" : "false");
    foreground_ = foreground;
    return *this;
}

void
daemon_t::signal_handler(int sig __attribute__((unused)))
{
    running_ = false;
}

// write our PID to a file to enable automation to kill us cleanly
bool
daemon_t::write_pid_file()
{
    if (pidfile_.length() == 0)
        return true;

    int pidfd = open(pidfile_.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (pidfd < 0)
    {
        eprintf("Cannot open PID file %s: %s\n", pidfile_.c_str(), strerror(errno));
        return false;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", (int)getpid());
    np::util::do_write(pidfd, buf, strlen(buf));
    close(pidfd);
    return true;
}

bool
daemon_t::cleanup_pid_file()
{
    if (pidfile_.length() == 0)
        return true;

    if (unlink(pidfile_.c_str()) < 0)
    {
        eprintf("Cannot remove PID file %s: %s\n", pidfile_.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool
daemon_t::close_file_descriptors_and_open_logfile()
{
    // Open file descriptors which will become the STDIN
    // and STDOUT/STDERR of the daemon process.
    int nullfd = open("/dev/null", O_RDONLY, 0);
    if (nullfd < 0)
    {
        eprintf("Cannot open /dev/null: %s\n", strerror(errno));
        return false;
    }

    int logfd = -1;
    if (logfile_.length() > 0)
    {
        logfd = open(logfile_.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0666);
        if (logfd < 0)
        {
            eprintf("Cannot open log file %s: %s\n", logfile_.c_str(), strerror(errno));
            close(nullfd);
            return false;
        }
    }

    // Close all file descriptors, which will have the
    // side effect of dropping our controlling terminal
    // TODO: do we need to be careful not to accidentally close Valgrind's fd?
    int tablesize = getdtablesize();
    for (int fd = 0 ; fd < tablesize ; fd++)
    {
        if (fd != nullfd && fd != logfd)
            close(fd);
    }
    dup2(nullfd, STDIN_FILENO);
    fflush(stdout);
    fflush(stderr);
    if (logfd < 0)
    {
        dup2(nullfd, STDOUT_FILENO);
        dup2(nullfd, STDERR_FILENO);
    }
    else
    {
        dup2(logfd, STDOUT_FILENO);
        dup2(logfd, STDERR_FILENO);
    }
    if (logfd >= 0 && logfd != STDOUT_FILENO && logfd != STDERR_FILENO)
        close(logfd);
    logfd = -1;
    if (nullfd != STDIN_FILENO)
        close(nullfd);
    nullfd = -1;

    return true;
}

bool
daemon_t::daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        eprintf("Failed to fork(): %s\n", strerror(errno));
        return false;
    }
    if (pid > 0)
    {
        /* parent */
        exit(0);
    }
    /* child */

    // write our PID to a file to enable automation to kill us cleanly
    if (!write_pid_file())
        return false;

    if (!close_file_descriptors_and_open_logfile())
        return false;

    // get a new session ID and group ID
    setsid();

    return true;
}

void
daemon_t::setup_signal_handlers()
{
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

bool
daemon_t::start()
{
    if (!foreground_ && !daemonize())
        return false;

    setup_signal_handlers();

    bool r = mainloop();

    if (!foreground_ && !cleanup_pid_file())
        r = false;

    return r;
}

// close the namespaces
}; };
