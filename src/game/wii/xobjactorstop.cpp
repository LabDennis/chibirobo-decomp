#include "game/wii/xobj.h"

// the Wii keeps the unit's GameCube functions in reverse order (retail 0x80138560 on)

/**
 * @note Address: 0x80138560
 * @note Size: 0x4
 */
void XObjActor::OnPushDisplacement(const Vec& displacement)
{
}

/**
 * @note Address: 0x80138564
 * @note Size: 0x1C
 */
Vec XObjActor::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x80138580
 * @note Size: 0x18
 */
extern "C" void ResetActorMovementStep(void* object)
{
	XObjActor* actor    = (XObjActor*)object;
	actor->mKindStep    = 0;
	actor->mKindCounter = 0;
	actor->mMoveSpeed   = 0.0f;
}
