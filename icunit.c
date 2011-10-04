/* icunit.c - intercept CUnit assert failures in CUT */
#include "u4c_priv.h"
#include "except.h"
#include <valgrind/valgrind.h>

bool CU_assertImplementation(bool bValue,
			     unsigned int uiLine,
			     char strCondition[],
			     char strFile[],
			     char strFunction[] __attribute__((unused)),
			     bool bFatal __attribute__((unused)))
{
//     fprintf(stderr, "    %s at %s:%u\n", strCondition, strFile, uiLine);
    if (!bValue)
    {
	VALGRIND_PRINTF_BACKTRACE("Assert %s failed at %s:%u\n",
				  strCondition, strFile, uiLine);
	u4c_throw(event(EV_ASSERT, strCondition,
		  strFile, uiLine, strFunction));
    }
    return true;
}

