#include "game/xobjleticker.h"
#include "game/xchr.h"
#include "game/ccameraaction.h"
#include "game/cmotioncontrolcamera.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cpad.h"
#include "game/cparticle.h"
#include "game/gmath.h"
#include "game/animframe.h"
#include <math.h>
extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <dolphin/os.h>
}
#include <sysdolphin/controller.h>

// XObjLeticker, and the camera that turns around it in steps, CCameraActionLeticker

extern "C" {
void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
BOOL HSD_AudioSFXSetPitchByID(s32 sfx, s16 pitch);
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle);
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle);
bool IsLetickerJointClearOfObjects(XObj* obj, HSD_JObj* jobj);
#if !defined(VERSION_PAL)
extern const char sLetickerEyeXZFormat[];
extern const char sLetickerTurnAngleFormat[];
extern const char sLetickerUninitializedMessage[];
extern const char sLetickerLevelEntryFormat[];
extern const char sLetickerCameraResetMessage[];
#endif
extern AnimSfxEvent sLetickerAnimSfxEvents[];
extern const f32 sLetickerLeftTurnSpeed, sLetickerRightTurnSpeed;
void SelectHighestActiveFloor(XObj* obj);
extern CCameraActionLeticker gLetickerCameraAction;
extern XObjFloor* sLetickerFloorsByLevel[];
extern XObjFloor* sLetickerLiftFloors[2];
}

// sets the object's floor boxes, they take effect at once
static inline void SetFloors(XObj* obj, XObjFloor* floors)
{
	obj->mFloors = floors;
	obj->_250[1] = 0;
	obj->_250[0] = 0;
	UpdateObjectFloorXZExtent(obj);
	UpdateObjectFloorHeights(obj);
}

// puts an anim in the first free slot of the object's queue
static inline void PushAnim(XObj* obj, s32 id, u16 frame, s16 rate)
{
	for (int i = 0; i < 16; i++) {
		s32 j = (obj->mAnmQueue.mNext + i) % 16;
		if (obj->mAnmQueue.mEntry[j].mId == -1) {
			obj->mAnmQueue.mEntry[j].mId    = id;
			obj->mAnmQueue.mEntry[j].mFrame = frame;
			obj->mAnmQueue.mEntry[j].mRate  = rate;
			return;
		}
	}
}

static inline f32 Cos(f32 x)
{
	return cos(x);
}

static inline f32 Sin(f32 x)
{
	return sin(x);
}

// which quarter an angle faces, 0 front, 1 left, 2 back, 3 right
static inline s32 GetSide(f32 angle)
{
	s32 side;
	if (Cos(0.7853982f) <= Cos(angle)) {
		side = 0;
	} else if (Cos(angle) <= Cos(3.9269907f)) {
		side = 2;
	} else if (0.0f < Sin(angle)) {
		side = 1;
	} else {
		side = 3;
	}
	return side;
}

// whether the eye at that angle stays inside the room
static inline bool CanTurn(CCameraActionLeticker* camera, f32 angle)
{
	if (!camera->mBounded) {
		return true;
	}
	Vec eye;
	camera->CalcEye(&eye, angle);
#if !defined(VERSION_PAL)
	OSReport(sLetickerEyeXZFormat, eye.x, eye.z);
#endif
	if (eye.x < camera->mMinX || camera->mMaxX < eye.x || eye.z < camera->mMinZ || camera->mMaxZ < eye.z) {
#if !defined(VERSION_PAL)
		OSReport(sLetickerTurnAngleFormat, eye.x, eye.z, (s32)(57.295776f * camera->mTurnTo));
#endif
		return false;
	}
	return true;
}

/**
 * @note Address: 0x800E2EAC
 * @note Size: 0x1A4
 */
void XObjLeticker::OnPushDisplacement(const Vec& displacement)
{
	XObj* part;
	for (s32 i = 0; i < mLinkedCount; i++) {
		part = gGame.GetObject(mLinked[i]);
		part->SetTranslateX(mMoveTranslate.x, 0);
		part->SetTranslateZ(mMoveTranslate.z, 0);
	}
	if (0.0f < PSVECMag(&displacement)) {
		vf12C(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	} else {
		vf12C(1, 0xFF, 0, 0, 1.0f);
		SetAnim(7, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	if (mPtcl) {
		(&gGame.mParticle)->Kill(0xA5);
		mPtcl = FALSE;
	}
}

/**
 * @note Address: 0x800E3050
 * @note Size: 0x1DC
 */
void XObjLeticker::vf148()
{
	if (mState != 3) {
		gGame.mStatus.mFlags &= ~0x80ULL;
		gGame.mMotionControlCamera.PopAction();
	}
	_A2D = FALSE;
	_A30 = 0;
	if (mState == 2) {
		mMaxLevel         = mLevelBySide[GetSide(mHeading)];
		mMaxLevelProgress = 10.0f * mMaxLevel;
		mLevelProgress    = 10.0f * mCurrentLevel;
	}
	vf12C(1, 0, 0, 0, 0.5f);
	SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800E322C
 * @note Size: 0x8C4
 */
BOOL XObjLeticker::vf134(f32 amount)
{
	if (gGame.GetPlayer()->mMoveTranslate.y < mMoveTranslate.y - 5.0f)
		return FALSE;
	CPad pad(0);
	f32 heading
	    = (f32)atan2(gGame.GetPlayer()->mMoveTranslate.x - mMoveTranslate.x, gGame.GetPlayer()->mMoveTranslate.z - mMoveTranslate.z);
	f32 angle = heading - HSD_JObjGetRotationY(mJObj);
	s32 side  = GetSide(angle);
	switch (side) {
	case 0:
		angle = 0.0f;
		break;
	case 1:
		angle = HALF_PI;
		break;
	case 2:
		angle = PI;
		break;
	case 3:
		angle = -HALF_PI;
		break;
	}
	if (!_A2D) {
		_A2D     = TRUE;
		mWaiting = FALSE;
		if (mState != 3) {
			CCameraActionLeticker& camera = gLetickerCameraAction;
			gGame.mMotionControlCamera.PushAction(&camera);
			camera.mTarget = &mStackTopPosition;
			if (mCameraBounded) {
				camera.mBounded = true;
				camera.mMinX    = mCameraMinX;
				camera.mMaxX    = mCameraMaxX;
				camera.mMinZ    = mCameraMinZ;
				camera.mMaxZ    = mCameraMaxZ;
				for (s32 i = 0; i < 4 && !CanTurn(&camera, camera.mAngle); ++i)
					camera.mTurnTo = camera.mAngle += HALF_PI;
			}
		} else {
			gGame.GetObject(mLinked[0])->mStatus.mFlags &= ~0x10000ULL;
			gGame.GetObject(mLinked[0])->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		if (mState == 2) {
			mMaxLevel         = mLevelBySide[side];
			mMaxLevelProgress = 10.0f * mMaxLevel;
		}
		if (mPtcl) {
			gGame.mParticle.Kill(0xA5);
			mPtcl = FALSE;
		}
	}
	if (mWaiting) {
		if (!(pad.mPad->mButton & HSD_PAD_STICK_DOWN)) {
			if (--_A30 <= 0)
				return FALSE;
			return TRUE;
		}
		mWaiting = FALSE;
	}
	++_A30;
	f32 delta     = amount * mSpeedScale;
	BOOL starting = _A30 < 40;
	switch (mState) {
	case 1:
		if (!(u8)TurnLetickerAndCheckClearance(this, angle)) {
			if (mHeight == 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f) {
				XObj* part = gGame.GetObject(mLinked[1]);
				PlayGameSound(&gGame.mSound, 0x191, 255);
				vf12C(1, 255, 0, 0, 0.5f);
				SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				part->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				PushAnim(part, 0, 0, 100);
				_A30     = 30;
				mWaiting = TRUE;
			}
			return TRUE;
		}
		break;
	case 2:
		if (!(u8)TurnLetickerLinkedPartAndTrackJoint(this, angle))
			return starting;
		break;
	case 3:
		mStackTopPosition = mMoveTranslate;
		gGame.GetObject(mLinked[0])->SetPos(mMoveTranslate, 0);
		gGame.GetObject(mLinked[0])->SetRotateY(mMoveRotate.y, 1.0f);
		if (mMaxLevelProgress <= mLevelProgress + delta) {
			gGame.GetObject(mLinked[0])->mStatus.mFlags |= 0x10000ULL;
			gGame.GetObject(mLinked[0])->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		} else {
			XObj* part = gGame.GetObject(mLinked[0]);
			if (part->mAnimId != 0)
				part->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		break;
	}
	if (gGame.GetPlayer()->mParams.mBattery <= 0.0f)
		return FALSE;
	if (pad.mPad->mButton & HSD_PAD_STICK_DOWN) {
		mLevelProgress -= delta;
		if (mLevelProgress < 0.0f) {
			mLevelProgress = 0.0f;
			return starting;
		}
	} else {
		mLevelProgress += delta;
		if (mMaxLevelProgress <= mLevelProgress) {
			mLevelProgress = mMaxLevelProgress;
			return starting;
		}
	}
	if (mAnimId != 1 && mAnimId != 2)
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	return TRUE;
}

/**
 * @note Address: 0x800E3AF0
 * @note Size: 0x4
 */
void XObjLeticker::OnHitFromPosition(const Vec& pos, f32 f)
{
}

static inline void StartLetickerRide(XObjLeticker* lift, XObj* part, XObjChr* player, s32 anim, s32 partAnim, s32 sound)
{
	lift->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	part->mStatus.mFlags |= 0x10000ULL;
	part->SetAnim(partAnim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	part->SetPos(lift->mMoveTranslate, 0);
	player->_980.mFlags = 0;
	player->_980.mFlags |= 0x40000000;
	player->mPad.mReset = FALSE;
	StopGameSfx(&gGame.mSound, 0xB0);
	PlayGameSound(&gGame.mSound, sound, 255);
	lift->mStep = 10;
}

/**
 * @note Address: 0x800E3AF4
 * @note Size: 0xC9C
 */
s32 XObjLeticker::HandleEventCommand(s32 count, s32* args)
{
	XObjChr* player = gGame.GetPlayer();
	switch (args[0]) {
	case 0:
		mLinkedCount = 0;
		mMaxLevel    = args[1];
		for (s32 i = 0; i < 4; ++i)
			mLevelBySide[i] = mMaxLevel;
		for (s32 i = 2; i < count && (u32)mLinkedCount < 2; ++i) {
			mLinked[mLinkedCount] = args[i];
			++mLinkedCount;
		}
		mStackTopPosition = mMoveTranslate;
		mSpeedScale       = 1.0f;
		if (mLinkedCount > 0) {
			XObj* part = gGame.GetObject(mLinked[0]);
			switch (part->mCoord._00->_00) {
			case 0xEE: {
				mState = 1;
				mFunc  = &XObjLeticker::StateLift;
				if (mMaxLevel > 30)
					mMaxLevel = 30;
				mBox[0].mId              = 2;
				mBox[0].mCenterOffset[0] = 0;
				mBox[0].mCenterOffset[1] = 1620;
				mBox[0].mCenterOffset[2] = 0;
				mBox[0].mYawDegrees      = 0;
				mBox[0].mExtent[0]       = 400;
				mBox[0].mExtent[1]       = 1000;
				mBox[0].mExtent[2]       = 200;
				s32 path[]               = { 0 };
				HSD_JObjSetFlagsAll(FindJointByPath(part->mJObj, path, 1), 0x10);
				break;
			}
			case 0x114:
				mState = 2;
				mFunc  = &XObjLeticker::StateCount;
				if (mMaxLevel > 10)
					mMaxLevel = 10;
				break;
			case 0x266:
				mState = 3;
				mFunc  = NULL;
				for (s32 i = 0; mFloors[i].mId != -1; ++i)
					_250[0] |= 1U << i;
				SelectHighestActiveFloor(this);
				break;
			}
		}
		mMaxLevelProgress    = 10.0f * mMaxLevel;
		mHeight              = 0.0f;
		mReachedTargetHeight = FALSE;
		mCameraBounded       = FALSE;
		mCurrentLevel        = 0;
		break;
	case 1: {
		mLevelProgress = count < 2 || !args[1] ? 0.0f : 10.0f * args[1];
		mHeading       = count < 3 || !args[2] ? 0.0f : DEG2RAD * args[2];
		bool setHeight = count >= 4 && args[3] != 0;
		if (mLinkedCount == 0) {
#if !defined(VERSION_PAL)
			OSReport(sLetickerUninitializedMessage);
#endif
			break;
		}
		switch (mState) {
		case 1: {
			XObj* base = gGame.GetObject(mLinked[0]);
			XObj* top  = gGame.GetObject(mLinked[1]);
			if (setHeight) {
				mHeight       = 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f;
				mCurrentLevel = mHeight * 0.25f;
			}
			base->SetRotateY(mHeading + HSD_JObjGetRotationY(mJObj), 0.1f);
			top->SetRotateY(mHeading + HSD_JObjGetRotationY(mJObj), 0.1f);
			break;
		}
		case 2: {
			XObj* part    = gGame.GetObject(mLinked[0]);
			mCurrentLevel = 0;
			part->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			part->SetRotateY(mHeading + HSD_JObjGetRotationY(mJObj), 0.1f);
			break;
		}
		case 3:
			SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mLevelProgress = 0.0f;
			mStep          = 0;
			break;
		}
		break;
	}
	case 2:
		if (args[1]) {
			mPtcl      = TRUE;
			s32 path[] = { 0 };
			gGame.mParticle.Create(0xA5, 0, FindJointByPath(mJObj, path, 1), TRUE);
		} else {
			mPtcl = FALSE;
		}
		break;
	case 3:
		switch (args[1]) {
		case 0:
			return (s32)((9.0f + mLevelProgress) / 10.0f);
		case 1:
			return 90 * GetSide(mHeading);
		case 2:
			return !(mLevelProgress < mMaxLevelProgress);
		case 3:
			return _A2D != 0;
		case 4:
			return (s32)(100.0f * mSpeedScale);
		}
		break;
	case 4:
		switch (args[1]) {
		case 0:
			mSpeedScale = args[2] / 100.0f;
			break;
		case 1:
			for (s32 i = 0; i < 4; ++i)
				mLevelBySide[i] = args[i + 2];
#if !defined(VERSION_PAL)
			for (u32 i = 0; i < 4; ++i)
				OSReport(sLetickerLevelEntryFormat, i, mLevelBySide[i]);
#endif
			break;
		case 2:
			mCameraBounded = TRUE;
			mCameraMinX    = args[2] / 100.0f;
			mCameraMaxX    = args[3] / 100.0f;
			mCameraMinZ    = args[4] / 100.0f;
			mCameraMaxZ    = args[5] / 100.0f;
			break;
		}
		break;
	case 5:
		if (mState == 3) {
			XObj* part = gGame.GetObject(mLinked[0]);
			switch (args[1]) {
			case 0:
				mLevelProgress = 0.0f;
				SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				part->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				player->_980.mFlags = 0;
				player->_980.mFlags |= 0x20000000;
				player->_980.mFlags |= 7;
				player->mPad.mReset = TRUE;
				player->SetRotationY(mMoveRotate.y, 0);
				mStep = 0;
				break;
			case 1:
				StartLetickerRide(this, part, player, 4, 2, 0xAE);
				break;
			case 2:
				StartLetickerRide(this, part, player, 5, 3, 0xAF);
				break;
			case 3:
				if (count <= 2 || args[2]) {
					player->mLift.SetState(1, &player->mPlug);
					player->mLift._24 = 0;
				} else if (player->mLift.mTarget) {
					player->mLift.mState = 6;
				}
				break;
			}
			mFunc = &XObjLeticker::StateRide;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x800E54E0
 * @note Size: 0x1A8
 */
void XObjLeticker::StateRide()
{
	XObj* player   = gGame.GetPlayer();
	s32 path[2]    = { 0, 9 };
	HSD_JObj* jobj = FindJointByPath(mJObj, path, 2);
	switch (mStep) {
	case 0:
		break;
	case 10:
		Vec pos;
		pos.x = pos.y = pos.z = 0.0f;
		pos.y                 = 0.5f;
		PSMTXMultVec(HSD_JObjGetMtxPtr(jobj), &pos, &pos);
		player->SetPos(pos, 0);
		player->SetRotationY(player->mMoveRotate.y - 0.5235988f, 0);
		break;
	}
}

extern "C" {

// whether the spot under the joint is free, nothing near it in the way
/**
 * @note Address: 0x800E4790
 * @note Size: 0x31C
 */
bool IsLetickerJointClearOfObjects(XObj* obj, HSD_JObj* jobj)
{
	CVec pos;
	if (jobj != NULL) {
		MtxPtr mtx = HSD_JObjGetMtxPtr(jobj);
		pos.x = pos.y = pos.z = 0.0f;
		PSMTXMultVec(mtx, &pos, &pos);
	}
	pos.y -= 6.0f;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* target = gGame.GetObject(i);
		if (!(target->mStatus.mFlags & 1ULL) || !(target->mStatus.mFlags & 0x10000ULL)) {
			continue;
		}
		if ((s32)obj->mId == (s32)target->mId) {
			continue;
		}
		if ((s16)target->GetPool() == 2) {
			continue;
		}
		f32 x    = target->mMoveTranslate.x - pos.x;
		f32 z    = target->mMoveTranslate.z - pos.z;
		f32 dist = sqrtf(x * x + z * z);
		if (dist > 100.0f + (5.0f + target->mFloorXZExtent)) {
			continue;
		}
		CVec hit, normal;
		if (target->vfFC(&pos, &pos, 5.0f, 12.0f, &hit, &normal, 0, 0) & 2) {
			return false;
		}
	}
	return true;
}

/**
 * @note Address: 0x800E4AAC
 * @note Size: 0x3AC
 */
s32 GetClearLetickerApproachSide(XObj* obj, const Vec* pos)
{
	f32 angle = WrapAngle((f32)atan2(pos->x - obj->mMoveTranslate.x, pos->z - obj->mMoveTranslate.z) - obj->mMoveRotate.y);
	if (3.0f + obj->mMoveTranslate.y < gGame.GetPlayer()->mMoveTranslate.y) {
		return 0;
	}
	if (Cos(0.7853982f) < Cos(angle)) {
		s32 path[2] = { 0, 2 };
		if (IsLetickerJointClearOfObjects(obj, FindJointByPath(obj->mJObj, path, 2))) {
			return -1;
		}
	} else if (Cos(angle) < Cos(3.9269907f)) {
		s32 path[2] = { 0, 6 };
		if (IsLetickerJointClearOfObjects(obj, FindJointByPath(obj->mJObj, path, 2))) {
			return -2;
		}
	} else if (0.0f < Sin(angle)) {
		s32 path[2] = { 0, 4 };
		if (IsLetickerJointClearOfObjects(obj, FindJointByPath(obj->mJObj, path, 2))) {
			return -3;
		}
	} else {
		s32 path[2] = { 0, 8 };
		if (IsLetickerJointClearOfObjects(obj, FindJointByPath(obj->mJObj, path, 2))) {
			return -4;
		}
	}
	return 0;
}

} // extern "C"

/**
 * @note Address: 0x800E4E58
 * @note Size: 0x364
 */
void XObjLeticker::DrawObjectEffects()
{
	XObj* part = gGame.GetObject(mLinked[0]);
	switch (mState) {
	case 3:
		if (!(0.0f < mLevelProgress) || (!_A2D && part->mAnimId != 1)) {
			break;
		}
		Mtx mtx;
		u32 colors[50];
		Vec pts[50];
		Vec pos = mMoveTranslate;
		PSMTXTrans(mtx, pos.x, pos.y, pos.z);
		f32 twist = 6.0f * (mLevelProgress / mMaxLevelProgress) * (f32)sin(0.017453292f * (mFuncTime * 5)) + 72.0f;
		u32 i;
		for (i = 0; i < 50; i++) {
			f32 angle = 0.017453292f * mFuncTime + 0.017453292f * (twist * (s32)i);
			f32 y     = 12.0f + 0.2f * (24.0f * (s32)i / 50.0f) * mLevelProgress / mMaxLevelProgress;
			pts[i].x  = 7.1f * (f32)sin(angle);
			pts[i].y  = y;
			pts[i].z  = 7.1f * (f32)cos(angle);
			colors[i] = ((48 - (s32)i) * 32 / 48 + 32) | 0x40F000;
		}
		pts[49].x = pts[44].x;
		pts[49].y = pts[44].y;
		pts[49].z = pts[44].z;
		PSMTXMultVecArray(mtx, pts, pts, 50);
		DrawGsColoredLineStrip(&gGame.mGs, pts, colors, 50);
		if (!IsGameSoundPlaying(&gGame.mSound, 0xB0)) {
			PlayGameSound(&gGame.mSound, 0xB0, 0xFF);
			HSD_AudioSFXSetPitchByID(0xB0, 600.0f * (mSpeedScale - 1.0f));
		} else {
			SetGameSfxVolume(&gGame.mSound, 0xB0, GetSoundVolumeAtPosition(&gGame.mSound, &part->mMoveTranslate, 0), 0);
		}
		break;
	}
}

/**
 * @note Address: 0x800E5688
 * @note Size: 0x410
 */
void XObjLeticker::StateCount()
{
	XObj* part;
	s32 count = (s32)mLevelProgress / 10;
	if (mMaxLevel < count) {
		count = 10;
	}
	part = gGame.GetObject(mLinked[0]);
	if (mCurrentLevel != count) {
		if (GetAnimEndFrame(part->mJObj) <= GetAnimFrame(part->mJObj)) {
			if (mCurrentLevel < count) {
				mCurrentLevel++;
				f32 rate = 1.0f;
				if (!_A2D) {
					rate = 8.0f;
				}
				part->SetAnim(mCurrentLevel, FALSE, NULL, NULL, NULL, 0.0f, rate, 0.0f, 10.0f);
			} else if (_A2D) {
				mCurrentLevel--;
				part->SetAnim(24 - mCurrentLevel, FALSE, NULL, NULL, NULL, 0.0f, 2.0f, 0.0f, 10.0f);
				if (mCurrentLevel == 0) {
					PushAnim(part, 0, 0, 100);
				}
			} else {
				mLevelProgress = (mCurrentLevel - 1) * 10;
			}
			if (_A2D) {
				PlayGameSound(&gGame.mSound, 0xAC, 0xFF);
				HSD_AudioSFXSetPitchByID(0xAC, 600.0f * (mSpeedScale - 1.0f));
			}
		}
		SetFloors(part, sLetickerFloorsByLevel[mCurrentLevel]);
	}
	if (!(mStatus.mFlags & 0x0800000000000000ULL)) {
		vf12C(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
}

/**
 * @note Address: 0x800E5A98
 * @note Size: 0x3E0
 */
void XObjLeticker::StateLift()
{
	XObj* base = gGame.GetObject(mLinked[0]);
	XObj* top  = gGame.GetObject(mLinked[1]);
	if (base == NULL || top == NULL) {
		return;
	}
	f32 height = 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f;
	if (mHeight == height) {
		mReachedTargetHeight = TRUE;
	} else if (mHeight < height) {
		mHeight += 0.5f;
		if (height < mHeight) {
			mHeight = height;
		}
	} else if (height < mHeight && 0.0f < mHeight) {
		mHeight -= 0.5f;
		if (mHeight < 0.0f) {
			mHeight = 0.0f;
		}
	}
	if (mHeight <= 2.5f) {
		mHeight = 2.5f;
	}
	s32 step = mHeight / 4.0f;
	if (mCurrentLevel != step) {
		if (!IsGameSoundPlaying(&gGame.mSound, 0xA8)) {
			PlayGameSound(&gGame.mSound, 0xA8, 0xFF);
		}
		HSD_AudioSFXSetPitchByID(0xA8, 600.0f * (mSpeedScale - 1.0f));
		mCurrentLevel        = step;
		mReachedTargetHeight = FALSE;
	}
	top->SetTranslateY(9.0f + (mHeight + HSD_JObjGetTranslationY(mJObj)), 0);
	mStackTopPosition.x = top->mMoveTranslate.x;
	mStackTopPosition.y = top->mMoveTranslate.y;
	mStackTopPosition.z = top->mMoveTranslate.z;
	if (mStatus.mFlags & 0x0800000000000000ULL) {
		f32 h;
		if (mHeight < 10.0f) {
			h = 19.0f;
		} else {
			h = 5.0f + (9.0f + mHeight) - 2.0f;
		}
		sLetickerLiftFloors[0]->mExtent[1]       = (s32)h * 100;
		sLetickerLiftFloors[0]->mCenterOffset[1] = (s32)(h / sqrtf(3.0f)) * 100;
		SetFloors(this, sLetickerLiftFloors[0]);
	} else {
		s32 k                    = (mHeight - 12.5f) / 4.0f;
		mBox[0].mCenterOffset[1] = k * 0xA9 + 0x654;
		mBox[0].mExtent[1]       = k * 400 + 1000;
		base->mFloors            = mBox;
		base->_250[1]            = 0;
		base->_250[0]            = 0;
		UpdateObjectFloorHeights(base);
		SetFloors(this, sLetickerLiftFloors[1]);
		vf12C(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	}
}

static inline void PlayLetickerSound(XObjLeticker& obj)
{
	s32 anim  = obj.mAnimId;
	f32 frame = GetAnimFrame(obj.mJObj);
	obj.mSfx.Update(anim, frame, GetSoundVolumeAtPosition(&gGame.mSound, &obj.mMoveTranslate, 0));
}

/**
 * @note Address: 0x800E5E78
 * @note Size: 0x598
 */
void XObjLeticker::UpdateObject(u32 frames)
{
	if (mAnimId == 1) {
		if (GetAnimEndFrame(mJObj) <= frames + GetAnimFrame(mJObj)) {
			SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	} else if (mAnimId == 3) {
		if (GetAnimEndFrame(mJObj) <= frames + GetAnimFrame(mJObj)) {
			SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
	UpdateObject__4XObjFUl(this, frames);
	mFuncTime += frames;
	if (mFunc) {
		(this->*mFunc)();
	}
	s32 path[3]    = { 0, 0, 0 };
	HSD_JObj* jobj = FindJointByPath(mJObj, path, 2);
	XObj* player   = gGame.GetPlayer();
	f32 angle      = atan2(player->mMoveTranslate.x - mMoveTranslate.x, player->mMoveTranslate.z - mMoveTranslate.z);
	HSD_JObjSetRotationY(jobj, angle - HSD_JObjGetRotationY(mJObj));
	PlayLetickerSound(*this);
}

/**
 * @note Address: 0x800E6410
 * @note Size: 0x98
 */
void XObjLeticker::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mLinkedCount   = 0;
	mFunc          = NULL;
	mCurrentLevel  = 0;
	mFuncTime      = 0;
	mLevelProgress = 0.0f;
	mHeight        = 0.0f;
	mPtcl          = TRUE;
	_A2D           = FALSE;
	_A30           = 0;
	mState         = 0;
	mHeading       = 0.0f;
	mBox[0].mId    = -1;
	mBox[1].mId    = -1;
	mSfx.mEvents   = sLetickerAnimSfxEvents;
}

/**
 * @note Address: 0x800E64A8
 * @note Size: 0x2E8
 */
void CCameraActionLeticker::Update(u32 step)
{
	CPad pad(0);
	u64 buttons     = gGame.mStatus.mFlags & 0x80000000;
	u32 leftButton  = buttons ? HSD_PAD_DPADLEFT : HSD_PAD_CSTICK_LEFT;
	u32 rightButton = buttons ? HSD_PAD_DPADRIGHT : HSD_PAD_CSTICK_RIGHT;

	if (mTurnFrames == 0) {
		if (pad.mPad->mTrigger & leftButton) {
			if (CanTurn(this, mTurnTo - 1.5707964f)) {
				mTurnTo -= 1.5707964f;
				mTurnSpeed  = sLetickerLeftTurnSpeed;
				mTurnFrames = 18;
			}
		} else if (pad.mPad->mTrigger & rightButton) {
			if (CanTurn(this, 1.5707964f + mTurnTo)) {
				mTurnTo += 1.5707964f;
				mTurnSpeed  = sLetickerRightTurnSpeed;
				mTurnFrames = 18;
			}
		}
	} else {
		mTurnFrames -= step;
		if (mTurnFrames <= 0) {
			mAngle      = mTurnTo;
			mTurnFrames = 0;
		} else {
			mAngle += mTurnSpeed * step;
		}
	}
	CCameraActionLadr::Update(step);
}

/**
 * @note Address: 0x800E6790
 * @note Size: 0x68
 */
void CCameraActionLeticker::vf14()
{
#if !defined(VERSION_PAL)
	OSReport(sLetickerCameraResetMessage);
#endif
	CCameraActionLadr::vf14();
	mTurnTo     = 0.0f;
	mTurnFrames = 0;
	mBounded    = false;
	gGame.mStatus.mFlags |= 0x80;
}

/**
 * @note Address: 0x800E67F8
 * @note Size: 0x20
 */
Vec CCameraActionLeticker::GetCameraTarget()
{
	return *mTarget;
}

CCameraActionLeticker gLetickerCameraAction;
