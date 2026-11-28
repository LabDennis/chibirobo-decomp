#include "game/xchr.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/csound.h"
#include "game/cgs.h"
#include "game/texturecopy.h"
#include "game/ccamera.h"
#include "game/cparticle.h"
#include "game/stagedata.h"
#include "game/cjobjset.h"
#include "game/cjobjinlineattach.h"
#include "game/xobjattachment.h"

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

static void UpdateCordJoint(XObjPlug* plug)
{
	s32 path[]      = { 0, 0 };
	HSD_JObj* joint = FindJointByPath(plug->mJObj, path, ARRAY_SIZE(path));
	if (joint == NULL) {
		plug->mCordJointPos = plug->mMoveTranslate;
		plug->mCordJointTip = plug->mCordJointPos;
		plug->mCordJointTip.z += 5.0f;
		return;
	}

	MtxPtr matrix         = HSD_JObjGetMtxPtr(joint);
	Vec tip               = { 0.0f, 0.0f, 5.0f };
	plug->mCordJointPos.x = matrix[0][3];
	plug->mCordJointPos.y = matrix[1][3];
	plug->mCordJointPos.z = matrix[2][3];
	PSMTXMultVec(matrix, &tip, &plug->mCordJointTip);
}

void XObjPlug::UpdateObject(u32 frames)
{
	XObj::UpdateObject(frames);
	if (mStatus.mFlags & 1) {
		GetPlayerPlugCordAnchorTransform(this, &mCordAnchor, NULL, NULL);
		if ((mStatus.mFlags & 0x0400000000000000ULL) || mConnection != 0) {
			AttachPlugCord(this);
			UpdateCordJoint(this);
		} else if (mCordEndsPinned == 1) { // neither carried nor connected: unpin the ends
			SetPlugCordEndpointSource(&mCord, 1, NULL);
			SetPlugCordEndpointSource(&mCord, 2, NULL);
			mCordEndsPinned = 0;
		}
		const Vec& tint = mColorStep[1].mValue;
		mCord.mColor = mCord._60 = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8) | 0xff; // tint rgb at full alpha
		UpdatePlugCordSimulation(&mCord, frames);
	} else {
		ResetObjectState();
	}
	mWire.UpdateObject(frames);
	if ((mStatus.mFlags & 1) && !(mStatus.mFlags & 0x0400000000000000ULL) && mConnection == 0) {
		Vec pos = GetPlugCordPointPosition(&mCord, 18); // the plug follows the cord's free end
		SetPos(pos, 0);
		CVec tip, base, rot;
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
		Vec offset = { 0, -0.2f, 0 }; // sits 0.2 under the joint
		PSMTXMultVec(matrix, &offset, &offset);
		SetPos(offset, 0);
		if (mWire.mSparkMode == 0) {
			if (mWire.mSparkTimer < 0) {
				CVec pos(mMoveTranslate);
				CVec scale(0.5f, 0.5f, 0.5f);
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
	const bool createEffect = object->mEffect >= 0;
	KillWireEffect(object);
	if (createEffect)
		object->mGenerator = gGame.mParticle.Create(0x10052, 0, object->mJObj, FALSE);
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
	XObjChr* player = gGame.GetPlayer();
	s32 path[]      = { 0, 1, 0, 1, 1 };
	HSD_JObj* joint = FindJointByPath(player->mJObj, path, ARRAY_SIZE(path));

	Mtx playerTransform;
	MtxPtr matrix;
	if (joint != NULL) {
		matrix = HSD_JObjGetMtxPtr(joint);
	} else {
		const Vec& pos = player->mMoveTranslate;
		PSMTXTrans(playerTransform, pos.x, pos.y, pos.z);
		matrix = playerTransform;
	}

	if (position != NULL) {
		Vec offset = { 0.0f, 0.0f, -0.4f };
		PSMTXMultVec(matrix, &offset, position);
	}
	if (rotation != NULL)
		HSD_MtxGetRotation(matrix, rotation);
	if (scale != NULL)
		HSD_MtxGetScale(matrix, scale);
}

} // extern "C"

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

extern "C" {

/**
 * @note Address: 0x800D9414
 * @note Size: 0x104
 */
BOOL XObjPlug::LoadFromPlacement(const CoordInfo& info)
{
	switch (mLoadState) {
	case 0:
		if (!XObj::LoadFromPlacement(info)) {
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
