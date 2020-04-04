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

#include "np/util/log.hxx"
#include <time.h>
#include <sys/time.h>

using namespace np::log;

level_t np::log::level = INFO;
static FILE *stream = stderr;
static const char * const level_names[] = {"", "DEBUG", "INFO", "WARN", "ERROR"};

void np::log::basic_config(level_t lev, FILE *fp)
{
    if (lev)
        level = lev;
    if (fp)
        stream = fp;
}

void np::log::_printf(level_t lev, const char *function, unsigned long line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    _vprintf(lev, function, line, fmt, args);
    va_end(args);
}

void np::log::_vprintf(level_t lev, const char *function, unsigned long line, const char *fmt, va_list args)
{
    size_t used = 0;
    char buf[16384];
    size_t maxlen = sizeof(buf) - 16;   /* leave a red zone JIC */

    /*
     * Add a simple prefix which makes it easy to pick out NovaProva's
     * log messages from other noise on stderr.
     */
    used += ::snprintf(buf+used, maxlen-used, "np: [");

    /*
     * Append a current timestamp from the system clock with microsecond
     * precision.
     */
    used += np::util::abs_format_iso8601_buf(np::util::abs_now(),
                                             buf+used, maxlen-used,
                                             NP_MICROSECONDS);

    /* Append the function, line, and level */
    used += ::snprintf(buf+used, maxlen-used, "][%s:%lu][%s] ", function, line, level_names[lev]);

    /* Append the caller's message */
    char *message_start = &buf[used];
    used += ::vsnprintf(buf+used, maxlen-used, fmt, args);

    /* Ensure there's at least one newline at the end */
    if (buf[used-1] != '\n')
    {
        buf[used++] = '\n';
        buf[used] = '\0';
    }

    /*
     * Write out the message.
     *
     * Note we're very careful to use a single fwrite() per line so that
     * libc's thread locking on stderr will ensure we don't get log
     * messages from multiple threads mixed up.
     *
     * Also, we handle multi-line log messages in a way which preserves
     * the semantic of having the line prefix (timestamp etc) on every
     * physical line in the file, while optimizing the amount of memory
     * copying in the common case of a single line message.
     */
    char *end = &buf[used];
    for (;;)
    {
        char *nl = ::strchr(message_start, '\n');
        assert(nl);
        ::fwrite(buf, 1, (nl+1-buf), stream);
        if (nl+1 == end)
            break;
        memmove(message_start, nl+1, end - (nl+1));
        end -= (nl+1-message_start);
    }
}

