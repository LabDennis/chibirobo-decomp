// These joint diagnostics are shared by the player routines in the retail object.
#define JOBJ_H_ASSERT_JOINT(line, joint) ((joint) ? (void)0 : __assert("jobj.h", line, "jobj"))
#include "game/xchr.h"
#include "game/evt.h"

/**
 * @note Address: 0x800B7B5C
 * @note Size: 0x1F8
 */
// puts him back to a clean state, keeps what he was holding to pick it up again after
extern "C" void Release__7XObjChrFv(XObjChr* chr)
{
	chr->_980.mFlags = 0;
	chr->_980.mFlags |= 0x20000000;
	chr->_980.mFlags |= 7;
	chr->mPad.mReset = TRUE;
	HSD_JObj* jobj   = chr->mJObj;
	if (jobj != NULL) {
		HSD_JObjClearMtxFlags(jobj);
	}

	if (chr->mLift.mState != 0) {
		XObj* target = chr->mLift.mTarget;
		if (target != NULL) {
			if ((s16)target->GetPool() == 8) {
				chr->mSavedTarget      = target->GetObjFlags();
				chr->mSavedTargetAnim  = chr->mLift.mTarget->mAnimId;
				chr->mSavedTargetState = chr->mLift.mTargetState;
			}
		}
	} else {
		switch (chr->GetAttachment()) {
		case 4:
		case 5:
		case 6:
		case 7:
			chr->mAttachment = 0;
			break;
		}
	}

	chr->mAnim.Release();
	chr->mAttachmentAnim.Release();
	chr->mSuitAnim.Release();
	RemovePlayerSuit(chr);
	if (chr->mEvt.mData.mScript != NULL) {
		ResetEventScriptState(&chr->mEvt, TRUE, FALSE);
	}
}
