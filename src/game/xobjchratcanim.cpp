#include "game/xchr.h"

extern "C" const char* gAttachmentAnimationPaths[];

/**
 * @note Address: 0x800B9758
 * @note Size: 0xA4
 */
// reads the attachment's anims, true once they're in
extern "C" BOOL LoadPlayerAttachmentAnimation(XObjChr* chr)
{
	const char* path = gAttachmentAnimationPaths[chr->mAttachment];
	if (path != NULL) {
		if (LoadJointAnimationAsync(&chr->mAttachmentAnim, path)) {
			chr->SetAnim(17, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}
