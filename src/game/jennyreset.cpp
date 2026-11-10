#include "game/xobjactor.h"
#include "game/xchr.h"

extern "C" {
extern XObjActorJenny::StateFunc sJennyIdleState;
extern const float sJennyJointZero;
}

/**
 * @note Address: 0x801262F0
 * @note Size: 0x12C
 */
void XObjActorJenny::ResetActorState()
{
	XObjChr* player = gGame.GetPlayer();
	if (player->_980.mFlags & 0x40000000) {
		player->_980.mFlags = 0;
		player->_980.mFlags |= 0x20000000;
		player->_980.mFlags |= 7;
		player->mPad.mReset = TRUE;
	}
	_998.mFlags = 0;
	_C90.mFlags = 0;
	_C98.mFlags = 0;
	_980        = sJennyJointZero;
	_984        = sJennyJointZero;
	_988        = sJennyJointZero;
	_C84        = _988;
	_C88        = sJennyJointZero;
	_C8C        = 0;
	_CA0        = 0;
	mState      = sJennyIdleState;
	ResetActorMovementStep(this);
	for (u32 i = 0; i < 20; i++) {
		mCmd[i] = 0;
	}
	_C90.mFlags &= ~8;
}
