#include "game/xobjspiderboss.h"
#include "game/cgame.h"
#include "game/infoscrn.h"
#include "game/hsd_cobj.h"

extern "C" {
extern const char sSpiderBossAObjFile[7];
extern const char sSpiderBossAObjCondition[5];
extern const f32 sSpiderBossZero;
}

/**
 * @note Address: 0x801002DC
 * @note Size: 0x148
 */
void XObjSpiderBoss::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			if (!InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				s32 id         = (mSpiderStatus.mFlags & 0x40000000) ? mAltAnimId : mAnimId;
				HSD_AObj* aobj = GetAObj(mJObj);
				f32 frame = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj, sSpiderBossAObjFile, sSpiderBossAObjCondition) : sSpiderBossZero;
				mAnimSfx.Update(id, frame, 255);
			}
		}
	}
}

/**
 * @note Address: 0x80100424
 * @note Size: 0xC
 */
extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* spider)
{
	return (spider->mSpiderStatus.mFlags & 0x200) != 0;
}
