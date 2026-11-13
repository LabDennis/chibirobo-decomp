#include "game/xobjactor.h"

/**
 * @note Address: 0x8010706C
 * @note Size: 0x18
 */
extern "C" void ResetActorMovementStep(void* object)
{
	XObjActor* actor    = (XObjActor*)object;
	actor->mKindStep    = 0;
	actor->mKindCounter = 0;
	actor->mMoveSpeed   = 0.0f;
}

/**
 * @note Address: 0x80107084
 * @note Size: 0x1C
 */
Vec XObjActor::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x801070A0
 * @note Size: 0x4
 */
void XObjActor::OnPushDisplacement(const Vec& displacement)
{
}
