#include "game/xchr.h"

// the animation file of each attachment
const char* gAttachmentAnimationPaths[] = {
	NULL, "cb_robo_6.dat", "cb_robo_7.dat", "cb_robo_8.dat", "cb_robo_9.dat", "cb_robo_10.dat", "cb_robo_11.dat", "cb_robo_12.dat", NULL,
};

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
