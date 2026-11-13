#include "game/xobjspider.h"
#include "game/xobjactor.h"
#include "game/heldpropmatrix.h"
#include "game/hsd_jobj.h"
#include "game/spiderplacement.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cpad.h"
#include "game/cscrn.h"
#include "game/xchr.h"
#include "game/xobjflags.h"
#include "game/ut.h"
#include <sysdolphin/rumble.h>
#include "game/globals.h"
#include "game/gmath.h"
#include "game/animframe.h"

XObjSpider::~XObjSpider()
{
}

extern "C" {
#include <dolphin/mtx/mtx.h>
}

extern "C" {
u8 gSpiderPlacementWork[0xD4];
Vec gSpiderPosition, gSpiderScale, gSpiderRotation;
#if defined(VERSION_JP)
u8 gSpiderWorkEnd[0x134 - 0xF8];
#else
u8 gSpiderWorkEnd[0x140 - 0xF8];
#endif
XObj* gSpiderCarryModels[16];
s32 gSpiderCarrySlotOwners[16];
CoinBounce gSpiderCoinBounces[11];
void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim);
void UpdateSpiderCoinBounces(void* unused, u32 frames);
void ResetSpiderCoinBounces(void* unused);
void PlaySpiderVariantSound(XObjSpider* spider);
s32 FindNearbyDroppedSpider(XObjSpider* spider);
bool HasNearbyDroppedSpider(XObjSpider* spider);
bool IsSpiderInsideMovementArea(XObjSpider* spider);
void UpdateSpiderCarryModel(CarryInfo* carry, u8 slot, u32 frames, s32 mode);
f32 GetSpiderDistanceXZ(Vec* first, Vec* second);
void UpdateSpiderShot(XObjSpider* spider, u32 frames);
void LaunchSpiderShot(XObjSpider* spider);
void UpdateSpiderAttachments(XObjSpider* spider, u32 frames);
void ExecuteSpiderCommand(XObjSpider* spider, u32 frames);
void UpdateSpiderHitReaction(XObjSpider* spider, u32 frames);
void ApplySpiderPlacement(XObjSpider* spider);
void ResetSpiderState(XObjSpider* spider);

CVec GetStagePlacementScale(XObj* obj);
void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z);
s32 GetAttachmentCountById(AttachmentValues* values, s32 id);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void HSD_StateInvalidate(s32 state);
int rand();
void OSReport(const char* fmt, ...);

AnimSfxEvent gSpiderAnimSfxEvents[] = {
	{ 2, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ 12, 1.0f, 0.0f, 1, 0x137, 0xFF, 0, 0, 0 },
	{ -1 },
}; // the spider's anim sounds
u32 gSpiderCoinBounceUpdateContext; // the coins' manager token, the callbacks never touch it; its real type is unknown
s32 lbl_8065B2BC;
u8 lbl_8065B2C0;
s32 gSpiderCoinBounceCount; // how many coins are out
}

// NONMATCHING: the sinit's copy source sits at another offset in retail's
// sSpiderStateData block; data layout differs.
// the state handlers by number, Update calls them
XObjSpider::Func gSpiderStateHandlers[10] = {
	&XObjSpider::State0, &XObjSpider::State1, &XObjSpider::State2, &XObjSpider::State3, &XObjSpider::State4,
	&XObjSpider::State5, &XObjSpider::State6, &XObjSpider::State7, &XObjSpider::State8, &XObjSpider::State9,
};

// puts a CObjVec at a value and its target there too
static inline void SetNow(CObjVec& vec, const Vec& v)
{
	vec.Set(v);
	vec.mTarget.Set(v);
}

// the slot's model plays its anim 2 from where its anim ends, once
static inline void HitCarried(XObjSpider* spider)
{
	if (!(spider->mCarry.mFlags.mFlags & 2)) {
		XObj* model = gSpiderCarryModels[spider->mSlot];
		if (model != NULL && spider->mCarry.mState == 2) {
			f32 end = GetAnimEndFrame(model->mJObj);
			model->SetAnim(2, TRUE, NULL, NULL, NULL, end - GetAnimFrame(model->mJObj), 1.0f, 0.0f, 10.0f);
			spider->mCarry.mState++;
			spider->mCarry.mFlags.mFlags |= 2;
		}
	}
}

// turns it toward a point and says how far that is
static inline f32 HeadTo(XObjSpider* spider, const Vec& to)
{
	spider->mDir = atan2(to.x - spider->mMoveTranslate.x, to.z - spider->mMoveTranslate.z);
	f32 x        = to.x - spider->mMoveTranslate.x;
	f32 y        = to.y - spider->mMoveTranslate.y;
	f32 z        = to.z - spider->mMoveTranslate.z;
	return sqrtf(x * x + y * y + z * z);
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

// where the robot is
static inline Vec GetChrPos()
{
	return gGame.GetPlayer()->mMoveTranslate;
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

// a step sound, which one of the five goes by the spider's index
static inline void PlayStep(XObjSpider* spider)
{
	switch (spider->mPoolIndex % 5) {
	case 0:
		PlayGameSound(&gGame.mSound, 0x12B, 255);
		break;
	case 1:
		PlayGameSound(&gGame.mSound, 0x12C, 255);
		break;
	case 2:
		PlayGameSound(&gGame.mSound, 0x12D, 255);
		break;
	case 3:
		PlayGameSound(&gGame.mSound, 0x12E, 255);
		break;
	case 4:
		PlayGameSound(&gGame.mSound, 0x12F, 255);
		break;
	}
}

// how far apart two points are on the floor
static inline f32 DistXZ(const Vec* first, const Vec* second)
{
	f32 x = first->x - second->x;
	f32 y = 0.0f;
	f32 z = first->z - second->z;
	return sqrtf(x * x + y * y + z * z);
}

/**
 * @note Address: 0x800F4818
 * @note Size: 0x50
 */
void XObjSpider::ResetForNewGame()
{
	ResetSpiderState(this);
	SetState(0);
	mFlags.mFlags |= 0x04000000;
}

/**
 * @note Address: 0x800F4868
 * @note Size: 0x58
 */
void XObjSpider::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
	ResetSpiderState(this);
	SetState(0);
	mFlags.mFlags |= 0x04000000;
}

static inline void PlaceWallSpider(XObjSpider* spider)
{
	spider->SetState(8);
	spider->mFlags.mFlags |= 0x8000;
	spider->mAreaPos.x = -52.0f;
	spider->mAreaPos.y = 0.0f;
	spider->mAreaPos.z = 0.0f;
	spider->mAreaWidth = 2.0f * Abs(spider->mAreaPos.x);
	spider->mAreaDepth = 0.0f;
	spider->mWall      = 3;
	spider->mWallPos.z = spider->mMoveTranslate.z;
	TakeSlot(spider);
}

/**
 * @note Address: 0x800F38F4
 * @note Size: 0xA5C
 */
extern "C" void ApplySpiderPlacement(XObjSpider* spider)
{
	spider->mMoveTranslate.x = PlacementPosition(spider->mCoord, gSpiderPosition).x;
	spider->mMoveTranslate.y = PlacementPosition(spider->mCoord, gSpiderPosition).y;
	spider->mMoveTranslate.z = PlacementPosition(spider->mCoord, gSpiderPosition).z;
	spider->mHome.x          = PlacementPosition(spider->mCoord, gSpiderPosition).x;
	spider->mHome.y          = PlacementPosition(spider->mCoord, gSpiderPosition).y;
	spider->mHome.z          = PlacementPosition(spider->mCoord, gSpiderPosition).z;
	spider->mDir             = PlacementRotation(spider->mCoord, gSpiderRotation).y;
	spider->mRadius          = 20.0f * PlacementScale(spider->mCoord, gSpiderScale).x;
	switch (spider->_25C) {
	case 0:
		spider->SetState(0);
		spider->mFlags.mFlags |= 0x08000000;
		TakeSlot(spider);
		break;
	case 1:
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
			f32 width          = spider->mAreaWidth;
			f32 depth          = spider->mAreaDepth;
			f32 centerX        = width * 0.5f + spider->mAreaPos.x;
			f32 centerZ        = depth * 0.5f + spider->mAreaPos.z;
			f32 scaledWidth    = width * 0.9f;
			f32 scaledDepth    = depth * 0.9f;
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
		// Retail writes the facing direction before reloading the placement angle.
		if (PlacementRotation(spider->mCoord, gSpiderRotation).z != 0.0f)
			spider->mDir = PI;
		else
			spider->mDir = 0.0f;
		spider->mDir = PlacementRotation(spider->mCoord, gSpiderRotation).z;
		break;
	default:
		spider->SetState(4);
		spider->mFlags.mFlags |= 0x400;
		spider->mAreaPos.x = -50.0f;
		spider->mAreaPos.y = 0.0f;
		spider->mAreaPos.z = -58.0f;
		spider->mAreaWidth = 80.0f;
		spider->mAreaDepth = 134.0f;
		break;
	}
	if (spider->mFlags.mFlags & 0x40) {
		spider->mFlags.mFlags |= 0x80;
		spider->mFlags.mFlags |= 0x100;
		spider->mMoveTranslate.y = spider->_260 + spider->mHome.y;
	}
}

/**
 * @note Address: 0x800F3898
 * @note Size: 0x5C
 */
BOOL XObjSpider::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		mFlags.mFlags &= ~0x04000000U;
		ResetSpiderState(this);
		ApplySpiderPlacement(this);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x800F373C
 * @note Size: 0x15C
 */
// NONMATCHING: regalloc
void XObjSpider::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mSfx.mEvents = gSpiderAnimSfxEvents;
	if (mPoolIndex == 0) {
		for (u32 i = 0; i < 16; ++i) {
			gSpiderCarryModels[i]     = NULL;
			gSpiderCarrySlotOwners[i] = -1;
		}
		lbl_8065B2C0  = 0;
		lbl_8065B2BC  = 0;
		s32 found     = 0;
		s32 count     = gGame.mXObjs.mObjectCount;
		XObj** output = gSpiderCarryModels;
		for (s32 i = 0; i < count; ++i) {
			XObj* candidate = gGame.mXObjs.mObjects[i];
			if (candidate->mStatus.mFlags & 1ULL) {
				UnkStageObj* placement = candidate->mCoord._00;
				if (placement->mObjectId == 0x13C) {
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
 * @note Address: 0x800F31E0
 * @note Size: 0x55C
 */
void XObjSpider::UpdateObject(u32 frames)
{
	CPad pad(0);

	switch (mRumble) {
	case 0:
		break;
	case 1:
		if (gGame.mStatus.mFlags & 0x800000000ULL) {
			CPadOne* rumble = &gGame.mPad.mPad[0];
			if (rumble->mConnected) {
				HSD_PadRumbleOn(rumble->mPort);
			}
			mRumbleTimer = 24 / frames;
			mRumble++;
		} else {
			mRumble = 0;
		}
		break;
	case 2:
		if (--mRumbleTimer < 0) {
			CPadOne* rumble = &gGame.mPad.mPad[0];
			if (rumble->mConnected) {
				HSD_PadRumbleOffN(rumble->mPort);
			}
			if (rumble->mConnected) {
				HSD_PadRumbleRemove(rumble->mPort);
			}
			mRumble = 0;
		}
		break;
	}

	if (gGame.mStatus.mFlags & 0x400) {
		return;
	}
	if (mPoolIndex == 0) {
		UpdateSpiderCoinBounces(&gSpiderCoinBounceUpdateContext, frames);
	}
	if (!(mStatus.mFlags & 0x10000) && mState != 1) {
		return;
	}
	UpdateAnimationSounds();
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
		SetNow(mMoveTranslate, mMoveTranslate + move * (f32)frames);
		SetPos(mMoveTranslate, 0);
	}
#if defined(VERSION_PAL)
	mAnimStartFrame = GetAnimFrame(mJObj);
#endif
	UpdateObject__4XObjFUl(this, frames);
	UpdateSpiderCarryModel(&mCarry, mSlot, frames, 0);
	UpdateSpiderAttachments(this, frames);
}

/**
 * @note Address: 0x800F3184
 * @note Size: 0x5C
 */
void XObjSpider::DrawObject()
{
	DrawWithView(NULL);
	bool sideways = false;
	if (mWall == 3)
		sideways = true;
	DrawSpiderCarryModel(&mCarry, mSlot, sideways);
}

/**
 * @note Address: 0x800F3180
 * @note Size: 0x4
 */
void XObjSpider::DrawGroundShadow()
{
}

/**
 * @note Address: 0x800F2E44
 * @note Size: 0x33C
 */
// NONMATCHING: the float registers of the ring loop are numbered the other way
// the shadow, a disc and a ring around it that fades out
void XObjSpider::DrawGroundShadowAt(const Vec* origin, f32 opacity)
{
	if (!(mStatus.mFlags & 8ULL))
		return;
	switch (_25C) {
	case 3:
	case 4:
	case 5:
	case 7:
	case 8:
	case 9:
		return;
	}
	Vec pos  = *origin;
	Vec rot  = { -HALF_PI, 0.0f, 0.0f };
	Vec size = mMoveScale * (f32)(mAttr._00 != NULL ? mAttr._00->mShadowSize : 0);
	size.x   = mRadius;
	size.y   = mRadius;
	size.z   = mRadius;
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	u32 edge = 128.0f * opacity;
	DrawGsCircle(&gGame.mGs, 12, 160.0f * opacity, edge, pos.x, pos.y, pos.z, size.x, size.z, rot.x, rot.y, rot.z);
	PrimVertex verts[26];
	for (int i = 0; i < 13; i++) {
		f32 angle              = (PI / 6) * i;
		verts[i * 2].pos.x     = pos.x - size.x * 0.5f * (f32)sin(angle);
		verts[i * 2].pos.z     = pos.z + size.z * 0.5f * (f32)cos(angle);
		verts[i * 2].pos.y     = pos.y;
		verts[i * 2].color     = edge;
		verts[i * 2 + 1].pos.x = pos.x - (1.5f + size.x * 0.5f) * (f32)sin(angle);
		verts[i * 2 + 1].pos.z = pos.z + (1.5f + size.z * 0.5f) * (f32)cos(angle);
		verts[i * 2 + 1].pos.y = pos.y;
		verts[i * 2 + 1].color = 0;
	}
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, verts, 26);
}

/**
 * @note Address: 0x800F2E40
 * @note Size: 0x4
 */
void XObjSpider::DrawScreenOverlay()
{
}

/**
 * @note Address: 0x800F2E3C
 * @note Size: 0x4
 */
void XObjSpider::DrawDebugStatus()
{
}

/**
 * @note Address: 0x800F29A4
 * @note Size: 0x498
 */
// NONMATCHING: the joint path and the two points are laid out differently on the stack
// the thread it hangs from, a line from its joint up to the ceiling
void XObjSpider::DrawObjectEffects()
{
	if (mFlags.mFlags & 0x80) {
		Vec top;
		CJObj joint;
		u32 color        = (u32)(90.0f * mThreadAlpha) | 0xFFFFFF00;
		s32 threadPath[] = { 0 };
		joint.Attach(FindJointByPath(mJObj, threadPath, 1));
		top = GetJointPosition(joint.mJObj);
		CVec pos(top);
		f32 endX = pos.x;
		f32 endZ = pos.z;
		CGs* gs  = &gGame.mGs;
		HSD_StateInvalidate(-1);
		CGs* gs2 = &gGame.mGs;
		if (gs2->mCurRenderMode != NULL) {
			CGsRenderMode* mode = gs->mCurRenderMode;
			if (mode->mPEDirty == 0) {
				mode->mPEDirty = 1;
			}
			if (mode->mCullDirty == 0) {
				mode->mCullDirty = 1;
			}
			if (mode->mLineDirty == 0) {
				mode->mLineDirty = 1;
			}
			if (mode->mPointDirty == 0) {
				mode->mPointDirty = 1;
			}
		}
		CGsTevStage* stage = gs->mCurTevStage;
		if (stage != NULL) {
			if (stage->mStageDirty == 0) {
				stage->mStageDirty = 1;
			}
			if (stage->mNumGensDirty == 0) {
				stage->mNumGensDirty = 1;
			}
			if (stage->mTexGenDirty == 0) {
				stage->mTexGenDirty = 1;
			}
		}
		CGsChannel* channel = gs->mCurChannel;
		if (channel != NULL) {
			if (channel->mDirty == 0) {
				channel->mDirty = 1;
			}
		}
		CGs* gs3 = &gGame.mGs;
		if (gs2->mCurRenderMode != NULL) {
			ApplyGsRenderMode(gs3->mCurRenderMode, 1);
		}
		if (gs3->mCurTevStage != NULL) {
			ApplyGsTevStage(gs3->mCurTevStage, 1);
		}
		if (gs3->mCurChannel != NULL) {
			ApplyGsChannel(gs3->mCurChannel, 1);
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsLineWidth(gs2->mCurRenderMode, 0x12, 5);
		DrawGsLineSegment(&gGame.mGs, NULL, pos.x, pos.y, pos.z, color, endX, 300.0f, endZ, color);
		SetGsLineWidth(gs2->mCurRenderMode, 0xC, 5);
	}
}

/**
 * @note Address: 0x800F2454
 * @note Size: 0x550
 */
// the event commands, 100 ties another object to one of its joints or lets go of all
// of them, anything else is kept for ExecuteSpiderCommand
s32 XObjSpider::HandleEventCommand(s32 argc, s32* argv)
{
	for (int i = 0; i < argc; i++) { }
	switch (argv[0]) {
	case 100: {
		const s32 objectId = argv[1];
		if (objectId == 0) {
			mFlags.mFlags &= ~0x20000000;
			for (u32 i = 0; i < 8; i++) {
				if (mTies[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mTies[i].mObj->mJObj);
					mTies[i].mObj   = NULL;
					mTies[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mTies[i].mObj != NULL && i < 8; i++) { }
			if (i < 8) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i, objectId);
#endif
				mFlags.mFlags |= 0x20000000;
				if (argv[1] >= 10000) {
					mTies[i].mObj = FindObj(argv[1]);
				} else {
					mTies[i].mObj = gGame.GetObject((s16)argv[1]);
				}
#if !defined(VERSION_PAL)
				if (mTies[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = argv[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = argv[3 + j];
				}
				mTies[i].mJoint = GetJointByPath(mJObj, path, depth);
				Mtx mtx;
				XObj* obj = mTies[i].mObj;
				PSMTXTrans(mtx, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXConcat(HSD_JObjGetMtxPtr(mTies[i].mJoint), mtx, mTies[i].mMtx);
				HSD_JObjCopyMtx(mTies[i].mObj->mJObj, mTies[i].mMtx);
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
 * @note Address: 0x800F2364
 * @note Size: 0xF0
 */
void XObjSpider::OnHitFromPosition(const Vec& from, f32 strength)
{
	if (mFlags.mFlags & 2)
		return;
	if (mFlags.mFlags & 0x10)
		return;
	if (mFlags.mFlags & 0x8000)
		return;
	if (GetAttachmentCountById(&gGame.GetPlayer()->mAttachments, 2) < 2)
		return;
	OnHitFromPosition__4XObjFRC3Vecf(this, &from, strength);
	mFlags.mFlags |= 2;
	Vec pos      = { 0.0f, 0.0f, 0.0f };
	pos          = from;
	mHitPos      = pos;
	mHitStrength = strength;
}

/**
 * @note Address: 0x800F2350
 * @note Size: 0x14
 */
s32 XObjSpider::OnPlayerContact(s32 unused, s32 value)
{
	mFlags.mFlags |= 1;
	return value;
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
 * @note Address: 0x800F0090
 * @note Size: 0xAEC
 */
extern "C" void UpdateSpiderShot(XObjSpider* spider, u32 frames)
{
	switch (spider->mShotState) {
	case 0:
		break;
	case 1:
		if (spider->mWall == 3) {
			{
				CJObj joint;
				s32 path[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
				joint.Attach(FindJointByPath(spider->mJObj, path, 8));
				CVec origin(GetJointPosition(joint.mJObj));
				StepSpiderShotDistance(spider, origin);
				f32 dx = spider->mShotTo.x - origin.x;
				f32 dz = spider->mShotTo.z - origin.z;

				f32 dy                  = spider->mShotTo.y - origin.y;
				f32 pitch               = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
				f32 angle               = dx > 0.0f ? pitch - HALF_PI : -(HALF_PI + pitch);
				f32 shotX               = origin.x + spider->mShotProgress * (spider->mShotRange * (f32)cos(angle));
				f32 shotY               = origin.y + spider->mShotProgress * (spider->mShotRange * (f32)sin(angle));
				spider->mShotPosition.x = shotX;
				spider->mShotPosition.y = shotY;
				spider->mShotPosition.z = origin.z;
				CVec from(spider->mShotOrigin), to(spider->mShotPosition);
				spider->mCarry.mPos   = from;
				spider->mCarry.mAngle = atan2(to.y - spider->mCarry.mPos.y, to.x - spider->mCarry.mPos.x);
			}
			FinishSpiderShotStep(spider);
		} else {
			{
				CJObj joint;
				s32 path[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
				joint.Attach(FindJointByPath(spider->mJObj, path, 8));
				CVec origin(GetJointPosition(joint.mJObj));
				StepSpiderShotDistance(spider, origin);
				f32 dx = spider->mShotTo.x - origin.x;
				f32 dz = spider->mShotTo.z - origin.z;

				f32 angle               = atan2(dx, dz);
				f32 shotX               = origin.x + spider->mShotProgress * (spider->mShotRange * (f32)sin(angle));
				f32 shotZ               = origin.z + spider->mShotProgress * (spider->mShotRange * (f32)cos(angle));
				spider->mShotPosition.x = shotX;
				spider->mShotPosition.y = 8.0f;
				spider->mShotPosition.z = shotZ;
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
 * @note Address: 0x800F0B7C
 * @note Size: 0x4EC
 */
// NONMATCHING: the return temp and the target sit the other way on the stack
// shoots the carried model from its joint, ahead of it or at the robot
extern "C" void LaunchSpiderShot(XObjSpider* spider)
{
	CJObj joint;
	s32 path[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
	joint.Attach(FindJointByPath(spider->mJObj, path, 8));
	CVec pos(GetJointPosition(joint.mJObj));
	switch (spider->_25C) {
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
		Vec chr         = gGame.GetPlayer()->mMoveTranslate;
		spider->mShotTo = chr;
		break;
	}
	spider->mShotState = 1;
	spider->_A48       = 10;
	{
		XObj* self = gGame.mXObjs.mPools[17].Get(spider->mPoolIndex);
		CVec to(spider->mShotTo);
		spider->mCarry.mFlags.mFlags |= 1;
		spider->mCarry.mPos.x = pos.x;
		spider->mCarry.mPos.y = pos.y;
		spider->mCarry.mPos.z = pos.z;
		spider->mCarry.mAngle = atan2(to.y - spider->mCarry.mPos.y, to.x - spider->mCarry.mPos.x);
		spider->mCarry.mObj   = self;
		spider->mCarry.mState = 1;
	}
	PlayGameSound(&gGame.mSound, 0x12A, 255);
}

/**
 * @note Address: 0x800F1068
 * @note Size: 0x6C8
 */
// NONMATCHING: the matrix pointers and the tie loop take other registers
// reads the object it holds up and keeps it on its joint, and moves the tied objects
// along with their joints
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
			spider->mEffect = ReadObj(spider->mEffectId);
		} else {
			spider->mEffect = NULL;
		}
		spider->mEffectOn++;
	case 2:
		if (spider->mEffect != NULL) {
			if (spider->mEffect->mStatus.mFlags & 0x0040000000000000ULL) {
				break;
			}
			gGame.mStatus.mFlags |= 0x1000;
		}
		spider->mEffectOn++;
	case 3:
		if (spider->mEffectId < 0 || spider->mEffect == NULL) {
			break;
		}
		spider->mEffect->SetScale(1.0f, 1.0f, 1.0f, 0);
		CJObj joint;
		s32 holdPath[] = { 0 };
		joint.Attach(FindJointByPath(spider->mJObj, holdPath, 1));
		PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), spider->mEffectMtx);
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
		SetHeldPropMatrix(spider->mEffect, mtx);
		spider->mEffect->mStatus.mFlags &= ~0x0200000000000000ULL;
		break;
	}
	if (spider->mFlags.mFlags & 0x20000000) {
		for (u32 i = 0; i < 8; i++) {
			if (spider->mTies[i].mJoint != NULL && spider->mTies[i].mObj != NULL) {
				Mtx mtx;
				XObj* obj = spider->mTies[i].mObj;
				PSMTXTrans(mtx, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXConcat(HSD_JObjGetMtxPtr(spider->mTies[i].mJoint), mtx, spider->mTies[i].mMtx);
				HSD_JObjCopyMtx(spider->mTies[i].mObj->mJObj, spider->mTies[i].mMtx);
			}
		}
	}
}

/**
 * @note Address: 0x800F1B10
 * @note Size: 0x840
 */
// the hits, it loses hits when the robot hits it, more for a hard hit, then blinks red
// for a while. and when the robot touches it, the robot gets hit
extern "C" void UpdateSpiderHitReaction(XObjSpider* spider, u32 frames)
{
	switch (spider->mHitState) {
	case 0:
		if ((spider->mFlags.mFlags & 2) && !(spider->mFlags.mFlags & 0x10)) {
			StopGameSfx(&gGame.mSound, 0x12A);
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
	Vec chr = gGame.GetPlayer()->mMoveTranslate;
	if (spider->mWall == 3) {
		f32 x = chr.x - spider->mShotPosition.x;
		f32 y = 8.0f + chr.y - spider->mShotPosition.y;
		f32 z = chr.z - chr.z;
		if (sqrtf(x * x + y * y + z * z) < 8.0f && (spider->mFlags.mFlags & 0x800)) {
			gGame.GetPlayer()->OnHitFromPosition(chr, 7.0f);
			HitCarried(spider);
		}
	} else {
		f32 x    = chr.x - spider->mShotPosition.x;
		f32 y    = 0.0f;
		f32 z    = chr.z - spider->mShotPosition.z;
		f32 chrY = chr.y;
		if (sqrtf(x * x + y * y + z * z) < 2.0f && (spider->mFlags.mFlags & 0x800) && Abs(chrY - spider->mMoveTranslate.y) < 4.0f) {
			gGame.GetPlayer()->OnHitFromPosition(chr, 7.0f);
			HitCarried(spider);
		}
	}
	ExecuteSpiderCommand(spider, frames);
	UpdateSpiderShot(spider, frames);
}

/**
 * @note Address: 0x800F1730
 * @note Size: 0x3E0
 */
// does what the last event command said
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
		spider->mMoveTranslate.y = (f32)spider->_260 + spider->mHome.y;
		spider->mMoveTranslate.z = spider->mHome.z;
		spider->mMoveTranslate.mTarget.Set(spider->mMoveTranslate.x, spider->mMoveTranslate.y, spider->mMoveTranslate.z);
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
 * @note Address: 0x800F0058
 * @note Size: 0x38
 */
void XObjSpider::State0(u32 frames)
{
	if (_25C == 11 && (mFlags.mFlags & 0x400000))
		SetState(4);
}

/**
 * @note Address: 0x800EFDB0
 * @note Size: 0x2A8
 */
// hit, stops, then plays anim 21 and goes on, or dies when it has no hits left
void XObjSpider::State1(u32 frames)
{
	switch (mPhase) {
	case 0:
		PlayGameSound(&gGame.mSound, 0x129, 255);
		mFlags.mFlags &= ~0x800U;
		mFlags.mFlags |= 0x10;
		mSpeed = 0.0f;
		if (mHits <= 0) {
			mFlags.mFlags |= 0x10000;
			SetState(3);
		} else {
			SetAnim(21, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			++mPhase;
		}
		break;
	case 1:
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		f32 current    = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		if (end - (f32)frames <= current) {
			mFlags.mFlags &= ~0x10U;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			SetState(4);
		}
		break;
	}
}

/**
 * @note Address: 0x800EFDAC
 * @note Size: 0x4
 */
void XObjSpider::State2(u32 frames)
{
}

/**
 * @note Address: 0x800EF958
 * @note Size: 0x454
 */
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
		StopGameSfx(&gGame.mSound, 0x132);
		SetAnim(0x19, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			if (!(gGame.mStatus.mFlags & 0x100)) {
				StartScreenShake(&gGame.mQuake, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f);
			}
			mPhase++;
		}
		break;
	case 2:
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
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

extern "C" f32 GetJointAnimationCurrentFrame(CJObj* obj);
extern "C" f32 GetJointAnimationEndFrame(CJObj* obj);
extern "C" void ApproachHudValue(f32* value, f32* target, f32 step);
extern "C" BOOL IsPlayerFlashingFromHit(XObjChr* chr);

static inline void CombatAnim(XObjSpider* spider, s32 anim, BOOL restart = FALSE, f32 rate = 1.0f)
{
	spider->SetAnim(anim, restart, NULL, NULL, NULL, 0.0f, rate, 0.0f, 10.0f);
}

static inline bool CombatAnimDone(XObjSpider* spider, u32 frames)
{
	f32 now = GetJointAnimationCurrentFrame(spider);
	return GetJointAnimationEndFrame(spider) - frames <= now;
}

#if defined(VERSION_PAL)
#define SPIDER_AT_FRAME(spider, mark)                                                         \
	(((mark) <= GetJointAnimationCurrentFrame(spider) && (mark) >= (spider)->mAnimStartFrame) \
	 || ((mark) <= GetJointAnimationCurrentFrame(spider) && (spider)->mAnimStartFrame > GetJointAnimationCurrentFrame(spider)))
#else
#define SPIDER_AT_FRAME(spider, mark) ((mark) <= GetJointAnimationCurrentFrame(spider) && (mark) >= GetJointAnimationCurrentFrame(spider))
#endif

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

// once the turn's anim is through, the spider faces the way it did before and attacks again
static inline void EndSpiderCombatTurn(XObjSpider* spider, u32 frames)
{
	if (CombatAnimDone(spider, frames)) {
		spider->mDir = spider->mSavedDir;
		CombatAnim(spider, 15);
		spider->mFacing    = spider->mDir;
		spider->mTurnSpeed = .1f;
		spider->mPhase     = 1;
	}
}

/**
 * @note Address: 0x800EE050
 * @note Size: 0x1804
 */
void XObjSpider::State4(u32 frames)
{
	Vec pos = gGame.GetPlayer()->mMoveTranslate;
	CVec chr(pos);
	f32 dist   = GetSpiderDistanceXZ(&chr, &mMoveTranslate);
	BOOL event = gGame.mEvt.mData.mScript != NULL;
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
			_98C = mFacing;
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
			if (CombatAnimDone(this, frames))
				CombatAnim(this, 6, TRUE);
			else
				CombatAnim(this, 6);
		} else if (mDir < mFacing) {
			if (CombatAnimDone(this, frames))
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
		if (CombatAnimDone(this, frames)) {
			CombatAnim(this, 10);
			mCounter = rand() % 5 + 5;
			++mPhase;
		}
		break;
	case 3: {
		mDir     = CombatHeading(this, chr);
		f32 diff = mDir - mFacing;
		if (diff < 0) {
			if (CombatAnimDone(this, frames))
				CombatAnim(this, 14, TRUE);
		} else if (diff > 0) {
			if (CombatAnimDone(this, frames))
				CombatAnim(this, 13, TRUE);
		} else if (CombatAnimDone(this, frames))
			CombatAnim(this, 10);
		mFacing = mDir;
		if (--mCounter < 0 && SpiderInView(this)) {
			CombatAnim(this, 24);
			PlayGameSound(&gGame.mSound, 0x1A0, 255);
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
		if (CombatAnimDone(this, frames)) {
			CombatAnim(this, 1);
			mFacing = mDir;
			mPhase  = 1;
		}
		break;
	case 5:
		if (CombatAnimDone(this, frames)) {
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
		    && Abs(chr.y - mMoveTranslate.y) < 8.0f) {
			gGame.GetPlayer()->OnHitFromPosition(pos, 10);
			if (!(gGame.mStatus.mFlags & 0x100))
				StartScreenShake(&gGame.mQuake, 0, 2, 0, 0, 0, 0, 0.4f);
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
		if (CombatAnimDone(this, frames)) {
			mHome      = mMoveTranslate;
			mTurnSpeed = .1f;
			mPhase     = 0;
		}
		break;
	case 8:
		CombatFootsteps(this);
		if (GetSpiderDistanceXZ(&mTarget, &mMoveTranslate) <= 1.0f) {
			mSpeed = 0;
			if (CombatAnimDone(this, frames)) {
				CombatAnim(this, 1);
				mCounter = 0;
				mPhase   = 0;
			}
		}
		break;
	case 10:
		mDir -= 0.10471976f;
		EndSpiderCombatTurn(this, frames);
		break;
	case 11:
		mDir += 0.10471976f;
		EndSpiderCombatTurn(this, frames);
		break;
	case 20:
#if defined(VERSION_PAL)
		if ((30.0f <= GetJointAnimationCurrentFrame(this) && 30.0f > mAnimStartFrame)
		    || (30.0f <= GetJointAnimationCurrentFrame(this) && mAnimStartFrame > GetJointAnimationCurrentFrame(this)))
#else
		if (SPIDER_AT_FRAME(this, 30) || SPIDER_AT_FRAME(this, 31) || SPIDER_AT_FRAME(this, 32))
#endif
		{
			LaunchSpiderShot(this);
			++mPhase;
		}
		break;
	case 21:
		if (!(mCarry.mFlags.mFlags & 1)) {
			StopGameSfx(&gGame.mSound, 0x12A);
			mCounter = rand() % 30 + 30;
			if (CombatAnimDone(this, frames))
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
		if (CombatAnimDone(this, frames)) {
			CombatAnim(this, 8);
			Vec other = gGame.mXObjs.mPools[17].Get(mDroppedSpiderIndex)->mMoveTranslate;
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
			if (CombatAnimDone(this, frames)) {
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
				if (CombatAnimDone(this, frames)) {
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
 * @note Address: 0x800EF854
 * @note Size: 0x104
 */
extern "C" f32 GetSpiderDistanceXZ(Vec* first, Vec* second)
{
	return DistXZ(first, second);
}

/**
 * @note Address: 0x800EDEE4
 * @note Size: 0x16C
 */
// lands, drops into its anim, then takes the first free slot
void XObjSpider::State5(u32 frames)
{
	switch (mPhase) {
	case 0:
		mFlags.mFlags |= 0x40;
		mFlags.mFlags |= 0x400;
		mFlags.mFlags |= 0x8000;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
 * @note Address: 0x800ED2A4
 * @note Size: 0xC40
 */
// hangs from the ceiling and swings, drops down its thread when the robot comes near,
// climbs back, then takes a slot and walks off
void XObjSpider::State6(u32 frames)
{
	Vec pos = GetChrPos();
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
			SetAnim(0x11, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mPhase = 1;
		}
		break;
	case 1:
		mCounter++;
		f32 swing        = (f32)sin(DEG2RAD * (f32)((mCounter * 4) % 360));
		f32 offset       = 8.0f * swing;
		mMoveTranslate.x = mHome.x + offset - 4.0f;
		if (_25C == 11) {
			if (mFlags.mFlags & 0x200000) {
				PlayGameSound(&gGame.mSound, 0x131, 255);
				mFlags.mFlags |= 0x100000;
				mPhase++;
				SetAnim(0x10, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mSpeed = -1.6f;
			}
		} else {
			f32 dist;
			if (mWall == 3) {
				f32 x = chr.x - mMoveTranslate.x;
				f32 y = chr.y - mMoveTranslate.y;
				f32 z = 0.0f;
				dist  = sqrtf(x * x + y * y + z * z);
			} else {
				f32 x = chr.x - mMoveTranslate.x;
				f32 y = 0.0f;
				f32 z = chr.z - mMoveTranslate.z;
				dist  = sqrtf(x * x + y * y + z * z);
			}
			if (dist <= 68.0f && gGame.IsRunning() && gGame.mEvt.mData.mScript == NULL) {
				if (CountDroppedSpiders() < 10) {
					PlayGameSound(&gGame.mSound, 0x131, 255);
					mFlags.mFlags |= 0x100000;
					mPhase++;
					SetAnim(0x10, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					mSpeed = -1.6f;
				}
			}
		}
		break;
	case 2:
		if (mMoveTranslate.y < 20.0f + mHome.y) {
			StopGameSfx(&gGame.mSound, 0x131);
			mPhase++;
			SetAnim(0x12, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mDir    = atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mFacing = mDir;
			mSpeed  = 0.0f;
		}
		break;
	case 3:
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mPhase++;
			SetAnim(0x13, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mDir    = atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mFacing = mDir;
			mSpeed  = -0.5f;
		}
		break;
	case 4:
		if (mMoveTranslate.y < 8.0f + mHome.y) {
			mPhase++;
			SetAnim(0x14, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			PlayGameSound(&gGame.mSound, 0x128, 255);
		}
		break;
	case 5:
		if (mMoveTranslate.y < mHome.y) {
			mMoveTranslate.y = mHome.y;
			mSpeed           = 0.0f;
		}
		f32 landEnd = GetAnimEndFrame(mJObj);
		if (landEnd - frames <= GetAnimFrame(mJObj)) {
			mFlags.mFlags &= ~0x40;
			mFlags.mFlags &= ~0x80;
			mFlags.mFlags &= ~0x100;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mFlags.mFlags &= ~0x8000;
			TakeSlot(this);
			SetState(4);
		}
		break;
	case 6:
		if (pad.mPad->mTrigger & 0x200) {
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

#if defined(VERSION_PAL)
#define REACHED_STEP_FRAME(frame)                                   \
	(((frame) <= GetAnimFrame(mJObj) && (frame) >= mAnimStartFrame) \
	 || ((frame) <= GetAnimFrame(mJObj) && mAnimStartFrame > GetAnimFrame(mJObj)))
#else
#define REACHED_STEP_FRAME(frame) ((frame) <= GetAnimFrame(mJObj) && (frame) >= GetAnimFrame(mJObj))
#endif

/**
 * @note Address: 0x800EC91C
 * @note Size: 0x988
 */
// backs off to its home spot, its steps sounding, then waits out the event
void XObjSpider::State7(u32 frames)
{
	switch (mPhase) {
	case 0:
		SetAnim(0xC, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		mSpeed = 0.0f;
		mPhase++;
		break;
	case 1:
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			SetAnim(8, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			mTarget.x = mHome.x;
			mTarget.y = mMoveTranslate.y;
			mTarget.z = mHome.z;
			mDir      = PI + (f32)atan2(mTarget.x - mMoveTranslate.x, mTarget.z - mMoveTranslate.z);
			mSpeed    = -0.5f;
			mPhase++;
		}
		break;
	case 2:
		if (REACHED_STEP_FRAME(1.0f)) {
			PlayStep(this);
		} else if (REACHED_STEP_FRAME(15.0f)) {
			PlayStep(this);
		}
		if (DistXZ(&mTarget, &mMoveTranslate) <= 2.0f) {
			mSpeed      = 0.0f;
			f32 walkEnd = GetAnimEndFrame(mJObj);
			if (walkEnd - frames <= GetAnimFrame(mJObj)) {
				SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
#undef REACHED_STEP_FRAME

/**
 * @note Address: 0x800EC918
 * @note Size: 0x4
 */
void XObjSpider::State8(u32 frames)
{
}

/**
 * @note Address: 0x800EAF18
 * @note Size: 0xA10
 */
// the slot model's anims, it opens, follows mObj's joint while the anim plays, then
// closes and hides
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
		gSpiderCarryModels[slot]->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		carry->mState++;
		break;
	case 2:
		if (carry->mObj != NULL) {
			CJObj joint;
			s32 path[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
			joint.Attach(FindJointByPath(carry->mObj->mJObj, path, 8));
			carry->mPos = GetJointPosition(joint.mJObj);
		}
		f32 end = GetAnimEndFrame(gSpiderCarryModels[slot]->mJObj);
		if (end - frames <= GetAnimFrame(gSpiderCarryModels[slot]->mJObj)) {
			gSpiderCarryModels[slot]->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			carry->mState++;
		}
		break;
	case 3:
		if (carry->mObj != NULL) {
			CJObj joint;
			s32 path[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
			joint.Attach(FindJointByPath(carry->mObj->mJObj, path, 8));
			carry->mPos = GetJointPosition(joint.mJObj);
		}
		f32 closeEnd = GetAnimEndFrame(gSpiderCarryModels[slot]->mJObj);
		if (closeEnd - frames <= GetAnimFrame(gSpiderCarryModels[slot]->mJObj)) {
			gSpiderCarryModels[slot]->SetAnim(0, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			carry->mState = 0;
			carry->mFlags.mFlags &= ~1;
			carry->mFlags.mFlags &= ~2;
		}
		break;
	}
}

/**
 * @note Address: 0x800EBE20
 * @note Size: 0xAF8
 */
// a test mode, the pad's direction sends it a step that way, up its thread and back
void XObjSpider::State9(u32 frames)
{
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x20) {
		mPhase           = 0;
		mSpeed           = 0.0f;
		mMoveTranslate.x = mHome.x;
		mMoveTranslate.y = mHome.y;
		mMoveTranslate.z = mHome.z;
		SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		mFlags.mFlags &= ~0x100;
	}
	switch (mPhase) {
	case 0:
		if (pad.mPad->mTrigger & 0x200) {
			u32 button = pad.mPad->mButton;
			if (button & 8) {
				mTarget.x = mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z - 60.0f;
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & 4) {
				mTarget.x = mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = 60.0f + mHome.z;
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & 1) {
				mTarget.x = mHome.x - 60.0f;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z;
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			} else if (button & 2) {
				mTarget.x = 60.0f + mHome.x;
				mTarget.y = mHome.y;
				mTarget.z = mHome.z;
				SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				mPhase++;
				mSpeed = 0.3f;
				mDir   = PI;
			}
		}
		break;
	case 1:
		if (HeadTo(this, mTarget) < 20.0f) {
			mPhase++;
			SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		break;
	case 2:
		if (HeadTo(this, mTarget) < 1.0f) {
			mFlags.mFlags |= 0x100;
			mSpeed = 0.0f;
		}
		HSD_AObj* aobj = GetAObj(mJObj);
		f32 end        = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		aobj           = GetAObj(mJObj);
		if (end - frames <= (aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f)) {
			mFlags.mFlags |= 0x100;
			mSpeed = 0.0f;
			mPhase++;
			SetAnim(5, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
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
		f32 backEnd = GetAnimEndFrame(mJObj);
		if (backEnd - frames <= GetAnimFrame(mJObj)) {
			done++;
		}
		if (done >= 2) {
			mPhase = 0;
			SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		break;
	}
	}
}

/**
 * @note Address: 0x800EA99C
 * @note Size: 0x1BC
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
 * @note Address: 0x800EAB58
 * @note Size: 0x3C0
 */
extern "C" void DrawSpiderCarryModel(CarryInfo* carry, u8 slot, bool sideways)
{
	if (slot != 255 && (carry->mFlags.mFlags & 1)) {
		XObj* model = gSpiderCarryModels[slot];
		if (model != NULL) {
			HSD_JObj* joint = model->mJObj;
			HSD_JObjSetTranslateX(joint, carry->mPos.x);
			HSD_JObjSetTranslateY(joint, carry->mPos.y);
			HSD_JObjSetTranslateZ(joint, carry->mPos.z);
			if (sideways) {
				HSD_JObjSetRotationX(joint, HALF_PI);
				HSD_JObjSetRotationZ(joint, HALF_PI + carry->mAngle);
			} else {
				HSD_JObjSetRotationY(joint, carry->mAngle);
			}
			HSD_JObjDispAll(joint, NULL, 7, 0);
		}
	}
}

#if defined(VERSION_PAL)
void XObjSpider::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	mAnimStartFrame = frame;
}
#endif
/**
 * @note Address: 0x800EA40C
 * @note Size: 0x148
 */
// NONMATCHING: the jobj and the anim id swap registers
void XObjSpider::UpdateAnimationSounds()
{
	if (mStatus.mFlags & 1ULL) {
		if (mStatus.mFlags & 0x10000ULL) {
			HSD_JObj* joint = mJObj;
			s32 id          = mAnimId;
			HSD_AObj* aobj  = GetAObj(joint);
			f32 frame       = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
			mSfx.Update(id, frame, GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0));
		}
	}
}

/**
 * @note Address: 0x800EA554
 * @note Size: 0x22C
 */
// NONMATCHING: the pool loop steps another register
// puts one of the free coins at position and plays its anim
extern "C" void SpawnSpiderBounceCoin(Vec* position, Vec* target, s32 anim)
{
	if (gSpiderCoinBounceCount >= 11)
		return;
	u32 stride = gGame.mXObjs.mPools[7].mStride;
	u8* pool   = gGame.mXObjs.mPools[7].mObjects;
	u32 offset = 0;
	for (s32 i = 0; i < 96; ++i, offset += stride) {
		XObjCoin* coin = (XObjCoin*)(pool + offset);
		if (coin->mStatus.mFlags & 1ULL) {
			if (coin->GetKind() == 0x104) {
				if ((coin->mStatus.mFlags & 1ULL) && !coin->mBouncing) {
					coin->mBouncing                                   = 1;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPhase = 0;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCount += 2;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.x    = position->x;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.y    = position->y + 0.2f;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mPos.z    = position->z;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.x = target->x;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.y = target->y;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mTarget.z = target->z;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin     = coin;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateX(position->x, 0);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateY(position->y + 0.2f, 0);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetTranslateZ(position->z, 0);
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->mStatus.mFlags |= 0x100000ULL;
					gSpiderCoinBounces[gSpiderCoinBounceCount].mCoin->SetAnim(anim, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					++gSpiderCoinBounceCount;
					break;
				}
			}
		}
	}
}

/**
 * @note Address: 0x800EA780
 * @note Size: 0x21C
 */
// moves the coins, shown, hopping, then falling to where they land
extern "C" void UpdateSpiderCoinBounces(void* unused, u32 frames)
{
	f32 x, y, z;
	f32 lift = 0.2f; // how far the coins float above their spot
	f32 fall = 0.8f * (f32)frames;
	for (s32 i = 0; i < 11; ++i) {
		CoinBounce* entry = &gSpiderCoinBounces[i];
		XObj* coin        = entry->mCoin;
		if (coin != NULL) {
			s32 phase   = entry->mPhase;
			s32 counter = entry->mCount;
			f32 targetY = entry->mTarget.y;
			x           = entry->mPos.x;
			y           = entry->mPos.y;
			z           = entry->mPos.z;
			switch (phase) {
			case 0:
				coin->mStatus.mFlags |= 0x10000ULL;
				++phase;
				break;
			case 1: {
				++counter;
				f32 height = 10.0f * (f32)sin(DEG2RAD * (f32)((counter * 16) % 360)) + lift;
				if (height < lift) {
					coin->SetTranslateY(y + lift, 0);
					++phase;
				} else {
					coin->SetTranslateY(y + height, 0);
				}
				break;
			}
			case 2:
				y -= fall;
				if (y <= targetY) {
					coin->mStatus.mFlags &= ~0x100000ULL;
					y = targetY;
					++phase;
				}
				coin->SetTranslateY(y + lift, 0);
				break;
			case 3:
				break;
			}
			entry->mPhase = phase;
			entry->mCount = counter;
			entry->mPos.x = x;
			entry->mPos.y = y;
			entry->mPos.z = z;
		}
	}
}

// takes all the coins back
static inline void ResetCoins()
{
	gSpiderCoinBounceCount = 0;
	for (int i = 0; i < 11; i++) {
		gSpiderCoinBounces[i].mCoin  = NULL;
		gSpiderCoinBounces[i].mPhase = 0;
		gSpiderCoinBounces[i].mCount = 0;
	}
	for (int i = 0; i < 96; i++) {
		XObjCoin* coin = (XObjCoin*)gGame.mXObjs.mPools[7].Get(i);
		if (coin->GetKind() == 0x104) {
			coin->mBouncing = 0;
		}
	}
}

/**
 * @note Address: 0x800EB928
 * @note Size: 0x108
 */
void PlaySpiderVariantSound(XObjSpider* spider)
{
	switch (spider->mPoolIndex % 5) {
	case 0:
		PlayGameSound(&gGame.mSound, 0x12B, 255);
		break;
	case 1:
		PlayGameSound(&gGame.mSound, 0x12C, 255);
		break;
	case 2:
		PlayGameSound(&gGame.mSound, 0x12D, 255);
		break;
	case 3:
		PlayGameSound(&gGame.mSound, 0x12E, 255);
		break;
	case 4:
		PlayGameSound(&gGame.mSound, 0x12F, 255);
		break;
	}
}

/**
 * @note Address: 0x800EBA30
 * @note Size: 0x1C0
 */
// the index of another spider that dropped down closer than 24, -1 when there is none
s32 FindNearbyDroppedSpider(XObjSpider* spider)
{
	for (int i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObjSpider* other = (XObjSpider*)obj;
			if ((other->mFlags.mFlags & 0x100000) && other->mPoolIndex != spider->mPoolIndex) {
				f32 x = other->mMoveTranslate.x - spider->mMoveTranslate.x;
				f32 y = other->mMoveTranslate.y - spider->mMoveTranslate.y;
				f32 z = other->mMoveTranslate.z - spider->mMoveTranslate.z;
				if (sqrtf(x * x + y * y + z * z) < 24.0f) {
					return other->mPoolIndex;
				}
			}
		}
	}
	return -1;
}

/**
 * @note Address: 0x800EBBF0
 * @note Size: 0x1C0
 */
// whether another spider that dropped down is closer than 24
bool HasNearbyDroppedSpider(XObjSpider* spider)
{
	for (int i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObjSpider* other = (XObjSpider*)obj;
			if ((other->mFlags.mFlags & 0x100000) && other->mPoolIndex != spider->mPoolIndex) {
				f32 x = other->mMoveTranslate.x - spider->mMoveTranslate.x;
				f32 y = other->mMoveTranslate.y - spider->mMoveTranslate.y;
				f32 z = other->mMoveTranslate.z - spider->mMoveTranslate.z;
				if (sqrtf(x * x + y * y + z * z) < 24.0f) {
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x800EBDB0
 * @note Size: 0xC
 */
u8 HasSpiderFinishedDeathAnimation(const XObjSpider* spider)
{
	return (spider->mFlags.mFlags >> 19) & 1;
}

/**
 * @note Address: 0x800EBDBC
 * @note Size: 0x64
 */
// whether it is inside its area, kept mRadius off the edges
bool IsSpiderInsideMovementArea(XObjSpider* spider)
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

/**
 * @note Address: 0x800F4350
 * @note Size: 0x4C8
 */
// NONMATCHING: .rodata, retail has six more Vecs in front of the zero Vecs
// puts everything back to how it starts
void ResetSpiderState(XObjSpider* spider)
{
	spider->mDir          = 0.0f;
	spider->mSpeed        = 0.0f;
	spider->mFacing       = 0.0f;
	spider->_98C          = 0.0f;
	spider->mTurnSpeed    = 0.1f;
	spider->mFallSpeed    = 0.0f;
	spider->mState        = 0;
	spider->mPrevState    = 0;
	spider->mPhase        = 0;
	spider->mCounter      = 0;
	spider->mFlags.mFlags = 0;
	spider->mWait         = 0;
	spider->_9C0          = 1.0f;
	spider->_9C4          = 1.0f;
	Vec hitPos            = { 0.0f, 0.0f, 0.0f };
	spider->mHitPos       = hitPos;
	Vec home              = { 0.0f, 0.0f, 0.0f };
	spider->mHome         = home;
	Vec wallPos           = { 0.0f, 0.0f, 0.0f };
	spider->mWallPos      = wallPos;
	Vec target            = { 0.0f, 0.0f, 0.0f };
	spider->mTarget       = target;
	Vec v08               = { 0.0f, 0.0f, 0.0f };
	spider->mShotOrigin   = v08;
	Vec v14               = { 0.0f, 0.0f, 0.0f };
	spider->mShotPosition = v14;
	spider->mShotRange    = 0.0f;
	spider->mShotProgress = 0.0f;
	spider->mShotState    = 0;
	spider->_A48          = 0;
	spider->mHitStrength  = 0.0f;
	spider->mWall         = 0;
	spider->mMaxHits      = 3;
	spider->mHits         = spider->mMaxHits;
	spider->mCommand      = 0;
	spider->mArgs[0]      = 0;
	spider->_A88          = 0;
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
	spider->_A6C                 = 0.0f;
	spider->_B10                 = 2;
	spider->_B14[0]              = 0;
	spider->_B14[1]              = 0;
	spider->_B14[2]              = 0;
	spider->_B14[3]              = 0;
	spider->_B24[0]              = 100;
	spider->_B24[1]              = 100;
	spider->_B24[2]              = 100;
	spider->_B54                 = 0;
	spider->_B50                 = 0;
	spider->mEffect              = NULL;
	spider->mEffectId            = 0;
	spider->mEffectOn            = 0;
	for (int i = 0; i < 8; i++) {
		spider->mTies[i].mObj   = NULL;
		spider->mTies[i].mJoint = NULL;
	}
	if (spider->mPoolIndex == 0) {
		ResetCoins();
	}
	spider->mHitState           = 0;
	spider->mHitTimer           = 0;
	spider->mDroppedSpiderIndex = -1;
	spider->mRumble             = 0;
	spider->mRumbleTimer        = 0;
}
