#include "game/xobjspiderboss.h"
#include "game/hsd_cobj.h"

/**
 * @note Address: 0x80103B80
 * @note Size: 0x340
 */
// gets back up, blinking
void XObjSpiderBoss::State1(u32 frames)
{
	switch (mStep) {
	case 0:
		mSpiderStatus.mFlags |= 0x10;
		SetAnim(11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		++mStep;
		mBlinkTimer = 0;
		mSpiderStatus.mFlags &= ~0x200;
		break;
	case 1: {
		if (mBlinkTimer % 30 < 15) {
			SetColor(1, 128, 0, 0, 0.0f);
		} else {
			SetColor(1, 0, 0, 0, 0.0f);
		}
		++mBlinkTimer;
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		if (end - frames <= frame) {
			SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mBlinkTimer = 0;
			SetColor(1, 0, 0, 0, 0.0f);
			mStep      = 0;
			mTimer     = 0;
			mPrevState = mState;
			mState     = 0;
			mSpiderStatus.mFlags |= 0x8000;
			mSpiderStatus.mFlags |= 0x02000000;
			mSpiderStatus.mFlags |= 0x40000000;
			mSpiderStatus.mFlags &= ~0x10;
			mSpiderStatus.mFlags |= 0x80000;
			mHits = 16;
		}
		break;
	}
	}
}
