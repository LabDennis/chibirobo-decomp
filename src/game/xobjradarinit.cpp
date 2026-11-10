#include "game/xobjattachment.h"
#include <math.h>

// XObjRadar::InitializeObject
/**
 * @note Address: 0x800D6F98
 * @note Size: 0x50
 */
extern "C" void InitializeObject__9XObjRadarFUl(XObjRadar* object, u32 id)
{
	InitializeObject__4XObjFUl(object, id);
	object->vf94();
	object->mYaw = object->mPitch = NAN;
}
