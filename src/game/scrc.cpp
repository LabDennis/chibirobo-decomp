#include "game/xobjactor.h"
#include "game/xobjspiderboss.h"
#include "game/xobjspider.h"
#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/cpad.h"
#include "game/cscrn.h"
#include "game/xchr.h"
#include "game/infoscrn.h"
#include "game/stagedata.h"
#include "game/ctpl.h"
#include "game/ut.h"
#include "game/gmath.h"
#include "game/animframe.h"
#include "game/video.h"
#include "game/spiderplacement.h"
#include <sysdolphin/rumble.h>
#include <stdlib.h>

// XObjActor3, the actor most of the stage's small moving things are. which one it is comes
// from the stage table, see GetKind and GetObjFlags

extern "C" {
u16 GetItemFlagsMasked(s16 item, s32 kind);
BOOL IsPlayerFlashingFromHit(XObj* obj);
void HideScrapCounter(void* mgr);
void ShowScrapCounter(void* mgr);
void HideMoolahCounter(void* mgr);
void ShowMoolahCounter(void* mgr);
void HideHappyPointCounter(void* mgr);
void ShowHappyPointCounter(void* mgr);
CVec GetStagePlacementScale(XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void PSMTXRotRad(Mtx m, char axis, f32 rad);
void LaunchSpiderBossPart(XObjActor3* actor);
void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames);
void InitParticlePool(CPtcl* pool, void* particles, s32 count, s32 stride);
void ResetActor3Fields(XObjActor3* actor);
extern AnimSfxEvent gHaisuiNezumiAnimSfxEvents[];

Vec gActor3PlacementPosition;
Vec gActor3PlacementRotation;
extern s32 gEventVariables[]; // the script's variables

s32 gSpiderBossVolleySlot;
s32 gActor3Kind15ActivationCounter;
}

static inline Vec ActorPlacementRotation(const XObjActor3* actor)
{
	Vec rot              = gActor3PlacementRotation;
	const UnkStageObj* p = actor->mCoord._00;
	rot.x                = DEG2RAD * p->_10[0];
	rot.y                = DEG2RAD * p->_10[1];
	rot.z                = DEG2RAD * p->_10[2];
	return rot;
}

// the robot is out of reach, past the front edge or in one of its climbing anims
static inline bool IsPlayerAway(XObjChr* player, Vec pos)
{
	if (pos.z < -4.28f) {
		return true;
	}
	switch (player->mAnimId) {
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
		return true;
	}
	return false;
}

// the anims the robot reaches further in
static inline bool IsLongReachAnim(s32 anim)
{
	switch (anim) {
	case 0x4:
	case 0xD:
	case 0x35:
	case 0x79:
	case 0x89:
	case 0x10D:
	case 0x819:
	case 0x898:
		return true;
	}
	return false;
}

// the robot's anims it can't be talked to in
static inline bool IsBusyAnim(s32 anim)
{
	switch (anim) {
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x8:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0x27:
	case 0x28:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x43:
	case 0x97:
	case 0xA1:
	case 0xED:
	case 0xEE:
	case 0xF6:
	case 0x814:
	case 0x816:
	case 0x817:
	case 0x81F:
	case 0x820:
	case 0x824:
	case 0x879:
	case 0x87A:
	case 0x8AE:
	case 0x8AF:
	case 0x8B7:
	case 0x8C0:
	case 0x8C1:
	case 0x8C2:
	case 0x8C3:
	case 0x8C4:
	case 0x8C5:
		return true;
	}
	return false;
}

extern "C" BOOL IsPlayerWithinActorReach(XObjActor3*);
extern "C" BOOL IsUsingMug(XActLift*);
extern "C" f32 GetPlayerCollisionRadius(XObjChr*);

static inline void StartKind9Effect(XObjActor3* actor, u32 id, f32 size)
{
	Vec p, scale;
	p.x     = actor->mMoveTranslate.x;
	p.y     = 12.0f + actor->mMoveTranslate.y;
	p.z     = 12.0f + actor->mMoveTranslate.z;
	scale.x = scale.y = scale.z = size;
	actor->_1DC0[0]             = (s32)gGame.mParticle.CreateSRT(id, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
}

static inline void EmitKind9Dust(XObjActor3* actor)
{
	f32 x = actor->mMoveTranslate.x, y = actor->mMoveTranslate.y, z = actor->mMoveTranslate.z;
	f32 angle  = DEG2RAD * (rand() % 360);
	f32 sine   = (f32)sin(angle);
	f32 vx     = sine * (rand() % 100) / 1000.0f;
	f32 cosine = (f32)cos(angle);
	f32 vz     = cosine * (rand() % 100) / 1000.0f;
	f32 vy     = (rand() % 100) / 800.0f;
	Vec pos;
	pos.x          = x + 16.0f * (vx * (rand() % 100) / 100.0f);
	pos.y          = y;
	pos.z          = z + 16.0f * (vz * (rand() % 100) / 100.0f);
	f32 width      = 1.0f + (rand() % 200) / 200.0f;
	Vec scale      = { width, width, 1 };
	Vec vel        = { vx, vy, vz };
	CPtclOne* dust = SpawnPoolParticle(&actor->mDust.mPtcl, &pos, &scale, &vel, 0xffffffa0);
	if (dust) {
		dust->mAngle      = DEG2RAD * (rand() % 360);
		dust->mAngleSpeed = DEG2RAD * (20 - rand() % 40);
		dust->mColor      = 0xffffffa0;
		dust->mTex        = &actor->mTex[0];
	}
}

static inline void StunKind9Actor(XObjActor3* actor)
{
	actor->SetAnim(11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	PlayGameSound(&gGame.mSound, 0xb0004, 255);
	actor->_B8C.mFlags |= 0x200;
	actor->_B80 = 20;
}

static inline bool Kind9AnimFinished(XObjActor3* actor, u32 frames)
{
	f32 end     = GetAnimEndFrame(actor->mJObj);
	f32 current = GetAnimFrame(actor->mJObj);
	return end - frames <= current;
}

void XObjActor3::Kind9(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	CJObj joint;
	Vec pos = player->mMoveTranslate;
	if ((_B8C.mFlags & 0x20) && _B80 < 5 && _B80 >= 1 && (s32)--_BA4[6] < 0)
		_B8C.mFlags &= ~0x20;
	if (_B80 != 0 && _B80 < 20 && Abs(pos.x - mMoveTranslate.x) <= 10 && pos.y >= 4 + mMoveTranslate.y && pos.y <= 8 + mMoveTranslate.y)
		StunKind9Actor(this);
	switch (_B80) {
	case 0:
		if (!(_B8C.mFlags & 4)) {
			_B88 = 30;
			++_B80;
		}
		break;
	case 1:
		SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (--_B88 < 0) {
			_B88 = rand() % 120 + 120;
			_B7C = rand() % 2 == 0 ? -HALF_PI : HALF_PI;
			_B80 = 2;
		}
		if (IsPlayerWithinActorReach(this))
			_B80 = 5;
		break;
	case 2:
		SetAnim(3, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (_B7C == -HALF_PI)
			mMoveTranslate.x -= 0.1f * frames;
		else
			mMoveTranslate.x += 0.1f * frames;
		if (mMoveTranslate.x < _BC0) {
			mMoveTranslate.x = _BC0;
			_B70             = _B7C;
			_B80             = 3;
			SetAnim(8, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		} else if (mMoveTranslate.x > _BC4) {
			mMoveTranslate.x = _BC4;
			_B70             = _B7C;
			_B80             = 3;
			SetAnim(8, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		}
		if (--_B88 < 0 && (_B8C.mFlags & 0x20)) {
			_B80 = 4;
			_B88 = 90;
			StartKind9Effect(this, 0x28, 3);
		}
		if (IsPlayerWithinActorReach(this))
			_B80 = 5;
		break;
	case 3:
		if (_B70 == HALF_PI) {
			_B7C -= 0.04f * frames;
			if (_B7C <= -HALF_PI) {
				_B7C = -HALF_PI;
				_B80 = 2;
			}
		} else {
			_B7C += 0.04f * frames;
			if (_B7C >= HALF_PI) {
				_B7C = HALF_PI;
				_B80 = 2;
			}
		}
		break;
	case 4:
		SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		if (--_B88 < 0) {
			_B88 = rand() % 120 + 30;
			_B80 = rand() % 2 + 2;
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
		}
		if (IsPlayerWithinActorReach(this)) {
			_B80 = 5;
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
		}
		break;
	case 5:
#ifdef VERSION_PAL
		_BA4[6] = (10 * gVideoFramesPerSecond) / frames;
#else
		_BA4[6] = 600 / frames;
#endif
		_B8C.mFlags |= 0x20;
		StartKind9Effect(this, 0x26, 3);
		_B88 = 15;
		SetAnim(5, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		_B7C = mMoveTranslate.x < pos.x ? HALF_PI : -HALF_PI;
		PlayGameSound(&gGame.mSound, 0xb0005, 255);
		_B80 = 6;
		break;
	case 6:
		if (Kind9AnimFinished(this, frames)) {
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
			s32 path[] = { 0, 0, 1, 1 };
			joint.Attach(GetJointByPath(mJObj, path, 4));
			Vec p = joint.GetPos();
			SetAnim(7, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			if (_B7C == -HALF_PI)
				p.x += 2;
			else {
				p.x -= 2;
				p.z += 4;
			}
			Vec scale = { 1, 1, 1 };
			_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0x84, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
			_B80      = 7;
		}
		break;
	case 7:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(4, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
			_B88 = 2;
			_B80 = 8;
		}
		break;
	case 8:
		if (_B7C == -HALF_PI)
			mMoveTranslate.x -= 0.6f * frames;
		else
			mMoveTranslate.x += 0.6f * frames;
		if (mMoveTranslate.x < _BC0) {
			mMoveTranslate.x = _BC0;
			_B80             = 4;
			_B88             = 90;
			StartKind9Effect(this, 0x28, 3);
		} else if (mMoveTranslate.x > _BC4) {
			mMoveTranslate.x = _BC4;
			_B80             = 4;
			_B88             = 90;
			StartKind9Effect(this, 0x28, 3);
		} else {
			Vec projected = { pos.x, pos.y, mMoveTranslate.z };
			f32 dx = projected.x - mMoveTranslate.x, dy = projected.y - mMoveTranslate.y, dz = projected.z - mMoveTranslate.z;
			if (sqrtf(dz * dz + (dx * dx + dy * dy)) < 32 && !IsPlayerAway(gGame.GetPlayer(), gGame.GetPlayer()->mMoveTranslate)
			    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				SetAnim(9, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				_B80 = 9;
			} else if (--_B88 < 0) {
				_B88 = 2;
				EmitKind9Dust(this);
			}
		}
		break;
	case 9:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(10, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 10;
			PlayGameSound(&gGame.mSound, 0xb0002, 255);
		}
		break;
	case 10: {
		f32 radius = GetPlayerCollisionRadius(gGame.GetPlayer());
		f32 offset;
		if (_B7C == -HALF_PI) {
			mMoveTranslate.x -= 1.4f * (0.6f * frames);
			offset = radius < 2 ? 1.5f : 5.5f;
		} else {
			mMoveTranslate.x += 1.4f * (0.6f * frames);
			offset = radius < 2 ? -1.5f : -5.5f;
		}
		if (mMoveTranslate.x < _BC0) {
			mMoveTranslate.x = _BC0;
			StunKind9Actor(this);
		} else if (mMoveTranslate.x > _BC4) {
			mMoveTranslate.x = _BC4;
			StunKind9Actor(this);
		} else {
			Vec projected = { pos.x + offset, pos.y, mMoveTranslate.z };
			f32 dx = projected.x - mMoveTranslate.x, dy = projected.y - mMoveTranslate.y, dz = projected.z - mMoveTranslate.z;
			if (sqrtf(dz * dz + (dx * dx + dy * dy)) < 10 && !IsPlayerFlashingFromHit(player)
			    && !IsPlayerAway(gGame.GetPlayer(), gGame.GetPlayer()->mMoveTranslate)) {
				CPad pad(0);
				if (radius < 2) {
					SetAnim(11, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
					if (!IsBusyAnim(gGame.GetPlayer()->mAnimId) && gGame.GetPlayer()->mAttachmentRequest < 0
					    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
						_B8C.mFlags |= 0x80;
						gGame.mStatus.mFlags &= ~0x800000000000ULL;
					} else {
						PlayGameSound(&gGame.mSound, 0xb0004, 255);
						_B8C.mFlags |= 0x200;
					}
					_B80 = 20;
				} else if (IsUsingMug(&gGame.GetPlayer()->mLift))
					StunKind9Actor(this);
			}
		}
		break;
	}
	case 20:
		if (--_B88 < 0) {
			_B88 = 2;
			EmitKind9Dust(this);
		}
		if (Kind9AnimFinished(this, frames)) {
			mStatus.mFlags |= 0x100000;
			SetAnim(13, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
#ifdef VERSION_PAL
			_B88 = (3 * gVideoFramesPerSecond) / frames;
#else
			_B88 = 180 / frames;
#endif
			_B80 = 21;
			if (_B8C.mFlags & 0x80)
				_B8C.mFlags &= ~0x80;
			if (_B8C.mFlags & 0x200) {
				StartKind9Effect(this, 5, 1);
				_B8C.mFlags &= ~0x200;
			}
		}
		break;
	case 21: {
		if (gGame.mEvt.mData.mScript)
			break;
		f32 distance = Abs(pos.x - mMoveTranslate.x);
		if (--_B88 < 0 && distance >= 12) {
			_B88 = 0;
			SetAnim(12, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 22;
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
			mStatus.mFlags &= ~0x100000ULL;
		}
		break;
	}
	case 22:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B88 = 0;
			_B80 = 1;
		}
		break;
	case 100:
		if (_B88 & 1)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--_B88 < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			++_B80;
		}
		break;
	}
}

/**
 * @note Address: 0x8010A574
 * @note Size: 0x78C
 */
BOOL XObjActor3::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		bool ready = false;
		_B74       = 0.0f;
		_B78       = 0.0f;

		_B80        = 0;
		_B88        = 0;
		_B8C.mFlags = 0;
		_B8C.mFlags |= 4;
		mEventCommand = 0;
		mEventArg1    = 0;
		mEventArg2    = 0;
		mEventArg3    = 0;
		_BA4[0]       = 0;
		_BA4[1]       = 0;
		_BA4[2]       = 0;
		_BA4[3]       = 0;
		_BA4[4]       = 0;
		_BA4[5]       = 0;
		_BA4[6]       = 0;
		_BC0          = 0.0f;
		_BC4          = 0.0f;
		_BC8          = 0.0f;
		Vec zero      = { 0.0f, 0.0f, 0.0f };
		_BCC          = zero.x;
		_BD0          = zero.y;
		_BD4          = zero.z;

		mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x;
		mMoveTranslate.y = PlacementPosition(mCoord, gActor3PlacementPosition).y;
		mMoveTranslate.z = PlacementPosition(mCoord, gActor3PlacementPosition).z;
		mMoveRotate.x    = ActorPlacementRotation(this).x;
		mMoveRotate.y    = ActorPlacementRotation(this).y;
		mMoveRotate.z    = ActorPlacementRotation(this).z;
		_B7C             = ActorPlacementRotation(this).y;
		for (int i = 0; i < 8; ++i)
			mParts[i].Clear();
		if (mPoolIndex == 0) {
			gSpiderBossVolleySlot          = 0;
			gActor3Kind15ActivationCounter = 0;
		}
		switch (GetKind()) {
		case 0x42B:
		case 0x149:
			if (_980.ReadEx("tpl/stage02.tpl", 0, 0))
				ready = true;
			return ready;
		case 0x31B: {

			mTex[0] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[122 - StatusTextureOffset], 0);
			mTex[1] = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[123 - StatusTextureOffset], 0);
			return TRUE;
		}
		default:
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x8010AD00
 * @note Size: 0x20
 */
void XObjActor3::vf94()
{
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010AD20
 * @note Size: 0x34
 */
void XObjActor3::ResetObjectState()
{
	XObj::ResetObjectState();
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010AD54
 * @note Size: 0x34
 */
void XObjActor3::InitializeObject(u32 id)
{
	XObj::InitializeObject(id);
	ResetActor3Fields(this);
}

/**
 * @note Address: 0x8010AD88
 * @note Size: 0x1EC
 */
extern "C" void ResetActor3Fields(XObjActor3* actor)
{
	actor->_B70        = 0.0f;
	actor->_B74        = 0.0f;
	actor->_B78        = 0.0f;
	actor->_B7C        = 0.0f;
	actor->_B80        = 0;
	actor->_B88        = 0;
	actor->_B8C.mFlags = 0;
	actor->_B8C.mFlags |= 4;
	actor->mEventCommand = 0;
	actor->mEventArg1    = 0;
	actor->mEventArg2    = 0;
	actor->mEventArg3    = 0;
	actor->_BA4[0]       = 0;
	actor->_BA4[1]       = 0;
	actor->_BA4[2]       = 0;
	actor->_BA4[3]       = 0;
	actor->_BA4[4]       = 0;
	actor->_BA4[5]       = 0;
	actor->_BA4[6]       = 0;
	actor->_BC0          = 0.0f;
	actor->_BC4          = 0.0f;
	actor->_BC8          = 0.0f;
	Vec zero             = { 0.0f, 0.0f, 0.0f };
	actor->_BCC          = zero.x;
	actor->_BD0          = zero.y;
	actor->_BD4          = zero.z;
	for (int i = 0; i < 8; i++) {
		actor->mParts[i].Clear();
	}
	if (actor->mPoolIndex == 0) {
		gSpiderBossVolleySlot          = 0;
		gActor3Kind15ActivationCounter = 0;
	}
	actor->vf12C(1, 0, 0, 0, 0.0f);
	actor->_BDC.x             = 0.0f;
	actor->_BDC.y             = 0.0f;
	actor->_BDC.z             = 0.0f;
	actor->_BEC               = 0;
	actor->mTimerPanelAlpha   = 0.0f;
	actor->mCountdownFrames   = 0.0f;
	actor->mCountdownRunning  = 0;
	actor->mCounterFadeMode   = 0;
	actor->mCounterPanelAlpha = 0.0f;
	actor->mSpiderCount       = 0;
	actor->_C08               = 0;
	if (actor->GetKind() == 0x31B) {
		InitParticlePool(&actor->mDust.mPtcl, actor->mDust.mPtcls, 0x50, 0x32);
		actor->mAnimSfx.mEvents = gHaisuiNezumiAnimSfxEvents;
	}
	actor->_1DC0[0]     = 0;
	actor->_1DC0[1]     = 0;
	actor->_1DC0[2]     = 0;
	actor->_1DC0[3]     = 0;
	actor->_1DC0[4]     = 0;
	actor->_1DC0[5]     = 0;
	actor->mRumble      = 0;
	actor->mRumbleTimer = 0;
	actor->mHeld        = NULL;
	actor->_1DE4        = 0;
	actor->mSe          = -1;
}

/**
 * @note Address: 0x80114CC8
 * @note Size: 0x518
 */
void XObjActor3::Kind2(u32)
{
	CJObj joint;
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x411: {
		for (s32 i = 0; i < 0x120; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				s32 path[] = { 0, 0, 3, 0 };
				joint.Attach(GetJointFromRoot(clock->mJObj, 4, path));
				Mtx mount, translate, rotation, result;
				PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -(12.0f * time)));
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				HSD_JObjCopyMtx(mJObj, result);
				return;
			}
		}
		return;
	}
	case 0x413:
		mMoveRotate.z = PI * (2.0f * -(12.0f * time));
		break;
	case 0x435:
		mMoveRotate.x = PI * (2.0f * (12.0f * time));
		break;
	default:
		mMoveRotate.z = PI * (2.0f * (12.0f * time));
	}
}

/**
 * @note Address: 0x801147C0
 * @note Size: 0x508
 */
void XObjActor3::Kind3(u32)
{
	CJObj joint;
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x412: {
		for (s32 i = 0; i < 0x120; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				s32 path[] = { 0, 0, 3, 0 };
				joint.Attach(GetJointFromRoot(clock->mJObj, 4, path));
				Mtx mount, translate, rotation, result;
				PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -time) + 2.617994f);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				HSD_JObjCopyMtx(mJObj, result);
				return;
			}
		}
		return;
	}
	case 0x414:
		mMoveRotate.z = PI * (2.0f * -time) + 2.617994f;
		break;
	case 0x436:
		mMoveRotate.x = PI * (2.0f * time) + 3.6651917f;
		break;
	default:
		mMoveRotate.z = PI * (2.0f * time) + 3.6651917f;
	}
}

/**
 * @note Address: 0x801146F8
 * @note Size: 0xC8
 */
void XObjActor3::Kind4(u32 frames)
{
	_B88++;
	f32 s     = sin(0.017453292f * ((_B88 * 6) % 360));
	f32 swing = 0.25f;
	if (GetKind() == 0x437) {
		mMoveRotate.x = s * swing;
	} else {
		mMoveRotate.z = s * swing;
	}
}

/**
 * @note Address: 0x801146F4
 * @note Size: 0x4
 */
void XObjActor3::Kind5(u32 frames)
{
}

/**
 * @note Address: 0x8011463C
 * @note Size: 0xB8
 */
// NONMATCHING: register allocation
void XObjActor3::Kind6(u32 frames)
{
	s16 id;
	s16 num;
	s32 i;
	s32 count = 0;
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		for (i = 0; i < 0x100; i++) {
			InventoryItem* items = gGame.GetPlayer()->mParams.mInventory.mItems;
			id                   = items[i].mId;
			num                  = items[i].mCount;
			if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
				count++;
			}
		}
		gGame.mEvt._10 = count;
		mEventCommand  = 0;
		break;
	}
}

/**
 * @note Address: 0x80114060
 * @note Size: 0x4E0
 */
// holds the 0x40B item at its first joint while flag 0x100 is up
// NONMATCHING: the loop and the joint's vectors take each other's registers
void XObjActor3::Kind8(u32 frames)
{
	Mtx m;
	Mtx t;
	Mtx a;
	CJObj joint;
	if (_B8C.mFlags & 0x100) {
		CVec pos;
		s32 path[1] = { 0 };
		joint.Attach(GetJointFromRoot(mJObj, 1, path));
		pos      = GetJointPosition(joint.mJObj);
		s32 kind = -1;
		if (_B8C.mFlags & 0x100) {
			kind = 0x40B;
		}
		if (kind >= 0) {
			for (s32 i = 0; i < 0x120; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), m);
					PSMTXTrans(t, 0.0f, -14.0f, 0.0f);
					PSMTXConcat(m, t, a);
					HSD_JObj* jobj = obj->mJObj;
					MtxPtr mp      = a;
					HSD_JObjCopyMtx(jobj, mp);
					if (mStatus.mFlags & 0x10000) {
						obj->mStatus.mFlags |= 0x10000ULL;
					} else {
						obj->mStatus.mFlags &= ~0x10000ULL;
					}
				}
			}
		}
	}
}

/**
 * @note Address: 0x80114540
 * @note Size: 0xFC
 */
void XObjActor3::Kind7(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		if (mEventArg1 != 0) {
			ShowScrapCounter(&gGame.mInfoScrn);
		} else {
			HideScrapCounter(&gGame.mInfoScrn);
		}
		mEventCommand = 0;
		break;
	case 2:
		if (mEventArg1 != 0) {
			ShowHappyPointCounter(&gGame.mInfoScrn);
			ShowMoolahCounter(&gGame.mInfoScrn);
			ShowScrapCounter(&gGame.mInfoScrn);
		} else {
			HideHappyPointCounter(&gGame.mInfoScrn);
			HideMoolahCounter(&gGame.mInfoScrn);
			HideScrapCounter(&gGame.mInfoScrn);
		}
		mEventCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x801115FC
 * @note Size: 0x47C
 */
// falls from above the robot, knocks it down on a hit, blinks out on the floor
void XObjActor3::Kind10(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	switch (_B80) {
	case 0:
		if (_B8C.mFlags & 4) {
			break;
		}
		mMoveTranslate.x = pos.x - (rand() % 3600) / 100.0f + 18.0f;
		mMoveTranslate.y = 64.0f;
		mMoveTranslate.z = pos.z - (rand() % 3600) / 100.0f + 18.0f;
		mStatus.mFlags |= 0x10000;
		mStatus.mFlags |= 4;
		_B70 = 4.0f;
		_B88 = rand() % 30 + 30;
		_B80++;
		break;
	case 1:
		if (--_B88 < 0) {
			_B80++;
		}
		break;
	case 2:
		f32 dx   = pos.x - mMoveTranslate.x;
		f32 dy   = pos.y - mMoveTranslate.y;
		f32 dz   = pos.z - mMoveTranslate.z;
		f32 dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < 8.0f && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 10.0f);
			_B88 = 30;
			_B80 = 3;
			break;
		}
		if (_B70 >= -2.3f) {
			_B70 -= 0.36f * frames;
		} else {
			_B70 = -2.3f;
		}
		mMoveTranslate.y += _B70;
		if (mMoveTranslate.y < 0.0f) {
			mMoveTranslate.y = 0.0f;
			_B88             = 30;
			_B80             = 3;
		}
		break;
	case 3:
		if ((_B88 & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--_B88 < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mStatus.mFlags &= ~4ULL;
			_B8C.mFlags |= 4;
			_B80 = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x8011132C
 * @note Size: 0x2D0
 */
// whether the robot stands where the actor can reach it
extern "C" BOOL IsPlayerWithinActorReach(XObjActor3* actor)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	if (!(actor->_B8C.mFlags & 4)) {
		if (IsPlayerAway(player, pos)) {
			return FALSE;
		}
		if (pos.y >= actor->mMoveTranslate.y - 16.0f && pos.y <= actor->mMoveTranslate.y + 16.0f) {
			if (actor->_B7C == 1.5707964f) {
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->_BC4) {
					return TRUE;
				}
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->_BC0) {
					if (IsLongReachAnim(player->mAnimId)) {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 32.0f) {
							return TRUE;
						}
					} else {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 12.0f) {
							return TRUE;
						}
					}
				}
			} else {
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->_BC0) {
					return TRUE;
				}
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->_BC4) {
					if (IsLongReachAnim(player->mAnimId)) {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 32.0f) {
							return TRUE;
						}
					} else {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 12.0f) {
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

static inline void AdvanceActorAlongHeading(XObjActor3* actor, u32 frames, const f32& speed)
{
	CVec velocity;
	velocity.x              = speed * (f32)sin(actor->_B7C);
	velocity.z              = speed * (f32)cos(actor->_B7C);
	velocity.y              = 0;
	const CVec& pos         = actor->mMoveTranslate + velocity * (f32)frames;
	actor->mMoveTranslate.x = pos.x;
	actor->mMoveTranslate.y = pos.y;
	actor->mMoveTranslate.z = pos.z;
	actor->mMoveTranslate.SetTarget(pos);
	actor->SetPos(actor->mMoveTranslate, 0);
}

static inline bool ReadBossMount(XObjSpiderBoss* boss, Vec& position)
{
	if (!(boss->mStatus.mFlags & 1) || !(boss->mStatus.mFlags & 0x10000))
		return false;
	position.x = HSD_JObjGetTranslationX(boss->mJObj);
	position.y = HSD_JObjGetTranslationY(boss->mJObj);
	position.z = HSD_JObjGetTranslationZ(boss->mJObj);
	HSD_JObjGetRotationX(boss->mJObj);
	HSD_JObjGetRotationY(boss->mJObj);
	HSD_JObjGetRotationZ(boss->mJObj);
	return true;
}

// Each impact has its own entry in the original action table.
static inline void BounceBossPart(XObjActor3* part, f32 speed)
{
	part->_B78 = speed;
	++part->_B88;
}
void XObjActor3::Kind11(u32 frames)
{
	CPad pad(0);
	switch (_B80) {
	case 0: {
		if (_B8C.mFlags & 4)
			break;
		Vec origin = { 0, 0, 0 };
		ReadBossMount((XObjSpiderBoss*)gGame.mXObjs.mPools[19].First(), origin);
		switch (GetKind()) {
		case 0x3cd:
			_BD8 = 16;
			_BC8 = -PI / 4;
			_BD0 = 10;
			break;
		case 0x3ce:
			_BD8 = 16;
			_BC8 = PI / 4;
			_BD0 = 10;
			break;
		case 0x3cf:
			_BD8 = 10;
			_BC8 = PI;
			_BD0 = 16;
			break;
		case 0x3d0:
			_BD8 = 0;
			_BC8 = HALF_PI;
			_BD0 = 30;
			break;
		case 0x3d1:
			_BD8 = 30;
			_BC8 = PI;
			_BD0 = 10;
			break;
		case 0x3d2:
			_BD8 = 40;
			_BC8 = 3 * HALF_PI;
			_BD0 = 24;
			break;
		case 0x3d3:
			_BD8 = 30;
			_BC8 = 3 * PI / 4;
			_BD0 = 14;
			break;
		case 0x3d4:
			_BD8 = 20;
			_BC8 = 0;
			_BD0 = 10;
			break;
		case 0x3d5:
			_BD8 = 16;
			_BC8 = 3 * HALF_PI;
			_BD0 = 10;
			break;
		default:
			_BD8 = 32;
			_BC8 = DEG2RAD * (rand() % 360);
			_BD0 = (48.f - (rand() % 1800) / 100) * .5f;
			break;
		}
		_BCC = _BD8 * (f32)sin(_BC8);
		_BD4 = _BD8 * (f32)cos(_BC8);
		_B7C = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		mStatus.mFlags |= 4;
		_B88 = 0;
		++_B80;
		break;
	}
	case 1: {
		XObjSpiderBoss* boss = (XObjSpiderBoss*)gGame.mXObjs.mPools[19].First();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			SetScale(1, 1, 1, 0);
			CJObj joint;
			s32 path[] = { 0 };
			joint.Attach(GetJointFromRoot(boss->mJObj, 1, path));
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, _BCC, _BD0, _BD4);
			PSMTXConcat(result, translation, result);
			HSD_JObj* jobj = mJObj;
			MtxPtr matrix  = result;
			HSD_JObjCopyMtx(jobj, matrix);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15)
					vf12C(1, 128, 0, 0, 0);
				else
					vf12C(1, 0, 0, 0, 0);
			}
		}
		break;
	}
	case 2: {
		vf12C(1, 0, 0, 0, 0);
		_B78 -= .09f * frames;
		mMoveTranslate.y += _B78;
		f32 floor = 0;
		switch (GetKind()) {
		case 0x3d2:
			floor = 8;
			break;
		case 0x3d3:
			floor = 16;
			break;
		}
		if (mMoveTranslate.y <= floor) {
			mMoveTranslate.y = floor;
			switch (_B88) {
			case 0: {
				++_B88;
				_B78 = 8;
				PlayGameSound(&gGame.mSound, 0x160020, 255);
				Vec p;
				p.x       = mMoveTranslate.x;
				p.y       = mMoveTranslate.y - floor;
				p.z       = mMoveTranslate.z;
				Vec scale = { 1, 1, 1 };
				_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0xb5, 1, p, scale, (Vec) { 0, 0, 0 }, TRUE);
				break;
			}
			case 1:
				_B78 = 7;
				++_B88;
				PlayGameSound(&gGame.mSound, 0x160020, 255);
				gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
				break;
			case 2:
				BounceBossPart(this, 6);
				break;
			case 3:
				BounceBossPart(this, 6);
				break;
			case 4:
				BounceBossPart(this, 5);
				break;
			case 5:
				BounceBossPart(this, 5);
				break;
			case 6:
				BounceBossPart(this, 4);
				break;
			case 7:
				BounceBossPart(this, 4);
				break;
			case 8:
				BounceBossPart(this, 4);
				break;
			case 9:
				_B78 = 0;
				_B80 = 3;
				_B88 = 30;
				break;
			}
		}
		if (_B78 >= -3.4499998f)
			_B78 -= .36f * frames;
		else
			_B78 = -3.4499998f;
		AdvanceActorAlongHeading(this, frames, .6f);
		break;
	}
	case 3:
		if ((_B88 & 3) < 2)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--_B88 < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			_B8C.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			_B80 = 0;
		}
		break;
	case 5: {
		XObjSpiderBoss* boss = (XObjSpiderBoss*)gGame.mXObjs.mPools[19].First();
		Vec origin           = { 0, 0, 0 };
		if (ReadBossMount(boss, origin))
			_B7C = atan2(boss->mMoveTranslate.x - mMoveTranslate.x, boss->mMoveTranslate.z - mMoveTranslate.z);
		f32 dx = origin.x - mMoveTranslate.x, dy = origin.y - mMoveTranslate.y, dz = origin.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + dy * dy + dz * dz) < 16)
			_B80 = 6;
		AdvanceActorAlongHeading(this, frames, .6f);
		break;
	}
	case 6: {
		Vec origin = { 0, 0, 0 };
		ReadBossMount((XObjSpiderBoss*)gGame.mXObjs.mPools[19].First(), origin);
		_BC8 = DEG2RAD * (rand() % 360);
		_BCC = 32 * (f32)sin(_BC8);
		_BD0 = (48.f - (rand() % 1800) / 100) * .5f;
		_BD4 = 32 * (f32)cos(_BC8);
		_B7C = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		_B80 = 1;
		if (pad.mPad->mTrigger & 0x10) {
			_B80 = 5;
			mStatus.mFlags |= 0x10000;
			_B8C.mFlags &= ~4;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8010FE68
 * @note Size: 0x12C
 */
extern "C" void EnableSpiderBossPartByIndex(s32 n)
{
	XObjActor3* obj;
	s32 offset;
	u8* data;
	s32 size;
	s32 count = 0;
	s32 i;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (i = 0, offset = 0; i < 48; ++i, offset += size) {
		obj = (XObjActor3*)(data + offset);
		if (XObj::GetDataItem(obj->mAttr._00) == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1)) {
				if (obj->_B80 == 0) {
					obj->_B8C.mFlags &= ~4;
				}
				return;
			}
			count++;
		}
	}
}

/**
 * @note Address: 0x8010FDA0
 * @note Size: 0xC8
 */
// starts the n-th kind 0xB actor
// NONMATCHING: register allocation
extern "C" void LaunchSpiderBossPartByIndex(s32 n)
{
	u8* data;
	XObjActor3* obj;
	s32 size;
	s32 count = 0;
	s32 i;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (i = 0; i < 48; i++) {
		obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr._00) == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				LaunchSpiderBossPart(obj);
				return;
			}
			count++;
		}
	}
}

/**
 * @note Address: 0x8010F318
 * @note Size: 0x804
 */
void XObjActor3::Kind12(u32 frames)
{
	switch (_B80) {
	case 0:
		if (!(_B8C.mFlags & 4)) {
			_BC8 = 0.0f;
			_BCC = 0.0f;
			_BD0 = 0.0f;
			_BD4 = 0.0f;
			mStatus.mFlags |= 0x100000;
			mStatus.mFlags |= 4;
			++_B80;
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = (XObjSpiderBoss*)gGame.mXObjs.mPools[19].First();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			_B7C = atan2(boss->mMoveTranslate.x - mMoveTranslate.x, boss->mMoveTranslate.z - mMoveTranslate.z);
			SetScale(1.0f, 1.0f, 1.0f, 0);
			CJObj joint;
			s32 path[] = { 0 };
			joint.Attach(GetJointFromRoot(boss->mJObj, 1, path));
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, 0.0f, 0.0f, 0.0f);
			PSMTXConcat(result, translation, result);
			HSD_JObj* jobj = mJObj;
			MtxPtr matrix  = result;
			HSD_JObjCopyMtx(jobj, matrix);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15)
					vf12C(1, 128, 0, 0, 0.0f);
				else
					vf12C(1, 0, 0, 0, 0.0f);
			}
		}
		break;
	}
	case 2: {
		vf12C(1, 0, 0, 0, 0.0f);
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			mStatus.mFlags &= ~0x10000ULL;
			_B8C.mFlags |= 4;
			PlayGameSound(&gGame.mSound, 0x160020, 0xFF);
			_B80 = 0;
		}
		break;
	}
	case 3:
		if ((_B88 & 3) < 2)
			mStatus.mFlags |= 0x10000;
		else
			mStatus.mFlags &= ~0x10000ULL;
		if (--_B88 < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			_B8C.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			_B80 = 0;
			Release();
		}
		break;
	}
}

/**
 * @note Address: 0x8010FB1C
 * @note Size: 0x284
 */
// jumps the actor onto its holder, at its angle and distance around it
extern "C" void LaunchSpiderBossPart(XObjActor3* actor)
{
	XObj* holder = gGame.mXObjs.mPools[19].Get(0);
	if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && actor->_B80 == 1) {
		f32 s          = sin(actor->_BC8 + HSD_JObjGetRotationY(holder->mJObj));
		f32 x          = actor->_BD8 * s;
		f32 y          = actor->_BD0;
		f32 c          = cos(actor->_BC8 + HSD_JObjGetRotationY(holder->mJObj));
		f32 z          = actor->_BD8 * c;
		actor->_B80    = 2;
		actor->_B78    = 6.0f;
		HSD_JObj* jobj = actor->mJObj;
		HSD_JObjClearMtxFlags(jobj);
		actor->mMoveTranslate.x = x + HSD_JObjGetTranslationX(holder->mJObj);
		actor->mMoveTranslate.y = y + 8.0f + 10.0f;
		actor->mMoveTranslate.z = z + HSD_JObjGetTranslationZ(holder->mJObj);
		actor->SetPos(actor->mMoveTranslate, 0);
		actor->_B7C = (f32)atan2(holder->mMoveTranslate.x - actor->mMoveTranslate.x, holder->mMoveTranslate.z - actor->mMoveTranslate.z)
		            + 3.1415927f;
		PlayGameSound(&gGame.mSound, 0x160021, 0xFF);
	}
}

/**
 * @note Address: 0x8010F208
 * @note Size: 0x110
 */
extern "C" void EnableSpiderBossCenterPart(void)
{
	XObjActor3* obj;
	s32 offset;
	u8* data;
	s32 size;
	s32 i;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (i = 0, offset = 0; i < 48; ++i, offset += size) {
		obj = (XObjActor3*)(data + offset);
		if (XObj::GetDataItem(obj->mAttr._00) == 0xC && (obj->mStatus.mFlags & 1)) {
			if (obj->_B80 == 0) {
				obj->_B8C.mFlags &= ~4;
			}
			return;
		}
	}
}

/**
 * @note Address: 0x8010EF70
 * @note Size: 0x298
 */
// jumps the first kind 0xC actor onto the holder's middle
// NONMATCHING: register allocation
extern "C" void LaunchSpiderBossCenterPart(void)
{
	u8* data;
	s32 size;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr._00) == 0xC && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			XObj* holder = (XObj*)gGame.mXObjs.mPools[19].mObjects;
			if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && obj->_B80 == 1) {
				obj->_B80      = 2;
				obj->_B78      = 6.0f;
				HSD_JObj* jobj = obj->mJObj;
				HSD_JObjClearMtxFlags(jobj);
				f32 ofs               = 0.0f; // the holder's middle, where LaunchSpiderBossPart goes round it
				obj->mMoveTranslate.x = ofs + HSD_JObjGetTranslationX(holder->mJObj);
				obj->mMoveTranslate.y = 18.0f;
				obj->mMoveTranslate.z = ofs + HSD_JObjGetTranslationZ(holder->mJObj);
				obj->SetPos(obj->mMoveTranslate, 0);
				obj->_B7C = (f32)atan2(holder->mMoveTranslate.x - obj->mMoveTranslate.x, holder->mMoveTranslate.z - obj->mMoveTranslate.z)
				          + 3.1415927f;
				obj->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				PlayGameSound(&gGame.mSound, 0x160021, 0xFF);
			}
			return;
		}
	}
}

extern "C" bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* boss);

static inline XObjSpiderBoss* ProjectileOwner()
{
	return (XObjSpiderBoss*)gGame.mXObjs.mPools[19].mObjects;
}

static inline void PlaceBossProjectile(XObjActor3* actor, XObjSpiderBoss* boss, int spread)
{
	actor->mMoveTranslate.x = HSD_JObjGetTranslationX(boss->mJObj);
	f32 base                = HSD_JObjGetTranslationY(boss->mJObj);
	actor->mMoveTranslate.y = 16 + base + rand() % spread;
	if (gSpiderBossVolleySlot == 0)
		actor->mMoveTranslate.y = HSD_JObjGetTranslationY(boss->mJObj);
	actor->mMoveTranslate.z = HSD_JObjGetTranslationZ(boss->mJObj);
	actor->SetPos(actor->mMoveTranslate, 0);
	actor->mStatus.mFlags |= 0x10000;
	actor->mStatus.mFlags |= 4;
}

static inline void AimBossVolley(XObjActor3* actor, const Vec& center, f32 radius)
{
	actor->mStatus.mFlags |= 0x10000;
	actor->_BEC = gSpiderBossVolleySlot;
	if (gSpiderBossVolleySlot == 0) {
		actor->_BCC = center.x;
		actor->_BD0 = center.y;
		actor->_BD4 = center.z;
	} else {
		actor->_BCC = center.x + radius * (f32)sin(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
		actor->_BD0 = center.y;
		actor->_BD4 = center.z + radius * (f32)cos(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
	}
	++gSpiderBossVolleySlot;
	++actor->_B80;
}

static inline bool BossProjectileLanded(XObjActor3* actor, u32 frames)
{
	actor->_B78 -= .09f * frames;
	actor->mMoveTranslate.y += actor->_B78;
	return actor->mMoveTranslate.y <= -1;
}

static inline void SteerFallingProjectile(XObjActor3* actor, u32 frames)
{
	if (actor->_B78 >= -3.4499998f)
		actor->_B78 -= .36f * frames;
	else
		actor->_B78 = -3.4499998f;
	actor->_B7C = atan2(actor->_BCC - actor->mMoveTranslate.x, actor->_BD4 - actor->mMoveTranslate.z);
	AdvanceActorAlongHeading(actor, frames, actor->_B74);
}

static inline void HideBossProjectile(XObjActor3* actor, bool resetPitch)
{
	actor->mStatus.mFlags &= ~0x10000ULL;
	actor->mStatus.mFlags &= ~4ULL;
	actor->_B8C.mFlags |= 4;
	actor->SetAnim(0, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
	if (resetPitch)
		actor->mMoveRotate.x = 0;
	actor->_B80 = 0;
}

void XObjActor3::Kind13(u32 frames)
{
	XObjChr* player = gGame.GetPlayer();
	Vec pos         = player->mMoveTranslate;
	switch (_B80) {
	case 0:
		if (!(_B8C.mFlags & 4)) {
			XObjSpiderBoss* boss = ProjectileOwner();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				PlaceBossProjectile(this, boss, 64);
				_B78   = 16;
				f32 dx = pos.x - mMoveTranslate.x, dz = pos.z - mMoveTranslate.z;
				_B74 = .01f * sqrtf(dx * dx + 0.f + dz * dz);
				if (_B74 < 1.2f)
					_B74 = 1.2f;
				if (_B74 > 24)
					_B74 = 24;
				AimBossVolley(this, pos, 24);
			}
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = ProjectileOwner();
		bool blocked = ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) && IsSpiderBossProjectileHitSuppressed(boss);
		f32 dx = pos.x - mMoveTranslate.x, dy = pos.y - mMoveTranslate.y, dz = pos.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + dy * dy + dz * dz) < 8 && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)
		    && !blocked) {
			player->OnHitFromPosition(pos, 6);
			mRumble = 1;
			_B88    = 30;
			gGame.mParticle.Kill((HSD_Generator*)_1DC0[0]);
			Vec scale = { 1, 1, 1 };
			_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, scale, (Vec) { 0, 0, 0 }, TRUE);
			PlayGameSound(&gGame.mSound, 0x127, 255);
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 2;
			break;
		}
		if (BossProjectileLanded(this, frames)) {
			_B78             = 0;
			mMoveTranslate.y = 0;
			_B88             = 30;
			if (_BEC == 0) {
				Vec scale = { 1, 1, 1 };
				_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, scale, (Vec) { 0, 0, 0 }, TRUE);
				PlayGameSound(&gGame.mSound, 0x127, 255);
			}
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 2;
		} else
			SteerFallingProjectile(this, frames);
		break;
	}
	case 2:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, false);
		break;
	case 10:
		if (!(_B8C.mFlags & 4)) {
			XObjSpiderBoss* boss = ProjectileOwner();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				mMoveTranslate.x = HSD_JObjGetTranslationX(boss->mJObj);
				mMoveTranslate.y = 10 + HSD_JObjGetTranslationY(boss->mJObj);
				mMoveTranslate.z = HSD_JObjGetTranslationZ(boss->mJObj);
				SetPos(mMoveTranslate, 0);
				mStatus.mFlags |= 0x10000;
				mStatus.mFlags |= 4;
				_B74          = 2;
				_B7C          = atan2(pos.x - mMoveTranslate.x, pos.z - mMoveTranslate.z);
				mMoveRotate.x = -HALF_PI;
				mStatus.mFlags |= 0x10000;
				_B88 = 120;
				++_B80;
			}
		}
		break;
	case 11: {
		f32 dx = pos.x - mMoveTranslate.x, dz = pos.z - mMoveTranslate.z;
		if (sqrtf(dx * dx + 0.f + dz * dz) < 8 && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 6);
			mRumble   = 1;
			_B88      = 30;
			Vec scale = { 1, 1, 1 };
			_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0xb2, 1, mMoveTranslate, scale, (Vec) { 0, _B7C, 0 }, TRUE);
			PlayGameSound(&gGame.mSound, 0x127, 255);
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 12;
		} else if (--_B88 < 0)
			HideBossProjectile(this, true);
		else
			AdvanceActorAlongHeading(this, frames, _B74);
		break;
	}
	case 12:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, true);
		break;
	case 20:
		if (!(_B8C.mFlags & 4)) {
			XObjSpiderBoss* boss = ProjectileOwner();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				PlaceBossProjectile(this, boss, 16);
				_B78 = 10;
				Vec center
				    = { HSD_JObjGetTranslationX(boss->mJObj), HSD_JObjGetTranslationY(boss->mJObj), HSD_JObjGetTranslationZ(boss->mJObj) };
				_B74 = 2.4f;
				AimBossVolley(this, center, 36);
			}
		}
		break;
	case 21:
		if (BossProjectileLanded(this, frames)) {
			_B78             = 0;
			mMoveTranslate.y = 0;
			_B88             = 30;
			if (_BEC == 0) {
				Vec scale = { 1, 1, 1 };
				_1DC0[0]  = (s32)gGame.mParticle.CreateSRT(0xb1, 1, mMoveTranslate, scale, (Vec) { 0, 0, 0 }, TRUE);
				mRumble   = 1;
			}
			SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
			_B80 = 22;
		} else
			SteerFallingProjectile(this, frames);
		break;
	case 22:
		if (GetAnimEndFrame(mJObj) - frames <= GetAnimFrame(mJObj))
			HideBossProjectile(this, false);
		break;
	}
}

/**
 * @note Address: 0x8010D390
 * @note Size: 0x2CC
 */
extern "C" void CommandSpiderBossProjectiles(s32 mode)
{
	if (mode == 0) {
		for (s32 i = 0; i < 48; i++) {
			XObjActor3* obj = (XObjActor3*)gGame.mXObjs.mPools[5].Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1)) {
				obj->_B8C.mFlags &= ~4;
				obj->_B80             = 0;
				gSpiderBossVolleySlot = 0;
			}
		}
	}
	if (mode == 2) {
		for (s32 i = 0; i < 48; i++) {
			XObjActor3* obj = (XObjActor3*)gGame.mXObjs.mPools[5].Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1) && obj->_B8C.Check(4)) {
				obj->_B8C.mFlags &= ~4;
				obj->_B80             = 20;
				gSpiderBossVolleySlot = 0;
			}
		}
	} else {
		XObjActor3* obj;
		s32 offset;
		u8* data;
		s32 size;
		s32 i;
		size = gGame.mXObjs.mPools[5].mStride;
		data = gGame.mXObjs.mPools[5].mObjects;
		for (i = 0, offset = 0; i < 48; ++i, offset += size) {
			obj = (XObjActor3*)(data + offset);
			if (obj->GetKind() == 0x3E0) {
				if (obj->mStatus.mFlags & 1) {
					if (obj->_B8C.Check(4)) {
						obj->_B8C.mFlags &= ~4;
						obj->_B80 = 10;
						return;
					}
				} else {
					return;
				}
			}
		}
	}
}

/**
 * @note Address: 0x8010D114
 * @note Size: 0x27C
 */
void XObjActor3::UpdateSpiderCounterPanel(u32 frames)
{
	switch (_BEC) {
	case 0:
		_BEC             = 4;
		mTimerPanelAlpha = 0.0f;
		break;
	case 1:
		mTimerPanelAlpha = 1.0f;
		break;
	case 2:
		mTimerPanelAlpha = 0.0f;
		break;
	case 3:
		mTimerPanelAlpha += 0.08f * frames;
		if (mTimerPanelAlpha >= 1.0f) {
			mTimerPanelAlpha = 1.0f;
		}
		break;
	case 4:
		mTimerPanelAlpha -= 0.08f * frames;
		if (mTimerPanelAlpha < 0.0f) {
			mTimerPanelAlpha = 0.0f;
		}
		break;
	}
	if (mCountdownRunning) {
		mCountdownFrames -= frames;
	}
	switch (mCounterFadeMode) {
	case 0:
		mCounterFadeMode   = 4;
		mCounterPanelAlpha = 0.0f;
		break;
	case 1:
		mCounterPanelAlpha = 1.0f;
		break;
	case 2:
		mCounterPanelAlpha = 0.0f;
		break;
	case 3:
		mCounterPanelAlpha += 0.08f * frames;
		if (mCounterPanelAlpha >= 1.0f) {
			mCounterPanelAlpha = 1.0f;
		}
		break;
	case 4:
		mCounterPanelAlpha -= 0.08f * frames;
		if (mCounterPanelAlpha < 0.0f) {
			mCounterPanelAlpha = 0.0f;
		}
		break;
	}
	s32 count = 0;
	for (s32 i = 0; i < 16; i++) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 0x10000) && !HasSpiderFinishedDeathAnimation(static_cast<XObjSpider*>(obj))) {
			count++;
		}
	}
	mSpiderCount = count;
}

/**
 * @note Address: 0x8010CE80
 * @note Size: 0x294
 */
void XObjActor3::Kind15(u32 frames)
{
	switch (_B80) {
	case 0:
		if (_B8C.mFlags & 4) {
			break;
		}
		_B80++;
		SetAnim(1, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		if (++gActor3Kind15ActivationCounter >= 10) {
			gActor3Kind15ActivationCounter = 0;
		}
		if (!IsGameSoundPlaying(&gGame.mSound, 0x127)) {
			PlayGameSound(&gGame.mSound, 0x127, 0xFF);
		}
		break;
	case 1:
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			_B80++;
			SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
		break;
	case 2:
		_B8C.mFlags |= 4;
		_B80 = 0;
		break;
	}
}

/**
 * @note Address: 0x8010CDDC
 * @note Size: 0xA4
 */
// NONMATCHING: regalloc
extern "C" void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z)
{
	XObjActor3* obj;
	for (s32 i = 0; i < 48; i++) {
		obj = (XObjActor3*)gGame.mXObjs.mPools[5].Get(i);
		switch (obj->GetKind()) {
		case 0x43A:
			if ((obj->mStatus.mFlags & 1) && obj->_B8C.Check(4)) {
				obj->_B8C.mFlags &= ~4;
				obj->mMoveTranslate.x = x;
				obj->mMoveTranslate.y = y;
				obj->mMoveTranslate.z = z;
			}
			break;
		}
	}
}

static inline void ResetBossAttachment(XObjActor3* obj)
{
	obj->_B80 = 0;
	obj->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	obj->_B8C.mFlags |= 4;
	obj->_B8C.mFlags |= 0x10;
	obj->SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	obj->_B8C.mFlags &= ~4;
	obj->_B8C.mFlags &= ~0x10;
	obj->SetScale(0.1f, 0.1f, 0.1f, 0);
	obj->_B80 = 0;
}

template <int depth>
static inline Vec BossJointPosition(CJObj& joint, XObj* boss, s32 (&path)[depth])
{
	joint.Attach(GetJointFromRoot(boss->mJObj, depth, path));
	return joint.GetPos();
}

/**
 * @note Address: 0x8010C304
 * @note Size: 0xAD8
 */
void XObjActor3::Kind16(u32 frames)
{
	CJObj joint;
	XObj* boss = gGame.mXObjs.mPools[19].First();
	if (_B8C.mFlags & 0x10)
		return;
	switch (_B80) {
	case 0:
		_B8C.mFlags |= 4;
		_B8C.mFlags |= 0x10;
		SetAnim(0, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		break;
	case 1: {
		s32 path[] = { 0, 7 };
		Vec pos    = BossJointPosition(joint, boss, path);
		mMoveTranslate.Set(pos);
		_B7C = HSD_JObjGetRotationY(boss->mJObj);
		break;
	}
	case 2: {
		s32 path[]       = { 0, 2, 0, 1, 1, 0, 0 };
		Vec pos          = BossJointPosition(joint, boss, path);
		_B7C             = HSD_JObjGetRotationY(boss->mJObj);
		mMoveTranslate.x = 2.0f * (f32)sin(_B7C) + pos.x;
		mMoveTranslate.y = pos.y;
		mMoveTranslate.z = 2.0f * (f32)cos(_B7C) + pos.z;
		break;
	}
	case 4: {
		s32 path[] = { 0, 7 };
		Vec pos    = BossJointPosition(joint, boss, path);
		mMoveTranslate.Set(pos);
		_B7C    = HSD_JObjGetRotationY(boss->mJObj);
		f32 end = GetAnimEndFrame(mJObj);
		if (end - frames <= GetAnimFrame(mJObj)) {
			ResetBossAttachment(this);
			Release();
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8010C0F0
 * @note Size: 0x214
 */
// plays anim n on the first kind 0x10 actor and sizes it for it
extern "C" void PlaySpiderBossAttachmentAnimation(s32 n)
{
	u8* data;
	s32 size;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr._00) == 0x10 && (obj->mStatus.mFlags & 1)) {
			obj->SetAnim(n, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			obj->_B8C.mFlags &= ~4;
			obj->_B8C.mFlags &= ~0x10;
			switch (n) {
			case 0:
				obj->SetScale(0.1f, 0.1f, 0.1f, 0);
				obj->_B80 = 0;
				break;
			case 1:
				obj->SetScale(1.4f, 1.4f, 1.4f, 0);
				obj->_B80 = 1;
				break;
			case 2:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->_B80 = 2;
				break;
			case 3:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->_B80 = 1;
				break;
			case 4:
				obj->SetScale(1.0f, 1.0f, 1.0f, 0);
				obj->_B80 = 4;
				break;
			}
			return;
		}
	}
}

/**
 * @note Address: 0x8010BF80
 * @note Size: 0x170
 */
// puts the first kind 0x10 actor back to its rest pose, small
extern "C" void ResetSpiderBossAttachment(void)
{
	u8* data;
	s32 size;
	size = gGame.mXObjs.mPools[5].mStride;
	data = gGame.mXObjs.mPools[5].mObjects;
	for (s32 i = 0; i < 48; i++) {
		XObjActor3* obj = (XObjActor3*)(data + i * size);
		if (XObj::GetDataItem(obj->mAttr._00) == 0x10 && (obj->mStatus.mFlags & 1)) {

			ResetBossAttachment(obj);
			return;
		}
	}
}

/**
 * @note Address: 0x8010BC0C
 * @note Size: 0x370
 */
// slides out to the left from about ten to noon and back in the afternoon
void XObjActor3::Kind18(u32 frames)
{
	f32 time = gGame.mTimeState.mTime;
	switch (_B80) {
	case 0:
		if (time >= 0.42f && time < 0.5f) {
			_B80++;
		}
		break;
	case 1:
		mMoveTranslate.x -= 0.2f * frames;
		if (mMoveTranslate.x <= PlacementPosition(mCoord, gActor3PlacementPosition).x - 10.0f) {
			mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x - 10.0f;
			_B80++;
		}
		break;
	case 2:
		if (time >= 0.5f) {
			_B80++;
		}
		break;
	case 3:
		mMoveTranslate.x += 0.2f * frames;
		if (mMoveTranslate.x >= PlacementPosition(mCoord, gActor3PlacementPosition).x) {
			mMoveTranslate.x = PlacementPosition(mCoord, gActor3PlacementPosition).x;
			_B80             = 0;
		}
		break;
	}
}

/**
 * @note Address: 0x8010BF7C
 * @note Size: 0x4
 */
void XObjActor3::Kind17(u32 frames)
{
}

/**
 * @note Address: 0x8010BC08
 * @note Size: 0x4
 */
void XObjActor3::Kind19(u32 frames)
{
}

/**
 * @note Address: 0x8010B528
 * @note Size: 0x28C
 */
extern "C" void HandleKind9ActorCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->_B8C.mFlags &= ~4;
			actor->_BC0 = actor->mEventArg2 / 100.0f;
			actor->_BC4 = actor->mEventArg3 / 100.0f;
		} else {
			actor->_B8C.mFlags |= 4;
			actor->_BC0 = 0.0f;
			actor->_BC4 = 0.0f;
			actor->_B80 = 0;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 2:
		if ((actor->_B8C.mFlags & 0x80) && !(actor->_B8C.mFlags & 4)) {
			gEventVariables[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 3:
		actor->_B8C.mFlags &= ~0x80;
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 4:
		XObjChr* player = gGame.GetPlayer();
		if (!IsBusyAnim(player->mAnimId) && player->mAttachmentRequest < 0) {
			gEventVariables[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
}

/**
 * @note Address: 0x8010B7B4
 * @note Size: 0x454
 */
// carries the robot while it rides the actor, at the actor's first joint
// NONMATCHING: the robot and the joint's vectors take each other's registers
extern "C" void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	if ((actor->_B8C.mFlags & 0x40) && (player->_980.mFlags & 0x40000000)) {
		Mtx scaleMtx;
		Mtx rotMtx;
		Mtx transMtx;
		Mtx mtx;
		Mtx m;
		s32 path[2] = { 0, 0 };
		joint.Attach(GetJointFromRoot(actor->mJObj, 2, path));
		PSMTXCopy(HSD_JObjGetMtxPtr(joint.mJObj), m);
		Vec scale;
		HSD_MtxGetScale(m, &scale);
		scale.x = GetStagePlacementScale(player).x / scale.x;
		scale.y = GetStagePlacementScale(player).y / scale.y;
		scale.z = GetStagePlacementScale(player).z / scale.z;
		PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
		PSMTXConcat(m, scaleMtx, mtx);
		PSMTXRotRad(rotMtx, 'y', 0.0f);
		PSMTXConcat(mtx, rotMtx, mtx);
		PSMTXTrans(transMtx, 0.0f, 3.7f, 30.0f);
		PSMTXConcat(mtx, transMtx, mtx);
		HSD_JObj* jobj = player->mJObj;
		MtxPtr mp      = mtx;
		HSD_JObjCopyMtx(jobj, mp);
	}
}

/**
 * @note Address: 0x8010B394
 * @note Size: 0x194
 */
// the script's commands to the actor, mEventCommand is the command and mEventArg1 its argument
extern "C" void HandleSpiderCounterCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 0:
		break;
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->_BEC = 3;
		} else {
			actor->_BEC = 4;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 2:
		if (actor->mEventArg1 != 0) {
			actor->mCountdownRunning = 1;
		} else {
			actor->mCountdownRunning = 0;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 3:
		actor->mCountdownFrames = (actor->mEventArg1 / 100.0f) * FRAME_RATE;
		actor->mEventCommand    = 0;
		actor->mEventArg1       = 0;
		break;
	case 4:
		gEventVariables[actor->mEventArg1] = TimerFramesToHundredths(actor->mCountdownFrames);
		actor->mEventCommand               = 0;
		actor->mEventArg1                  = 0;
		break;
	case 5:
		if (actor->mEventArg1 != 0) {
			actor->mCounterFadeMode = 3;
		} else {
			actor->mCounterFadeMode = 4;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 6:
		actor->mSpiderCount  = actor->mEventArg1;
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 7:
		gEventVariables[actor->mEventArg1] = actor->mSpiderCount;
		actor->mEventCommand               = 0;
		actor->mEventArg1                  = 0;
		break;
	}
}

/**
 * @note Address: 0x8010AF74
 * @note Size: 0x420
 */
// the robot gets on the actor at its first joint, or off it
// NONMATCHING: the robot and the joint's vectors take each other's registers
extern "C" void HandleActorRiderCommand(XObjActor3* actor)
{
	CJObj joint;
	XObjChr* player = gGame.GetPlayer();
	switch (actor->mEventCommand) {
	case 1:
		switch (actor->mEventArg1) {
		case 1:
			actor->_B8C.mFlags |= 0x40;
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x40000000;
			player->mPad.mReset = 0;
			break;
		default:
			actor->_B8C.mFlags &= ~0x40;
			s32 path[2] = { 0, 0 };
			joint.Attach(GetJointFromRoot(actor->mJObj, 2, path));
			CVec pos(GetJointPosition(joint.mJObj));
			player->SetPos(pos, 0);
			player->_980.mFlags = 0;
			player->_980.mFlags |= 0x20000000;
			player->_980.mFlags |= 7;
			player->mPad.mReset = 1;
			HSD_JObjClearMtxFlags(player->mJObj);
			break;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
}

/**
 * @note Address: 0x801151E0
 * @note Size: 0xBC
 */
void XObjActor3::UpdateObjectGauge(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 1, 1);
		mEventCommand = 0;
		break;
	case 2:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, gGame.mXObjs.mPools[5].Get(mPoolIndex), 0, 1);
		mEventCommand = 0;
		break;
	}
}

/**
 * @note Address: 0x8011529C
 * @note Size: 0x4
 */
void XObjActor3::Kind0(u32 frames)
{
}
