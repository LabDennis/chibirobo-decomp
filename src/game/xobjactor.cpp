#include "game/xobjactorbase.h"
#include "game/xobjactor.h"
#include "game/xobjspidersmall.h"
#include "game/cgame.h"
#include "game/objectfloorquery.h"
#include <math.h>
#include "game/xchr.h"
#include "game/gmath.h"
#include "game/actorupdateconstants.h"
#include "game/hsd_cobj.h"

const f32 XObjActor::sZero = 0.0f;

/**
 * @note Address: 0x80106A14
 * @note Size: 0x78
 */
XObjActor::~XObjActor()
{
}

/**
 * @note Address: 0x80106A8C
 * @note Size: 0x4
 */
void XObjActor::ResetActorState()
{
}

/**
 * @note Address: 0x80106A90
 * @note Size: 0x8
 */
HSD_JObj* XObjActor::GetHeadJoint()
{
	return mJObj;
}

/**
 * @note Address: 0x80106A98
 * @note Size: 0x28
 */
CVec XObjActor::GetHeadWorldPosition()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x80106AC0
 * @note Size: 0x90
 */
extern "C" s32 CountEmergedSmallSpiders()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)(gGame.mXObjs.mPools[18].mObjects + i * gGame.mXObjs.mPools[18].mStride);
		if (spider->mStatus.mFlags & 1ULL) {
			if (HasSmallSpiderEmerged(spider)) {
				++count;
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x80106B50
 * @note Size: 0x90
 */
// small spiders holding on to chibi robo
extern "C" s32 CountSmallSpidersGrabbingPlayer()
{
	s32 count = 0;
	for (s32 i = 0; i < 16; ++i) {
		XObjSpiderSmall* spider = (XObjSpiderSmall*)(gGame.mXObjs.mPools[18].mObjects + i * gGame.mXObjs.mPools[18].mStride);
		if (spider->mStatus.mFlags & 1ULL) {
			if (IsSmallSpiderGrabbingPlayer(spider)) {
				++count;
			}
		}
	}
	return count;
}

/**
 * @note Address: 0x80106BE0
 * @note Size: 0xC4
 */
void XObjActor::UpdateAnimationSounds(f32 frame)
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			u8 volume;
			if (mActorStatus.mFlags & 0x800) {
				volume = 255;
			} else {
				volume = GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0);
			}
			mAnimSfx.Update(mAnimId, frame, volume);
		}
	}
}

/**
 * @note Address: 0x80106CA4
 * @note Size: 0x3C8
 */
void XObjActor::UpdateGroundMovement(u32 frames)
{
	// The floor search starts at the stage origin.
	Vec floorProbe = { 0.0f, 0.0f, 0.0f };
	CVec step;
	step.x          = mMoveSpeed * (f32)sin(mHeading);
	step.z          = mMoveSpeed * (f32)cos(mHeading);
	step.y          = 0.0f;
	const CVec next = mMoveTranslate + step * (f32)frames;
	mMoveTranslate.Set(next);
	mMoveTranslate.SetTarget(next);

	const f32 offset = GetKind() == 15 ? 6.0f : 0.0f;
	XObjMap* map     = static_cast<XObjMap*>(gGame.mXObjs.mPools[3].First());
	mActorStatus.mFlags &= ~0x20;
	if (FindMapFloor(map, &floorProbe, &mFloorHit, NULL)) {
		mGroundHeight = mFloorHit.mHeight + offset;
		if (!(mActorStatus.mFlags & 8)) {
			mMoveTranslate.y = mFloorHit.mHeight + offset;
			mActorStatus.mFlags |= 0x20;
		} else if (mActorStatus.mFlags & 0x10) {
			if (mFallSpeed >= -2.3f)
				mFallSpeed -= 0.36f * (f32)frames;
			else
				mFallSpeed = -2.3f;
			if (mMoveTranslate.y <= mGroundHeight)
				mMoveTranslate.y = mGroundHeight;
			mMoveTranslate.y += mFallSpeed;
			if (mMoveTranslate.y <= mGroundHeight) {
				mFallSpeed = 0.0f;
				mActorStatus.mFlags &= ~8;
				mActorStatus.mFlags &= ~0x10;
			}
		} else {
			mMoveTranslate.y += mFallSpeed;
		}
	} else {
		mMoveTranslate.y = offset;
		mActorStatus.mFlags |= 0x20;
	}
	const Vec position = mMoveTranslate;
	SetPos(position, 0);
	SetRotateY(mFacing, 0.0f);
}

/**
 * @note Address: 0x8010706C
 * @note Size: 0x18
 */
void XObjActor::ResetMovementStep()
{
	mKindStep    = 0;
	mKindCounter = 0;
	mMoveSpeed   = sZero;
}

/**
 * @note Address: 0x80107084
 * @note Size: 0x1C
 */
Vec XObjActor::GetCameraTarget()
{
	return mMoveTranslate;
}

/**
 * @note Address: 0x801070A0
 * @note Size: 0x4
 */
void XObjActor::OnPushDisplacement(const Vec& displacement)
{
}

extern "C" void OSReport(const char*, ...);

/**
 * @note Address: 0x801070A4
 * @note Size: 0x498
 */
s32 XObjActor::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100: {
		s32 id = args[1];
		if (id == 0) {
			mActorStatus.mFlags &= ~0x1000;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mParts[i].mObj->mJObj);
					mParts[i].mObj   = NULL;
					mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i = 0;
			while (i < ARRAY_SIZE(mParts) && mParts[i].mObj) {
				++i;
			}
			if (i < ARRAY_SIZE(mParts)) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i, id);
#endif
				mActorStatus.mFlags |= 0x1000;
				if (args[1] >= 10000) {
					mParts[i].mObj = FindObj(args[1]);
				} else {
					mParts[i].mObj = gGame.GetObject((s16)args[1]);
				}
#if !defined(VERSION_PAL)
				if (mParts[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = args[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = args[3 + j];
				}
				mParts[i].mJoint = GetJointByPath(mJObj, path, depth);
				XObjJointPart::UpdateMatrix(*this, i);
			}
		}
		break;
	}
	case 101:
		if (args[1] == 0) {
			mActorStatus.mFlags &= ~0x800;
		} else {
			mActorStatus.mFlags |= 0x800;
		}
		break;
	}
	return 0;
}

/**
 * @note Address: 0x8010753C
 * @note Size: 0xC
 */
bool XObjActor::IsHitPending()
{
	return (mActorStatus.mFlags & 2) != 0;
}

/**
 * @note Address: 0x80107548
 * @note Size: 0xA0
 */
// OnHitFromPosition, knocked from pos
void XObjActor::OnHitFromPosition(const Vec& pos, f32 strength)
{
	XObj::OnHitFromPosition(pos, strength);
	mActorStatus.mFlags |= 2;
	Vec hit      = { pos.x, pos.y, pos.z };
	mHitPos      = hit;
	mHitStrength = strength;
}

/**
 * @note Address: 0x801075E8
 * @note Size: 0x14
 */
s32 XObjActor::OnPlayerContact(const CVec&, s32 value)
{
	mActorStatus.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x801075FC
 * @note Size: 0x4
 */
void XObjActor::DrawDebugStatus()
{
}

/**
 * @note Address: 0x80107600
 * @note Size: 0x20
 */
void XObjActor::DrawScreenOverlay()
{
	XObj::DrawScreenOverlay();
}

/**
 * @note Address: 0x80107620
 * @note Size: 0x70
 */
void XObjActor::DrawObject()
{
	if (mActorStatus.mFlags & 0x80) {
		Push(GetHeadJoint(), 0x80, mHeadYaw);
	}
	DrawWithView(NULL);
}

/**
 * @note Address: 0x80107690
 * @note Size: 0x8
 */
// the head's turn limits
f32 XObjActor::GetMinHeadPitch()
{
	return -0.34906584f; // -20 degrees
}

/**
 * @note Address: 0x80107698
 * @note Size: 0x8
 */
f32 XObjActor::GetMaxHeadPitch()
{
	return 0.34906584f; // +20 degrees
}

/**
 * @note Address: 0x801076A0
 * @note Size: 0x8
 */
f32 XObjActor::GetMinHeadYaw()
{
	return -1.3962634f; // -80 degrees
}

/**
 * @note Address: 0x801076A8
 * @note Size: 0x8
 */
f32 XObjActor::GetMaxHeadYaw()
{
	return 1.3962634f; // +80 degrees
}

/**
 * @note Address: 0x801076B0
 * @note Size: 0x2C
 */
// look at a Vec*
extern "C" void SetActorLookAtPosition(CXObj* object, const Vec* target, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->mActorStatus.mFlags |= 0x80; // flag 0x80: the head tracks
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
	actor->mActorStatus.mFlags |= 0x400; // 0x400: also pitch the head toward the player
}

/**
 * @note Address: 0x801076DC
 * @note Size: 0x20
 */
extern "C" void SetActorLookAngle(CXObj* object, f32 angle, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode  = 3; // at an angle
	actor->mLookAngle = angle;
	actor->mLookRate  = rate;
}

/**
 * @note Address: 0x801076FC
 * @note Size: 0x20
 */
extern "C" void SetActorLookTarget(CXObj* object, const Vec* target, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode   = 1;
	actor->mLookRate   = rate;
	actor->mLookTarget = target;
}

/**
 * @note Address: 0x8010771C
 * @note Size: 0x34
 */
// look ahead again
extern "C" void ResetActorLook(CXObj* object, f32 rate)
{
	XObjActor* actor = (XObjActor*)object;
	actor->mActorStatus.mFlags |= 0x80;
	actor->mLookMode  = 0; // back to forward
	actor->mLookRate  = rate;
	actor->mLookAngle = XObjActor::sZero;
	actor->mActorStatus.mFlags &= ~0x400;
	actor->mHeadPitch = XObjActor::sZero;
}

static inline void SetTargetYaw(XObjActor* actor, const CVec& head)
{
	f32 facing        = actor->GetRotationY();
	actor->mLookAngle = (f32)atan2(actor->mLookTarget->x - head.x, actor->mLookTarget->z - head.z) - facing;
}

static inline void SetTargetPitch(XObjActor* actor, const CVec& target, const CVec& head)
{
	f32 dx = target.x - head.x, dz = target.z - head.z, dy = target.y - head.y;
	f32 horizontal    = sqrtf(dx * dx + dz * dz);
	f32 angle         = (f32)atan2(horizontal, sqrtf(dy * dy));
	actor->mHeadPitch = dy < 0.0f ? HALF_PI - angle : angle - HALF_PI;
}

/**
 * @note Address: 0x80107750
 * @note Size: 0x690
 */
void XObjActor::UpdateAfterAnim(u32 frames)
{
	if (mActorStatus.mFlags & 0x80) {
		if (mLookMode == 1) {
			SetTargetYaw(this, GetHeadWorldPosition());
			f32 limit = (f32)cos(GetMaxHeadYaw());
			if ((f32)cos(mLookAngle) < limit) {
				if (0.0f < (f32)sin(mLookAngle))
					mLookAngle = GetMaxHeadYaw();
				else
					mLookAngle = GetMinHeadYaw();
			}
		}
		f32 step = (f32)cos(mLookRate);
		if ((f32)cos(mLookAngle - mHeadYaw) <= step) {
			if (0.0f < (f32)sin(mHeadYaw - mLookAngle))
				mHeadYaw -= mLookRate;
			else
				mHeadYaw += mLookRate;
		} else {
			mHeadYaw = mLookAngle;
			if (mLookMode == 0)
				mActorStatus.mFlags &= ~0x80;
		}
		Push(GetHeadJoint(), 0x80, mHeadYaw);
	}
	if (mActorStatus.mFlags & 0x400) {
		CVec target;
		target.x = gGame.GetPlayer()->mMoveTranslate.x;
		target.y = 4.0f + gGame.GetPlayer()->mMoveTranslate.y;
		target.z = gGame.GetPlayer()->mMoveTranslate.z;
		SetTargetPitch(this, target, GetHeadWorldPosition());
		if (mHeadPitch > GetMaxHeadPitch())
			mHeadPitch = GetMaxHeadPitch();
		else if (mHeadPitch < GetMinHeadPitch())
			mHeadPitch = GetMinHeadPitch();
		Push(GetHeadJoint(), 0x40, mHeadPitch);
	}
	if (mActorStatus.mFlags & 0x1000) {
		for (u32 i = 0; i < ARRAY_SIZE(mParts); ++i) {
			if (mParts[i].mJoint && mParts[i].mObj)
				XObjJointPart::UpdateMatrix(*this, i);
		}
	}
}

STATIC_ASSERT(sizeof(XObjActorLiterals) == 0xA8);

/**
 * @note Address: 0x80107DE0
 * @note Size: 0x138
 */
void XObjActor::UpdateObject(u32 frames)
{
	ProcessPendingEvents(frames);
	RunStateHandler(frames);
	if (mActorStatus.mFlags & 4) { // flag 4: moves on the ground
		UpdateGroundMovement(frames);
	}
	XObj::UpdateObject(frames);
	UpdateAfterAnim(frames);
	HSD_AObj* aobj = GetAObj(mJObj);
	f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	UpdateAnimationSounds(frame);
}

/**
 * @note Address: 0x80107F18
 * @note Size: 0xFC
 */
BOOL XObjActor::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		mHeading   = sZero;
		mMoveSpeed = sZero;
		mFacing    = sZero;
		for (u32 i = 0; i < ARRAY_SIZE(mCmd); ++i) {
			mCmd[i] = 0;
		}
		mAttachedId     = -1;
		mAttached       = NULL;
		mHeldObjects[0] = NULL;
		mHeldObjects[1] = NULL;
		ClearStateHandler();
		ResetActorState();
		mLookMode   = 0;
		mHeadYaw    = sZero;
		mLookAngle  = sZero;
		mLookRate   = sZero;
		mLookTarget = NULL;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80108014
 * @note Size: 0x1DC
 */
void XObjActor::ResetForNewGame()
{
	const Vec* initial  = gActorUpdateConstants.mInit;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_98C                = PI;
	mKindStep           = 0;
	mKindCounter        = 0;
	mActorStatus.mFlags = 0;
	_9A0                = 0;
	_9A4                = 0;
	_9A8                = 1.0f;
	_9AC                = 1.0f;

	mHitPos   = initial[0];
	mHomePos  = initial[1];
	mVelocity = initial[2];
	_A40      = initial[3];

	mHitStrength  = 0.0f;
	mFallSpeed    = 0.0f;
	mGroundHeight = 0.0f;

	for (u32 i = 0; i < ARRAY_SIZE(mCmd) - 1; ++i) {
		mCmd[i] = 0;
	}
	_A38 = 0.0f;
	_A3C = 0.0f;
	for (u32 i = 0; i < ARRAY_SIZE(mParts); ++i) {
		mParts[i].mObj   = NULL;
		mParts[i].mJoint = NULL;
	}
	mAnimSfx.mEvents = NULL;
	mAttachedId      = -1;
	mAttached        = NULL;
	mHeldObjects[0]  = NULL;
	mHeldObjects[1]  = NULL;
	mLookMode        = 0;
	mHeadYaw         = 0.0f;
	mLookAngle       = 0.0f;
	mLookRate        = 0.0f;
	mLookTarget      = NULL;
}

/**
 * @note Address: 0x801081F0
 * @note Size: 0x200
 */
void XObjActor::ResetObjectState()
{
	const Vec* reset = gActorUpdateConstants.mReset;
	XObj::ResetObjectState();
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	_98C                = PI;
	mKindStep           = 0;
	mKindCounter        = 0;
	mActorStatus.mFlags = 0;
	_9A0                = 0;
	_9A4                = 0;
	_9A8                = 1.0f;
	_9AC                = 1.0f;

	mHitPos   = reset[0];
	mHomePos  = reset[1];
	mVelocity = reset[2];
	_A40      = reset[3];

	mHitStrength  = 0.0f;
	mFallSpeed    = 0.0f;
	mGroundHeight = 0.0f;
	mCmd[0]       = 0;
	mCmd[1]       = 0;
	mCmd[18]      = 0;
	for (u32 i = 0; i < 16; ++i) {
		mCmd[i + 2] = 0; // commands 2..17
	}
	_A38 = 0.0f;
	_A3C = 0.0f;
	for (u32 i = 0; i < ARRAY_SIZE(mParts); ++i) {
		mParts[i].mObj   = NULL;
		mParts[i].mJoint = NULL;
	}
	mAttachedId     = -1;
	mAttached       = NULL;
	mHeldObjects[0] = NULL;
	mHeldObjects[1] = NULL;
	mLookMode       = 0;
	mHeadYaw        = 0.0f;
	mLookAngle      = 0.0f;
	mLookRate       = 0.0f;
	mLookTarget     = 0;
}

/**
 * @note Address: 0x801083F0
 * @note Size: 0x34
 */
void XObjActor::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	mAnimSfx.mEvents = NULL;
}
