#include "game/xobjattachment.h"
#include <math.h>

// XObjRadar::InitializeObject
/**
 * @note Address: 0x800D6F98
 * @note Size: 0x50
 */
void XObjRadar::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	ResetForNewGame();
	mYaw = mPitch = NAN;
}
