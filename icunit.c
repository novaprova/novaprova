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
	u4c_throw(u4c::event_t(u4c::EV_ASSERT, strCondition).at_line(
		  strFile, uiLine).in_function(strFunction).with_stack());
    }
    return true;
}

