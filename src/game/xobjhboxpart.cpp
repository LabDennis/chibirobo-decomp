#include "game/xobjhbox.h"
#include "game/stagedata.h"

/**
 * @note Address: 0x800E0F40
 * @note Size: 0x4C
 */
void XObjHBox::Release()
{
	mPart.mObj.Release();
	XObj::Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x800E0F8C
 * @note Size: 0x14C
 */
BOOL XObjHBox::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(this, &info))
			break;
		mEventScript = 0;
		++mLoadState;
		// fall through, the part starts reading right away
	case 1:
		if (!mPart.mObj.LoadFromPlacement(CoordInfo(GetSpawnData(36))))
			break;
		mPart.mObj.mStatus.mFlags &= ~0x10000ULL;
		mPart.mObj.SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		mPart.mObj.SetPos(mMoveTranslate, 0);
		++mLoadState;
		break;
	}
	if (mLoadState > 1) {
		mLoadState = 0;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800E10D8
 * @note Size: 0x60
 */
void XObjHBox::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mLoadState = 0;
	mRing      = 0.0f;
	mPart.mObj.ResetObjectState();
	mPart.mHit   = 0.0f;
	mPart.mRange = 5.0f;
}

/**
 * @note Address: 0x800E1138
 * @note Size: 0x30
 */
void XObjHBox::ResetForNewGame()
{
	mPart.mObj.ResetForNewGame();
}

/**
 * @note Address: 0x800E1168
 * @note Size: 0x50
 */
void XObjHBox::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	mPart.mObj.InitializeObject(id);
}
