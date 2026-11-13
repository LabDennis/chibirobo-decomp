#include "game/xchr.h"
#include "game/cjobjinlineattach.h"
#include "game/texturecopy.h"
#include "game/stagedata.h"

// XObjPlug, the plug chibi robo pulls around: its load and init

extern "C" {

// XObjPlug::LoadFromPlacement, the plug, then its wire effect from spawn 16
/**
 * @note Address: 0x800D9414
 * @note Size: 0x104
 */
BOOL XObjPlug::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
			break;
		}
		mLoadState++;
	case 1:
		if (mWire.LoadFromPlacement(CoordInfo(GetSpawnData(16)))) {
			Vec pos = gGame.GetPlayer()->mMoveTranslate;
			ResetPlugCordPoints(&mCord, pos.x, pos.y, pos.z);
			mLoadState++;
		}
		break;
	}
	return mLoadState > 1;
}

// XObjPlug::ResetObjectState, the cord goes back to the player and the plug's joint is straightened
/**
 * @note Address: 0x800D9518
 * @note Size: 0x53C
 */
void XObjPlug::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	_D3C = 0;
	if (mStatus.mFlags & 1) {
		Vec position = gGame.GetPlayer()->mMoveTranslate;
		ResetPlugCordPoints(&mCord, position.x, position.y, position.z);
		s32 path[] = { 0 };
		CJObj joint(FindJointByPath(mJObj, path, 1));
		joint.SetRotationX(0.0f, FALSE);
		joint.SetRotationZ(0.0f, FALSE);
	}
	SetPlugCordEndpointSource(&mCord, 0, &mCordAnchor);
	SetPlugCordEndpointSource(&mCord, 1, NULL);
	SetPlugCordEndpointSource(&mCord, 2, NULL);
	_D38 = 0;
	mWire.ResetObjectState();
	mWire.mSparkMode = -1;
}

// XObjPlug::vf94
/**
 * @note Address: 0x800D9A54
 * @note Size: 0x28
 */
void XObjPlug::vf94()
{
	mStatus.mFlags &= ~0x0400000000000000ULL;
}

/**
 * @note Address: 0x800D9A7C
 * @note Size: 0x8
 */
void SetPlugCordSpeed(XObjPlug* object, f32 speed)
{
	object->mCord.mSpeed = speed;
}

// XObjPlug::InitializeObject
/**
 * @note Address: 0x800D9A84
 * @note Size: 0x8C
 */
void XObjPlug::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	InitPlugCord(&mCord, 20, 0.0f, 0.0f, 0.0f, 0.5f);
	mCord.mColor = 0x141414FF;
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
	mLoadState   = 0;
	mWire.InitializeObject(id);
}

} // extern "C"
