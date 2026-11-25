#include <dolphin/db.h>

#include "__os.h"

extern DBInterface* __DBInterface;

/**
 * @note Address: 0x801612E4
 * @note Size: 0x1C
 */
BOOL __DBIsExceptionMarked(__OSException exception)
{
	return __DBInterface->exceptionMask & (1U << exception);
}

/**
 * @note Address: 0x80161300
 * @note Size: 0x50
 */

void DBPrintf(const char* format, ...)
{
	// Retail builds discard debugger messages.
}
