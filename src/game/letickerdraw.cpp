#include "game/xobjleticker.h"
#include "game/cgame.h"

/**
 * @note Address: 0x800E51BC
 * @note Size: 0x324
 */
// NONMATCHING: control flow
void XObjLeticker::vfAC()
{
	DrawWithView(0);
	switch (mState) {
	case 1: {
		XObj* part     = gGame.GetObject(mLinked[0]);
		s32 path[8]    = { 0, 0, 0, 0, 0, 0, 0, 0 };
		HSD_JObj* jobj = FindJointByPath(part->mJObj, path, 7);
		HSD_JObjClearFlags(jobj, 0x10);
		f32 y;
		for (s32 i = 0; 0.0f < (y = mHeight - 24.0f * i); i++) {
			HSD_JObjSetTranslateY(jobj, y);
			HSD_JObjDispAll(jobj, NULL, 7, 0);
		}
		HSD_JObjSetFlags(jobj, 0x10);
		HSD_JObjSetTranslateY(jobj, 0.0f);
		break;
	}
	case 2:
		gGame.GetObject(mLinked[0])->mStatus.mFlags &= ~0x80000000ULL;
		break;
	default:
		break;
	}
}
