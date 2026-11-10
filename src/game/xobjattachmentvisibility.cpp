#include "game/xobjattachment.h"

extern "C" {
/**
 * @note Address: 0x800D552C
 * @note Size: 0x54
 */
void Show__14XObjAttachmentFv(XObjAttachment* object)
{
	object->mVisible = TRUE;
	if (object->mStatus.mFlags & 1) {
		HSD_JObjClearFlagsAll(object->mJObj, 0x10);
	}
}

/**
 * @note Address: 0x800D5580
 * @note Size: 0x3C
 */
void vfAC__14XObjAttachmentFv(XObjAttachment* object)
{
	if (object->mVisible) {
		object->DrawWithView(NULL);
	}
}
}
