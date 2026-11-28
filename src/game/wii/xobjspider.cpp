#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjflags.h"
#include "game/wii/cpad.h"
#include "game/wii/ceffect.h"
#include "game/attachmentvalues.h"
#include "game/gmath.h"
#include "game/wii/spiderplacement.h"
#include "game/stagedata.h"
#include "arithmetic.h"
#include <stdlib.h>

// the Wii file of the GameCube's xobjspider.cpp (retail 0x8012063C-0x80128C84): its functions run in the
// Wii's order, ~XObjSpider lives with the pool ctors and the coin and carry helpers take the work block

// one of the coins a spider knocks loose, it hops up and drops to mTarget
struct CoinBounce {
	XObj* mCoin; // _00
	Vec mPos;    // _04
	s32 mPhase;  // _10
	s32 mCount;  // _14
	Vec mTarget; // _18
};
STATIC_ASSERT(sizeof(CoinBounce) == 0x24);

extern CustomShadow gCustomShadow;

extern "C" {
void ResetSpiderState(XObjSpider* spider);
void ApplySpiderPlacement(XObjSpider* spider);
void ResetSpiderCoinBounces(void* unused);
void UpdateSpiderCoinBounces(void* unused, u32 frames);
void UpdateSpiderHitReaction(XObjSpider* spider, u32 frames);
void ExecuteSpiderCommand(XObjSpider* spider, u32 frames);
void LaunchSpiderShot(XObjSpider* spider);
void UpdateSpiderShot(XObjSpider* spider, u32 frames);
void UpdateSpiderAttachments(XObjSpider* spider, u32 frames);
void UpdateSpiderCarryModel(CarryInfo* carry, u8 slot, u32 frames, s32 mode);
void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim);
void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z);
s32 FindNearbyDroppedSpider(XObjSpider* spider);
bool HasNearbyDroppedSpider(XObjSpider* spider);
void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
u32 GetPlayerActionParameter(s32 index);
void ApproachHudValue(f32* value, f32* target, f32 step);
BOOL IsPlayerFlashingFromHit(WiiPlayerState* chr);
CVec GetStagePlacementScale(const XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);

extern s32 gRefreshRate;
extern AnimSfxEvent gSpiderAnimSfxEvents[];
extern XObj* gSpiderCarryModels[16];
extern XObj* gSmallSpiderEffectObjects[12];
extern s32 gSpiderCarrySlotOwners[16];
extern u32 gSpiderCoinBounceUpdateContext; // the coins' manager token, the callbacks never touch it
extern s32 gSpiderCarryCount;
extern u8 gSpiderCarryFlag;
extern CoinBounce gSpiderCoinBounces[11];
extern s32 gSpiderCoinBounceCount; // how many coins are out
// the states, by number
extern void (XObjSpider::*gSpiderStateHandlers[10])(u32 frames);
}

// puts a vector and its target at a place with nowhere to go
static inline void SetNow(XObjSpider* spider, const Vec& value)
{
	spider->mMoveTranslate.Set(value);
	spider->mMoveTarget0.Set(value);
}

// takes the first of the slot models nobody has
static inline void TakeSlot(XObjSpider* spider)
{
	for (int i = 0; i < 16; i++) {
		if (gSpiderCarrySlotOwners[i] == -1) {
			spider->mSlotCarry.mObj   = gSpiderCarryModels[i];
			spider->mSlot             = i;
			gSpiderCarrySlotOwners[i] = spider->mPoolIndex;
			return;
		}
	}
}

static inline void HitCarried(XObjSpider* spider)
{
	if (!(spider->mCarry.mFlags.mFlags & 2)) {
		XObj* model = gSpiderCarryModels[spider->mSlot];
		if (model != NULL && spider->mCarry.mState == 2) {
			f32 frame = model->GetAnimFrame();
			f32 end   = model->GetAnimEndFrame();
			model->SetAnim(2, TRUE, end - frame, 1.0f, 0.0f, 15.0f);
			spider->mCarry.mState++;
			spider->mCarry.mFlags.mFlags |= 2;
		}
	}
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
// NONMATCHING: regalloc (retail zero in r9, ours r10) and retail reads mPoolIndex after the tie stores
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

static inline void PlaceWallSpider(XObjSpider* spider)
{
	spider->SetState(8);
	spider->mFlags.mFlags |= 0x8000;
	spider->mAreaPos.x = -52.0f;
	spider->mAreaPos.y = 0.0f;
	spider->mAreaPos.z = 0.0f;
	spider->mAreaWidth = 2.0f * nw4r::math::FAbs(spider->mAreaPos.x);
	spider->mAreaDepth = 0.0f;
	spider->mWall      = 3;
	spider->mWallPos.z = spider->mMoveTranslate.z;
	TakeSlot(spider);
}

/**
 * @note Address: 0x80120A08
 * @note Size: 0x878
 */
// NONMATCHING: the stage02 area math schedules its loads and stores in another order
extern "C" void ApplySpiderPlacement(XObjSpider* spider)
{
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord).z;
	spider->mHome.x          = PlacementPosition(spider->mCoord).x;
	spider->mHome.y          = PlacementPosition(spider->mCoord).y;
	spider->mHome.z          = PlacementPosition(spider->mCoord).z;
	spider->mDir             = PlacementRotation(spider->mCoord).y;
	spider->mRadius          = 20.0f * PlacementScale(spider->mCoord).x;
	switch (spider->mPlacementParam0) {
	case 0: // held still until event command 7 lets it go
		spider->SetState(0);
		spider->mFlags.mFlags |= 0x08000000;
		TakeSlot(spider);
		break;
	case 1: // hangs from the ceiling and drops down when the robot comes near
		spider->SetState(6);
		spider->mFlags.mFlags |= 0x40;
		spider->mFlags.mFlags |= 0x400;
		spider->mFlags.mFlags |= 0x8000;
		if (IsStage("stage02")) {
			spider->mAreaPos.x = -188.0f;
			spider->mAreaPos.y = 1.0f;
			spider->mAreaPos.z = 63.0f;
			spider->mAreaWidth = 221.0f;
			spider->mAreaDepth = 216.0f;
			// shrinks the area to 90% around its center
			f32 centerX = spider->mAreaPos.x + spider->mAreaWidth * 0.5f;
			f32 centerZ = spider->mAreaPos.z + spider->mAreaDepth * 0.5f;
			spider->mAreaWidth *= 0.9f;
			spider->mAreaDepth *= 0.9f;
			spider->mAreaPos.x = centerX - spider->mAreaWidth * 0.5f;
			spider->mAreaPos.z = centerZ - spider->mAreaDepth * 0.5f;
		} else {
			spider->mAreaPos.x = -127.0f;
			spider->mAreaPos.y = 0.0f;
			spider->mAreaPos.z = -127.0f;
			spider->mAreaWidth = 254.0f;
			spider->mAreaDepth = 254.0f;
		}
		break;
	case 2:
		PlaceWallSpider(spider);
		break;
	case 3:
		PlaceWallSpider(spider);
		spider->mFlags.mFlags |= 0x02000000;
		spider->mDir = PlacementRotation(spider->mCoord).z;
		break;
	default: // fights the robot inside a fixed area
		spider->SetState(4);
		spider->mFlags.mFlags |= 0x400;
		spider->mAreaPos.x = -50.0f;
		spider->mAreaPos.y = 0.0f;
		spider->mAreaPos.z = -58.0f;
		spider->mAreaWidth = 80.0f;
		spider->mAreaDepth = 134.0f;
		break;
	}
	// a hanging one shows its thread and starts mPlacementParam1 above its home
	if (spider->mFlags.Check(0x40)) {
		spider->mFlags.mFlags |= 0x80;
		spider->mFlags.mFlags |= 0x100;
		spider->mMoveTranslate.y = spider->mPlacementParam1 + spider->mHome.y;
	}
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
		mSoundDelay = 0.5f * gRefreshRate;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80121314
 * @note Size: 0x144
 */
// NONMATCHING: regalloc, the model, gGame and owner table bases sit one register over
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
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
			XObj* candidate = gGame.mXObjs.mObjects[i];
			if (candidate->mStatus.mFlags & 1ULL) {
				if (candidate->mCoord.mPlacement->mObjectId == 0x13C) {
					gSpiderCarryModels[found++] = candidate;
					if (found >= 16)
						break;
				}
			}
		}
	}
}

/**
 * @note Address: 0x80121458
 * @note Size: 0x504
 */
// NONMATCHING: only the CPad vtable reference differs, retail's copy (0x802E6DF8) has no name: naming it collides
// with cpaddtor.o's in the link
void XObjSpider::UpdateObject(u32 frames)
{
	if (mSoundDelay > 0) {
		mSoundDelay -= frames;
		if (mSoundDelay < 0) {
			mSoundDelay = 0;
		}
	}

	// set to 1 when a charge hits the robot, rumbles the remote for 24 frames if rumble is turned on
	switch (mRumble) {
	case 0:
		break;
	case 1:
		if (gGame.GetStatusFlags(0x800000000ULL)) { // the rumble option
			WiiRumble& rumble = gWiiPads[0].mRumble[0];
			rumble.flags |= RumbleRunning;
			rumble.remaining = 24;
			mRumbleTimer     = 24 / frames;
			mRumble++;
		} else {
			mRumble = 0;
		}
		break;
	case 2:
		if (--mRumbleTimer < 0) {
			mRumble = 0;
		}
		break;
	}

	if (gGame.GetStatusFlags(0x400)) { // the robot's menu is open
		return;
	}
	if (mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderCoinBounceUpdateContext, frames);
	}
	// a spider that isn't drawn stands still, unless it is taking a hit (state 1)
	if (!(mStatus.mFlags & 0x10000) && mState != 1) {
		return;
	}
	if (mSoundDelay == 0) {
		UpdateAnimationSounds();
	}
	UpdateSpiderHitReaction(this, frames);
	if (!(mFlags.mFlags & 0x08000000)) {
		(this->*gSpiderStateHandlers[mState])(frames);
		CVec move;
		if (mWall == 4) {
			mMoveTranslate.x = mWallPos.x;
			move.y           = mSpeed * (f32)sin(mDir);
			move.z           = mSpeed * (f32)cos(mDir);
			move.x           = 0.0f;
			mMoveRotate.z    = -HALF_PI;
			SetRotateY(mDir, mTurnSpeed);
		} else if (mWall == 5) {
			mMoveTranslate.x = mWallPos.x;
			move.y           = mSpeed * (f32)sin(mDir);
			move.z           = mSpeed * (f32)cos(mDir);
			move.x           = 0.0f;
			mMoveRotate.z    = HALF_PI;
			SetRotateY(mDir, mTurnSpeed);
		} else if (mWall == 2) {
			mMoveTranslate.z = mWallPos.z;
			move.y           = mSpeed * (f32)sin(mDir);
			move.x           = mSpeed * (f32)cos(mDir);
			move.z           = 0.0f;
			mMoveRotate.x    = -HALF_PI;
			mMoveRotate.z    = mDir - HALF_PI;
		} else if (mWall == 3) {
			mMoveTranslate.z = mWallPos.z;
			move.y           = mSpeed * (f32)sin(mDir);
			move.x           = mSpeed * (f32)cos(mDir);
			move.z           = 0.0f;
			mMoveRotate.x    = HALF_PI;
			mMoveRotate.z    = HALF_PI + mDir;
		} else if (mFlags.mFlags & 0x100) {
			move.x = 0.0f;
			move.z = 0.0f;
			move.y = mSpeed;
			SetRotateY(mDir, mTurnSpeed);
		} else {
			if (mFlags.mFlags & 8) {
				mMoveTranslate.y += mFallSpeed;
			} else {
				mMoveTranslate.y = mHome.y;
			}
			move.x = mSpeed * (f32)sin(mDir);
			move.z = mSpeed * (f32)cos(mDir);
			move.y = 0.0f;
			SetRotateY(mDir, mTurnSpeed);
		}
		SetNow(this, mMoveTranslate + move * (f32)frames);
		SetPos(mMoveTranslate);
	}
	mAnimStartFrame = GetAnimFrame();
	XObj::UpdateObject(frames);
	UpdateSpiderCarryModel(&mCarry, mSlot, frames, 0);
	UpdateSpiderAttachments(this, frames);
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
 * @note Address: 0x80121994
 * @note Size: 0x190
 */
void XObjSpider::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 8ULL))
		return;
	switch (mPlacementParam0) {
	case 3:
	case 4:
	case 5:
	case 7:
	case 8:
	case 9:
		return;
	}
	Vec pos  = *origin;
	Vec size = mMoveScale * (f32)(mAttr.mObjData != NULL ? mAttr.mObjData->mShadowSize : 0) * 0.6f * mRadius;
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	gCustomShadow.Draw(pos, size, (u32)(170.0f * opacity));
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
 * @note Address: 0x80121B2C
 * @note Size: 0x1EC
 */
// NONMATCHING: frame and calls match; the render-state setup still uses different scratch GPRs
void XObjSpider::DrawObjectEffects()
{
	if (mFlags.mFlags & 0x80) {
		u32 color = (u32)(90.0f * mThreadAlpha) | 0xFFFFFF00;
		CVec pos  = GetJointPos(FindResNode("obj25"));
		Vec end;
		end.x = pos.x;
		end.y = 300.0f;
		end.z = pos.z;
		if (gGame.mGs.mCurRenderState != NULL) {
			gGame.mGs.mCurRenderState->Apply();
		}
		if (gGame.mGs.mCurTevStage != NULL) {
			gGame.mGs.mCurTevStage->Apply();
		}
		if (gGame.mGs.mCurChannel != NULL) {
			gGame.mGs.mCurChannel->Apply();
		}
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		gGame.mGs.mCurRenderState->SetLineWidth(14, GX_TO_ONE);
		DrawGsLineSegment(&gGame.mGs, NULL, pos.x, pos.y, pos.z, color, end.x, end.y, end.z, color);
		gGame.mGs.mCurRenderState->SetLineWidth(12, GX_TO_ONE);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	}
}

/**
 * @note Address: 0x80121D18
 * @note Size: 0x3B8
 */
// the event commands, 100 ties another object to one of its nodes or lets go of all
// of them, anything else is kept for ExecuteSpiderCommand
// NONMATCHING: call list and size match; register assignment in the event-id and position-component paths differs
s32 XObjSpider::HandleEventCommand(s32 argc, s32* argv)
{
	switch (argv[0]) {
	case 100: {
		const s32 objectId = argv[1];
		if (objectId == 0) {
			mFlags.mFlags &= ~0x20000000;
			for (u32 i = 0; i < 8; i++) {
				if (mTies[i].mObj != NULL) {
					mTies[i].mObj->ClearBaseMtx();
					mTies[i].mObj = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mTies[i].mObj != NULL && i < 8; i++) { }
			if (i < 8) {
				mFlags.mFlags |= 0x20000000;
				if (argv[1] >= 10000) {
					mTies[i].mObj = gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, argv[1]) : NULL;
				} else {
					mTies[i].mObj = gGame.mXObjs.mObjects[(s16)argv[1]];
				}
				s32 path[8];
				s32 depth = argv[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = argv[3 + j];
				}
				mTies[i].mJoint = vf284(depth, path);
				Mtx mtx;
				f32 z = mTies[i].mObj->GetPos()->z;
				f32 y = mTies[i].mObj->GetPos()->y;
				f32 x = mTies[i].mObj->GetPos()->x;
				PSMTXTrans(mtx, x, y, z);
				PSMTXConcat(GetWorldMtx(mTies[i].mJoint), mtx, mTies[i].mMtx);
				mTies[i].mObj->SetBaseMtx(mTies[i].mMtx);
			}
		}
		break;
	}
	default:
		if (argc < 1) {
			return 0;
		}
		mCommand = argv[0];
		if (argc < 2) {
			mArgs[0] = 0;
		} else {
			mArgs[0] = argv[1];
		}
		if (argc < 3) {
			mArgs[1] = 0;
		} else {
			mArgs[1] = argv[2];
		}
		if (argc < 4) {
			mArgs[2] = 0;
		} else {
			mArgs[2] = argv[3];
		}
		if (argc < 5) {
			mArgs[3] = 0;
		} else {
			mArgs[3] = argv[4];
		}
		if (argc < 6) {
			mArgs[4] = 0;
		} else {
			mArgs[4] = argv[5];
		}
		break;
	}
	return 0;
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
	if (GetAttachmentCountById(&gGame.GetPlayer()->mAttachments, 2) < 2)
		return;
	XObj::OnHitFromPosition(from, strength);
	mFlags.mFlags |= 2;
	Vec pos      = { from.x, from.y, from.z };
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
 * @note Address: 0x801221B8
 * @note Size: 0x5E4
 */
// NONMATCHING: retail keeps adjusted player and shot vectors in extra stack temporaries; ours uses f28 and a 0x70 frame
extern "C" void UpdateSpiderHitReaction(XObjSpider* spider, u32 frames)
{
	switch (spider->mHitState) {
	case 0:
		if ((spider->mFlags.mFlags & 2) && !(spider->mFlags.mFlags & 0x10)) {
			StopWiiSfx(&gGame.mSound, 0x197, 0.0f);
			spider->mFlags.mFlags &= ~2;
			if (spider->mHitStrength >= (s32)GetPlayerActionParameter(0x1F) / 100.0f) {
				spider->mHits -= 5;
				if (spider->mHits <= 0) {
					spider->mHits = 0;
				}
			} else if (--spider->mHits <= 0) {
				spider->mHits = 0;
			}
			if (spider->mHits <= 0) {
				spider->mSpeed = 0.0f;
				spider->SetState(3);
				spider->mFlags.mFlags |= 0x10;
			} else {
				spider->mHitState = 1;
				spider->mHitTimer = 0;
				spider->mFlags.mFlags |= 0x10;
			}
		}
		break;
	case 1:
		if (spider->mCounter % 30 < 15) {
			spider->SetColor(1, 0x80, 0, 0, 0.0f);
		} else {
			spider->SetColor(1, 0, 0, 0, 0.0f);
		}
		spider->mHitTimer++;
		if (spider->mHitTimer > 20) {
			spider->mFlags.mFlags &= ~0x10;
			spider->SetColor(1, 0, 0, 0, 0.0f);
			spider->mHitTimer = 0;
			spider->mHitState = 0;
		}
		break;
	}
	Vec chr = *gGame.GetPlayer()->GetPos();
	if (spider->mWall == 3) {
		f32 x = chr.x - spider->mShotPosition.x;
		f32 y = 8.0f + chr.y - spider->mShotPosition.y;
		f32 z = chr.z - chr.z;
		if (FSqrt(x * x + y * y + z * z) < 8.0f && (spider->mFlags.mFlags & 0x800)) {
			gGame.GetPlayer()->OnHitFromPosition(chr, 7.0f);
			HitCarried(spider);
		}
	} else {
		f32 x    = chr.x - spider->mShotPosition.x;
		f32 y    = 0.0f;
		f32 z    = chr.z - spider->mShotPosition.z;
		f32 chrY = chr.y;
		if (FSqrt(x * x + y * y + z * z) < 2.0f && (spider->mFlags.mFlags & 0x800)
		    && nw4r::math::FAbs(chrY - spider->mMoveTranslate.y) < 4.0f) {
			gGame.GetPlayer()->OnHitFromPosition(chr, 7.0f);
			HitCarried(spider);
		}
	}
	ExecuteSpiderCommand(spider, frames);
	UpdateSpiderShot(spider, frames);
}

/**
 * @note Address: 0x8012279C
 * @note Size: 0x39C
 */
extern "C" void ExecuteSpiderCommand(XObjSpider* spider, u32 frames)
{
	switch (spider->mCommand) {
	case 0:
		break;
	case 1:
		spider->SetState(3);
		spider->mCommand = 0;
		break;
	case 2:
		if (spider->mFlags.mFlags & 0x10000)
			gGame.mEvt.mSelectResult = 1;
		else
			gGame.mEvt.mSelectResult = 0;
		spider->mCommand = 0;
		break;
	case 3:
		if (spider->mFlags.mFlags & 0x80000)
			gGame.mEvt.mSelectResult = 1;
		else
			gGame.mEvt.mSelectResult = 0;
		spider->mCommand = 0;
		break;
	case 4:
		spider->mFlags.mFlags |= 0x00200000;
		spider->mCommand = 0;
		break;
	case 5:
		spider->mFlags.mFlags |= 0x00400000;
		spider->mCommand = 0;
		break;
	case 6:
		spider->mStatus.mFlags |= 0x00100000;
		spider->mCommand = 0;
		break;
	case 7:
		if (spider->mArgs[0] == 1)
			spider->mFlags.mFlags &= ~0x08000000;
		else
			spider->mFlags.mFlags |= 0x08000000;
		spider->mCommand = 0;
		spider->mArgs[0] = 0;
		break;
	case 8:
		ApplySpiderPlacement(spider);
		spider->mCommand = 0;
		break;
	case 9:
		spider->mHome.x          = spider->mArgs[0] / 100.0f;
		spider->mHome.y          = spider->mArgs[1] / 100.0f;
		spider->mHome.z          = spider->mArgs[2] / 100.0f;
		spider->mStartPos.x      = spider->mHome.x;
		spider->mStartPos.y      = spider->mHome.y;
		spider->mStartPos.z      = spider->mHome.z;
		spider->mMoveTranslate.x = spider->mHome.x;
		spider->mMoveTranslate.y = (f32)spider->mPlacementParam1 + spider->mHome.y;
		spider->mMoveTranslate.z = spider->mHome.z;
		spider->mMoveTarget0.Set(spider->mMoveTranslate.x, spider->mMoveTranslate.y, spider->mMoveTranslate.z);
		spider->mCommand = 0;
		break;
	case 10:
		spider->mAreaPos.x = spider->mArgs[0] / 100.0f;
		spider->mAreaPos.y = spider->mArgs[1] / 100.0f;
		spider->mAreaPos.z = spider->mArgs[2] / 100.0f;
		spider->mAreaWidth = spider->mArgs[3];
		spider->mAreaDepth = spider->mArgs[4];
		spider->mCommand   = 0;
		break;
	case 11:
		spider->mCommand = 0;
		break;
	case 12:
		spider->mCommand = 0;
		break;
	case 13:
		spider->mFlags.mFlags |= 0x10000000;
		spider->mCommand = 0;
		break;
	case 14:
		spider->mEffectId = spider->mArgs[0];
		spider->mEffectOn = 1;
		spider->mCommand  = 0;
		break;
	case 15:
		spider->mEffectOn = 0;
		spider->mEffectId = -1;
		if (spider->mEffect != NULL)
			spider->mEffect->Release();
		spider->mCommand = 0;
		break;
	case 16:
		spider->mCommand = 0;
		break;
	case 17:
		spider->mCommand = 0;
		break;
	case 18:
		spider->mCommand = 0;
		break;
	case 19:
		spider->SetState(5);
		spider->mCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x80122B38
 * @note Size: 0x3A0
 */
extern "C" void UpdateSpiderAttachments(XObjSpider* spider, u32 frames)
{
	switch (spider->mEffectOn) {
	case 0:
		break;
	case 1:
		if (!((XObjActorTao*)gGame.mXObjs.mPools[14].mObjects)->IsFree()) {
			break;
		}
		if (spider->mEffect != NULL) {
			spider->mEffect->Release();
		}
		if (spider->mEffectId >= 0) {
			spider->mEffect
			    = SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(spider->mEffectId)), 0, TRUE);
		} else {
			spider->mEffect = NULL;
		}
		spider->mEffectOn++;
	case 2:
		if (spider->mEffect != NULL) {
			if (spider->mEffect->mStatus.mFlags & 0x0040000000000000ULL) {
				break;
			}
			spider->mEffect->mStatus.mFlags |= 0x10000;
			gGame.SetStatusFlags(0x1000);
		}
		spider->mEffectOn++;
	case 3:
		if (spider->mEffectId < 0 || spider->mEffect == NULL) {
			break;
		}
		spider->mEffect->SetScale(1.0f, 1.0f, 1.0f);
		PSMTXCopy(spider->GetWorldMtx(spider->FindResNode("obj25")), spider->mEffectMtx);
		Vec scale;
		HSD_MtxGetScale(spider->mEffectMtx, &scale);
		scale.x = GetStagePlacementScale(spider->mEffect).x / scale.x;
		scale.y = GetStagePlacementScale(spider->mEffect).y / scale.y;
		scale.z = GetStagePlacementScale(spider->mEffect).z / scale.z;
		Mtx scaleMtx;
		PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
		Mtx up;
		Mtx mtx;
		PSMTXConcat(spider->mEffectMtx, scaleMtx, mtx);
		PSMTXTrans(up, 0.0f, 5.0f, 0.0f);
		PSMTXConcat(mtx, up, mtx);
		spider->mEffect->SetBaseMtx(mtx);
		spider->mEffect->mStatus.mFlags &= ~0x0200000000000000ULL;
		break;
	}
	if (spider->mFlags.mFlags & 0x20000000) {
		for (u32 i = 0; i < 8; i++) {
			if (spider->mTies[i].mObj != NULL) {
				Mtx mtx;
				f32 y, z;
				z     = spider->mTies[i].mObj->GetPos()->z;
				y     = spider->mTies[i].mObj->GetPos()->y;
				f32 x = spider->mTies[i].mObj->GetPos()->x;
				PSMTXTrans(mtx, x, y, z);
				PSMTXConcat(spider->GetWorldMtx(spider->mTies[i].mJoint), mtx, spider->mTies[i].mMtx);
				spider->mTies[i].mObj->SetBaseMtx(spider->mTies[i].mMtx);
			}
		}
	}
}

/**
 * @note Address: 0x80122ED8
 * @note Size: 0x230
 */
// the GameCube's LaunchSpiderShot
// NONMATCHING: variant 12/13 and the carried position schedule their float loads around sin/cos and atan2 differently
extern "C" void LaunchSpiderShot(XObjSpider* spider)
{
	CVec pos = spider->GetJointPos(spider->FindResNode("cyl1"));
	switch (spider->mPlacementParam0) {
	case 0:
		spider->mShotTo.x = pos.x + 72.0f * (f32)sin(spider->mDir);
		spider->mShotTo.y = pos.y;
		spider->mShotTo.z = pos.z + 72.0f * (f32)cos(spider->mDir);
		break;
	case 12:
	case 13:
		spider->mShotTo.x = pos.x + 72.0f * (f32)sin(HALF_PI + spider->mDir);
		spider->mShotTo.y = pos.y + 72.0f * (f32)cos(HALF_PI + spider->mDir);
		spider->mShotTo.z = pos.z;
		break;
	default:
		Vec chr         = *gGame.GetPlayer()->GetPos();
		spider->mShotTo = chr;
		break;
	}
	spider->mShotState = 1;
	spider->_A74       = 10;
	XObj* self         = gGame.mXObjs.mPools[17].Get(spider->mPoolIndex);
	CVec to(spider->mShotTo);
	spider->mCarry.mFlags.mFlags |= 1;
	spider->mCarry.mPos.x = pos.x;
	spider->mCarry.mPos.y = pos.y;
	spider->mCarry.mPos.z = pos.z;
	spider->mCarry.mAngle = atan2(to.y - spider->mCarry.mPos.y, to.x - spider->mCarry.mPos.x);
	spider->mCarry.mObj   = self;
	spider->mCarry.mState = 1;
	PlayWiiSoundEffect(&gGame.mSound, 0x197, 1.0f, 0.0f);
}

static inline void StepSpiderShotDistance(XObjSpider* spider, const CVec& origin)
{
	spider->mShotRange  = 72.0f;
	spider->mShotOrigin = origin;
	spider->mShotProgress += 0.06f;
	if (spider->mShotProgress >= 1.0f)
		spider->mShotProgress = 1.0f;
}

static inline void FinishSpiderShotStep(XObjSpider* spider)
{
	spider->mFlags.mFlags |= 0x800;
	if (spider->mShotProgress >= 1.0f) {
		spider->mShotState    = 0;
		spider->mShotProgress = 0.0f;
		spider->mFlags.mFlags &= ~0x800;
	}
}

/**
 * @note Address: 0x80123108
 * @note Size: 0x494
 */
// NONMATCHING: the wall branch assigns different saved float registers to dx/dy/dz and the two FSqrt results
extern "C" void UpdateSpiderShot(XObjSpider* spider, u32 frames)
{
	switch (spider->mShotState) {
	case 0:
		break;
	case 1:
		if (spider->mWall == 3) {
			{
				CVec origin = spider->GetJointPos(spider->FindResNode("cyl1"));
				StepSpiderShotDistance(spider, origin);
				f32 dx = spider->mShotTo.x - origin.x;
				f32 dz = spider->mShotTo.z - origin.z;

				f32 dy                  = spider->mShotTo.y - origin.y;
				f32 pitch               = atan2(FSqrt(dx * dx + dz * dz), FSqrt(dy * dy));
				f32 angle               = dx > 0.0f ? pitch - HALF_PI : -(HALF_PI + pitch);
				Vec shot                = origin;
				shot.x                  = origin.x + spider->mShotProgress * (spider->mShotRange * (f32)cos(angle));
				shot.y                  = origin.y + spider->mShotProgress * (spider->mShotRange * (f32)sin(angle));
				spider->mShotPosition.x = shot.x;
				spider->mShotPosition.y = shot.y;
				spider->mShotPosition.z = shot.z;
				CVec from(spider->mShotOrigin), to(spider->mShotPosition);
				spider->mCarry.mPos   = from;
				spider->mCarry.mAngle = atan2(to.y - spider->mCarry.mPos.y, to.x - spider->mCarry.mPos.x);
			}
			FinishSpiderShotStep(spider);
		} else {
			{
				CVec origin = spider->GetJointPos(spider->FindResNode("cyl1"));
				StepSpiderShotDistance(spider, origin);
				f32 dx = spider->mShotTo.x - origin.x;
				f32 dz = spider->mShotTo.z - origin.z;

				f32 angle               = atan2(dx, dz);
				Vec shot                = origin;
				shot.x                  = origin.x + spider->mShotProgress * (spider->mShotRange * (f32)sin(angle));
				shot.z                  = origin.z + spider->mShotProgress * (spider->mShotRange * (f32)cos(angle));
				spider->mShotPosition.x = shot.x;
				spider->mShotPosition.y = 8.0f;
				spider->mShotPosition.z = shot.z;
				CVec from(spider->mShotOrigin), to(spider->mShotPosition);
				spider->mCarry.mPos   = from;
				spider->mCarry.mAngle = atan2(to.x - spider->mCarry.mPos.x, to.z - spider->mCarry.mPos.z);
			}
			FinishSpiderShotStep(spider);
		}
		break;
	case 2:
		break;
	}
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
 * @note Address: 0x801235D4
 * @note Size: 0x1A4
 */
// hit, stops, then plays anim 21 and goes on, or dies when it has no hits left
void XObjSpider::State1(u32 frames)
{
	switch (mPhase) {
	case 0:
		PlayWiiSoundEffect(&gGame.mSound, 0x196, 1.0f, 0.0f);
		mFlags.mFlags &= ~0x800U;
		mFlags.mFlags |= 0x10;
		mSpeed = 0.0f;
		if (mHits <= 0) {
			mFlags.mFlags |= 0x10000;
			SetState(3);
		} else {
			SetAnim(21, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			++mPhase;
		}
		break;
	case 1:
		f32 current = GetAnimFrame();
		if (GetAnimEndFrame() - (f32)frames <= current) {
			mFlags.mFlags &= ~0x10U;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			SetState(4);
		}
		break;
	}
}

/**
 * @note Address: 0x80123778
 * @note Size: 0x4
 */
void XObjSpider::State2(u32 frames)
{
}

/**
 * @note Address: 0x8012377C
 * @note Size: 0x354
 */
// NONMATCHING: retail hands StartScreenShake float literals (lfs) where the Wii prototype in game/cscrn.h takes
// doubles, so ours loads double literals
// dies, blinks, stops its sound, lets go of its slot, shakes the screen, then leaves
// one of the coins where it was
void XObjSpider::State3(u32 frames)
{
	mCounter++;
	if (mCounter % 30 < 15) {
		SetColor(1, 0, 0x80, 0, 0.0f);
	} else {
		SetColor(1, 0, 0, 0, 0.0f);
	}
	Vec pos;
	switch (mPhase) {
	case 0:
		StopWiiSfx(&gGame.mSound, 0x19F, 0.0f);
		SetAnim(0x19, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (mSlot != 255) {
			if (mSlotCarry.mObj != NULL) {
				mSlotCarry.mObj->mStatus.mFlags &= ~0x10000ULL;
			}
			gSpiderCarrySlotOwners[mSlot] = -1;
			mSlotCarry.mObj               = NULL;
			mSlot                         = 255;
		}
		mWait = 44;
		mPhase++;
		break;
	case 1:
		mWait -= frames;
		if (mWait < 0) {
			ActivateWaitingKind43AActorsAtPosition(mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
			if (!gGame.GetStatusFlags(0x100)) {
				StartScreenShake(&gGame.mQuake, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f);
			}
			mPhase++;
		}
		break;
	case 2:
		f32 frame = GetAnimFrame();
		if (GetAnimEndFrame() - frames <= frame) {
			mPhase++;
			mCounter = 0;
			mStatus.mFlags &= ~0x10000ULL;
			mFlags.mFlags |= 0x80000;
			mFlags.mFlags &= ~0x100000;
			pos.x = mMoveTranslate.x;
			pos.y = mMoveTranslate.y;
			pos.z = mMoveTranslate.z;
			s32 anim;
			if (rand() % 2 > 0) {
				anim = 2;
			} else if (rand() % 4 > 1) {
				anim = 1;
			} else {
				anim = 0;
			}
			Vec target;
			target.x = mHome.x;
			target.y = 0.0f;
			target.z = mHome.z;
			SpawnSpiderBounceCoin(&pos, &target, anim);
		}
		break;
	case 3:
		break;
	}
}

static inline int CountDroppedSpiders()
{
	XObjSpider* other;
	s32 dropped = 0;
	for (int i = 0; i < 16; i++) {
		other = (XObjSpider*)gGame.mXObjs.mPools[17].Get(i);
		if ((other->mStatus.mFlags & 1) && (other->mFlags.mFlags & 0x100000)) {
			dropped++;
		}
	}
	return dropped;
}

static inline f32 DistXZ(const Vec* first, const Vec* second)
{
	f32 x = first->x - second->x;
	f32 y = 0.0f;
	f32 z = first->z - second->z;
	return FSqrt(x * x + y * y + z * z);
}

static inline f32 GetSpiderDistanceXZ(Vec* first, Vec* second)
{
	return DistXZ(first, second);
}

static inline bool IsSpiderInsideMovementArea(XObjSpider* spider)
{
	f32 x     = spider->mMoveTranslate.x;
	f32 z     = spider->mMoveTranslate.z;
	f32 left  = spider->mAreaPos.x + spider->mRadius;
	f32 right = (spider->mAreaPos.x + spider->mAreaWidth) - spider->mRadius;
	f32 near  = spider->mAreaPos.z + spider->mRadius;
	f32 far   = (spider->mAreaPos.z + spider->mAreaDepth) - spider->mRadius;
	if (left > x || right < x || near > z || far < z)
		return FALSE;
	return TRUE;
}

static inline void PlaySpiderVariantSound(XObjSpider* spider)
{
	switch (spider->mPoolIndex % 5) {
	case 0:
		PlayWiiSoundEffect(&gGame.mSound, 0x198, 1.0f, 0.0f);
		break;
	case 1:
		PlayWiiSoundEffect(&gGame.mSound, 0x199, 1.0f, 0.0f);
		break;
	case 2:
		PlayWiiSoundEffect(&gGame.mSound, 0x19A, 1.0f, 0.0f);
		break;
	case 3:
		PlayWiiSoundEffect(&gGame.mSound, 0x19B, 1.0f, 0.0f);
		break;
	case 4:
		PlayWiiSoundEffect(&gGame.mSound, 0x19C, 1.0f, 0.0f);
		break;
	}
}

static inline void CombatAnim(XObjSpider* spider, s32 anim, BOOL restart = FALSE, f32 rate = 1.0f)
{
	spider->SetAnim(anim, restart, 0.0f, rate, 0.0f, 15.0f);
}

#define SPIDER_AT_FRAME(spider, mark)                                            \
	(((mark) <= (spider)->GetAnimFrame() && (mark) >= (spider)->mAnimStartFrame) \
	 || ((mark) <= (spider)->GetAnimFrame() && (spider)->mAnimStartFrame > (spider)->GetAnimFrame()))

static inline void CombatFootsteps(XObjSpider* spider)
{
	if (SPIDER_AT_FRAME(spider, 1.0f))
		PlaySpiderVariantSound(spider);
	else if (SPIDER_AT_FRAME(spider, 15.0f))
		PlaySpiderVariantSound(spider);
}

static inline bool SpiderInView(XObjSpider* spider)
{
	f32 radius = spider->GetFloorXZExtent() / 2;
	if (radius < 1.0f)
		radius = 1.0f;
	f32 height = spider->GetCollisionHeight();
	if (height < 1.0f)
		height = 1.0f;
	f32 width, screenHeight;
	return spider->ProjectVisibleBounds(&width, &screenHeight, NULL, radius, height) ? true : false;
}

static inline f32 HeadTo(XObjSpider* spider, const Vec& to)
{
	spider->mDir = atan2(to.x - spider->mMoveTranslate.x, to.z - spider->mMoveTranslate.z);
	f32 x        = to.x - spider->mMoveTranslate.x;
	f32 y        = to.y - spider->mMoveTranslate.y;
	f32 z        = to.z - spider->mMoveTranslate.z;
	return FSqrt(x * x + y * y + z * z);
}

static inline f32 CombatHeading(XObjSpider* spider, const Vec& pos)
{
	return atan2(pos.x - spider->mMoveTranslate.x, pos.z - spider->mMoveTranslate.z);
}

static inline void RetreatTarget(XObjSpider* spider)
{
	spider->mTarget.x = spider->mMoveTranslate.x + 48.0f * (f32)sin(PI + spider->mDir);
	spider->mTarget.y = spider->mMoveTranslate.y;
	spider->mTarget.z = spider->mMoveTranslate.z + 48.0f * (f32)cos(PI + spider->mDir);
}

static inline void EndSpiderCombatTurn(XObjSpider* spider, u32 frames)
{
	if (spider->GetAnimEndFrame() - frames <= spider->GetAnimFrame()) {
		spider->mDir = spider->mSavedDir;
		CombatAnim(spider, 15);
		spider->mFacing    = spider->mDir;
		spider->mTurnSpeed = .1f;
		spider->mPhase     = 1;
	}
}

/**
 * @note Address: 0x80123AD0
 * @note Size: 0x2350
 */
// NONMATCHING: the call sequence, CFG and frame match; inlined distance/combat helpers still use different saved FPRs and load schedules
void XObjSpider::State4(u32 frames)
{
	Vec pos = *gGame.GetPlayer()->GetPos();
	CVec chr(pos);
	f32 dist  = GetSpiderDistanceXZ(&chr, &mMoveTranslate);
	u8* event = gGame.mEvt.mData.mScript;
	if (event) {
		SetState(7);
		return;
	}
	if (((mFlags.mFlags & 0x400) || event) && !IsSpiderInsideMovementArea(this)) {
		SetState(7);
		return;
	}
	switch (mPhase) {
	case 0:
		CombatAnim(this, 1);
		mSpeed = 0.0f;
		if (dist <= 120.0f) {
			mFacing  = mDir;
			mCounter = (rand() % 30 + 30) / frames;
			++mPhase;
			_9B4 = mFacing;
		}
		break;
	case 1:
		if (--mCounter < 0) {
			mPhase = 40;
			break;
		}
		mDir = CombatHeading(this, chr);
		if (mFacing - mDir > PI)
			mDir = mDir + TWO_PI;
		else if (mFacing - mDir < -PI)
			mDir = mDir - TWO_PI;
		ApproachHudValue(&mFacing, &mDir, 0.018f);
		if (mDir > mFacing) {
			if (GetAnimEndFrame() - frames <= GetAnimFrame())
				CombatAnim(this, 6, TRUE);
			else
				CombatAnim(this, 6);
		} else if (mDir < mFacing) {
			if (GetAnimEndFrame() - frames <= GetAnimFrame())
				CombatAnim(this, 7, TRUE);
			else
				CombatAnim(this, 7);
		} else
			CombatAnim(this, 15);
		if (dist <= 68.0f) {
			mTurnSpeed = 0;
			CombatAnim(this, 9);
			mTurnSpeed = 0.1f;
			mPhase     = 2;
		} else if (dist > 120) {
			mTurnSpeed = 0.1f;
			mPhase     = 0;
		}
		break;
	case 2:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			CombatAnim(this, 10);
			mCounter = rand() % 5 + 5;
			++mPhase;
		}
		break;
	case 3: {
		mDir     = CombatHeading(this, chr);
		f32 diff = mDir - mFacing;
		if (diff < 0) {
			if (GetAnimEndFrame() - frames <= GetAnimFrame())
				CombatAnim(this, 14, TRUE);
		} else if (diff > 0) {
			if (GetAnimEndFrame() - frames <= GetAnimFrame())
				CombatAnim(this, 13, TRUE);
		} else if (GetAnimEndFrame() - frames <= GetAnimFrame())
			CombatAnim(this, 10);
		mFacing = mDir;
		if (--mCounter < 0 && SpiderInView(this)) {
			CombatAnim(this, 24);
			PlayWiiSoundEffect(&gGame.mSound, 0x20D, 1.0f, 0.0f);
			mPhase = 20;
			return;
		}
		if (dist <= 48.0f) {
			if (!SpiderInView(this))
				return;
			CombatAnim(this, 26);
			mTarget = pos;
			mDir    = CombatHeading(this, mTarget);
			mPhase  = 5;
		}
		break;
	}
	case 4:
		mDir   = CombatHeading(this, chr);
		mSpeed = 0.0f;
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			CombatAnim(this, 1);
			mFacing = mDir;
			mPhase  = 1;
		}
		break;
	case 5:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mTarget = pos;
			mDir    = CombatHeading(this, mTarget);
			CombatAnim(this, 11, FALSE, 2.0f);
			mSpeed   = 1.25f;
			mCounter = frames * 24;
			mFlags.mFlags |= 0x1000;
			mPhase = 6;
		}
		break;
	case 6:
		if (GetSpiderDistanceXZ(&chr, &mMoveTranslate) < 20.0f && !IsPlayerFlashingFromHit(gGame.GetPlayer()) && (mFlags.mFlags & 0x1000)
		    && nw4r::math::FAbs(chr.y - mMoveTranslate.y) < 8.0f) {
			gGame.GetPlayer()->OnHitFromPosition(pos, 10);
			if (!gGame.GetStatusFlags(0x100))
				StartScreenShake(&gGame.mQuake, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f);
			CombatAnim(this, 12);
			mSpeed = 0;
			mFlags.mFlags &= ~0x1000;
			mRumble = 1;
			mPhase  = 7;
		} else {
			CombatFootsteps(this);
			if (--mCounter < 0) {
				CombatAnim(this, 12);
				mSpeed = 0;
				mFlags.mFlags &= ~0x1000;
				mPhase = 7;
			} else if (FindNearbyDroppedSpider(this) != -1) {
				mDroppedSpiderIndex = FindNearbyDroppedSpider(this);
				mPhase              = 30;
			}
		}
		break;
	case 7:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mHome      = mMoveTranslate;
			mTurnSpeed = .1f;
			mPhase     = 0;
		}
		break;
	case 8:
		CombatFootsteps(this);
		if (GetSpiderDistanceXZ(&mTarget, &mMoveTranslate) <= 1.0f) {
			mSpeed = 0;
			if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
				CombatAnim(this, 1);
				mCounter = 0;
				mPhase   = 0;
			}
		}
		break;
	case 10:
		mDir -= PI / 30;
		EndSpiderCombatTurn(this, frames);
		break;
	case 11:
		mDir += PI / 30;
		EndSpiderCombatTurn(this, frames);
		break;
	case 20:
		if (SPIDER_AT_FRAME(this, 30)) {
			LaunchSpiderShot(this);
			++mPhase;
		}
		break;
	case 21:
		if (!(mCarry.mFlags.mFlags & 1)) {
			StopWiiSfx(&gGame.mSound, 0x197, 0.0f);
			mCounter = rand() % 30 + 30;
			if (GetAnimEndFrame() - frames <= GetAnimFrame())
				CombatAnim(this, 10);
			CombatAnim(this, 12);
			mPhase = 4;
		}
		break;
	case 30:
		CombatAnim(this, 12);
		mSpeed = 0;
		++mPhase;
		break;
	case 31:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			CombatAnim(this, 8);
			Vec other = *gGame.mXObjs.mPools[17].Get(mDroppedSpiderIndex)->GetPos();
			mDir      = CombatHeading(this, other);
			RetreatTarget(this);
			mCounter = 24 / frames;
			mDir     = PI + CombatHeading(this, mTarget);
			mSpeed   = -.5f;
			++mPhase;
		}
		break;
	case 32:
		CombatFootsteps(this);
		if ((GetSpiderDistanceXZ(&mTarget, &mMoveTranslate) <= 2 || --mCounter < 0) && !HasNearbyDroppedSpider(this)) {
			mSpeed = 0;
			if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
				CombatAnim(this, 1);
				mPhase = 0;
			}
		}
		break;
	case 33:
		if (!HasNearbyDroppedSpider(this) || --mCounter < 0) {
			CombatAnim(this, 8);
			RetreatTarget(this);
			mCounter = 24 * frames;
			mDir     = PI + CombatHeading(this, mTarget);
			mSpeed   = -.5f;
			mPhase   = 32;
		}
		break;
	case 40:
		CombatAnim(this, 2, FALSE, 2.0f);
		mTarget  = pos;
		mDir     = CombatHeading(this, mTarget);
		mCounter = (rand() % 32 + 32) / frames;
		mSpeed   = 1;
		++mPhase;
		break;
	case 41: {
		CombatFootsteps(this);
		f32 remaining = GetSpiderDistanceXZ(&mTarget, &mMoveTranslate);
		if (remaining <= 68.0f) {
			mSpeed = 0;
			CombatAnim(this, 9);
			mTurnSpeed = .1f;
			mPhase     = 2;
		} else {
			if (remaining <= 2.0f || --mCounter < 0) {
				mSpeed = 0;
				if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
					CombatAnim(this, 1);
					mPhase = 0;
					break;
				}
			}
			if (FindNearbyDroppedSpider(this) != -1) {
				mDroppedSpiderIndex = FindNearbyDroppedSpider(this);
				mPhase              = 30;
			}
		}
		break;
	}
	}
}

/**
 * @note Address: 0x80125E20
 * @note Size: 0x128
 */
// lands, drops into its anim, then takes the first free slot
void XObjSpider::State5(u32 frames)
{
	switch (mPhase) {
	case 0:
		mFlags.mFlags |= 0x40;
		mFlags.mFlags |= 0x400;
		mFlags.mFlags |= 0x8000;
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		mPhase++;
		break;
	case 1:
		if (gGame.mEvt.mData.mScript == NULL) {
			mFlags.mFlags &= ~0x40;
			mFlags.mFlags &= ~0x80;
			mFlags.mFlags &= ~0x100;
			mFlags.mFlags &= ~0x8000;
			TakeSlot(this);
			SetState(4);
		}
		break;
	case 2:
		break;
	}
}

/**
 * @note Address: 0x80125F48
 * @note Size: 0x8DC
 */
// NONMATCHING: the two proximity checks swap x/z and the zero FPR; the CPad vtable referent differs as in UpdateObject
void XObjSpider::State6(u32 frames)
{
	Vec pos = *gGame.GetPlayer()->GetPos();
	CVec chr(pos);
	CPad pad(0);
	switch (mPhase) {
	case 0:
		mCounter = rand() % 30;
		mPhase   = 10;
		break;
	case 10:
		if (--mCounter < 0) {
			mCounter = rand() % 30;
			SetAnim(0x11, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			mPhase = 1;
		}
		break;
	case 1:
		mCounter++;
		f32 swing        = (f32)sin(DEG2RAD * (f32)((mCounter * 4) % 360));
		f32 offset       = 8.0f * swing;
		mMoveTranslate.x = mHome.x + offset - 4.0f;
		if (mPlacementParam0 == 11) {
			if (mFlags.mFlags & 0x200000) {
				PlayWiiSoundEffect(&gGame.mSound, 0x19E, 1.0f, 0.0f);
				mFlags.mFlags |= 0x100000;
				mPhase++;
				SetAnim(0x10, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mSpeed = -1.6f;
			}
		} else {
			f32 dist;
			if (mWall == 3) {
				f32 x = chr.x - mMoveTranslate.x;
				f32 y = chr.y - mMoveTranslate.y;
				f32 z = 0.0f;
				dist  = FSqrt(x * x + y * y + z * z);
			} else {
				f32 x = chr.x - mMoveTranslate.x;
				f32 y = 0.0f;
				f32 z = chr.z - mMoveTranslate.z;
				dist  = FSqrt(x * x + y * y + z * z);
			}
			if (dist <= 68.0f && gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
				if (CountDroppedSpiders() < 10) {
					PlayWiiSoundEffect(&gGame.mSound, 0x19E, 1.0f, 0.0f);
					mFlags.mFlags |= 0x100000;
					mPhase++;
					SetAnim(0x10, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					mSpeed = -1.6f;
				}
			}
		}
		break;
	case 2:
		if (mMoveTranslate.y < 20.0f + mHome.y) {
			StopWiiSfx(&gGame.mSound, 0x19E, 0.0f);
			mPhase++;
			SetAnim(0x12, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mDir    = atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mFacing = mDir;
			mSpeed  = 0.0f;
		}
		break;
	case 3:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mPhase++;
			SetAnim(0x13, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mDir    = atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mFacing = mDir;
			mSpeed  = -0.5f;
		}
		break;
	case 4:
		if (mMoveTranslate.y < 8.0f + mHome.y) {
			mPhase++;
			SetAnim(0x14, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			PlayWiiSoundEffect(&gGame.mSound, 0x195, 1.0f, 0.0f);
		}
		break;
	case 5:
		if (mMoveTranslate.y < mHome.y) {
			mMoveTranslate.y = mHome.y;
			mSpeed           = 0.0f;
		}
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mFlags.mFlags &= ~0x40;
			mFlags.mFlags &= ~0x80;
			mFlags.mFlags &= ~0x100;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mFlags.mFlags &= ~0x8000;
			TakeSlot(this);
			SetState(4);
		}
		break;
	case 6:
		if (pad.mPad->mTrigger & (1ULL << RemoteB)) {
			mFlags.mFlags = 0;
			mFlags.mFlags |= 0x40;
			mFlags.mFlags |= 0x80;
			mFlags.mFlags |= 0x100;
			mMoveTranslate.y = 48.0f + mHome.y;
			mPhase           = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x80126824
 * @note Size: 0x600
 */
void XObjSpider::State7(u32 frames)
{
	switch (mPhase) {
	case 0:
		SetAnim(0xC, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		mSpeed = 0.0f;
		mPhase++;
		break;
	case 1:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mTarget.x = mHome.x;
			mTarget.y = mMoveTranslate.y;
			mTarget.z = mHome.z;
			mDir      = PI + (f32)atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mSpeed    = -0.5f;
			mPhase++;
		}
		break;
	case 2:
		CombatFootsteps(this);
		if (DistXZ(&mTarget, &mMoveTranslate) <= 2.0f) {
			mSpeed = 0.0f;
			if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
				SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				if (gGame.mEvt.mData.mScript != NULL) {
					mPhase++;
				} else {
					SetState(4);
				}
			}
		}
		break;
	case 3:
		if (gGame.mEvt.mData.mScript == NULL) {
			SetState(4);
		}
		break;
	}
}

/**
 * @note Address: 0x80126E24
 * @note Size: 0x4
 */
void XObjSpider::State8(u32 frames)
{
}

/**
 * @note Address: 0x80126E28
 * @note Size: 0x7A0
 */
void XObjSpider::State9(u32 frames)
{
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x10000ULL) {
		mPhase           = 0;
		mSpeed           = 0.0f;
		mMoveTranslate.x = mHome.x;
		mMoveTranslate.y = mHome.y;
		mMoveTranslate.z = mHome.z;
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		mFlags.mFlags &= ~0x100;
	}
	switch (mPhase) {
	case 0:
		if (pad.mPad->mTrigger & ((1ULL << RemoteB) | (1ULL << GameCubeB))) {
			u64 button = pad.mPad->mButton;
			if (button & (1ULL << RemoteUp)) {
				mTarget.x = mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z - 60.0f;
				SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & ((1ULL << RemoteDown) | (1ULL << (GameCubeDPad + 2)))) {
				mTarget.x = mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = 60.0f + mHome.z;
				SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & ((1ULL << RemoteLeft) | (1ULL << GameCubeDPad))) {
				mTarget.x = mHome.x - 60.0f;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z;
				SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & ((1ULL << RemoteRight) | (1ULL << (GameCubeDPad + 1)))) {
				mTarget.x = 60.0f + mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z;
				SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			}
		}
		break;
	case 1:
		if (HeadTo(this, mTarget) < 20.0f) {
			mPhase++;
			SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	case 2:
		if (HeadTo(this, mTarget) < 1.0f) {
			mFlags.mFlags |= 0x100;
			mSpeed = 0.0f;
		}
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			mFlags.mFlags |= 0x100;
			mPhase++;
			SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mSpeed = 0.3f;
		}
		break;
	case 3:
		if (mMoveTranslate.y >= 80.0f) {
			mPhase++;
			mSpeed = -0.3f;
		}
		break;
	case 4:
		if (mMoveTranslate.y <= 20.0f + mHome.y) {
			mPhase++;
			mMoveTranslate.y = 0.0f;
			mFlags.mFlags &= ~0x100;
			mTarget.x = mHome.x;
			mTarget.y = mHome.y;
			mTarget.z = mHome.z;
			SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mSpeed = 0.3f;
		}
		break;
	case 5: {
		f32 distance = HeadTo(this, mTarget);
		s32 done     = 0;
		if (distance < 1.0f) {
			mSpeed = 0.0f;
			done   = 1;
		}
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			done++;
		}
		if (done >= 2) {
			mPhase = 0;
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	}
	}
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
 * @note Address: 0x801275D4
 * @note Size: 0x188
 */
// NONMATCHING: retail keeps x in f27 and z in f29, ours the other way round
// whether another spider that dropped down is closer than 24
extern "C" bool HasNearbyDroppedSpider(XObjSpider* spider)
{
	for (int i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObjSpider* other = (XObjSpider*)obj;
			if ((other->mFlags.mFlags & 0x100000) && other->mPoolIndex != spider->mPoolIndex) {
				f32 x = other->mMoveTranslate.x - spider->mMoveTranslate.x;
				f32 y = other->mMoveTranslate.y - spider->mMoveTranslate.y;
				f32 z = other->mMoveTranslate.z - spider->mMoveTranslate.z;
				if (FSqrt(x * x + y * y + z * z) < 24.0f) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x8012775C
 * @note Size: 0x188
 */
// NONMATCHING: x and z swap f27 and f29 as in HasNearbyDroppedSpider
// the index of another spider that dropped down closer than 24, -1 when there is none
extern "C" s32 FindNearbyDroppedSpider(XObjSpider* spider)
{
	for (int i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObjSpider* other = (XObjSpider*)obj;
			if ((other->mFlags.mFlags & 0x100000) && other->mPoolIndex != spider->mPoolIndex) {
				f32 x = other->mMoveTranslate.x - spider->mMoveTranslate.x;
				f32 y = other->mMoveTranslate.y - spider->mMoveTranslate.y;
				f32 z = other->mMoveTranslate.z - spider->mMoveTranslate.z;
				if (FSqrt(x * x + y * y + z * z) < 24.0f) {
					return other->mPoolIndex;
				}
			}
		}
	}
	return -1;
}

/**
 * @note Address: 0x801278E4
 * @note Size: 0x2F0
 */
extern "C" void UpdateSpiderCarryModel(CarryInfo* carry, u8 slot, u32 frames, s32 mode)
{
	if (slot == 255) {
		carry->mState = 0;
		carry->mFlags.mFlags &= ~1;
		return;
	}
	if (gSpiderCarryModels[slot] == NULL) {
		carry->mState = 0;
		carry->mFlags.mFlags &= ~1;
		return;
	}
	switch (carry->mState) {
	case 0:
		break;
	case 1:
		gSpiderCarryModels[slot]->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		carry->mState++;
		break;
	case 2:
		if (carry->mObj != NULL) {
			CVec pos    = carry->mObj->GetJointPos(carry->mObj->FindResNode("cyl1"));
			carry->mPos = pos;
		}
		if (gSpiderCarryModels[slot]->GetAnimEndFrame() - frames <= gSpiderCarryModels[slot]->GetAnimFrame()) {
			gSpiderCarryModels[slot]->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			carry->mState++;
		}
		break;
	case 3:
		if (carry->mObj != NULL) {
			CVec pos    = carry->mObj->GetJointPos(carry->mObj->FindResNode("cyl1"));
			carry->mPos = pos;
		}
		if (gSpiderCarryModels[slot]->GetAnimEndFrame() - frames <= gSpiderCarryModels[slot]->GetAnimFrame()) {
			gSpiderCarryModels[slot]->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			carry->mState = 0;
			carry->mFlags.mFlags &= ~1;
			carry->mFlags.mFlags &= ~2;
		}
		break;
	}
}

/**
 * @note Address: 0x80127BD4
 * @note Size: 0x19C
 */
extern "C" void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways)
{
	if (slot != 255 && (carry->mFlags.mFlags & 1)) {
		if (gSpiderCarryModels[slot] != NULL) {
			gSpiderCarryModels[slot]->SetTranslateX(carry->mPos.x);
			gSpiderCarryModels[slot]->SetTranslateY(carry->mPos.y);
			gSpiderCarryModels[slot]->SetTranslateZ(carry->mPos.z);
			if (sideways) {
				gSpiderCarryModels[slot]->SetRotationX(HALF_PI);
				gSpiderCarryModels[slot]->SetRotationZ(HALF_PI + carry->mAngle);
			} else {
				gSpiderCarryModels[slot]->SetRotationY(carry->mAngle);
			}
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, gSpiderCarryModels[slot]->mObject.m_pScnMdl);
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->UpdateFrame();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcWorld();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcView();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->GatherDrawScnObj();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->DrawOpa();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->DrawXlu();
			WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
			while (scene->mPending != 0) {
				scene->PopPending();
			}
		}
	}
}

/**
 * @note Address: 0x80127D70
 * @note Size: 0x19C
 */
extern "C" void ResetSpiderCoinBounces(void* unused)
{
	gSpiderCoinBounceCount = 0;
	for (int i = 0; i < 11; i++) {
		gSpiderCoinBounces[i].mCoin  = NULL;
		gSpiderCoinBounces[i].mPhase = 0;
		gSpiderCoinBounces[i].mCount = 0;
	}
	for (int i = 0; i < 96; i++) {
		XObj* obj = gGame.mXObjs.mPools[7].Get(i);
		if (obj->GetKind() == 0x104) {
			((XObjCoin*)obj)->mBouncing = 0;
		}
	}
}

/**
 * @note Address: 0x80127F0C
 * @note Size: 0x254
 */
// moves the coins, shown, hopping, then falling to where they land
extern "C" void UpdateSpiderCoinBounces(void* unused, u32 frames)
{
	f32 lift = 0.2f; // how far the coins float above their spot
	for (s32 i = 0; i < 11; ++i) {
		CoinBounce* entry = &gSpiderCoinBounces[i];
		XObj* coin        = entry->mCoin;
		if (coin != NULL) {
			s32 phase   = entry->mPhase;
			s32 counter = entry->mCount;
			Vec pos     = { entry->mPos.x, entry->mPos.y, entry->mPos.z };
			f32 targetY = entry->mTarget.y;
			switch (phase) {
			case 0:
				coin->mStatus.mFlags |= 0x10000ULL;
				++phase;
				break;
			case 1: {
				++counter;
				f32 height = 10.0f * (f32)sin(DEG2RAD * (f32)((counter * 16) % 360)) + lift;
				if (height < lift) {
					coin->SetTranslateY(pos.y + lift);
					++phase;
				} else {
					coin->SetTranslateY(pos.y + height);
				}
				break;
			}
			case 2:
				pos.y -= 0.8f * (f32)frames;
				if (pos.y <= targetY) {
					coin->mStatus.mFlags &= ~0x100000ULL;
					pos.y = targetY;
					++phase;
				}
				coin->SetTranslateY(pos.y + lift);
				break;
			case 3:
				break;
			}
			entry->mPhase = phase;
			entry->mCount = counter;
			entry->mPos.x = pos.x;
			entry->mPos.y = pos.y;
			entry->mPos.z = pos.z;
		}
	}
}

/**
 * @note Address: 0x80128160
 * @note Size: 0x1E0
 */
// NONMATCHING: the pool loop steps another register, as on the GameCube
// puts one of the free coins at position and plays its anim
extern "C" void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim)
{
	f32 lift = 0.2f;
	if (gSpiderCoinBounceCount >= 11)
		return;
	for (s32 i = 0; i < 96; ++i) {
		XObjCoin* coin = (XObjCoin*)gGame.mXObjs.mPools[7].Get(i);
		if (coin->mStatus.mFlags & 1ULL) {
			if (coin->GetKind() == 0x104) {
				if ((coin->mStatus.mFlags & 1ULL) && !coin->mBouncing) {
					coin->mBouncing                                   = 1;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPhase = 0;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCount += 2;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.x    = position->x;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.y    = position->y + lift;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.z    = position->z;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.x = target->x;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.y = target->y;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.z = target->z;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin     = coin;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateX(position->x);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateY(position->y + lift);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateZ(position->z);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->mStatus.mFlags |= 0x100000ULL;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetAnim(anim, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					++gSpiderCoinBounceCount;
					break;
				}
			}
		}
	}
}

/**
 * @note Address: 0x80128340
 * @note Size: 0x38
 */
extern "C" void ResetSmallSpiderWebEffect(XObjSpiderSmallFx* fx)
{
	fx->mPos.x         = 0.0f;
	fx->mPos.y         = 0.0f;
	fx->mPos.z         = 0.0f;
	fx->_10.x          = 0.0f;
	fx->_10.y          = 0.0f;
	fx->_10.z          = 0.0f;
	fx->mRotY          = 0.0f;
	fx->_24            = 0.0f;
	fx->mState         = 0;
	fx->_2C            = 0;
	fx->mStatus.mFlags = 0;
}

/**
 * @note Address: 0x80128378
 * @note Size: 0x300
 */
extern "C" void UpdateSmallSpiderWebEffect(XObjSpiderSmallFx* shot, u8 i, u32 frames, BOOL unused)
{
	if (gGame.GetStatusFlags(0x400)) {
		return;
	}
	if (i == 0xFF) {
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		return;
	}
	if (gSmallSpiderEffectObjects[i] == NULL) {
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		return;
	}
	if (!(shot->mStatus.mFlags & 1)) {
		return;
	}
	switch (shot->mState) {
	case 0:
		break;
	case 1:
		gSmallSpiderEffectObjects[i]->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		shot->mState++;
		break;
	case 2: {
		f32 frame = gSmallSpiderEffectObjects[i]->GetAnimFrame();
		f32 end   = gSmallSpiderEffectObjects[i]->GetAnimEndFrame();
		if (end - frames <= frame) {
			gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			shot->mState = 0;
			shot->mStatus.mFlags &= ~1;
		}
		break;
	}
	case 3:
		gSmallSpiderEffectObjects[i]->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		MtxP mtx               = gSmallSpiderEffectObjects[i]->GetWorldMtx("ptcl_exp01");
		EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x1F, 0);
		if (effect != NULL) {
			effect->mtx = mtx;
			UpdateEffectMtx(effect);
		}
		shot->mState++;
		break;
	case 4: {
		f32 frame = gSmallSpiderEffectObjects[i]->GetAnimFrame();
		f32 end   = gSmallSpiderEffectObjects[i]->GetAnimEndFrame();
		if (end - frames <= frame) {
			gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			shot->mState = 0;
			shot->mStatus.mFlags &= ~1;
		}
		break;
	}
	case 10:
		gSmallSpiderEffectObjects[i]->SetAnim(0, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		shot->mState = 0;
		shot->mStatus.mFlags &= ~1;
		break;
	}
}

/**
 * @note Address: 0x80128678
 * @note Size: 0x14C
 */
extern "C" void DrawSmallSpiderWebEffect(XObjSpiderSmallFx* fx, u8 index)
{
	if (index != 255 && (fx->mStatus.mFlags & 1)) {
		if (gSmallSpiderEffectObjects[index] != NULL) {
			gSmallSpiderEffectObjects[index]->SetTranslateX(fx->mPos.x);
			gSmallSpiderEffectObjects[index]->SetTranslateY(fx->mPos.y);
			gSmallSpiderEffectObjects[index]->SetTranslateZ(fx->mPos.z);
			gSmallSpiderEffectObjects[index]->SetRotationY(fx->mRotY);
			gGame.mGs.mScene.mHead->vf034(gGame.mGs.mScene.mHead->mPending, gSmallSpiderEffectObjects[index]->mObject.m_pScnMdl);
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->UpdateFrame();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcWorld();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->CalcView();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->GatherDrawScnObj();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->DrawOpa();
			((nw4r::g3d::ScnRoot*)gGame.mGs.mScene.mHead)->DrawXlu();
			WiiGsSceneObj* scene = gGame.mGs.mScene.mHead;
			while (scene->mPending != 0) {
				scene->PopPending();
			}
		}
	}
}

static inline bool CanPlaySe()
{
	bool play = false;
	if (gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL && gGame.mMenu.mActive == 0 && !gGame.GetStatusFlags(0xC00)
	    && gGame.mTimeState.mDuration > 0.0f && gGame.mTimeState.mRunning == 0) {
		play = true;
	}
	return play;
}

/**
 * @note Address: 0x801287C4
 * @note Size: 0x1D4
 */
extern "C" void StartSmallSpiderWebEffect(XObjSpiderSmallFx* fx, const CVec& pos)
{
	fx->mStatus.mFlags |= 1;
	fx->mPos   = pos;
	fx->mState = 1;
	if (CanPlaySe()) {
		PlayWiiSoundEffect(&gGame.mSound, 0x1F1, 1.0f, 0.0f);
		GetSoundVolumeAtPosition(&gGame.mSound, &fx->mPos, 0);
	}
}

/**
 * @note Address: 0x80128998
 * @note Size: 0x30
 */
extern "C" void fn_80128998(XObjSpiderSmallFx* fx, const CVec& pos)
{
	fx->mStatus.mFlags |= 1;
	fx->mPos   = pos;
	fx->mState = 3;
}

/**
 * @note Address: 0x801289C8
 * @note Size: 0x18
 */
extern "C" void fn_801289C8(XObjSpiderSmallFx* fx)
{
	fx->mState = 10;
	fx->mStatus.mFlags |= 1;
}

/**
 * @note Address: 0x801289E0
 * @note Size: 0xBC
 */
extern "C" bool IsSmallSpiderEffectAnimationFinished(XObjSpiderSmallFx* fx, u8 slot, u32 frames)
{
	if (gSmallSpiderEffectObjects[slot] == NULL) {
		return TRUE;
	}
	f32 frame = gSmallSpiderEffectObjects[slot]->GetAnimFrame();
	f32 end   = gSmallSpiderEffectObjects[slot]->GetAnimEndFrame();
	if (end - frames <= frame) {
		return TRUE;
	}
	return FALSE;
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
