#include "game/actorupdateconstants.h"
#include "game/xobjactor.h"
#include "game/hsd_cobj.h"
#include "game/gmath.h"

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
		mHeading   = 0.0f;
		mMoveSpeed = 0.0f;
		mFacing    = 0.0f;
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
		mHeadYaw    = 0.0f;
		mLookAngle  = 0.0f;
		mLookRate   = 0.0f;
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
