/*
 * Copyright 2011-2014 Gregory Banks
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
#ifndef __NP_TRACE_HXX__
#define __NP_TRACE_HXX__ 1


namespace np {
namespace trace {

extern void init();

#if _NP_ENABLE_TRACE
extern char *buf;
extern const char hexdigits[];

#define trace(msg) \
    { \
	if (np::trace::buf) \
	{ \
	    static const char _m[] = (msg); \
	    for (register const char *_x = _m ; *_x ; _x++) \
		*np::trace::buf++ = *_x; \
	} \
    }
#define trace_hex(n) \
    { \
	if (np::trace::buf) \
	{ \
	    register uint64_t _n = (n); \
	    *np::trace::buf++ = '0'; \
	    *np::trace::buf++ = 'x'; \
	    for (register int _i = 15 ; _i >= 0 ; _i--) \
	    { \
		np::trace::buf[_i] = np::trace::hexdigits[_n & 0xf]; \
		_n >>= 4; \
	    } \
	    np::trace::buf += 16; \
	} \
    }
#else
#define trace(msg)
#define trace_hex(n)
#endif

// close the namespace
}; };

#endif /* __NP_TRACE_HXX__ */
