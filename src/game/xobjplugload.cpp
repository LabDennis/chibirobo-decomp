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
BOOL LoadFromPlacement__8XObjPlugFRC9CoordInfo(XObjPlug* object, const CoordInfo& info)
{
	switch (object->mLoadState) {
	case 0:
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(object, &info)) {
			break;
		}
		object->mLoadState++;
	case 1:
		if (object->mWire.LoadFromPlacement(CoordInfo(GetSpawnData(16)))) {
			Vec pos = gGame.GetPlayer()->mMoveTranslate;
			ResetPlugCordPoints(&object->mCord, pos.x, pos.y, pos.z);
			object->mLoadState++;
		}
		break;
	}
	return object->mLoadState > 1;
}

// XObjPlug::ResetObjectState, the cord goes back to the player and the plug's joint is straightened
/**
 * @note Address: 0x800D9518
 * @note Size: 0x53C
 */
void ResetObjectState__8XObjPlugFv(XObjPlug* object)
{
	ResetObjectState__4XObjFv(object);
	object->_D3C = 0;
	if (object->mStatus.mFlags & 1) {
		Vec position = gGame.GetPlayer()->mMoveTranslate;
		ResetPlugCordPoints(&object->mCord, position.x, position.y, position.z);
		s32 path[] = { 0 };
		CJObj joint(FindJointByPath(object->mJObj, path, 1));
		joint.SetRotationX(0.0f, FALSE);
		joint.SetRotationZ(0.0f, FALSE);
	}
	SetPlugCordEndpointSource(&object->mCord, 0, &object->mCordAnchor);
	SetPlugCordEndpointSource(&object->mCord, 1, NULL);
	SetPlugCordEndpointSource(&object->mCord, 2, NULL);
	object->_D38 = 0;
	object->mWire.ResetObjectState();
	object->mWire.mSparkMode = -1;
}

// XObjPlug::vf94
/**
 * @note Address: 0x800D9A54
 * @note Size: 0x28
 */
void vf94__8XObjPlugFv(XObjPlug* object)
{
	object->mStatus.mFlags &= ~0x0400000000000000ULL;
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
void InitializeObject__8XObjPlugFUl(XObjPlug* object, u32 id)
{
	InitializeObject__4XObjFUl(object, id);
	InitPlugCord(&object->mCord, 20, 0.0f, 0.0f, 0.0f, 0.5f);
	object->mCord.mColor = 0x141414FF;
	object->mCord._60    = 0x141414FF;
	object->mCord.mWidth = 0.2f;
	object->mLoadState   = 0;
	object->mWire.InitializeObject(id);
}

} // extern "C"
