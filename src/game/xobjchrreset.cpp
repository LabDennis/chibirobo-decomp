#include "game/xchr.h"
#include "game/evt.h"

/**
 * @note Address: 0x800B7B5C
 * @note Size: 0x1F8
 */
// puts him back to a clean state, keeps what he was holding to pick it up again after
void XObjChr::Release()
{
	RestoreControl();
	if (mJObj != NULL) {
		HSD_JObjClearMtxFlags(mJObj);
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
		case AttachmentToothbrush:
		case AttachmentSpoon:
		case AttachmentMug:
		case AttachmentSquirter:
			mAttachment = AttachmentNone;
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
