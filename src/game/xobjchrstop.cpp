#include "game/xchr.h"

// Shared with the adjacent player code in the original constant pool.
/**
 * @note Address: 0x800BAD80
 * @note Size: 0x2C
 */
extern "C" CVec GetPlayerJointLocalTranslationByPath(CJObj* object, s32 depth, s32* path)
{
	return CJObj_GetJointLocalTranslationByPath(object, depth, path);
}

/**
 * @note Address: 0x800BADAC
 * @note Size: 0xC4
 */
// stops him where he stands
extern "C" void StopPlayerMotion(XObjChr* chr)
{
	chr->_988.x         = 0.0f;
	chr->_988.y         = 0.0f;
	chr->_988.z         = 0.0f;
	chr->_988.mTarget.x = 0.0f;
	chr->_988.mTarget.y = 0.0f;
	chr->_988.mTarget.z = 0.0f;
	chr->_988.mSpeed.x  = 0.0f;
	chr->_988.mSpeed.y  = 0.0f;
	chr->_988.mSpeed.z  = 0.0f;
	chr->mMoveRotate.Set(chr->mMoveRotate.mTarget);
	if (chr->mPush.mState != 0) {
		ReleasePushAction(&chr->mPush);
		chr->mPush.mState = 0;
		chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		chr->_A2C = 3;
	}
}
