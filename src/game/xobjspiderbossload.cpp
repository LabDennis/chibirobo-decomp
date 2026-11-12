#include "game/xobjspiderboss.h"

extern "C" {
extern AnimSfxEvent gSpiderBossAnimSfxEvents[]; // the anim sound events
}

/**
 * @note Address: 0x80106000
 * @note Size: 0x38
 */
void XObjSpiderBoss::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mAnimSfx.mEvents = gSpiderBossAnimSfxEvents;
}

/**
 * @note Address: 0x80106038
 * @note Size: 0x50
 */
BOOL XObjSpiderBoss::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		ResetSpiderBossState(this);
		ApplySpiderBossPlacement(this);
		return TRUE;
	}
	return FALSE;
}
