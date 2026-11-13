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
	if (plug->_D38 == 0) {
		SetPlugCordEndpointSource(&plug->mCord, 1, &plug->mCordJointPos);
		SetPlugCordEndpointSource(&plug->mCord, 2, &plug->mCordJointTip);
		plug->_D38 = 1;
	}
}

static inline void UpdateCordJoint(XObjPlug* plug, CJObj& joint)
{
	Mtx matrix;
	PSMTXCopy(joint.GetMtx(), matrix);
	plug->mCordJointPos = CVec(matrix[0][3], matrix[1][3], matrix[2][3]);
	CVec tip(0, 0, 5);
	PSMTXMultVec(matrix, &tip, &tip);
	plug->mCordJointTip = tip;
}

void XObjPlug::UpdateObject(u32 frames)
{
	UpdateObject__4XObjFUl(this, frames);
	if (mStatus.mFlags & 1) {
		GetPlayerPlugCordAnchorTransform(this, &mCordAnchor, NULL, NULL);
		if (mStatus.mFlags & 0x0400000000000000ULL) {
			AttachPlugCord(this);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(mJObj, path, 2));
			UpdateCordJoint(this, joint);
		} else if (_D3C != 0) {
			AttachPlugCord(this);
			s32 path[] = { 0, 0 };
			CJObj joint(FindJointByPath(mJObj, path, 2));
			UpdateCordJoint(this, joint);
		} else if (_D38 == 1) {
			SetPlugCordEndpointSource(&mCord, 1, NULL);
			SetPlugCordEndpointSource(&mCord, 2, NULL);
			_D38 = 0;
		}
		const Vec& tint = mColorStep[1].mValue;
		mCord.mColor = mCord._60 = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8) | 0xff;
		UpdatePlugCordSimulation(&mCord, frames);
	} else {
		ResetObjectState();
	}
	mWire.UpdateObject(frames);
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 0x0400000000000000ULL) && _D3C == 0) {
		Vec pos = GetPlugCordPointPosition(&mCord, 18);
		SetPos(pos, 0);
		CVec tip, base, rot;
		tip    = GetPlugCordPointPosition(&mCord, 19);
		base   = GetPlugCordPointPosition(&mCord, 18);
		rot.y  = atan2(tip.x - base.x, tip.z - base.z);
		f32 dx = tip.x - base.x, dy = tip.y - base.y, dz = tip.z - base.z;
		f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
		rot.x     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
		rot.z     = 0;
		SetRot(rot, 0);
		Mtx matrix;
		PSMTXCopy(HSD_JObjGetMtxPtr(mJObj), matrix);
		Vec offset = { 0, -0.2f, 0 };
		PSMTXMultVec(matrix, &offset, &offset);
		SetPos(offset, 0);
		if (mWire.mSparkMode == 0) {
			if (mWire.mSparkTimer < 0) {
				CVec pos(mMoveTranslate);
				CVec scale(0.5f, 0.5f, 0.5f);
				Vec rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x1001a, 1, pos, scale, rotation, FALSE);
				mWire.mSparkTimer += HSD_Randi(10) + 16;
			}
			XSound* sound = &gGame.mSound;
			if (IsGameSoundPlaying(sound, 0xfb) != 1 && IsGameSoundPlaying(sound, 0xfc) != 1) {
				if (mWire.mSparkTimer % 2)
					PlayGameSound(sound, 0xfb, 255);
				else
					PlayGameSound(sound, 0xfc, 255);
			}
			mWire.mSparkTimer -= frames;
		}
	}
}

static inline void ClearWireAnim(XObjWireEff* object)
{
	object->mStatus.mFlags &= ~0x10000ULL;
	object->mAnim   = -1;
	object->mEffect = -1;
}

static inline void KillWireEffect(XObjWireEff* object)
{
	if (object->mGenerator != NULL) {
		gGame.mParticle.Kill(object->mGenerator);
		object->mGenerator = NULL;
	}
}

extern "C" {

/**
 * @note Address: 0x800D7D3C
 * @note Size: 0x7C
 */
void ClearWireEffect(XObjWireEff* object)
{
	ClearWireAnim(object);
	KillWireEffect(object);
}

/**
 * @note Address: 0x800D7DB8
 * @note Size: 0x16C
 */
void ConfigurePlugWireEffect(XObjWireEff* object, s32 anim, s32 effect, f32 rate, u32 color)
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

/**
 * @note Address: 0x800D80C8
 * @note Size: 0xC
 */
extern "C" void DisablePlugSparkEffect(XObjPlug* object)
{
	object->mWire.mSparkMode = -1;
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

// XObjPlug::DrawGroundShadow, the cord's shadow
/**
 * @note Address: 0x800D8184
 * @note Size: 0x1A8
 */
void XObjPlug::DrawGroundShadow()
{
	mCord.mColor = 0x14141440;
	mCord._60    = 0x14141440;
	mCord.mWidth = 0.15f;
	if (mJObj != NULL) {
		CGs* gs = &gGame.mGs;
		FlushCaptureGraphics(gs);
		DrawPlugCordSegments(&mCord, NULL, 1.0f, 1.0f, 1.0f);
		InvalidateCaptureGraphics(gs);
	}
	mCord.mColor = 0x141414FF;
	mCord._60    = 0x141414FF;
	mCord.mWidth = 0.2f;
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

/**
 * @note Address: 0x800D9068
 * @note Size: 0x34
 */
CVec GetPlugCordPointPosition(const XObjPlugCord* cord, s32 index)
{
	return cord->mPoints[index].mPos;
}

/**
 * @note Address: 0x800D909C
 * @note Size: 0x32C
 */
void GetPlayerPlugCordAnchorTransform(void*, Vec* position, Vec* rotation, Vec* scale)
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

} // extern "C"
