#include "game/xobjflags.h"
#include "game/xchr.h"
#include "game/cgame.h"
#include "game/stagedata.h"
#include <math.h>

extern "C" u32 GetPickupRewardValue(s32 index);

static inline bool IsCoinItemInRange(const XObjCoin* coin, s32 first, s32 end)
{
	bool inRange = false;
	if (coin->GetItem() >= first && coin->GetItem() < end)
		inRange = true;
	return inRange;
}

/**
 * @note Address: 0x800E1440
 * @note Size: 0x540
 */
s32 XObjCoin::OnPlayerContact(s32 unused, s32 value)
{
	if ((value & 3U) == 1)
		return value;
	if (mStatus.mFlags & 0x100000)
		return 0;

	XObjChr* player = gGame.GetPlayer();
	player->SetColor(1, 0, 120, 120, 0.0f);
	player->SetColor(1, 0, 0, 0, 0.2f);
	player->mFallDistance = 0.0f;
	mStatus.mFlags |= 0x4000000000000000ULL;
	SetFlag();

	s32 sound  = -1;
	s32 effect = -1;
	if (IsCoinItemInRange(this, 0, 100)) {
		AdjustPlayerMoney(&player->mParams, GetPickupRewardValue(GetItem()));
		sound = 0x89;
	} else if (IsCoinItemInRange(this, 100, 200)) {
		s32 scrap = GetPickupRewardValue(GetItem() - 97);
		if (!scrap) {
			AttrInfo attr(CoordInfo(GetSpawnData(mAnimId + 39)));
			scrap = GetPickupRewardValue(GetDataItem(attr._00) - 97);
		}
		AddPlayerScrap(&player->mParams, scrap);
		sound = 0x123;
	} else if (IsCoinItemInRange(this, 200, 300)) {
		Params& stats = player->mParams;
		s32 reward    = GetPickupRewardValue(GetItem() - 193);
		stats.SetBattery(reward + stats.mBattery);
		sound  = 0x8e;
		effect = 20;
	} else if (IsCoinItemInRange(this, 300, 400)) {
		AdjustPlayerExperience(&player->mParams, GetPickupRewardValue(GetItem() - 292));
		sound = 0x11d;
	}
	if (effect >= 0) {
		Vec rotation = {};
		gGame.mParticle.CreateSRT(0x10000 | effect, 1, mMoveTranslate, mMoveScale, rotation, FALSE);
	}
	if (sound >= 0)
		PlayGameSound(&gGame.mSound, sound, 255);
	_98C = 1.0f;
	return 0;
}

/**
 * @note Address: 0x800E1980
 * @note Size: 0x4E0
 */
void XObjCoin::UpdateObject(u32 frames)
{
	const f32 radians = 0.01745329252f;
	if (IsCoinItemInRange(this, 0, 100)) {
		f32 speed;
		if (mSpin >= 0.0f) {
			mSpin += 4.0f * radians;
			speed = (1.5f + (f32)sin(mSpin)) * 0.5f;
		} else {
			mSpin -= 4.0f * radians;
			speed = -(1.5f + (f32)sin(mSpin)) * 0.5f;
		}
		mMoveRotate.y += 8.0f * radians * speed * frames;
	} else if (IsCoinItemInRange(this, 100, 200)) {
		if (mSpin >= 0.0f)
			mMoveRotate.y += 2.0f * radians * frames;
		else
			mMoveRotate.y -= 2.0f * radians * frames;
	}
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x4000000000000000ULL) {
		if (IsCoinItemInRange(this, 0, 100)) {
			s32 anim       = mAnimId;
			HSD_AObj* aobj = GetAObj(mJObj);
			switch (anim) {
			case 0:
				SetAnim(2, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				break;
			case 1:
				break;
			case 2:
				if (aobj->flags & 0x44000000)
					Release();
				break;
			}
		} else {
			if (mMoveScale.x == 0.0f && mMoveScale.y == 0.0f && mMoveScale.z == 0.0f)
				Release();
			_98C -= 0.02f * frames;
			if (_98C <= 0.0f)
				_98C = 0.0f;
			PSVECScale(&mMoveScale, &mMoveScale, 1.12f * _98C);
		}
	}
	switch (mParam0) {
	case 1:
		mState      = 1;
		mFallFrames = 0;
		mParam0     = 0;
		mHeading    = radians * (s32)mParam1;
		mFallSpeed  = (s32)mParams[0] / 100.0f;
		mStatus.mFlags |= 0x100000;
		break;
	}
	UpdateFallingCoin(this, frames);
}
