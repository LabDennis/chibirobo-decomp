#include "types.h"

extern "C" {
bool OSRestoreInterrupts(uint);
uint OSDisableInterrupts();

/**
 * @note Address: 0x80150D4C
 * @note Size: 0x4
 */
void MWInitializeCriticalSection(uint* section)
{
}

/**
 * @note Address: 0x80150D1C
 * @note Size: 0x30
 */
void MWEnterCriticalSection(uint* section)
{
	*section = OSDisableInterrupts();
}

/**
 * @note Address: 0x80150CF8
 * @note Size: 0x24
 */
void MWExitCriticalSection(uint* section)
{
	OSRestoreInterrupts(*section);
}

};
