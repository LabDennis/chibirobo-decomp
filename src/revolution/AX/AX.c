#include "revolution/ax.h"
#include "Dolphin/OS.h"

const char* __AXVersion = "<< RVL_SDK - AX \trelease build: Jul 30 2008 19:18:46 (0x4199_60831) >>";
static BOOL __init;

void AXInit(void)
{
	if (!__init) {
		OSRegisterVersion(__AXVersion);
		__AXAllocInit();
		__AXVPBInit();
		__AXSPBInit();
		__AXAuxInit();
		__AXClInit();
		__AXOutInit(0);
		__init = TRUE;
	}
}

void AXQuit(void)
{
	if (__init) {
		__AXOutQuit();
		__AXAllocQuit();
		__AXVPBQuit();
		__AXSPBQuit();
		__AXAuxQuit();
		__AXClQuit();
		__init = FALSE;
	}
}

BOOL AXIsInit(void)
{
	return __init;
}
