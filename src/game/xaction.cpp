#include "game/xobjflags.h"
#include "game/xact.h"
#include "game/xobjattachment.h"

/**
 * @note Address: 0x800BB708
 * @note Size: 0x18
 */
// XAction::Reset
void XAction::Reset()
{
	mState         = 0;
	mMoveDirection = 0;
	mTarget        = NULL;
	mTargetState   = 0;
}

/**
 * @note Address: 0x800BB720
 * @note Size: 0x8
 */
s32 XObjFlags::GetFlagIndex()
{
	return _260;
}

/**
 * @note Address: 0x800BB728
 * @note Size: 0x54
 */
// XObjAttachment::Hide
void XObjAttachment::Hide()
{
	mVisible = FALSE;
	if (mStatus.mFlags & 1ULL)
		HSD_JObjSetFlagsAll(mJObj, 0x10);
}
