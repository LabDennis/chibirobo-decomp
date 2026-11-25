#include "game/xobjactorbase.h"

/**
 * @note Address: 0x8010706C
 * @note Size: 0x18
 */
void XObjActor::ResetMovementStep()
{
	mKindStep    = 0;
	mKindCounter = 0;
	mMoveSpeed   = sZero;
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
