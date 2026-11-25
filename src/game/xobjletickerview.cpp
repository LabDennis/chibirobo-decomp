#include "game/cgs.h"
#include "game/xobjleticker.h"
#include <sysdolphin/axdriver.h>
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/xchr.h"
#include <math.h>

extern "C" bool IsLetickerJointClearOfObjects(XObj* obj, HSD_JObj* joint);

/**
 * @note Address: 0x800E4AAC
 * @note Size: 0x3AC
 */
extern "C" s32 GetClearLetickerApproachSide(XObj* obj, const Vec* position)
{
	const f32 heading = atan2(position->x - obj->mMoveTranslate.x, position->z - obj->mMoveTranslate.z);
	const f32 angle   = WrapAngle(heading - obj->mMoveRotate.y);
	if (obj->mMoveTranslate.y + 3.0f < gGame.GetPlayer()->mMoveTranslate.y)
		return 0;

	enum ApproachSide {
		Front,
		Back,
		Left,
		Right,
	};

	const f32 cosine = cos(angle);
	ApproachSide side;
	if (cosine > (f32)cos(0.7853982f)) {
		side = Front;
	} else if (cosine < (f32)cos(3.9269907f)) {
		side = Back;
	} else {
		side = (f32)sin(angle) > 0.0f ? Left : Right;
	}

	const s32 approachJoints[] = { 2, 6, 4, 8 };
	s32 path[]                 = { 0, approachJoints[side] };
	HSD_JObj* joint            = FindJointByPath(obj->mJObj, path, ARRAY_SIZE(path));
	return IsLetickerJointClearOfObjects(obj, joint) ? -1 - static_cast<s32>(side) : 0;
}

/**
 * @note Address: 0x800E4E58
 * @note Size: 0x364
 */
void XObjLeticker::DrawObjectEffects()
{
	if (mState != 3 || !(mLevelProgress > 0.0f))
		return;

	XObj* part = gGame.GetObject(mLinked[0]);
	if (!mPowered && part->mAnimId != 1)
		return;

	Mtx transform;
	PSMTXTrans(transform, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);

	u32 colors[50];
	Vec points[50];
	const s32 count = ARRAY_SIZE(points);
	const f32 twist = 6.0f * (mLevelProgress / mMaxLevelProgress) * (f32)sin(DEG2RAD * (mFuncTime * 5)) + 72.0f;
	for (s32 i = 0; i < count; ++i) {
		const f32 angle = DEG2RAD * mFuncTime + DEG2RAD * (twist * i);
		points[i].x     = 7.1f * (f32)sin(angle);
		points[i].y     = 12.0f + 0.2f * (24.0f * i / count) * mLevelProgress / mMaxLevelProgress;
		points[i].z     = 7.1f * (f32)cos(angle);
		colors[i]       = ((48 - i) * 32 / 48 + 32) | 0x40F000;
	}

	points[49] = points[44];
	PSMTXMultVecArray(transform, points, points, count);
	DrawGsColoredLineStrip(&gGame.mGs, points, colors, count);

	if (IsGameSoundPlaying(&gGame.mSound, 0xB0)) {
		s32 volume = GetSoundVolumeAtPosition(&gGame.mSound, &part->mMoveTranslate, 0);
		SetGameSfxVolume(&gGame.mSound, 0xB0, volume, 0);
	} else {
		PlayGameSound(&gGame.mSound, 0xB0, 0xFF);
		HSD_AudioSFXSetPitchByID(0xB0, 600.0f * (mSpeedScale - 1.0f));
	}
}

/**
 * @note Address: 0x800E51BC
 * @note Size: 0x324
 */
void XObjLeticker::DrawObject()
{
	DrawWithView(0);
	if (mState == 1) {
		XObj* part      = gGame.GetObject(mLinked[0]);
		s32 path[7]     = {};
		HSD_JObj* joint = FindJointByPath(part->mJObj, path, ARRAY_SIZE(path));
		HSD_JObjClearFlags(joint, HIDDEN);

		for (s32 i = 0;; ++i) {
			const f32 height = mHeight - 24.0f * i;
			if (!(height > 0.0f))
				break;
			HSD_JObjSetTranslateY(joint, height);
			HSD_JObjDispAll(joint, NULL, 7, 0);
		}
		HSD_JObjSetFlags(joint, HIDDEN);
		HSD_JObjSetTranslateY(joint, 0.0f);
	} else if (mState == 2) {
		gGame.GetObject(mLinked[0])->mStatus.mFlags &= ~0x80000000ULL;
	}
}
