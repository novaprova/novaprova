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
	snprintf(__testname, sizeof(__testname), fmt, __VA_ARGS__); \
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
