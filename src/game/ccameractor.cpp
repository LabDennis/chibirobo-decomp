#include "game/hsd_jobj.h"
#include "game/ccamera.h"
#include "game/cgame.h"

extern "C" {
extern void* __vt__10CStopwatch[];
extern void* __vt__7CCamera[];
extern void* __vt__4CVec[];
}

/**
 * @note Address: 0x8001B97C
 * @note Size: 0x10
 */
// todo: weak copy
extern "C" CStopwatch* __ct__10CStopwatchFv(CStopwatch* watch)
{
	*(void***)((u8*)watch + 0x38) = __vt__10CStopwatch;
	return watch;
}

/**
 * @note Address: 0x8001B98C
 * @note Size: 0x2C
 */
// todo: weak copy
// NONMATCHING: regalloc
extern "C" CCamera* __ct__7CCameraFv(CCamera* camera)
{
	*(void***)((u8*)camera + 0x190)           = __vt__7CCamera;
	*(void***)((u8*)&camera->mEye + 0xC)      = __vt__4CVec;
	*(void***)((u8*)&camera->mInterest + 0xC) = __vt__4CVec;
	*(void***)((u8*)&camera->mAngle + 0xC)    = __vt__4CVec;
	*(void***)((u8*)&camera->mUp + 0xC)       = __vt__4CVec;
	*(void***)((u8*)&camera->mScale + 0xC)    = __vt__4CVec;
	return camera;
}
