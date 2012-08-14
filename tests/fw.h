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
#ifndef __novaprova_tests_fw_h__
#define __novaprova_tests_fw_h__ 1

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int setup(void);
extern int teardown(void);
extern int is_verbose(void);
extern char __testname[1024];

#ifdef __cplusplus
};
#endif

#define BEGIN(fmt, ...) \
    { \
	snprintf(__testname, sizeof(__testname), fmt, ## __VA_ARGS__); \
	printf(". %s ", __testname); \
	if (is_verbose()) printf("...\n"); \
	fflush(stdout); \
	if (setup()) { \
	    if (is_verbose()) printf(". %s ", __testname); \
	    printf("- setup FAIL\n"); \
	    fflush(stdout); \
	    return 1; \
	} \

#define CHECK(expr) \
	do { \
	    if (is_verbose()) { \
		printf("+ [%d] " #expr "\n", __LINE__); \
		fflush(stdout); \
	    } \
	    if (!(expr)) { \
		teardown(); \
		if (is_verbose()) printf(". %s ", __testname); \
		printf("FAIL\n"); \
		fflush(stdout); \
		return 1; \
	    } \
	} while(0)

#define END \
	teardown(); \
	if (is_verbose()) printf(". %s ", __testname); \
	printf("PASS\n"); \
	fflush(stdout); \
    }

#endif /* __novaprova_tests_fw_h__ */
