#ifndef _GAME_XACTIONBASE_H
#define _GAME_XACTIONBASE_H

#include "Version.h"
#include "game/xobjdata.h"

#if defined(VERSION_R24J01)
struct WiiPlayerState;
#else
class XObjChr;
#endif

class XAction {
public:
#if defined(VERSION_R24J01)
	WiiPlayerState* mOwner; // _00
#else
	XObjChr* mOwner; // _00
#endif
	s32 mState; // _04, 0 while idle
	            // _08 vtable

#if defined(VERSION_R24J01)
	virtual void Init(WiiPlayerState* owner); // _08
#else
	virtual void Init(XObjChr* owner); // _08, 0x800BBD7C
#endif
	virtual void Reset(); // _0C, GameCube 0x800BB708

	s32 mMoveDirection; // _0C
	CVec mTargetOffset; // _10, push keeps the offset to its target here
	XObj* mTarget;      // _20
	s32 mTargetState;   // _24
	s32 mTargetFloor;   // _28, the selected floor when the target was picked

	bool IsState(s32 state) const { return mState == state; }

	void SetTarget(XObj* target)
	{
		mTarget      = target;
		mTargetFloor = mTarget->mSelectedFloorIndex;
	}
#if !defined(VERSION_R24J01)
	void SetState(s32 state, XObj* target)
	{
		mState = state;
		if (target != NULL) {
			SetTarget(target);
		}
	}
#endif
};
STATIC_ASSERT(sizeof(XAction) == 0x2C);

#endif
