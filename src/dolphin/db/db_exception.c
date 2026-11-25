#include <dolphin/base/PPCArch.h>
#include <dolphin/db.h>

#include "__os.h"

int DBVerbose;
DBInterface* __DBInterface;

/**
 * @note Address: 0x80161264
 * @note Size: 0x28
 */
void DBInit(void)
{
	__DBInterface = OSPhysicalToCached(OS_DBINTERFACE_ADDR);
	__DBInterface->ExceptionDestination = (void (*)(void))OSCachedToPhysical((void*)__DBExceptionDestination);
	DBVerbose = 1;
}

/**
 * @note Address: 0x8016128C
 * @note Size: 0x48
 */
void __DBExceptionDestinationAux(void)
{
	u32 contextAddress = *(u32*)OS_CURRENTCONTEXT_PADDR;
	OSContext* context = OSPhysicalToCached(contextAddress);

	OSReport("DBExceptionDestination\n");
	OSDumpContext(context);
	PPCHalt();
}
