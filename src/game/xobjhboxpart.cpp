#include "game/xobjhbox.h"
#include "game/stagedata.h"

/**
 * @note Address: 0x800E0F40
 * @note Size: 0x4C
 */
extern "C" void Release__8XObjHBoxFv(XObjHBox* box)
{
	box->mPart.mObj.Release();
	Release__4XObjFv(box);
	box->mLoadState = 0;
}

/**
 * @note Address: 0x800E0F8C
 * @note Size: 0x14C
 */
extern "C" BOOL LoadFromPlacement__8XObjHBoxFRC9CoordInfo(XObjHBox* box, CoordInfo* info)
{
	switch (box->mLoadState) {
	case 0:
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(box, info))
			break;
		box->_80C = 0;
		++box->mLoadState;
		// fall through, the part starts reading right away
	case 1:
		if (!box->mPart.mObj.LoadFromPlacement(CoordInfo(GetSpawnData(36))))
			break;
		box->mPart.mObj.mStatus.mFlags &= ~0x10000ULL;
		box->mPart.mObj.SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		box->mPart.mObj.SetPos(box->mMoveTranslate, 0);
		++box->mLoadState;
		break;
	}
	if (box->mLoadState > 1) {
		box->mLoadState = 0;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E10D8
 * @note Size: 0x60
 */
extern "C" void ResetObjectState__8XObjHBoxFv(XObjHBox* box)
{
	ResetObjectState__4XObjFv(box);
	box->mLoadState = 0;
	box->mRing      = 0.0f;
	box->mPart.mObj.ResetObjectState();
	box->mPart.mHit   = 0.0f;
	box->mPart.mRange = 5.0f;
}

/**
 * @note Address: 0x800E1138
 * @note Size: 0x30
 */
extern "C" void vf94__8XObjHBoxFv(XObjHBox* box)
{
	box->mPart.mObj.vf94();
}

/**
 * @note Address: 0x800E1168
 * @note Size: 0x50
 */
extern "C" void InitializeObject__8XObjHBoxFUl(XObjHBox* box, u32 id)
{
	InitializeObject__4XObjFUl(box, id);
	box->mPart.mObj.InitializeObject(id);
}
