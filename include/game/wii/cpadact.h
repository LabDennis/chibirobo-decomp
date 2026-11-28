#ifndef GAME_WII_CPADACT_H
#define GAME_WII_CPADACT_H

#include "game/wii/cpad.h"
#include "game/cstatus64.h"

class CPadAct : public CStatus64 {
public:
	u64 mButton, mDirButton, mTrigger, mDirTrigger, mRepeat, mDirRepeat;
	StickPosition mStick, mSubStick;
	f32 mAnalogL, mAnalogR;
	Vec mRemoteAcceleration[5];
	Vec mNunchukAcceleration[5];
	Vec mPointer;
	f32 mPointerAngle;
	BOOL mInBounds;
	BOOL mPointerValid;
	s32 mTransitionFrames;
	BOOL mPointerChanged;
	f32 mPointerScale, mPointerFade, mPointerOpacity;
	bool mPointerSeen;
	s32 mFadeFrames;
	f32 mTransitionCooldown;
	f32 mTop, mBottom, mLeft, mRight;
	Vec mStablePointer;
	f32 mCameraInput;

	void Init();
	void ResetRumble(BOOL resetAll);
	void Update(u32 frames);
	u32 GetEventButtons() const;
	u32 GetEventTrigger() const;
	BOOL GetSwing(Vec* direction, f32* strength) const;
};
STATIC_ASSERT(sizeof(CPadAct) == 0x128);

extern "C" CPadAct gWiiPointer;

#endif
