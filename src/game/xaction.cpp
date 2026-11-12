#include "game/xobjflags.h"
#include "game/xact.h"
#include "game/xobjattachment.h"

/**
 * @note Address: 0x800BB708
 * @note Size: 0x18
 */
// XAction::Reset
extern "C" void Reset__7XActionFv(XAction* action)
{
	action->mState         = 0;
	action->mMoveDirection = 0;
	action->mTarget        = NULL;
	action->mTargetState   = 0;
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
extern "C" void Hide__14XObjAttachmentFv(XObjAttachment* attachment)
{
	attachment->mVisible = FALSE;
	if (attachment->mStatus.mFlags & 1ULL)
		HSD_JObjSetFlagsAll(attachment->mJObj, 0x10);
}
