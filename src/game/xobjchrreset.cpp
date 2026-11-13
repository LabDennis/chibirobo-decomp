// These joint diagnostics are shared by the player routines in the retail object.
#define JOBJ_H_ASSERT_JOINT(line, joint) ((joint) ? (void)0 : __assert("jobj.h", line, "jobj"))
#include "game/xchr.h"
#include "game/evt.h"

/**
 * @note Address: 0x800B7B5C
 * @note Size: 0x1F8
 */
// puts him back to a clean state, keeps what he was holding to pick it up again after
void XObjChr::Release()
{
	_980.mFlags = 0;
	_980.mFlags |= 0x20000000;
	_980.mFlags |= 7;
	mPad.mReset    = TRUE;
	HSD_JObj* jobj = mJObj;
	if (jobj != NULL) {
		HSD_JObjClearMtxFlags(jobj);
	}

	if (mLift.mState != 0) {
		XObj* target = mLift.mTarget;
		if (target != NULL) {
			if ((s16)target->GetPool() == 8) {
				mSavedTarget      = target->GetObjFlags();
				mSavedTargetAnim  = mLift.mTarget->mAnimId;
				mSavedTargetState = mLift.mTargetState;
			}
		}
	} else {
		switch (GetAttachment()) {
		case 4:
		case 5:
		case 6:
		case 7:
			mAttachment = 0;
			break;
		}
	}

	mAnim.Release();
	mAttachmentAnim.Release();
	mSuitAnim.Release();
	RemovePlayerSuit(this);
	if (mEvt.mData.mScript != NULL) {
		ResetEventScriptState(&mEvt, TRUE, FALSE);
	}
}
