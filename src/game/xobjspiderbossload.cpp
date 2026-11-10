#include "game/xobjspiderboss.h"

extern "C" {
extern AnimSfxEvent gSpiderBossAnimSfxEvents[]; // the anim sound events
}

/**
 * @note Address: 0x80106000
 * @note Size: 0x38
 */
extern "C" void ResetObjectState__14XObjSpiderBossFv(XObjSpiderBoss* spider)
{
	ResetObjectState__4XObjFv(spider);
	spider->mAnimSfx.mEvents = gSpiderBossAnimSfxEvents;
}

/**
 * @note Address: 0x80106038
 * @note Size: 0x50
 */
extern "C" BOOL LoadFromPlacement__14XObjSpiderBossFRC9CoordInfo(XObjSpiderBoss* spider, const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(spider, &info)) {
		ResetSpiderBossState(spider);
		ApplySpiderBossPlacement(spider);
		return TRUE;
	}
	return FALSE;
}
