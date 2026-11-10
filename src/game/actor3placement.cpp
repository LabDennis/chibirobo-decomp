#include "game/cgame.h"
#include "game/xobjactor.h"
#include "game/spiderplacement.h"

extern "C" {
extern Vec gActor3PlacementPosition;
extern Vec gActor3PlacementRotation;
extern s32 gSpiderBossVolleySlot;
extern s32 gActor3Kind15ActivationCounter;
}

extern "C" const f32 gActor3DegreesToRadians;
static inline Vec ActorPlacementRotation(const XObjActor3* actor)
{
	Vec rot                    = gActor3PlacementRotation;
	const UnkStageObj* p       = actor->mCoord._00;
	const f32 degreesToRadians = gActor3DegreesToRadians;
	rot.x                      = degreesToRadians * p->_10[0];
	rot.y                      = degreesToRadians * p->_10[1];
	rot.z                      = degreesToRadians * p->_10[2];
	return rot;
}
/**
 * @note Address: 0x8010A574
 * @note Size: 0x78C
 */
BOOL XObjActor3::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		bool ready = false;
		_B74       = 0.0f;
		_B78       = 0.0f;

		_B80        = 0;
		_B88        = 0;
		_B8C.mFlags = 0;
		_B8C.mFlags |= 4;
		mEventCommand = 0;
		mEventArg1    = 0;
		mEventArg2    = 0;
		mEventArg3    = 0;
		_BA4[0]       = 0;
		_BA4[1]       = 0;
		_BA4[2]       = 0;
		_BA4[3]       = 0;
		_BA4[4]       = 0;
		_BA4[5]       = 0;
		_BA4[6]       = 0;
		_BC0          = 0.0f;
		_BC4          = 0.0f;
		_BC8          = 0.0f;
		Vec zero      = { 0.0f, 0.0f, 0.0f };
		_BCC          = zero.x;
		_BD0          = zero.y;
		_BD4          = zero.z;

		mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x;
		mMoveTranslate.y = PlacementPosition(mCoord, gActor3PlacementPosition).y;
		mMoveTranslate.z = PlacementPosition(mCoord, gActor3PlacementPosition).z;
		mMoveRotate.x    = ActorPlacementRotation(this).x;
		mMoveRotate.y    = ActorPlacementRotation(this).y;
		mMoveRotate.z    = ActorPlacementRotation(this).z;
		_B7C             = ActorPlacementRotation(this).y;
		for (int i = 0; i < 8; ++i)
			mParts[i].Clear();
		if (mPoolIndex == 0) {
			gSpiderBossVolleySlot          = 0;
			gActor3Kind15ActivationCounter = 0;
		}
		switch (GetKind()) {
		case 0x42B:
		case 0x149:
			if (_980.ReadEx("tpl/stage02.tpl", 0, 0))
				ready = true;
			return ready;
		case 0x31B: {

			mTex[0] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[122 - StatusTextureOffset], 0);
			mTex[1] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[123 - StatusTextureOffset], 0);
			return TRUE;
		}
		default:
			return TRUE;
		}
	}
	return FALSE;
}
