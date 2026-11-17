#include "game/xchr.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/csound.h"
#include "game/cgs.h"
#include "game/texturecopy.h"
#include "game/ccamera.h"
#include "game/cparticle.h"
#include "game/stagedata.h"
#include "game/cjobjinlineattach.h"
extern "C" {
#include <sysdolphin/mtx.h>
#include <sysdolphin/random.h>
}

extern "C" {

static inline void AttachPlugCord(XObjPlug* plug)
{
	if (plug->mCordEndsPinned == 0) {
		SetPlugCordEndpointSource(&plug->mCord, 1, &plug->mCordJointPos);
		SetPlugCordEndpointSource(&plug->mCord, 2, &plug->mCordJointTip);
		plug->mCordEndsPinned = 1;
	}
}

} // extern "C"

static inline void ClearWireAnim(XObjWireEff* object)
{
	object->mStatus.mFlags &= ~0x10000ULL;
	object->mAnim   = -1; // -1 means none
	object->mEffect = -1;
}

static inline void KillWireEffect(XObjWireEff* object)
{
	if (object->mGenerator != NULL) {
		gGame.mParticle.Kill(object->mGenerator);
		object->mGenerator = NULL;
	}
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

/**
 * @note Address: 0x800D9A7C
 * @note Size: 0x8
 */
extern "C" void SetPlugCordSpeed(XObjPlug* object, f32 speed)
{
	object->mCord.mSpeed = speed;
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

// XObjPlug::ResetObjectState, the cord goes back to the player and the plug's joint is straightened
/**
 * @note Address: 0x800D9518
 * @note Size: 0x53C
 */
void XObjPlug::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mConnection = 0;
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
	mCordEndsPinned = 0;
	mWire.ResetObjectState();
	mWire.mSparkMode = -1;
}

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

// XObjPlug::Release
/**
 * @note Address: 0x800D93C8
 * @note Size: 0x4C
 */
void XObjPlug::Release()
{
	mWire.Release();
	XObj::Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x800D909C
 * @note Size: 0x32C
 */
extern "C" void GetPlayerPlugCordAnchorTransform(void*, Vec* position, Vec* rotation, Vec* scale)
{
	Mtx matrix;
	s32 path[] = { 0, 1, 0, 1, 1 };
	CJObj joint(FindJointByPath(gGame.GetPlayer()->mJObj, path, 5));
	if (joint.mJObj != NULL) {
		PSMTXCopy(joint.GetMtx(), matrix);
	}

	CVec unused; // never read, the dol still constructs it
	static CVec offset(0.0f, 0.0f, -0.4f);
	if (position != NULL) {
		PSMTXMultVec(matrix, &offset, position);
	}
	if (rotation != NULL) {
		HSD_MtxGetRotation(matrix, rotation);
	}
	if (scale != NULL) {
		HSD_MtxGetScale(matrix, scale);
	}
}

/**
 * @note Address: 0x800D9068
 * @note Size: 0x34
 */
extern "C" CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index)
{
	return cord->mPoints[index].mPos;
}

// XObjPlug::UpdateObject
void XObjPlug::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	if (mStatus.mFlags & 1) {
		GetPlayerPlugCordAnchorTransform(this, &mCordAnchor, NULL, NULL);
		if (mStatus.mFlags & 0x0400000000000000ULL) {
			Mtx matrix;
			AttachPlugCord(this);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(mJObj, path, 2));
			PSMTXCopy(joint.GetMtx(), matrix);
			mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
			CVec tip(0, 0, 5); // the cord's far end sits 5 along the joint's z
			PSMTXMultVec(matrix, &tip, &tip);
			mCordJointTip = tip;
		} else if (mConnection != 0) {
			Mtx matrix;
			AttachPlugCord(this);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(mJObj, path, 2));
			PSMTXCopy(joint.GetMtx(), matrix);
			mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
			CVec tip(0, 0, 5);
			PSMTXMultVec(matrix, &tip, &tip);
			mCordJointTip = tip;
		} else if (mCordEndsPinned == 1) { // neither carried nor connected: unpin the ends
			SetPlugCordEndpointSource(&mCord, 1, NULL);
			SetPlugCordEndpointSource(&mCord, 2, NULL);
			mCordEndsPinned = 0;
		}
		const Vec& tint = mColorStep[1].mValue;
		u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8) | 0xff; // tint rgb at full alpha
		mCord.mColor    = color;
		mCord._60       = color;
		UpdatePlugCordSimulation(&mCord, frames);
	} else {
		ResetObjectState();
	}
	mWire.UpdateObject(frames);
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 0x0400000000000000ULL) && mConnection == 0) {
		Vec pos = GetPlugCordPointPosition(&mCord, 18); // the plug follows the cord's free end
		SetPos(pos, 0);
		CVec tip, base, dir, rot, size; // dir and size are never read, the dol still constructs them
		tip    = GetPlugCordPointPosition(&mCord, 19);
		base   = GetPlugCordPointPosition(&mCord, 18);
		rot.y  = atan2(tip.x - base.x, tip.z - base.z); // the heading of the cord's last span
		f32 dx = tip.x - base.x, dy = tip.y - base.y, dz = tip.z - base.z;
		f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
		rot.x     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
		rot.z     = 0;
		SetRot(rot, 0);
		Mtx matrix;
		PSMTXCopy(HSD_JObjGetMtxPtr(mJObj), matrix);
		static Vec hang = { 0, -0.2f, 0 }; // sits 0.2 under the joint
		Vec offset      = hang;
		PSMTXMultVec(matrix, &offset, &offset);
		SetPos(offset, 0);
		if (mWire.mSparkMode == 0) {
			if (mWire.mSparkTimer < 0) {
				Vec pos, scale;
				pos          = mMoveTranslate;
				scale        = CVec(0.5f, 0.5f, 0.5f);
				Vec rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x1001a, 1, pos, scale, rotation, FALSE);
				mWire.mSparkTimer += HSD_Randi(10) + 16; // the next spark in 16 to 25 frames
			}
			XSound* sound = &gGame.mSound;
			if (IsGameSoundPlaying(sound, 0xfb) != 1 && IsGameSoundPlaying(sound, 0xfc) != 1) {
				if (mWire.mSparkTimer % 2) // the timer's parity picks one of the two spark sounds
					PlayGameSound(sound, 0xfb, 255);
				else
					PlayGameSound(sound, 0xfc, 255);
			}
			mWire.mSparkTimer -= frames;
		}
	}
}

// XObjPlug::DrawWithView
/**
 * @note Address: 0x800D8358
 * @note Size: 0x74
 */
void XObjPlug::DrawWithView(Mtx view)
{
	XObj::DrawWithView(view);
	mWire.DrawWithView(view);
	if (mJObj != NULL) {
		DrawPlugCordSegments(&mCord, view, 1.0f, 1.0f, 1.0f);
	}
}

// XObjPlug::DrawReflection
/**
 * @note Address: 0x800D832C
 * @note Size: 0x2C
 */
void XObjPlug::DrawReflection(Mtx view)
{
	DrawWithView(view);
}

// XObjPlug::DrawGroundShadow, the cord's shadow
/**
 * @note Address: 0x800D8184
 * @note Size: 0x1A8
 */
void XObjPlug::DrawGroundShadow()
{
	mCord.mColor = 0x14141440; // the shadow draws as a faint dark line
	mCord._60    = 0x14141440;
	mCord.mWidth = 0.15f;
	if (mJObj != NULL) {
		CGs* gs = &gGame.mGs;
		FlushCaptureGraphics(gs);
		DrawPlugCordSegments(&mCord, NULL, 1.0f, 1.0f, 1.0f);
		InvalidateCaptureGraphics(gs);
	}
	mCord.mColor = 0x141414FF; // restore the cord's colour for the next draw
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
}

// XObjPlug::DrawObjectEffects
/**
 * @note Address: 0x800D8138
 * @note Size: 0x4C
 */
void XObjPlug::DrawObjectEffects()
{
	if (mJObj != NULL) {
		DrawPlugCordNodes(&mCord, gGame.mGs.mCamera->mViewMtx, 1.0f, 1.0f, 1.0f);
	}
}

// XObjPlug::SetDestinationAlpha
/**
 * @note Address: 0x800D80E4
 * @note Size: 0x54
 */
void XObjPlug::SetDestinationAlpha(BOOL enabled, u8 value)
{
	XObj::SetDestinationAlpha(enabled, value);
	SetPlugCordAlphaMode(&mCord, enabled, value);
}

/**
 * @note Address: 0x800D80D4
 * @note Size: 0x10
 */
extern "C" void SetPlugSparkMode(XObjPlug* object, s32 value)
{
	object->mWire.mSparkMode  = value;
	object->mWire.mSparkTimer = -1;
}

/**
 * @note Address: 0x800D80C8
 * @note Size: 0xC
 */
extern "C" void DisablePlugSparkEffect(XObjPlug* object)
{
	object->mWire.mSparkMode = -1;
}

// XObjWireEff::ResetObjectState
/**
 * @note Address: 0x800D8068
 * @note Size: 0x60
 */
void XObjWireEff::ResetObjectState()
{
	XObj::ResetObjectState();
	ClearWireAnim(this);
	mGenerator = NULL;
}

// XObjWireEff::Release
/**
 * @note Address: 0x800D7FE4
 * @note Size: 0x84
 */
void XObjWireEff::Release()
{
	ClearWireAnim(this);
	KillWireEffect(this);
	XObj::Release();
}

// XObjWireEff::UpdateObject
/**
 * @note Address: 0x800D7F84
 * @note Size: 0x60
 */
void XObjWireEff::UpdateObject(u32 frames)
{
	if ((mStatus.mFlags & 1) && (mAnim >= 0 || mEffect >= 0)) {
		XObj::UpdateObject(frames);
	}
}

// XObjWireEff::DrawWithView
/**
 * @note Address: 0x800D7F24
 * @note Size: 0x60
 */
void XObjWireEff::DrawWithView(Mtx view)
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000)) {
		XObj::DrawWithView(view);
	}
}

/**
 * @note Address: 0x800D7DB8
 * @note Size: 0x16C
 */
extern "C" void ConfigurePlugWireEffect(XObjWireEff* object, s32 anim, s32 effect, f32 rate, u32 color)
{
	object->mAnim   = anim;
	object->mEffect = effect;
	object->mRate   = rate;
	if (object->mAnim >= 0) {
		object->mStatus.mFlags |= 0x10000ULL;
		object->SetAnim(object->mAnim, FALSE, NULL, NULL, NULL, 0.0f, object->mRate, 0.0f, 10.0f);
	} else {
		object->mStatus.mFlags &= ~0x10000ULL;
	}
	if (object->mEffect >= 0) {
		KillWireEffect(object);
		object->mGenerator = gGame.mParticle.Create(0x10052, 0, object->mJObj, FALSE);
	} else {
		KillWireEffect(object);
	}
	object->SetColor(0, color >> 24, color >> 16, color >> 8, 0.0f);
}

/**
 * @note Address: 0x800D7D3C
 * @note Size: 0x7C
 */
extern "C" void ClearWireEffect(XObjWireEff* object)
{
	ClearWireAnim(object);
	KillWireEffect(object);
}
