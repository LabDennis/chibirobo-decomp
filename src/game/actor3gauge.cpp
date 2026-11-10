#include "game/xobjactor.h"
#include "game/cgame.h"
#include "game/infoscrn.h"

/**
 * @note Address: 0x801151E0
 * @note Size: 0xBC
 */
void XObjActor3::UpdateObjectGauge(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 1, 1);
		mEventCommand = 0;
		break;
	case 2:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 0, 1);
		mEventCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x8011529C
 * @note Size: 0x4
 */
void XObjActor3::Kind0(u32 frames)
{
}
