#include "game/ccameraaction.h"

extern "C" void OSReport(const char* format, ...);

/**
 * @note Address: 0x8013801C
 * @note Size: 0x138
 */
extern "C" void InitCameraAreaEntries(CCameraActionAreaEx* state)
{
	static CamAreaEntry entries[sizeof(state->mEntryPool) / sizeof(state->mEntryPool[0])];
#if !defined(VERSION_PAL)
	OSReport("CCameraActionAreaEx :: "
	         "\x83\x4A\x83\x81\x83\x89\x83\x66\x81\x5B\x83\x5E\x82\xCC\x83\x41\x83\x68\x83\x8C\x83\x58\x82\xCD"
	         "0x%x\x82\xC5\x82\xB7\x81\x42\n",
	         reinterpret_cast<u32>(entries));
#endif
	for (u32 i = 0; i < sizeof(entries) / sizeof(entries[0]); ++i)
		state->mEntryPool[i] = &entries[i];
	state->mPrevZoomLevel = 1;
	state->mZoomLevel     = 1;
}
