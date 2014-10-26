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
#ifndef __NP_TRACE_H__
#define __NP_TRACE_H__ 1

extern void np_trace_init(void);

#if _NP_ENABLE_TRACE
extern char *_np_trace_buf;
extern const char _np_trace_hexdigits[];

#define np_trace(msg) \
    { \
	if (_np_trace_buf) \
	{ \
	    static const char _m[] = (msg); \
	    for (register const char *_x = _m ; *_x ; _x++) \
		*_np_trace_buf++ = *_x; \
	} \
    }
#define np_trace_hex(n) \
    { \
	if (_np_trace_buf) \
	{ \
	    register uint64_t _n = (n); \
	    *_np_trace_buf++ = '0'; \
	    *_np_trace_buf++ = 'x'; \
	    for (register int _i = 15 ; _i >= 0 ; _i--) \
	    { \
		_np_trace_buf[_i] = _np_trace_hexdigits[_n & 0xf]; \
		_n >>= 4; \
	    } \
	    _np_trace_buf += 16; \
	} \
    }
#else
#define np_trace(msg)
#define np_trace_hex(n)
#endif

#endif /* __NP_TRACE_H__ */
