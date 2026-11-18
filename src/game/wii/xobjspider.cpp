#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/attachmentvalues.h"

// the Wii file of the GameCube's xobjspider.cpp (retail 0x8012063C-0x80128C84): its functions run in the
// Wii's order, ~XObjSpider lives with the pool ctors and the coin and carry helpers take the work block

extern "C" {
void ResetSpiderState(XObjSpider* spider);
void ApplySpiderPlacement(XObjSpider* spider);
void ResetSpiderCoinBounces(void* unused);
void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
void HSD_MtxGetScale(Mtx mat, Vec* vec);

extern s32 gRefreshRate;
extern AnimSfxEvent gSpiderAnimSfxEvents[];
extern XObj* gSpiderCarryModels[16];
extern s32 gSpiderCarrySlotOwners[16];
extern u32 gSpiderCoinBounceUpdateContext; // the coins' manager token, the callbacks never touch it
extern s32 gSpiderCarryCount;
extern u8 gSpiderCarryFlag;
}

/**
 * @note Address: 0x8012063C
 * @note Size: 0x58
 */
void XObjSpider::InitializeObject()
{
	XObj::InitializeObject();
	ResetSpiderState(this);
	SetState(0);
	mFlags.mFlags |= 0x04000000;
}

/**
 * @note Address: 0x80120694
 * @note Size: 0x50
 */
void XObjSpider::ResetForNewGame()
{
	ResetSpiderState(this);
	SetState(0);
	mFlags.mFlags |= 0x04000000;
}

/**
 * @note Address: 0x801206E4
 * @note Size: 0x324
 */
// puts everything back to how it starts
extern "C" void ResetSpiderState(XObjSpider* spider)
{
	spider->mDir          = 0.0f;
	spider->mSpeed        = 0.0f;
	spider->mFacing       = 0.0f;
	spider->_9B4          = 0.0f;
	spider->mTurnSpeed    = 0.1f;
	spider->mFallSpeed    = 0.0f;
	spider->mState        = 0;
	spider->mPrevState    = 0;
	spider->mPhase        = 0;
	spider->mCounter      = 0;
	spider->mFlags.mFlags = 0;
	spider->mWait         = 0;
	spider->_9EC          = 1.0f;
	spider->_9F0          = 1.0f;
	Vec hitPos            = { 0.0f, 0.0f, 0.0f };
	spider->mHitPos       = hitPos;
	Vec home              = { 0.0f, 0.0f, 0.0f };
	spider->mHome         = home;
	Vec wallPos           = { 0.0f, 0.0f, 0.0f };
	spider->mWallPos      = wallPos;
	Vec target            = { 0.0f, 0.0f, 0.0f };
	spider->mTarget       = target;
	Vec shotOrigin        = { 0.0f, 0.0f, 0.0f };
	spider->mShotOrigin   = shotOrigin;
	Vec shotPosition      = { 0.0f, 0.0f, 0.0f };
	spider->mShotPosition = shotPosition;
	spider->mShotRange    = 0.0f;
	spider->mShotProgress = 0.0f;
	spider->mShotState    = 0;
	spider->_A74          = 0;
	spider->mHitStrength  = 0.0f;
	spider->mWall         = 0;
	spider->mMaxHits      = 3;
	spider->mHits         = spider->mMaxHits;
	spider->mCommand      = 0;
	spider->mArgs[0]      = 0;
	spider->_AB4          = 0;
	spider->mThreadAlpha  = 1.0f;
	spider->mStatus.mFlags |= 8;
	spider->mCarry.mPos.x        = 0.0f;
	spider->mCarry.mPos.y        = 0.0f;
	spider->mCarry.mPos.z        = 0.0f;
	spider->mCarry._10.x         = 0.0f;
	spider->mCarry._10.y         = 0.0f;
	spider->mCarry._10.z         = 0.0f;
	spider->mCarry.mAngle        = 0.0f;
	spider->mCarry._24           = 0.0f;
	spider->mCarry.mState        = 0;
	spider->mCarry._2C           = 0;
	spider->mCarry.mFlags.mFlags = 0;
	spider->mCarry.mObj          = NULL;
	spider->mRadius              = 20.0f;
	spider->mSlotCarry.mObj      = NULL;
	spider->mSlot                = 255;
	Vec areaPos                  = { 0.0f, 0.0f, 0.0f };
	spider->mAreaPos             = areaPos;
	spider->mAreaWidth           = 0.0f;
	spider->mAreaDepth           = 0.0f;
	Vec startPos                 = { 0.0f, 0.0f, 0.0f };
	spider->mStartPos            = startPos;
	spider->_A98                 = 0.0f;
	spider->_B3C                 = 2;
	spider->_B40[0]              = 0;
	spider->_B40[1]              = 0;
	spider->_B40[2]              = 0;
	spider->_B40[3]              = 0;
	spider->_B50[0]              = 100;
	spider->_B50[1]              = 100;
	spider->_B50[2]              = 100;
	spider->_B80                 = 0;
	spider->_B7C                 = 0;
	spider->mEffect              = NULL;
	spider->mEffectId            = 0;
	spider->mEffectOn            = 0;
	for (int i = 0; i < 8; i++) {
		spider->mTies[i].mObj = NULL;
	}
	if (spider->mPoolIndex == 0) {
		ResetSpiderCoinBounces(&gSpiderCoinBounceUpdateContext);
	}
	spider->mHitState           = 0;
	spider->mHitTimer           = 0;
	spider->mDroppedSpiderIndex = -1;
	spider->mRumble             = 0;
	spider->mRumbleTimer        = 0;
}

/**
 * @note Address: 0x80121280
 * @note Size: 0x94
 */
BOOL XObjSpider::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		mFlags.mFlags &= ~0x04000000U;
		ResetSpiderState(this);
		ApplySpiderPlacement(this);
		_BD4 = 0.5f * gRefreshRate;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80121314
 * @note Size: 0x144
 */
void XObjSpider::ResetObjectState()
{
	XObj::ResetObjectState();
	mSfx.Init((WiiPlayerState*)this, gSpiderAnimSfxEvents);
	// the first spider collects the shared carry slot models, the first 16 objects with id 0x13C
	if (mPoolIndex == 0) {
		for (u32 i = 0; i < 16; ++i) {
			gSpiderCarryModels[i]     = NULL;
			gSpiderCarrySlotOwners[i] = -1;
		}
		gSpiderCarryFlag  = 0;
		gSpiderCarryCount = 0;
		s32 found         = 0;
		s32 count         = gGame.mXObjs.mObjectCount;
		XObj** output     = gSpiderCarryModels;
		for (s32 i = 0; i < count; ++i) {
			XObj* candidate = gGame.mXObjs.mObjects[i];
			if (candidate->mStatus.mFlags & 1ULL) {
				if (candidate->mCoord.mPlacement->mObjectId == 0x13C) {
					++found;
					*output++ = candidate;
					if (found >= 16)
						break;
				}
			}
		}
	}
}

/**
 * @note Address: 0x8012195C
 * @note Size: 0x14
 */
void XObjSpider::DrawObject()
{
	DrawWithView(NULL);
}

/**
 * @note Address: 0x80121970
 * @note Size: 0x20
 */
void XObjSpider::DrawLate()
{
	bool sideways = false;
	if (mWall == 3)
		sideways = true;
	DrawSpiderCarryModel(&mCarry, mSlot, sideways);
}

/**
 * @note Address: 0x80121990
 * @note Size: 0x4
 */
void XObjSpider::DrawGroundShadow(f32 opacity)
{
}

/**
 * @note Address: 0x80121B24
 * @note Size: 0x4
 */
void XObjSpider::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x80121B28
 * @note Size: 0x4
 */
void XObjSpider::DrawDebugStatus()
{
}

/**
 * @note Address: 0x801220D0
 * @note Size: 0xD0
 */
void XObjSpider::OnHitFromPosition(const Vec& from, f32 strength)
{
	if (mFlags.mFlags & 2)
		return;
	if (mFlags.mFlags & 0x10)
		return;
	if (mFlags.mFlags & 0x8000)
		return;
	// only the robot's blaster (attachment 2) at level 2 or up hurts it
	if (GetAttachmentCountById((AttachmentValues*)gGame.GetPlayer()->mAttachments, 2) < 2)
		return;
	XObj::OnHitFromPosition(from, strength);
	mFlags.mFlags |= 2;
	Vec pos      = { 0.0f, 0.0f, 0.0f };
	pos          = from;
	mHitPos      = pos;
	mHitStrength = strength;
}

/**
 * @note Address: 0x801221A0
 * @note Size: 0x18
 */
s32 XObjSpider::OnPlayerContact(s32 unused, s32 value)
{
	mFlags.mFlags |= 1;
	return value;
}

/**
 * @note Address: 0x8012359C
 * @note Size: 0x38
 */
void XObjSpider::State0(u32 frames)
{
	if (mPlacementParam0 == 11 && (mFlags.mFlags & 0x400000))
		SetState(4);
}

/**
 * @note Address: 0x80123778
 * @note Size: 0x4
 */
void XObjSpider::State2(u32 frames)
{
}

/**
 * @note Address: 0x80126E24
 * @note Size: 0x4
 */
void XObjSpider::State8(u32 frames)
{
}

/**
 * @note Address: 0x801275C8
 * @note Size: 0xC
 */
extern "C" u8 HasSpiderFinishedDeathAnimation(const XObjSpider* spider)
{
	return (spider->mFlags.mFlags & 0x80000) != 0;
}

/**
 * @note Address: 0x80128A9C
 * @note Size: 0xC4
 */
void XObjSpider::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
			mSfx.Update(GetAnimId(), GetAnimFrame(), volume);
		}
	}
}

/**
 * @note Address: 0x80128B60
 * @note Size: 0x3C
 */
void XObjSpider::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, frame, rate, blend, blendRate);
	mAnimStartFrame = frame;
}

/**
 * @note Address: 0x80128B9C
 * @note Size: 0xB8
 */
// on anims 31 and 32 the base matrix carries it: its place and scale come from there
void XObjSpider::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	if (pFuncObj->GetCallbackNodeID() == 0 && mBaseMtxPtr != NULL && (GetAnimId() == 31 || GetAnimId() == 32)) {
		mMoveTranslate.x = mBaseMtx[0][3];
		mMoveTranslate.y = mBaseMtx[1][3];
		mMoveTranslate.z = mBaseMtx[2][3];
		HSD_MtxGetScale(mBaseMtx, &mMoveScale);
	}
}
