#include "game/xobjactor.h"
#include "game/xchr.h"

extern "C" {
extern XObjActorMama::StateFunc sMamaIdleState;
extern const float sMamaZero;
}

/**
 * @note Address: 0x8012926C
 * @note Size: 0xB0
 */
void XObjActorMama::ResetActorState()
{
	XObjChr* player = gGame.GetPlayer();
	if (player->_980.mFlags & 0x40000000) {
		player->_980.mFlags = 0;
		player->_980.mFlags |= 0x20000000;
		player->_980.mFlags |= 7;
		player->mPad.mReset = TRUE;
	}
	_998.mFlags = 0;
	_C8C.mFlags = 0;
	_980        = sMamaZero;
	_984        = sMamaZero;
	_988        = sMamaZero;
	_C84        = _988;
	_C88        = 0;
	mState      = sMamaIdleState;
	ResetActorMovementStep(this);
}
