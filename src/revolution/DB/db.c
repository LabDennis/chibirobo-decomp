#include "types.h"
#include "revolution/db.h"
#include "Dolphin/os.h"

int DBVerbose;
DBInterface* __DBInterface;

void __DBExceptionDestination(void);

/**
 * @note Address: 0x80161264
 * @note Size: 0x28
 */
void DBInit(void)
{
	__DBInterface                   = OSPhysicalToCached(OS_DBINTERFACE_ADDR);
	__DBInterface->ExceptionDestination = (void (*)())OSCachedToPhysical(__DBExceptionDestination);
	DBVerbose                         = TRUE;
}

/**
 * @note Address: N/A
 * @note Size: 0x1C
 */
void DBIsDebuggerPresent(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x8016128C
 * @note Size: 0x48
 */
void __DBExceptionDestinationAux(void)
{
	u32* contextAddr = (u32*)0xC0;
	OSContext* context = OSPhysicalToCached(*contextAddr);
	OSReport("DBExceptionDestination\n");
	OSDumpContext(context);
	PPCHalt();
}

/**
 * @note Address: 0x801612E4
 * @note Size: 0x1C
 */
BOOL __DBIsExceptionMarked(__OSException exception)
{
	u32 mask = 1 << exception;

	return __DBInterface->exceptionMask & mask;
}

/**
 * @note Address: N/A
 * @note Size: 0x44
 */
void __DBMarkException(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
void __DBSetPresent(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x80161300
 * @note Size: 0x50
 */

void DBPrintf(char*, ...)
{
}
