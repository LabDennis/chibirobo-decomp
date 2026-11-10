#include "game/xchr.h"

// Shared with the adjacent player code in the original constant pool.
extern "C" {
extern const f32 lbl_8065D35C; // 0.0f
extern const f32 lbl_8065D380; // 1.0f
extern const f32 lbl_8065D43C; // 10.0f
}

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
	chr->_988.x         = lbl_8065D35C;
	chr->_988.y         = lbl_8065D35C;
	chr->_988.z         = lbl_8065D35C;
	chr->_988.mTarget.x = lbl_8065D35C;
	chr->_988.mTarget.y = lbl_8065D35C;
	chr->_988.mTarget.z = lbl_8065D35C;
	chr->_988.mSpeed.x  = lbl_8065D35C;
	chr->_988.mSpeed.y  = lbl_8065D35C;
	chr->_988.mSpeed.z  = lbl_8065D35C;
	chr->mMoveRotate.Set(chr->mMoveRotate.mTarget);
	if (chr->mPush.mState != 0) {
		ReleasePushAction(&chr->mPush);
		chr->mPush.mState = 0;
		chr->SetAnim(17, FALSE, NULL, NULL, NULL, lbl_8065D35C, lbl_8065D380, lbl_8065D35C, lbl_8065D43C);
		chr->_A2C = 3;
	}
}
