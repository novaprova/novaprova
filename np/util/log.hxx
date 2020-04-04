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
#ifndef __np_log_hxx__
#define __np_log_hxx__ 1

#include "np/util/common.hxx"

namespace np {
namespace log {

enum level_t
{
    DEBUG=1,
    INFO,
    WARN,
    ERROR
};

extern void basic_config(level_t, FILE *);
extern level_t level;
extern void _printf(level_t lev, const char *function,
                    unsigned long line, const char *fmt, ...);
extern void _vprintf(level_t lev, const char *function,
                     unsigned long line, const char *fmt, va_list);
#define _np_logprintf(_level, ...) \
    do { \
        if (np::log::level <= (_level)) \
            np::log::_printf((_level), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__); \
    } while (0)
#define _np_logvprintf(_level, fmt, args) \
    do { \
        if (np::log::level <= (_level)) \
            np::log::_vprintf((_level), __PRETTY_FUNCTION__, __LINE__, fmt, args); \
    } while (0)

#if _NP_DEBUG
#define dprintf(...) _np_logprintf(np::log::DEBUG, __VA_ARGS__)
static inline bool is_enabled_for(level_t lev) { return np::log::level <= lev; }
#else
#define dprintf(...)
static inline bool is_enabled_for(level_t lev) { return lev != np::log::DEBUG && np::log::level <= lev; }
#endif
#define iprintf(...) _np_logprintf(np::log::INFO, __VA_ARGS__)
#define wprintf(...) _np_logprintf(np::log::WARN, __VA_ARGS__)
#define eprintf(...) _np_logprintf(np::log::ERROR, __VA_ARGS__)

// close the namespaces
}; };

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* __np_log_hxx__ */
