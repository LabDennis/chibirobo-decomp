#include "game/wii/xobjleticker.h" // first: its AnimSfx needs the real nw4r::math
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/ccameraaction.h"
#include "game/evt.h"
#include <math.h>

// the rising lifts and their camera action. The GameCube's objb_leticker.cpp carries
// the same code; the Wii keeps the member layout shifted 0x28 behind the bigger XObj
// and turns the camera with extra helpers living in the units above (fn_8016A81C,
// fn_8016AA78), so the action's Update stays out until those are decompiled

extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
extern "C" void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // retires the event effect with that id

extern const AnimSfxEvent lbl_80309360[]; // the leticker's lid sound event table

// which of the leticker's four sides angle is closest to, counting quarter turns
// counterclockwise; the GameCube reads Cos/Sin off a table, the Wii uses MSL cosf/sinf
static inline s32 GetSide(f32 angle)
{
	s32 side;
	if (std::cosf(0.7853982f) <= std::cosf(angle)) { // 45 degrees
		side = 0;
	} else if (std::cosf(angle) <= std::cosf(3.9269907f)) { // 225 degrees
		side = 2;
	} else if (0.0f < std::sinf(angle)) {
		side = 1;
	} else {
		side = 3;
	}
	return side;
}

/**
 * @note Address: 0x801192EC
 * @note Size: 0xAC
 */
void CCameraActionLeticker::Reset()
{
	CCameraActionLadr::Reset();
	mTurnFrames = 0;
	mTurnTo     = 0.0f;
	mBounded    = FALSE;
	gGame.SetStatusFlags(0x80);
	_76 = _75 = _74 = 0;
	_A4 = _A0 = 0;
	Vec v     = { 0.0f, 0.0f, 0.0f };
	_94       = v;
	_A8 = _90 = _8C = 0;
}

/**
 * @note Address: 0x8011A334
 * @note Size: 0x9C
 */
void XObjLeticker::ResetObjectState()
{
	XObj::ResetObjectState();
	mLinkedCount   = 0;
	mFunc          = NULL;
	mCurrentLevel  = 0;
	mFuncTime      = 0;
	mLevelProgress = 0.0f;
	mHeight        = 0.0f;
	mPtcl          = TRUE;
	mPowered       = FALSE;
	mPowerTimer    = 0;
	mState         = 0;
	mHeading       = 0.0f;
	mBox[0].mId    = -1;
	mBox[1].mId    = -1;
	mSfx.Init((WiiPlayerState*)this, (AnimSfxEvent*)lbl_80309360);
}

/**
 * @note Address: 0x8011A3D0
 * @note Size: 0x238
 */
void XObjLeticker::UpdateObject(u32 frames)
{
	if (GetAnimId() == 1) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	} else if (GetAnimId() == 3) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
	XObj::UpdateObject(frames);
	mFuncTime += frames;
	if (mFunc) {
		(this->*mFunc)();
	}
	f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
	mSfx.Update(GetAnimId(), GetAnimFrame(), volume);
}

/**
 * @note Address: 0x8011C504
 * @note Size: 0x95C
 * @note NONMATCHING: ported from the GameCube body; the Wii retail function is
 * present but the port is not written yet
 */

/**
 * @note Address: 0x8011D030
 * @note Size: 0x1B8
 */
void XObjLeticker::OnPushDisplacement(const Vec& displacement)
{
	for (s32 i = 0; i < mLinkedCount; i++) {
		XObj* part = gGame.mXObjs.mObjects[mLinked[i]];
		part->SetTranslateX(GetPos()->x);
		part->SetTranslateZ(GetPos()->z);
	}
	if (0.0f < PSVECMag(&displacement)) {
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
	} else {
		SetColor(1, 0xFF, 0, 0, 1.0f);
		SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
	}
	if (mPtcl) {
		fn_800A12F8(&gGame.mEvtPtcl, 0xA5, FALSE);
		mPtcl = FALSE;
	}
}

/**
 * @note Address: 0x8011CE60
 * @note Size: 0x1D0
 */
void XObjLeticker::OnUnplugged()
{
	if (mState != 3) {
		gGame.ClearStatusFlags(0x80);
		gGame.mMotionControlCamera.PopAction();
	}
	mPowered    = FALSE;
	mPowerTimer = 0;
	if (mState == 2) {
		mMaxLevel         = mLevelBySide[GetSide(mHeading)];
		mMaxLevelProgress = 10.0f * mMaxLevel;
		mLevelProgress    = 10.0f * mCurrentLevel;
	}
	SetColor(1, 0, 0, 0, 0.5f);
	SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x8011C500
 * @note Size: 0x4
 */
void XObjLeticker::OnHitFromPosition(const Vec& position, f32 strength)
{
}

/**
 * @note Address: 0x8011D89C
 * @note Size: 0x14
 */
void XObjLeticker::ResumeSounds(f32 fade)
{
	mSfx.Resume(fade);
}

/**
 * @note Address: 0x8011D8B0
 * @note Size: 0x14
 */
void XObjLeticker::PauseSounds(f32 fade)
{
	mSfx.Pause(fade);
}

/**
 * @note Address: 0x8011D8C4
 * @note Size: 0x8
 */
f32 CCameraActionLeticker::GetFollowDivisor()
{
	return 0.1f;
}

/**
 * @note Address: 0x8011D8CC
 * @note Size: 0x8
 */
u32 CCameraActionLeticker::GetCameraFlags()
{
	return 11;
}

/**
 * @note Address: 0x8011D8DC - the deleting destructor sits between GetCameraFlags
 * and GetMode in retail, the compiler picks its own spot
 */

/**
 * @note Address: 0x8011D8D4
 * @note Size: 0x8
 */
u32 CCameraActionLeticker::GetMode()
{
	return 0;
}

/**
 * @note Address: 0x8011D91C
 * @note Size: 0x20
 */
Vec CCameraActionLeticker::GetCameraTarget()
{
	return *mTarget;
}

// the action HandleEventCommand pushes while the leticker's camera is bounded;
// retail's __sinit constructs it in this unit
static CCameraActionLeticker gLetickerCameraAction;
