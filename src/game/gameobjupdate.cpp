#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/xobj.h"
#include "game/stagedata.h"

/**
 * @note Address: 0x8000C398
 * @note Size: 0xB8
 */
// CGame's vf24, every live object's DrawScreenOverlay, then the stage's function 4
void CGame::vf24()
{
	if (mStatus.mFlags & 1ULL) {
		for (s32 i = 0; i < mXObjs.mObjectCount; ++i) {
			XObj* object = mXObjs.mObjects[i];
			if (object->mStatus.mFlags & 1ULL)
				object->DrawScreenOverlay();
		}
		CallStageModule(4, 0, NULL);
	}
}
