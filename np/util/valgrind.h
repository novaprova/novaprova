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
#ifndef __np_valgrind_h__
#define __np_valgrind_h__ 1

#include "np/util/config.h"

#if HAVE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#else
#define RUNNING_ON_VALGRIND 0
#define VALGRIND_DISCARD_TRANSLATIONS(a, l)
#endif

#endif /* __np_valgrind_h__ */
