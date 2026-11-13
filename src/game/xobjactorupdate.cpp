#include "game/actorupdateconstants.h"
#include "game/xobjactor.h"
#include "game/hsd_cobj.h"
#include "game/gmath.h"

STATIC_ASSERT(sizeof(XObjActorLiterals) == 0xA8);

/**
 * @note Address: 0x80107DE0
 * @note Size: 0x138
 */
// UpdateObject, the update
void XObjActor::UpdateObject(u32 frames)
{
	XObjActor* actor = (XObjActor*)this;
	actor->ProcessPendingEvents(frames);
	actor->RunStateHandler(frames);
	if (actor->mActorStatus.mFlags & 4) {
		actor->UpdateGroundMovement(frames);
	}
	UpdateObject__4XObjFUl(actor, frames);
	actor->UpdateAfterAnim(frames);
	HSD_AObj* aobj = GetAObj(actor->mJObj);
	f32 frame      = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	actor->UpdateAnimationSounds(frame);
}

/**
 * @note Address: 0x80107F18
 * @note Size: 0xFC
 */
BOOL XObjActor::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		mHeading   = 0.0f;
		mMoveSpeed = 0.0f;
		mFacing    = 0.0f;
		for (u32 i = 0; i < 20; ++i) {
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
		mLookTarget = 0;
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
	XObjActorLiterals* lit = &gActorUpdateConstants;
	mHeading               = 0.0f;
	mMoveSpeed             = 0.0f;
	mFacing                = 0.0f;
	_98C                   = PI;
	mKindStep              = 0;
	mKindCounter           = 0;
	mActorStatus.mFlags    = 0;
	_9A0                   = 0;
	_9A4                   = 0;
	_9A8                   = 1.0f;
	_9AC                   = 1.0f;
	Vec v0                 = lit->mInit[0];
	mHitPos.x              = v0.x;
	mHitPos.y              = v0.y;
	mHitPos.z              = v0.z;
	Vec v1                 = lit->mInit[1];
	mHomePos.x             = v1.x;
	mHomePos.y             = v1.y;
	mHomePos.z             = v1.z;
	Vec v2                 = lit->mInit[2];
	mVelocity.x            = v2.x;
	mVelocity.y            = v2.y;
	mVelocity.z            = v2.z;
	Vec v3                 = lit->mInit[3];
	_A40.x                 = v3.x;
	_A40.y                 = v3.y;
	_A40.z                 = v3.z;
	mHitStrength           = 0.0f;
	mFallSpeed             = 0.0f;
	mGroundHeight          = 0.0f;
	mCmd[0]                = 0;
	mCmd[1]                = 0;
	mCmd[18]               = 0;
	for (u32 i = 0; i < 16; ++i) {
		mCmd[i + 2] = 0;
	}
	_A38 = 0.0f;
	_A3C = 0.0f;
	for (u32 i = 0; i < 8; ++i) {
		mParts[i].mObj   = NULL;
		mParts[i].mJoint = NULL;
	}
	mAnimSfx.mEvents = NULL;
	mAttachedId      = -1;
	mAttached        = NULL;
	mHeldObjects[0]  = NULL;
	mHeldObjects[1]  = 0;
	mLookMode        = 0;
	mHeadYaw         = 0.0f;
	mLookAngle       = 0.0f;
	mLookRate        = 0.0f;
	mLookTarget      = 0;
}

/**
 * @note Address: 0x801081F0
 * @note Size: 0x200
 */
void XObjActor::ResetObjectState()
{
	const Vec* reset = gActorUpdateConstants.mReset;
	ResetObjectState__4XObjFv(this);
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
	Vec v0              = reset[0];
	mHitPos.x           = v0.x;
	mHitPos.y           = v0.y;
	mHitPos.z           = v0.z;
	Vec v1              = reset[1];
	mHomePos.x          = v1.x;
	mHomePos.y          = v1.y;
	mHomePos.z          = v1.z;
	Vec v2              = reset[2];
	mVelocity.x         = v2.x;
	mVelocity.y         = v2.y;
	mVelocity.z         = v2.z;
	Vec v3              = reset[3];
	_A40.x              = v3.x;
	_A40.y              = v3.y;
	_A40.z              = v3.z;
	mHitStrength        = 0.0f;
	mFallSpeed          = 0.0f;
	mGroundHeight       = 0.0f;
	mCmd[0]             = 0;
	mCmd[1]             = 0;
	mCmd[18]            = 0;
	for (u32 i = 0; i < 16; ++i) {
		mCmd[i + 2] = 0;
	}
	_A38 = 0.0f;
	_A3C = 0.0f;
	for (u32 i = 0; i < 8; ++i) {
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
	InitializeObject__4XObjFUl(this, id);
	mAnimSfx.mEvents = NULL;
}
