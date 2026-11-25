#ifndef _GAME_XOBJCHRITEM_H
#define _GAME_XOBJCHRITEM_H

#include "game/xchr.h"
#include "game/stagedata.h"

extern "C" {
extern s32 gSpecialSuitSpawnIds[];
extern const char* gSuitAnimationPaths[];
}

static inline BOOL LoadPlayerSuitItem(XObjChr* chr)
{
	if (chr->mSuits.mCurrent == 0)
		return TRUE;
	if (chr->mSuits.mLoadStep == 0)
		return TRUE;
	switch (chr->mSuits.mLoadStep) {
	case 1: {
		XObj* item = &chr->mSuits.mObjects[chr->mSuits.mCurrent];
		s32 id     = chr->mSuits.mCurrent == 7 ? gSpecialSuitSpawnIds[chr->mSuits.mSpecial] : chr->mSuits.mCurrent + 25;
		if (!item->LoadFromPlacement(CoordInfo(GetSpawnData(id))))
			break;
		HSD_JObj** joint = &chr->mSuits.mJoints[chr->mSuits.mCurrent];
		if (*joint == NULL) {
			s32 path[4] = { 0, 1, 0, 0 };
			*joint      = FindJointByPath(item->mJObj, path, 4);
		}
		chr->mSuits.mLoadStep++;
	}
	case 2: {
		const char* path = gSuitAnimationPaths[chr->mSuits.mCurrent];
		BOOL ready       = path != NULL ? LoadJointAnimationAsync(&chr->mSuitAnim, path) : TRUE;
		if (ready) {
			chr->mSuits.mLoadStep = 0;
			return TRUE;
		}
		break;
	}
	}
	return FALSE;
}

#endif
