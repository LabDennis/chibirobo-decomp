#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjflags.h"
#include "game/wii/cpad.h"
#include "game/attachmentvalues.h"
#include "game/gmath.h"
#include "game/wii/spiderplacement.h"
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

extern "C" {
void ResetSpiderState(XObjSpider* spider);
void ApplySpiderPlacement(XObjSpider* spider);
void ResetSpiderCoinBounces(void* unused);
void UpdateSpiderCoinBounces(void* unused, u32 frames);
void UpdateSpiderHitReaction(XObjSpider* spider, u32 frames);
void ExecuteSpiderCommand(XObjSpider* spider, u32 frames);
void UpdateSpiderShot(XObjSpider* spider, u32 frames);
void UpdateSpiderAttachments(XObjSpider* spider, u32 frames);
void UpdateSpiderCarryModel(CarryInfo* carry, u8 slot, u32 frames, s32 mode);
void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim);
void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z);
void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
u32 GetPlayerActionParameter(s32 index);
void HSD_MtxGetScale(Mtx mat, Vec* vec);

extern s32 gRefreshRate;
extern AnimSfxEvent gSpiderAnimSfxEvents[];
extern XObj* gSpiderCarryModels[16];
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
			f32 scaledWidth    = spider->mAreaWidth * 0.9f;
			f32 scaledDepth    = spider->mAreaDepth * 0.9f;
			f32 centerX        = spider->mAreaWidth * 0.5f + spider->mAreaPos.x;
			f32 centerZ        = spider->mAreaDepth * 0.5f + spider->mAreaPos.z;
			spider->mAreaPos.x = centerX - scaledWidth * 0.5f;
			spider->mAreaPos.z = centerZ - scaledDepth * 0.5f;
			spider->mAreaWidth *= 0.9f;
			spider->mAreaDepth *= 0.9f;
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
		if (PlacementRotation(spider->mCoord).z != 0.0f)
			spider->mDir = PI;
		else
			spider->mDir = 0.0f;
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
	CPad pad(0); // unused, retail builds one here
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
 * @note Address: 0x80121D18
 * @note Size: 0x3B8
 */
// the event commands, 100 ties another object to one of its nodes or lets go of all
// of them, anything else is kept for ExecuteSpiderCommand
// NONMATCHING: retail loads each GetPos() component into a float right after its call (f30, f31), ours keeps
// the pointers and loads them at the PSMTXTrans call
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
				PSMTXTrans(mtx, mTies[i].mObj->GetPos()->x, mTies[i].mObj->GetPos()->y, mTies[i].mObj->GetPos()->z);
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
