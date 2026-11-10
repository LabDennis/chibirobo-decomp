#include "game/mm.h"
#include "game/progressive.h"
#include "game/video.h"

#include "Dolphin/OS/OSFastCast.h"

extern "C" {
#include <sysdolphin/initialize.h>
}

// the system setup the game runs before anything else

extern "C" {
extern GXRenderModeObj GXPal528IntDf;
void srand(u32 seed);
u32 OSGetTick(void);
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
u32 OSGetResetCode(void);
void OSSetSaveRegion(void* start, void* end);
void HSD_VIDrawDoneXFB(s32 idx);
}

/**
 * @note Address: 0x8001D8A0
 * @note Size: 0x20
 */
extern "C" void GameXFBDrawDoneCallback(int idx)
{
	HSD_VIDrawDoneXFB(idx);
}

// the two words the game keeps over a reset
extern "C" {
SavedVideoMode* gVideoModeResetState;
}

/**
 * @note Address: 0x8001D8C0
 * @note Size: 0xE8
 */
extern "C" void InitGameRuntime(void)
{
	OSInitFastCast();
	srand(OSGetTick());
	void* lo = OSGetArenaLo();
	void* hi = OSGetArenaHi();
	OSSetArenaLo(OSInitAlloc(lo, hi, 1));
	HSD_SetInitParameter(HSD_INIT_FIFO_SIZE, 0x40000);
	HSD_SetInitParameter(HSD_INIT_XFB_MAX_NUM, 3);
#if defined(VERSION_PAL)
	HSD_SetInitParameter(HSD_INIT_RENDER_MODE_OBJ, &GXPal528IntDf);
#endif
	HSD_Init();
	mm::Init();
	gVideoModeResetState = (SavedVideoMode*)OSAllocFromHeap(__OSCurrHeap, sizeof(SavedVideoMode));
	if (!(OSGetResetCode() >> 31)) {
		gVideoModeResetState->hasSelection    = 0;
		gVideoModeResetState->useStandardMode = 1;
	}
	OSSetSaveRegion(gVideoModeResetState, gVideoModeResetState + 1);
}

#if defined(VERSION_PAL)
f32 gProgressiveFadeFrames = 0.15f * FRAME_RATE;
#endif
