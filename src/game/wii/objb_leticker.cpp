#include "game/wii/xobjleticker.h" // first: its AnimSfx needs the real nw4r::math
#include "game/wii/xobj.h"
#include "game/wii/cgame.h"
#include "game/ccameraaction.h"
#include "game/gmath.h"
#include "game/evt.h"
#include "arithmetic.h"
#include <math.h>

// the rising lifts and their camera action. The GameCube's objb_leticker.cpp carries
// the same code; the Wii keeps the member layout shifted 0x28 behind the bigger XObj
// and turns the camera with extra helpers living in the units above (fn_8016A81C,
// fn_8016AA78), so the action's Update stays out until those are decompiled

extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
extern "C" void fn_800A12F8(void* effects, u32 id, BOOL retireParticles); // retires the event effect with that id
extern "C" void UpdateObjectFloorXZExtent(XObj* object);
extern "C" void UpdateObjectFloorHeights(XObj* object);
extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* src, Vec* dst, u32 count);
extern CCameraActionLeticker gLetickerCameraAction;

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

// the floor boxes; the GameCube tables carry the same numbers minus mFlags
static XObjFloor sLetickerLiftFloor0[2] = {
	{ 2, 0, 1, { 0, 0, 0 }, 0, { 1200, 5000, 1200 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLiftFloor1[2] = {
	{ 2, 0, 3, { 0, 517, 0 }, 0, { 1216, 878, 1216 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel10Floors[5] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -22750 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -28600 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel9Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -22750 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel8Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -21300 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel7Floors[4] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -19600 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel6Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -13800 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel5Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -12350 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel4Floors[3] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ 2, 0, 1, { 0, 1000, -10650 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel3Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -4650 }, 0, { 600, 150, 7650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel2Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -3200 }, 0, { 600, 150, 5000 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel1Floors[2] = {
	{ 2, 0, 1, { 0, 1000, -1500 }, 0, { 600, 150, 2650 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

static XObjFloor sLetickerLevel0Floors[2] = {
	{ 2, 0, 1, { 0, 2564, 6 }, 0, { 604, 2827, 121 } },
	{ -1, 0, 0, { 0, 0, 0 }, 0, { 0, 0, 0 } },
};

// the floor boxes active at each level, picked by mCurrentLevel
static XObjFloor* sLetickerFloorsByLevel[] = {
	sLetickerLevel0Floors, sLetickerLevel1Floors, sLetickerLevel2Floors,  sLetickerLevel3Floors,
	sLetickerLevel4Floors, sLetickerLevel5Floors, sLetickerLevel6Floors,  sLetickerLevel7Floors,
	sLetickerLevel8Floors, sLetickerLevel9Floors, sLetickerLevel10Floors,
};

// anim 7 plays sound 0x1FD at frame 10
static AnimSfxEvent sLetickerAnimSfxEvents[] = {
	{ 7, 10.0f, -1.0f, 1, 5, 0x1FD, 0xFF, 0, 0, 0 },
	{ -1 },
};

static XObjFloor* sLetickerLiftFloors[2] = { sLetickerLiftFloor0, sLetickerLiftFloor1 };

// sets the object's floor boxes, they take effect at once
static inline void SetFloors(XObj* obj, XObjFloor* floors)
{
	obj->mFloors        = floors;
	obj->mFloorMasks[1] = 0;
	obj->mFloorMasks[0] = 0;
	UpdateObjectFloorXZExtent(obj);
	UpdateObjectFloorHeights(obj);
}

// whether the camera's eye still sits inside the lift's bounds when it faces angle;
// ReceivePlugPower turns it a quarter turn at a time until it fits
static inline bool CanTurn(CCameraActionLeticker* camera, f32 angle)
{
	if (!camera->mBounded) {
		return true;
	}
	Vec eye;
	camera->CalcEye(&eye, angle);
	if (eye.x < camera->mMinX || camera->mMaxX < eye.x || eye.z < camera->mMinZ || camera->mMaxZ < eye.z) {
		return false;
	}
	return true;
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
	mSfx.Init((WiiPlayerState*)this, sLetickerAnimSfxEvents);
}

/**
 * @note Address: 0x8011A3D0
 * @note Size: 0x238
 */
void XObjLeticker::UpdateObject(u32 frames)
{
	if (GetAnimId() == 1) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
	} else if (GetAnimId() == 3) {
		if (GetAnimEndFrame() <= frames + GetAnimFrame()) {
			SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
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
 * @note Address: 0x8011A608
 * @note Size: 0x3D4
 */
void XObjLeticker::StateLift()
{
	XObj* base = gGame.mXObjs.mObjects[mLinked[0]];
	XObj* top  = gGame.mXObjs.mObjects[mLinked[1]];
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
	s32 step = mHeight / 4.0f; // each level is four units of height
	if (mCurrentLevel != step) {
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x115)) {
			PlayWiiSoundEffect(&gGame.mSound, 0x115, 1.0f, 0.0f);
		}
		SetGameSfxPitch(&gGame.mSound, 0x115, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
		mCurrentLevel        = step;
		mReachedTargetHeight = FALSE;
	}
	top->SetTranslateY(9.0f + (mHeight + GetPos()->y));
	mStackTopPosition = *top->GetPos();
	if (mStatus.mFlags & 0x0800000000000000ULL) {
		f32 h;
		if (mHeight < 10.0f) {
			h = 19.0f;
		} else {
			h = 5.0f + (9.0f + mHeight) - 2.0f;
		}
		sLetickerLiftFloors[0]->mExtent[1]       = (s32)h * 100;
		sLetickerLiftFloors[0]->mCenterOffset[1] = (s32)(h / FSqrt(3.0f)) * 100;
		SetFloors(this, sLetickerLiftFloors[0]);
	} else {
		mBox[0].mCenterOffset[1] = (s32)((mHeight - 12.5f) / 4.0f) * 0xA9 + 0x654;
		mBox[0].mExtent[1]       = (s32)((mHeight - 12.5f) / 4.0f) * 400 + 1000;
		base->mFloors            = mBox;
		base->mFloorMasks[1]     = 0;
		base->mFloorMasks[0]     = 0;
		UpdateObjectFloorHeights(base);
		SetFloors(this, sLetickerLiftFloors[1]);
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
}

// NONMATCHING: regalloc, retail loads mCurrentLevel into r3 and the 1 for mUnwindWatch into r0;
// ours swaps the two
/**
 * @note Address: 0x8011A9DC
 * @note Size: 0x444
 */
void XObjLeticker::StateCount()
{
	XObj* part;
	s32 count = (s32)mLevelProgress / 10;
	if (mMaxLevel < count) {
		count = 10;
	}
	part = gGame.mXObjs.mObjects[mLinked[0]];
	if (mUnwindWatch && mUnwindAnim != part->GetAnimId()) { // another anim cut in ahead of the queued unwind
		mUnwindSuperseded = TRUE;
		mUnwindWatch      = FALSE;
	}
	if (mCurrentLevel != count) {
		if (part->GetAnimEndFrame() + part->GetAnimRate(1) <= part->GetAnimFrame()) {
			if (mCurrentLevel < count) {
				mCurrentLevel++;
				f32 rate = 1.0f;
				if (!mPowered) {
					rate = 8.0f;
				}
				part->SetAnim(mCurrentLevel, FALSE, 0.0f, rate, 0.0f, 15.0f);
			} else if (mPowered) { // a powered unwind steps back down at double rate
				mCurrentLevel--;
				part->SetAnim(24 - mCurrentLevel, FALSE, 0.0f, 2.0f, 0.0f, 15.0f);
				if (mCurrentLevel == 0) {
					part->PushAnim(0, 0, 100);
					mUnwindWatch = TRUE;
					mUnwindAnim  = 24 - mCurrentLevel;
				}
			} else {
				mLevelProgress = (mCurrentLevel - 1) * 10;
			}
			if (mPowered) {
				PlayWiiSoundEffect(&gGame.mSound, 0x119, 1.0f, 0.0f);
				SetGameSfxPitch(&gGame.mSound, 0x119, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
			}
		}
		SetFloors(part, sLetickerFloorsByLevel[mCurrentLevel]);
	}
	if (!(mStatus.mFlags & 0x0800000000000000ULL)) {
		SetColor(1, 0, 0, 0, 0.5f);
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
}

/**
 * @note Address: 0x8011AE20
 * @note Size: 0xE4
 */
void XObjLeticker::StateRide()
{
	WiiPlayerState* player  = gGame.GetPlayer();
	nw4r::g3d::ResNode node = GetResNodeOrFirst(0xD);
	switch (mStep) {
	case 0:
		break;
	case 10: {
		Vec pos;
		pos.x = pos.z = 0.0f;
		pos.y         = 0.5f;
		PSMTXMultVec(GetWorldMtx(node), &pos, &pos);
		player->SetPos(pos);
		player->SetRotationY(player->mMoveRotate.y - 0.5235988f);
		break;
	}
	}
}

/**
 * @note Address: 0x8011AF04
 * @note Size: 0xE4
 */
void XObjLeticker::DrawObject()
{
	DrawWithView(NULL);
	GetPos(); // unused: retail keeps the vcall, the GameCube's segment-draw loop that read it is gone
	switch (mState) {
	case 1:
		gGame.mXObjs.mObjects[mLinked[0]]->GetResNodeOrFirst(7); // unused, as GetPos above
		break;
	case 2:
		gGame.mXObjs.mObjects[mLinked[0]]->mStatus.mFlags &= ~0x80000000ULL;
		break;
	case 3:
		break;
	}
}

// NONMATCHING: ours keeps one more float in a saved register (f21, a 0x10 bigger frame);
// retail's int-to-float conversion carries an xoris ours lacks
/**
 * @note Address: 0x8011B1A0
 * @note Size: 0x3D0
 */
void XObjLeticker::DrawObjectEffects()
{
	XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
	switch (mState) {
	case 3:
		if (!(0.0f < mLevelProgress) || (!mPowered && part->GetAnimId() != 1)) {
			break;
		}
		Mtx mtx;
		u32 colors[50];
		Vec pts[50];
		Vec pos = *GetPos();
		PSMTXTrans(mtx, pos.x, pos.y, pos.z);
		f32 twist = 6.0f * (mLevelProgress / mMaxLevelProgress) * (f32)sin(DEG2RAD * (mFuncTime * 5)) + 72.0f;
		u32 i;
		for (i = 0; i < 50; i++) { // a helix of points rising and twisting with progress
			f32 angle = DEG2RAD * mFuncTime + DEG2RAD * (twist * (s32)i);
			f32 y     = 12.0f + 0.2f * (24.0f * (s32)i / 50.0f) * mLevelProgress / mMaxLevelProgress;
			pts[i].x  = 7.1f * (f32)sin(angle);
			pts[i].y  = y;
			pts[i].z  = 7.1f * (f32)cos(angle);
			colors[i] = ((48 - (s32)i) * 32 / 48 + 32) | 0x40F000; // the low channel fades along the strip
		}
		pts[49].x = pts[44].x;
		pts[49].y = pts[44].y;
		pts[49].z = pts[44].z;
		PSMTXMultVecArray(mtx, pts, pts, 50);
		DrawGsColoredLineStrip(&gGame.mGs, pts, colors, 50);
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x11D)) {
			PlayWiiSoundEffect(&gGame.mSound, 0x11D, GetSoundVolumeAtPosition(&gGame.mSound, part->GetPos(), 0), 0.0f);
			SetGameSfxPitch(&gGame.mSound, 0x11D, (s16)(600.0f * (mSpeedScale - 1.0f)), 0.0f);
		} else {
			SetWiiSfxVolume(&gGame.mSound, 0x11D, GetSoundVolumeAtPosition(&gGame.mSound, part->GetPos(), 0), 0.0f);
		}
		break;
	}
}

extern "C" {

// forward, GetClearLetickerApproachSide runs ahead of it in retail .text
bool IsLetickerJointClearOfObjects(XObj* obj, nw4r::g3d::ResNode node);
// forwards, ReceivePlugPower calls them, they follow OnPushDisplacement in retail .text
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle);
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle);

/**
 * @note Address: 0x8011B570
 * @note Size: 0x224
 */
s32 GetClearLetickerApproachSide(XObj* obj, const Vec* pos)
{
	const Vec* objPos = obj->GetPos();
	f32 angle         = WrapAngle((f32)atan2(pos->x - objPos->x, pos->z - objPos->z) - obj->mMoveRotate.y);
	if (3.0f + obj->GetPos()->y < gGame.GetPlayer()->GetPos()->y) {
		return 0;
	}
	if ((f32)cos(0.7853982) < (f32)cos(angle)) { // 45 degrees
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(6))) {
			return -1;
		}
	} else if ((f32)cos(angle) < (f32)cos(3.9269907)) { // 225 degrees
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(10))) {
			return -2;
		}
	} else if (0.0f < (f32)sin(angle)) {
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(8))) {
			return -3;
		}
	} else {
		if (IsLetickerJointClearOfObjects(obj, obj->GetResNodeOrFirst(12))) {
			return -4;
		}
	}
	return 0;
}

// whether the spot under the joint is free, nothing near it in the way
// NONMATCHING: regalloc, target and loop walker swap r26/r27 and an f1/f2 temp swaps against retail
/**
 * @note Address: 0x8011B7BC
 * @note Size: 0x244
 */
bool IsLetickerJointClearOfObjects(XObj* obj, nw4r::g3d::ResNode node)
{
	CVec pos;
	MtxP mtx = obj->GetWorldMtx(node);
	pos.x = pos.y = pos.z = 0.0f;
	PSMTXMultVec(mtx, &pos, &pos);
	pos.y -= 6.0f;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		if (!(gGame.mXObjs.mObjects[i]->mStatus.mFlags & 1) || !(gGame.mXObjs.mObjects[i]->mStatus.mFlags & 0x10000)) {
			continue;
		}
		if (obj == gGame.mXObjs.mObjects[i]) {
			continue;
		}
		if ((s16)gGame.mXObjs.mObjects[i]->GetPool() == 2) {
			continue;
		}
		XObj* target    = gGame.mXObjs.mObjects[i];
		CVec* targetPos = target->GetPos();
		f32 z           = targetPos->z - pos.z;
		f32 x           = targetPos->x - pos.x;
		f32 dist        = FSqrt(x * x + z * z);
		if (dist > 100.0f + (5.0f + target->mFloorXZExtent[0])) {
			continue;
		}
		CVec hit, normal;
		if (target->TestFloorCollision(&pos, &pos, 5.0f, 12.0f, &hit, &normal, 0, 0) & 2) {
			return false;
		}
	}
	return true;
}

} // extern "C"

/**
 * @note Address: 0x8011BA00
 * @note Size: 0xB00
 * @note NONMATCHING: HandleEventCommand - the Wii port is not written yet
 */

/**
 * @note Address: 0x8011C500
 * @note Size: 0x4
 */
void XObjLeticker::OnHitFromPosition(const Vec& position, f32 strength)
{
}

// NONMATCHING: retail fetches the player's position before the lift's for the heading and
// re-reads mObjects[mLinked[0]] for each call in the ride case; ours keeps the part in a register
// and inlines the bounds setter's loads in a different order
/**
 * @note Address: 0x8011C504
 * @note Size: 0x95C
 */
BOOL XObjLeticker::ReceivePlugPower(f32 amount)
{
	WiiPlayerState* player = gGame.GetPlayer();
	if (player->GetPos()->y < GetPos()->y - 5.0f) {
		return FALSE;
	}
	CPad pad(0);
	f32 heading = (f32)atan2(player->GetPos()->x - GetPos()->x, player->GetPos()->z - GetPos()->z);
	f32 angle   = heading - GetRotY();
	// the pushed direction snaps the target rotation to a quarter turn
	s32 side = GetSide(angle);
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
	if (!mPowered) {
		mPowered = TRUE;
		mWaiting = FALSE;
		if (mState != 3) {
			gGame.mMotionControlCamera.PushAction(&gLetickerCameraAction);
			gLetickerCameraAction.mTarget = &mStackTopPosition;
			if (mCameraBounded) {
				gLetickerCameraAction.SetBounds(mCameraMinX, mCameraMaxX, mCameraMinZ, mCameraMaxZ);
				for (s32 i = 0; i < 4; ++i) {
					if (CanTurn(&gLetickerCameraAction, gLetickerCameraAction.mAngle)) {
						break;
					}
					gLetickerCameraAction.mTurnTo = gLetickerCameraAction.mAngle += HALF_PI;
				}
			}
		} else {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			part->mStatus.mFlags &= ~0x10000ULL;
			part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (mState == 2) {
			mMaxLevel         = mLevelBySide[side];
			mMaxLevelProgress = 10.0f * mMaxLevel;
			mUnwindSuperseded = FALSE;
			mUnwindWatch      = FALSE;
			mUnwindAnim       = 0;
		}
		if (mPtcl) {
			fn_800A12F8(&gGame.mEvtPtcl, 0xA5, FALSE);
			mPtcl = FALSE;
		}
	}
	if (mWaiting) {
		if (!(pad.mPad->mButton & (1ULL << (NunchukStick + 2)))) {
			mPowerTimer -= 1;
			if (mPowerTimer <= 0) {
				return FALSE;
			}
			return TRUE;
		}
		mWaiting = FALSE;
	}
	amount *= mSpeedScale;
	++mPowerTimer;
	bool starting = mPowerTimer < 40;
	switch (mState) {
	case 1: // the lift
		if (!TurnLetickerAndCheckClearance(this, angle)) {
			if (mHeight == 4.0f * (s32)(mLevelProgress / 3.0f) + 0.5f) {
				XObj* part = gGame.mXObjs.mObjects[mLinked[1]];
				PlayWiiSoundEffect(&gGame.mSound, 0x1FE, 1.0f, 0.0f);
				SetColor(1, 0xFF, 0, 0, 0.5f);
				SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				part->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				part->PushAnim(0, 0, 100);
				mPowerTimer = 30;
				mWaiting    = TRUE;
			}
			return TRUE;
		}
		break;
	case 2: // the counter
		if (!TurnLetickerLinkedPartAndTrackJoint(this, angle)) {
			return starting;
		}
		break;
	case 3: // the ride
		mStackTopPosition = *GetPos();
		gGame.mXObjs.mObjects[mLinked[0]]->SetPos(*GetPos());
		gGame.mXObjs.mObjects[mLinked[0]]->SetRotateY(mMoveRotate.y, 1.0f);
		if (mMaxLevelProgress <= mLevelProgress + amount) {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			part->mStatus.mFlags |= 0x10000ULL;
			part->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else {
			XObj* part = gGame.mXObjs.mObjects[mLinked[0]];
			if (part->GetAnimId() != 0) {
				part->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			}
		}
		break;
	}
	if (player->mParams.mBattery <= 0.0f) {
		return FALSE;
	}
	if (pad.mPad->mButton & (1ULL << (NunchukStick + 2))) {
		mLevelProgress -= amount;
		if (mLevelProgress < 0.0f) {
			mLevelProgress = 0.0f;
			return starting;
		}
	} else {
		mLevelProgress += amount;
		if (mMaxLevelProgress <= mLevelProgress) {
			mLevelProgress = mMaxLevelProgress;
			return starting;
		}
	}
	if (GetAnimId() != 1 && GetAnimId() != 2) {
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
	return TRUE;
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
	SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
}

// NONMATCHING: retail loads GetPos()->x/z straight from r3 before moving the part into r3;
// ours copies the result to r4 first, and the loop registers r28-r30 rotate
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
		SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	} else {
		SetColor(1, 0xFF, 0, 0, 1.0f);
		SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	}
	if (mPtcl) {
		fn_800A12F8(&gGame.mEvtPtcl, 0xA5, FALSE);
		mPtcl = FALSE;
	}
}

extern "C" {

/**
 * @note Address: 0x8011D1E8, an unnamed helper between OnPushDisplacement and the
 * turn helpers, is not ported yet
 */

// NONMATCHING: retail calls the Wii MSL's out-of-line __fpclassifyf where our math.h inlines it,
// and copies the CVec with float loads where ours copies words
/**
 * @note Address: 0x8011D34C
 * @note Size: 0x3A0
 */
BOOL TurnLetickerAndCheckClearance(XObjLeticker* lift, f32 angle)
{
	CPad pad(0);
	XObj* first    = gGame.mXObjs.mObjects[lift->mLinked[0]];
	XObj* second   = gGame.mXObjs.mObjects[lift->mLinked[1]];
	lift->mHeading = angle;
	first->SetRotateY(angle + lift->GetRotY(), 0.1f);
	second->SetRotateY(angle + lift->GetRotY(), 0.1f);
	if (pad.mPad->mButton & (1ULL << (NunchukStick + 2))) {
		return TRUE;
	}

	f32 offset = 4.0f * (s32)(lift->mLevelProgress / 3.0f) + 0.5f;
	CVec pos(*second->GetPos());
	pos.y = 6.0f + (3.0f + (9.0f + (offset + lift->GetPosY())));
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* target = gGame.mXObjs.mObjects[i];
		if (!(target->mStatus.mFlags & 1) || !(target->mStatus.mFlags & 0x10000)) {
			continue;
		}
		if (target == lift || target == first || target == second) {
			continue;
		}
		XObjSource* source = target->mAttr.mObjData != NULL ? target->mAttr.mObjData->mSource : NULL;
		if (source == NULL) {
			continue;
		}
		const Vec* targetPos = target->GetPos();
		f32 dx               = targetPos->x - pos.x;
		f32 dz               = targetPos->z - pos.z;
		if (FSqrt(dx * dx + dz * dz) > 100.0f + (5.0f + target->mFloorXZExtent[0])) {
			continue;
		}
		CVec hit, normal;
		u32 flags = target->TestFloorCollision(&pos, &pos, 5.5f, 6.0f, &hit, &normal, 0, 0);
		if (flags & 2) {
			return FALSE;
		}
		if (flags & 1) {
			f32 floor = target->mFloorHeights[target->mHitFloorIndex];
			if (__fpclassifyf(floor) == FP_NAN) {
				return FALSE;
			}
			if (pos.y <= floor) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

// NONMATCHING: retail adds angle and the lift's rotation once into f31; ours keeps the rotation
// and adds again, and the zero Vec's stores are scheduled later
/**
 * @note Address: 0x8011D6EC
 * @note Size: 0x1B0
 */
BOOL TurnLetickerLinkedPartAndTrackJoint(XObjLeticker* leticker, f32 angle)
{
	XObj* part = gGame.mXObjs.mObjects[leticker->mLinked[0]];
	f32 my     = leticker->GetRotY();
	f32 c      = cos(part->GetRotY() - (angle + my));
	if (c < (f32)cos(0.08726646f)) { // the part is more than five degrees off
		leticker->mLevelProgress = 0.0f;
		leticker->mPowerTimer    = 0;
		if (part->GetAnimId() == 0) {
			part->SetRotateY(angle + leticker->GetRotY(), 0.1f);
			leticker->mHeading = angle;
		}
	}
	if (!leticker->mUnwindSuperseded) {
		if (part->GetAnimFrame() < part->GetAnimEndFrame()) {
			return TRUE;
		}
	}
	if (leticker->mUnwindSuperseded) {
		leticker->mUnwindSuperseded = FALSE;
	}
	Vec pos;
	pos.x = pos.y = pos.z = 0.0f;
	PSMTXMultVec(part->GetWorldMtx(leticker->mCurrentLevel + 1), &pos, &leticker->mStackTopPosition);
	return TRUE;
}

} // extern "C"

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

// the action ReceivePlugPower pushes while the leticker's camera is bounded;
// retail's __sinit constructs it in this unit
CCameraActionLeticker gLetickerCameraAction;
