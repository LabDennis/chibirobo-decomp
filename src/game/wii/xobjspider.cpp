#include "game/wii/cgame.h"
#include "game/wii/xobj.h"
#include "game/wii/xobjflags.h"
#include "game/attachmentvalues.h"
#include "game/gmath.h"
#include "arithmetic.h"

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
extern CoinBounce gSpiderCoinBounces[11];
extern s32 gSpiderCoinBounceCount; // how many coins are out
}

// the placement entry's position, rotation and scale; the Wii builds all three axes each time
static inline Vec PlacementPosition(const CoordInfo& coord)
{
	Vec pos;
	const UnkStageObj* entry = coord.mPlacement;
	pos.x                    = entry->mPos[0] / 100.0f;
	pos.y                    = entry->mPos[1] / 100.0f;
	pos.z                    = entry->mPos[2] / 100.0f;
	return pos;
}

static inline Vec PlacementRotation(const CoordInfo& coord)
{
	Vec rot;
	const UnkStageObj* entry = coord.mPlacement;
	rot.x                    = DEG2RAD * entry->mRot[0];
	rot.y                    = DEG2RAD * entry->mRot[1];
	rot.z                    = DEG2RAD * entry->mRot[2];
	return rot;
}

static inline Vec PlacementScale(const CoordInfo& coord)
{
	Vec scale;
	const UnkStageObj* entry = coord.mPlacement;
	scale.x                  = entry->mScale[0] / 100.0f;
	scale.y                  = entry->mScale[1] / 100.0f;
	scale.z                  = entry->mScale[2] / 100.0f;
	return scale;
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
		_BD4 = 0.5f * gRefreshRate;
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
	if (GetAttachmentCountById((AttachmentValues*)gGame.GetPlayer()->mAttachments, 2) < 2)
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
