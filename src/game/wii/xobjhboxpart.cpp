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
	SpawnedEffect* spawned = (SpawnedEffect*)fn_800A11A4(&gGame.mEvtPtcl, id, 1);
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
void XObjHBox::ResetObjectState()
{
	XObj::ResetObjectState();
	mLoadState = 0;
	mRing      = 0.0f;
	XObj* part = &mPart.mObj;
	part->ResetObjectState();
	mPart.mHit   = 0.0f;
	mPart.mRange = 5.0f;
	mPart.mSfx   = 0;
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
		mEventScript = NULL;
		++mLoadState;
		// fall through, the part starts reading right away
	case 1:
		if (mPart.mObj.LoadFromPlacement(CoordInfo(GetSpawnData(0x26)))) {
			mPart.mObj.mStatus.mFlags &= ~0x10100ULL;
			mPart.mObj.SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mPart.mObj.SetPos(mMoveTranslate);
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
		f32 tz = GetPos()->z;
		f32 ty = GetPos()->y;
		f32 tx = GetPos()->x;
		PSMTXTrans(translation, tx, ty, tz);
		f32 sz = GetScale()->z;
		f32 sy = GetScale()->y;
		f32 sx = GetScale()->x;
		PSMTXScale(scale, sx, sy, sz);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXRotRad(pitch, 'x', mMoveRotate.x);
		PSMTXRotRad(roll, 'z', mMoveRotate.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, roll, result);
		PSMTXConcat(result, pitch, result);
		PSMTXConcat(result, scale, result);
		SetBaseMtx(result);
	}
	if (mStatus.mFlags & 0x2000000000000000ULL) { // the opening hit
		mStatus.mFlags &= ~0x2000000000000000ULL;
		ClearBaseMtx();
		mPart.mSfx = PlayWiiSoundEffect(&gGame.mSound, 0x18f, 1.0f, 0.0f);
		PlayBoxAnim(this, 1);
		mRing = 1.0f;
		Mtx translation, yaw, scale, result;
		PSMTXTrans(translation, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		PSMTXRotRad(yaw, 'y', mMoveRotate.y);
		PSMTXScale(scale, mMoveScale.x, mMoveScale.y, mMoveScale.z);
		PSMTXConcat(translation, yaw, result);
		PSMTXConcat(result, scale, result);
		SetBaseMtx(result);
		mPart.mObj.mStatus.mFlags |= 0x10000ULL;
		PlayBoxAnim(&mPart.mObj, 2);
		mPart.mObj.SetPos(*GetPos());
		mPart.mObj.SetRotationY(DEG2RAD * (rand() % 360));
		SpawnBoxEffect(this, 0x21);
		mFloors        = GetObjectCollisionList(mPart.mObj.mCoord.mPlacement->mObjectId);
		mFloorMasks[0] = mFloorMasks[1] = 0;
		s32 index                       = GetStagePlacementIndex(mCoord.mPlacement);
		if (index > 0)
			mEventScript = GetStageScriptByIndex(index);
		mPart.mObj.UpdateObject(frames);
	} else {
		if (GetAnimId() == 1) {
			if (IsAnimDone())
				PlayBoxAnim(this, 3);
		} else if (GetAnimId() == 3) {
			IsAnimDone();
		}
		if (mRing > 0.0f) { // the hit ring fades
			mRing -= 0.04f;
			if (mRing <= 0.0f)
				mRing = 0.0f;
		}
		if (!(mPart.mObj.mStatus.mFlags & 0x4000000000000000ULL)) {
			mPart.mObj.UpdateObject(frames);
			s32 id = mPart.mObj.GetAnimId();
			if (id > 1) {
				nw4r::g3d::ResNode joint = mPart.mObj.FindResNode("obj2");
				Vec pos                  = mPart.mObj.GetJointPos(joint);
				Vec rot                  = mPart.mObj.mMoveRotate;
				SetPos(pos);
				SetRotationY(rot.y);
			}
			switch (id) {
			case 2:
				if (mPart.mObj.IsAnimDone())
					PlayBoxAnim(&mPart.mObj, 3);
				break;
			case 3:
				if (mPart.mObj.IsAnimDone())
					PlayBoxAnim(&mPart.mObj, 4);
				break;
			case 4:
				if (mPart.mObj.IsAnimDone()) {
					PlayBoxAnim(&mPart.mObj, 5);
					SpawnBoxEffect(this, 0x2b);
				}
				break;
			case 5:
				if (mPart.mObj.IsAnimDone()) {
					PlayBoxAnim(&mPart.mObj, 5);
					mPart.mHit = 1.0f;
					mStatus.mFlags |= 0x4000000000000000ULL;
				}
				break;
			}
			if (mStatus.mFlags & 0x4000000000000000ULL) { // shrinking out
				mPart.mObj.SetAnimRate(0.0f);
				mPart.mHit -= 0.04f * frames;
				if (mPart.mHit <= 0.0f) {
					mPart.mHit = 0.0f;
					Release();
					return;
				}
				mPart.mObj.SetScale(1.12f * mPart.mHit);
				SetScale(1.12f * mPart.mHit);
			}
		}
	}
}

/**
 * @note Address: 0x80116FAC
 * @note Size: 0x5C
 */
void XObjHBox::DrawWithView(Mtx view)
{
	XObj::DrawWithView(view);
	if (mPart.mObj.mStatus.mFlags & 0x10000)
		mPart.mObj.DrawWithView(view);
}

/**
 * @note Address: 0x80117008
 * @note Size: 0x14
 */
void XObjHBox::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (mPart.mObj.mStatus.mFlags & 0x10000)
		XObj::DrawGroundShadowAt(origin, opacity);
}
