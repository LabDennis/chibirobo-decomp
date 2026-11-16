#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/wii/csound.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/stagedata.h"
#include <stdlib.h>

extern "C" {
u8* GetStageScriptByIndex(s32 index);
void* fn_800A11A4(void* particle, u32 code, s32 status);
void fn_802BEABC(void* spawned);
}

// what the Wii's particle spawn hands back (fn_800A11A4); the caller fills its TRS
// before fn_802BEABC registers it
struct SpawnedEffect {
	u8 _00[0xC];
	Vec mTranslate; // _0C
	Vec mRotate;    // _18
	Vec mScale;     // _24
};

static inline void PlayBoxAnim(XObj* object, s32 id)
{
	object->SetAnim(id, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

static inline void SpawnBoxEffect(XObjHBox* box, u32 id)
{
	Vec rotation           = { 0.0f, 0.0f, 0.0f };
	SpawnedEffect* spawned = (SpawnedEffect*)fn_800A11A4((u8*)&gGame + 0xBCA8, id, 1);
	if (spawned != NULL) {
		spawned->mTranslate = box->mMoveTranslate;
		spawned->mRotate    = rotation;
		spawned->mScale     = box->mMoveScale;
		fn_802BEABC(spawned);
	}
}

/**
 * @note Address: 0x80116528
 * @note Size: 0x6C
 */
s32 XObjHBox::ResetObjectState()
{
	XObj::ResetObjectState();
	mLoadState   = 0;
	mRing        = 0.0f;
	XObj* part   = &mPart.mObj;
	s32 result   = part->ResetObjectState();
	mPart.mHit   = 0.0f;
	mPart.mRange = 5.0f;
	mPart.mSfx   = 0;
	return result;
}

/**
 * @note Address: 0x80116594
 * @note Size: 0x130
 */
BOOL XObjHBox::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (XObj::LoadFromPlacement(info) == FALSE)
			break;
		mEventScript = 0;
		++mLoadState;
		// fall through, the part starts reading right away
	case 1:
		if (mPart.mObj.LoadFromPlacement(CoordInfo(GetSpawnData(0x26)))) {
			mPart.mObj.mStatus.mFlags &= ~0x10100ULL;
			mPart.mObj.SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mPart.mObj.vf090(mMoveTranslate);
			++mLoadState;
		}
		break;
	}
	if (mLoadState > 1) {
		mLoadState = 0;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801166C4
 * @note Size: 0x4C
 */
void XObjHBox::Release()
{
	mPart.mObj.Release();
	XObj::Release();
	mLoadState = 0;
}

// NONMATCHING: UpdateObject emits an identical instruction stream, but the retail frame is
// 0x10 larger (two 4-byte locals/temps we cannot reproduce from source) and the float literals
// live in a pooled .sdata2 unit owned by neighboring auto objects, so the unit stays NonMatching.
/**
 * @note Address: 0x80116710
 * @note Size: 0x89C
 */
void XObjHBox::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 0x1000000000000000ULL) { // rebuild the joint matrix from the trs
		Mtx translation, scale, yaw, pitch, roll, result;
		f32 tz = vf244()->z;
		f32 ty = vf244()->y;
		f32 tx = vf244()->x;
		PSMTXTrans(translation, tx, ty, tz);
		f32 sz = vf258()->z;
		f32 sy = vf258()->y;
		f32 sx = vf258()->x;
		PSMTXScale(scale, sx, sy, sz);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXRotRad(pitch, 'x', mMoveRotate.x);
		PSMTXRotRad(roll, 'z', mMoveRotate.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, roll, result);
		PSMTXConcat(result, pitch, result);
		PSMTXConcat(result, scale, result);
		vf1B4(result);
	}
	if (mStatus.mFlags & 0x2000000000000000ULL) { // the opening hit
		mStatus.mFlags &= ~0x2000000000000000ULL;
		vf1B8();
		mPart.mSfx = PlayWiiSoundEffect(&gGame.mSound, 0x18f, 1.0f, 0.0f);
		PlayBoxAnim(this, 1);
		mRing = 1.0f;
		Mtx translation, yaw, scale, result;
		PSMTXTrans(translation, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXScale(scale, mMoveScale.x, mMoveScale.y, mMoveScale.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, scale, result);
		vf1B4(result);
		mPart.mObj.mStatus.mFlags |= 0x10000ULL;
		PlayBoxAnim(&mPart.mObj, 2);
		mPart.mObj.vf090(*vf244());
		mPart.mObj.vf070(DEG2RAD * (rand() % 360));
		SpawnBoxEffect(this, 0x21);
		mFloors        = GetObjectCollisionList(mPart.mObj.mCoord.mPlacement->mObjectId);
		mFloorMasks[0] = mFloorMasks[1] = 0;
		s32 index                       = GetStagePlacementIndex(mCoord.mPlacement);
		if (index > 0)
			mEventScript = (u32)GetStageScriptByIndex(index);
		mPart.mObj.UpdateObject(frames);
	} else {
		if (vf11C() == 1) {
			if (vf134())
				PlayBoxAnim(this, 3);
		} else if (vf11C() == 3) {
			vf134();
		}
		if (mRing > 0.0f) { // the hit ring fades
			mRing -= 0.04f;
			if (mRing <= 0.0f)
				mRing = 0.0f;
		}
		if (!(mPart.mObj.mStatus.mFlags & 0x4000000000000000ULL)) {
			mPart.mObj.UpdateObject(frames);
			s32 id = mPart.mObj.vf11C();
			if (id > 1) {
				void* joint = mPart.mObj.vf22C("obj2");
				Vec pos     = mPart.mObj.vf230(&joint);
				Vec rot     = mPart.mObj.mMoveRotate;
				vf090(pos);
				vf070(rot.y);
			}
			switch (id) {
			case 2:
				if (mPart.mObj.vf134())
					PlayBoxAnim(&mPart.mObj, 3);
				break;
			case 3:
				if (mPart.mObj.vf134())
					PlayBoxAnim(&mPart.mObj, 4);
				break;
			case 4:
				if (mPart.mObj.vf134()) {
					PlayBoxAnim(&mPart.mObj, 5);
					SpawnBoxEffect(this, 0x2b);
				}
				break;
			case 5:
				if (mPart.mObj.vf134()) {
					PlayBoxAnim(&mPart.mObj, 5);
					mPart.mHit = 1.0f;
					mStatus.mFlags |= 0x4000000000000000ULL;
				}
				break;
			}
			if (mStatus.mFlags & 0x4000000000000000ULL) { // shrinking out
				mPart.mObj.vf114(0.0f);
				mPart.mHit -= 0.04f * frames;
				if (mPart.mHit <= 0.0f) {
					mPart.mHit = 0.0f;
					Release();
					return;
				}
				mPart.mObj.vf09C(1.12f * mPart.mHit);
				vf09C(1.12f * mPart.mHit);
			}
		}
	}
}

/**
 * @note Address: 0x80116FAC
 * @note Size: 0x5C
 */
s32 XObjHBox::vf178(const CoordInfo& info)
{
	s32 result = XObj::vf178(info);
	if (mPart.mObj.mStatus.mFlags & 0x10000)
		result = mPart.mObj.vf178(info);
	return result;
}

/**
 * @note Address: 0x80117008
 * @note Size: 0x14
 */
void XObjHBox::vf184(u32 frames)
{
	if (mPart.mObj.mStatus.mFlags & 0x10000)
		XObj::vf184(frames);
}
