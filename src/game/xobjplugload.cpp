#include "game/xchr.h"
#include "game/cgame.h"
#include "game/cjobjinlineattach.h"
#include "game/stagedata.h"

extern "C" {

/**
 * @note Address: 0x800D9414
 * @note Size: 0x104
 */
BOOL XObjPlug::LoadFromPlacement(const CoordInfo& info)
{
	if (mLoadState == 0) {
		if (!XObj::LoadFromPlacement(info))
			return FALSE;
		++mLoadState;
	}

	if (mLoadState == 1) {
		const CoordInfo wirePlacement(GetSpawnData(16));
		if (!mWire.LoadFromPlacement(wirePlacement))
			return FALSE;

		const Vec& position = gGame.GetPlayer()->mMoveTranslate;
		ResetPlugCordPoints(&mCord, position.x, position.y, position.z);
		++mLoadState;
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
	XObj::ResetObjectState();
	mConnection = 0;
	if (mStatus.mFlags & 1) {
		const Vec& position = gGame.GetPlayer()->mMoveTranslate;
		ResetPlugCordPoints(&mCord, position.x, position.y, position.z);
		s32 path[]          = { 0 };
		HSD_JObj* rootJoint = FindJointByPath(mJObj, path, 1);
		if (rootJoint != NULL) {
			CJObj joint(rootJoint);
			joint.SetRotationX(0.0f, FALSE);
			joint.SetRotationZ(0.0f, FALSE);
		}
	}
	SetPlugCordEndpointSource(&mCord, 0, &mCordAnchor);
	SetPlugCordEndpointSource(&mCord, 1, NULL);
	SetPlugCordEndpointSource(&mCord, 2, NULL);
	mCordEndsPinned = 0;
	mWire.ResetObjectState();
	mWire.mSparkMode = -1;
}

// XObjPlug::ResetForNewGame
/**
 * @note Address: 0x800D9A54
 * @note Size: 0x28
 */
void XObjPlug::ResetForNewGame()
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
	XObj::InitializeObject(id);
	InitPlugCord(&mCord, 20, 0.0f, 0.0f, 0.0f, 0.5f);
	mCord.mColor = 0x141414FF;
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
	mLoadState   = 0;
	mWire.InitializeObject(id);
}

} // extern "C"
