#include "game/wii/cgame.h"
#include "game/wii/infoscrn.h"
#include "game/wii/cpad.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include "game/globals.h"
#include "game/wii/spiderplacement.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
BOOL IsPlayerFlashingFromHit(WiiPlayerState* player);
BOOL IsUsingMug(XAction* action);
f32 GetPlayerCollisionRadius(WiiPlayerState* player);
f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 which);
void ShowScrapCounter(InfoScrn* scrn);
void HideScrapCounter(InfoScrn* scrn);
void ShowHappyPointCounter(InfoScrn* scrn);
void HideHappyPointCounter(InfoScrn* scrn);
void ShowMoolahCounter(InfoScrn* scrn);
void HideMoolahCounter(InfoScrn* scrn);
CVec GetStagePlacementScale(const XObj* obj);
XObj* ResolveEventObjectId(const CEvtData* data, s16 id);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
void HandleItem8ActorCommand(XObjActor3* actor);
void DrawStage02TimerPanel(XObjActor3* actor);
void DrawStage02ActorCounterPanel(XObjActor3* actor);
bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* boss);
void LaunchSpiderBossPart(XObjActor3* actor);
BOOL HasSpiderFinishedDeathAnimation(XObjSpider* spider);
extern s32 gRefreshRate;
extern s32 gPointerScreenWidth;
extern s32 gScreenSafeTop;
s32 gSpiderBossVolleySlot;
s32 gActor3Kind15ActivationCounter;
}

static void EmitKind9Dust(XObjActor3* actor, Vec* pos);

extern "C" AnimSfxEvent gHaisuiNezumiAnimSfxEvents[] = {
	{ 3, 11.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ 3, 53.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ 8, 11.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ 8, 53.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ 4, 1.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ 4, 11.0f, 0.0f, 1, 0, 0x42E, 0xFF, 0, 0, 0 },
	{ -1 },
};

extern "C" {
f32 gStage02TimerOutlineX[4] = { -2.0f, 2.0f, 2.0f, -2.0f };
f32 gStage02TimerOutlineY[4] = { -2.0f, -2.0f, 2.0f, 2.0f };
}

XObjActor3Func gActor3UpdateHandlers[20] = {
	&XObjActor3::Kind0,
	&XObjActor3::UpdateObjectGauge,
	&XObjActor3::Kind2,
	&XObjActor3::Kind3,
	&XObjActor3::Kind4,
	&XObjActor3::Kind5,
	&XObjActor3::Kind6,
	&XObjActor3::Kind7,
	&XObjActor3::Kind8,
	&XObjActor3::Kind9,
	&XObjActor3::Kind10,
	&XObjActor3::Kind11,
	&XObjActor3::Kind12,
	&XObjActor3::Kind13,
	&XObjActor3::UpdateSpiderCounterPanel,
	&XObjActor3::Kind15,
	&XObjActor3::Kind16,
	&XObjActor3::Kind17,
	&XObjActor3::Kind18,
	&XObjActor3::Kind19,
};

static inline bool IsPlayerAway(WiiPlayerState* player)
{
	Vec pos = *player->GetPos();
	if (pos.z < -4.28f) {
		return true;
	}
	switch (gGame.GetPlayer()->GetAnimId()) {
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
	case 0x155:
	case 0x156:
	case 0x157:
	case 0x158:
	case 0x159:
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

static inline void StunKind9Actor(XObjActor3* actor)
{
	actor->SetAnim(11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
	PlayWiiSoundEffect(&gGame.mSound, 0x42F, 1.0f, 0.0f);
	actor->mActorFlags.mFlags |= 0x200;
	actor->mKindStep = 20;
}

static inline BOOL Kind9AnimFinished(XObjActor3* actor, u32 frames)
{
	f32 end     = actor->GetAnimEndFrame();
	f32 current = actor->GetAnimFrame();
	return end - frames <= current;
}

void XObjActor3::Kind0(u32 frames)
{
}

// NONMATCHING: the distance expression keeps a second Vec temporary in retail;
// calls, constants and state transitions are otherwise reconstructed.
void XObjActor3::UpdateObjectGauge(u32 frames)
{
	switch (mEventCommand) {
	case 0:
		break;
	case 1:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, (WiiTitleObject*)gGame.mXObjs.mPools[5].Get(mPoolIndex), 1, 1, 0);
		mEventCommand = 0;
		break;
	case 2:
		InfoScrn_ApplyObjectGaugeCommand(&gGame.mInfoScrn, NULL, 0, 1, 0);
		mEventCommand = 0;
		break;
	}
}

void XObjActor3::Kind2(u32 frames)
{
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x411:
		for (s32 i = 0; i < 400; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				Mtx mount, translate, rotation, result;
				PSMTXCopy(clock->GetWorldMtx(clock->FindResNode("cyl266")), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -(12.0f * time)));
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				SetBaseMtx(result);
				return;
			}
		}
		break;
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

void XObjActor3::Kind3(u32 frames)
{
	f32 time = gGame.mTimeState.mTime;
	switch (GetKind()) {
	case 0x412:
		for (s32 i = 0; i < 400; i++) {
			XObj* clock = gGame.mXObjs.mPools[22].Get(i);
			if (clock->GetKind() == 0x2A1) {
				Mtx mount, translate, rotation, result;
				PSMTXCopy(clock->GetWorldMtx(clock->FindResNode("cyl266")), mount);
				PSMTXRotRad(rotation, 'z', PI * (2.0f * -time) + 2.617994f);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translate, 0.0f, 0.0f, 1.0f);
				PSMTXConcat(result, translate, result);
				SetBaseMtx(result);
				return;
			}
		}
		break;
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

void XObjActor3::Kind4(u32 frames)
{
	mFrameCount++;
	f32 wave  = std::sinf(DEG2RAD * ((mFrameCount * 6) % 360));
	f32 swing = 0.25f;
	if (GetKind() == 0x437) {
		mMoveRotate.x = wave * swing;
	} else {
		mMoveRotate.z = wave * swing;
	}
}

void XObjActor3::Kind5(u32 frames)
{
}

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
			id                   = items[i].GetId();
			num                  = items[i].GetCount();
			if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
				count++;
			}
		}
		gGame.mEvt.mSelectResult = count;
		mEventCommand            = 0;
		break;
	}
}

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

void XObjActor3::Kind8(u32 frames)
{
	if (mActorFlags.mFlags & 0x100) {
		nw4r::g3d::ResNode joint = GetResNodeOrFirst(1);
		CVec pos                 = GetJointPos(joint);
		s32 kind                 = -1;
		if (mActorFlags.mFlags & 0x100) {
			kind = 0x40B;
		}
		if (kind >= 0) {
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					Mtx m, t, a;
					PSMTXCopy(GetWorldMtx(joint), m);
					PSMTXTrans(t, 0.0f, -14.0f, 0.0f);
					PSMTXConcat(m, t, a);
					obj->SetBaseMtx(a);
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

void XObjActor3::Kind10(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	switch (mKindStep) {
	case 0:
		if (mActorFlags.mFlags & 4) {
			break;
		}
		mMoveTranslate.x = pos.x - (rand() % 3600) / 100.0f + 18.0f;
		mMoveTranslate.y = 64.0f;
		mMoveTranslate.z = pos.z - (rand() % 3600) / 100.0f + 18.0f;
		mStatus.mFlags |= 0x10000;
		mStatus.mFlags |= 4;
		_BA0        = 4.0f;
		mFrameCount = rand() % 30 + 30;
		mKindStep++;
		break;
	case 1:
		if (--mFrameCount < 0) {
			mKindStep++;
		}
		break;
	case 2:
		f32 dx   = pos.x - mMoveTranslate.x;
		f32 dy   = pos.y - mMoveTranslate.y;
		f32 dz   = pos.z - mMoveTranslate.z;
		f32 dist = FSqrt(dx * dx + dy * dy + dz * dz);
		if (dist < 8.0f && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 10.0f);
			mFrameCount = 30;
			mKindStep   = 3;
			break;
		}
		if (_BA0 >= -2.3f) {
			_BA0 -= 0.36f * frames;
		} else {
			_BA0 = -2.3f;
		}
		mMoveTranslate.y += _BA0;
		if (mMoveTranslate.y < 0.0f) {
			mMoveTranslate.y = 0.0f;
			mFrameCount      = 30;
			mKindStep        = 3;
		}
		break;
	case 3:
		if ((mFrameCount & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mStatus.mFlags &= ~4ULL;
			mActorFlags.mFlags |= 4;
			mKindStep = 0;
		}
		break;
	}
}

extern "C" bool IsPlayerWithinActorReach(XObjActor3* actor)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	if (!(actor->mActorFlags.mFlags & 4)) {
		if (IsPlayerAway(gGame.GetPlayer())) {
			return false;
		}
		if (pos.y >= actor->mMoveTranslate.y - 16.0f && pos.y <= actor->mMoveTranslate.y + 16.0f) {
			if (actor->mHeading == HALF_PI) {
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					return true;
				}
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					if (IsLongReachAnim(gGame.GetPlayer()->GetAnimId())) {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mMoveTranslate.x - 12.0f) {
							return true;
						}
					}
				}
			} else {
				if (actor->mMoveTranslate.x > pos.x && pos.x > actor->mWindowLeft) {
					return true;
				}
				if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mWindowRight) {
					if (IsLongReachAnim(gGame.GetPlayer()->GetAnimId())) {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 32.0f) {
							return true;
						}
					} else {
						if (actor->mMoveTranslate.x < pos.x && pos.x < actor->mMoveTranslate.x + 12.0f) {
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

// NONMATCHING: retail's effect temporaries use a 0x50 larger frame; the grab branch also
// repeats two player vtable reads that this build folds. Calls, sounds and state logic are ported.
void XObjActor3::Kind9(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	Vec p, scale;
	if (mActorFlags.mFlags & 0x20) {
		switch (mKindStep) {
		case 1:
		case 2:
		case 3:
		case 4:
			if ((s32)--mExtraEventArgs[6] < 0) {
				mActorFlags.mFlags &= ~0x20;
			}
			break;
		}
	}
	if (mKindStep != 0 && mKindStep < 20 && fabs(pos.x - mMoveTranslate.x) <= 10.0f && pos.y >= 4.0f + mMoveTranslate.y
	    && pos.y <= 8.0f + mMoveTranslate.y) {
		StunKind9Actor(this);
	}
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			mFrameCount = 30;
			++mKindStep;
		}
		break;
	case 1:
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (--mFrameCount < 0) {
			mFrameCount = rand() % 120 + 120;
			if (rand() % 2 == 0) {
				mHeading = -HALF_PI;
			} else {
				mHeading = HALF_PI;
			}
			mKindStep = 2;
		}
		if (IsPlayerWithinActorReach(this)) {
			mKindStep = 5;
		}
		break;
	case 2:
		SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (mHeading == -HALF_PI) {
			mMoveTranslate.x -= 0.1f * frames;
		} else {
			mMoveTranslate.x += 0.1f * frames;
		}
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			_BA0             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			_BA0             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		if (--mFrameCount < 0 && (mActorFlags.mFlags & 0x20)) {
			mKindStep              = 4;
			mFrameCount            = 90;
			p.x                    = mMoveTranslate.x;
			p.y                    = 12.0f + mMoveTranslate.y;
			p.z                    = 12.0f + mMoveTranslate.z;
			scale.x                = 3.0f;
			scale.y                = 3.0f;
			scale.z                = 3.0f;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x28, 0);
			if (effect != NULL) {
				effect->pos   = p;
				effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
				effect->scale = scale;
				UpdateEffectMtx(effect);
			}
			mGens[0] = effect;
		}
		if (IsPlayerWithinActorReach(this)) {
			mKindStep = 5;
		}
		break;
	case 3:
		if (_BA0 == HALF_PI) {
			mHeading -= 0.04f * frames;
			if (mHeading <= -HALF_PI) {
				mHeading  = -HALF_PI;
				mKindStep = 2;
			}
		} else {
			mHeading += 0.04f * frames;
			if (mHeading >= HALF_PI) {
				mHeading  = HALF_PI;
				mKindStep = 2;
			}
		}
		break;
	case 4:
		SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (--mFrameCount < 0) {
			mFrameCount = rand() % 120 + 30;
			mKindStep   = rand() % 2 + 2;
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
		}
		if (IsPlayerWithinActorReach(this)) {
			mKindStep = 5;
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
		}
		break;
	case 5:
		mExtraEventArgs[6] = (10 * gRefreshRate) / frames;
		mActorFlags.mFlags |= 0x20;
		p.x                    = mMoveTranslate.x;
		p.y                    = 12.0f + mMoveTranslate.y;
		p.z                    = 12.0f + mMoveTranslate.z;
		scale.x                = 3.0f;
		scale.y                = 3.0f;
		scale.z                = 3.0f;
		EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x26, 0);
		if (effect != NULL) {
			Vec rotation  = { 0.0f, 0.0f, 0.0f };
			effect->pos   = p;
			effect->rot   = rotation;
			effect->scale = scale;
			UpdateEffectMtx(effect);
		}
		mGens[0]    = effect;
		mFrameCount = 15;
		SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (mMoveTranslate.x < pos.x) {
			mHeading = HALF_PI;
		} else {
			mHeading = -HALF_PI;
		}
		PlayWiiSoundEffect(&gGame.mSound, 0x430, 1.0f, 0.0f);
		mKindStep = 6;
		break;
	case 6:
		if (Kind9AnimFinished(this, frames)) {
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
			p = GetJointPos(GetResNodeOrFirst(0xE));
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (mHeading == -HALF_PI) {
				p.x += 2.0f;
			} else {
				p.x -= 2.0f;
				p.z += 4.0f;
			}
			scale.x                = 1.0f;
			scale.y                = 1.0f;
			scale.z                = 1.0f;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x84, 0);
			if (effect != NULL) {
				effect->pos   = p;
				effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
				effect->scale = scale;
				UpdateEffectMtx(effect);
			}
			mGens[0]  = effect;
			mKindStep = 7;
		}
		break;
	case 7:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
			mFrameCount = 2;
			mKindStep   = 8;
		}
		break;
	case 8:
		if (mHeading == -HALF_PI) {
			mMoveTranslate.x -= 0.6f * frames;
		} else {
			mMoveTranslate.x += 0.6f * frames;
		}
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x       = mWindowLeft;
			mKindStep              = 4;
			mFrameCount            = 90;
			p.x                    = mMoveTranslate.x;
			p.y                    = 12.0f + mMoveTranslate.y;
			p.z                    = 12.0f + mMoveTranslate.z;
			scale.x                = 3.0f;
			scale.y                = 3.0f;
			scale.z                = 3.0f;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x28, 0);
			if (effect != NULL) {
				effect->pos   = p;
				effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
				effect->scale = scale;
				UpdateEffectMtx(effect);
			}
			mGens[0] = effect;
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x       = mWindowRight;
			mKindStep              = 4;
			mFrameCount            = 90;
			p.x                    = mMoveTranslate.x;
			p.y                    = 12.0f + mMoveTranslate.y;
			p.z                    = 12.0f + mMoveTranslate.z;
			scale.x                = 3.0f;
			scale.y                = 3.0f;
			scale.z                = 3.0f;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x28, 0);
			if (effect != NULL) {
				effect->pos   = p;
				effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
				effect->scale = scale;
				UpdateEffectMtx(effect);
			}
			mGens[0] = effect;
		} else {
			f32 dx = pos.x - mMoveTranslate.x;
			f32 dy = pos.y - mMoveTranslate.y;
			f32 dz = mMoveTranslate.z - mMoveTranslate.z;
			if (FSqrt(dz * dz + (dx * dx + dy * dy)) < 32.0f && !IsPlayerAway(gGame.GetPlayer())
			    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				SetAnim(9, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				mKindStep = 9;
			} else if (--mFrameCount < 0) {
				mFrameCount = 2;
				Vec dustPos = mMoveTranslate;
				EmitKind9Dust(this, &dustPos);
			}
		}
		break;
	case 9:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(10, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 10;
			PlayWiiSoundEffect(&gGame.mSound, 0x42D, 1.0f, 0.0f);
			if (mHeading == -HALF_PI) {
				p.x     = mMoveTranslate.x;
				p.y     = 12.0f + mMoveTranslate.y;
				p.z     = 12.0f + mMoveTranslate.z;
				scale.x = 0.2f;
				scale.y = 0.2f;
				scale.z = 0.2f;
			} else {
				p.x     = mMoveTranslate.x;
				p.y     = 12.0f + mMoveTranslate.y;
				p.z     = 12.0f + mMoveTranslate.z;
				scale.x = 0.2f;
				scale.y = 0.2f;
				scale.z = 0.2f;
			}
		}
		break;
	case 10: {
		f32 radius = GetPlayerCollisionRadius(gGame.GetPlayer());
		f32 offset;
		if (mHeading == -HALF_PI) {
			mMoveTranslate.x -= 1.4f * (0.6f * frames);
			offset = radius < 2.0f ? 1.5f : 5.5f;
		} else {
			mMoveTranslate.x += 1.4f * (0.6f * frames);
			offset = radius < 2.0f ? -1.5f : -5.5f;
		}
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			StunKind9Actor(this);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			StunKind9Actor(this);
		} else {
			f32 dx = pos.x + offset - mMoveTranslate.x;
			f32 dy = pos.y - mMoveTranslate.y;
			f32 dz = mMoveTranslate.z - mMoveTranslate.z;
			if (FSqrt(dz * dz + (dx * dx + dy * dy)) < 10.0f && !IsPlayerFlashingFromHit(player) && !IsPlayerAway(gGame.GetPlayer())) {
				CPad pad(0);
				if (radius < 2.0f) {
					SetAnim(11, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					gGame.GetPlayer()->GetPos(); // retail discards the position before checking the anim
					if (!IsBusyAnim(gGame.GetPlayer()->GetAnimId()) && gGame.GetPlayer()->mAttachmentRequest < 0
					    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
						mActorFlags.mFlags |= 0x80;
						gGame.GetPlayer()->GetAnimId(); // retail discards the id before clearing the status flags
						gGame.ClearStatusFlags(0x800000000000ULL);
					} else {
						PlayWiiSoundEffect(&gGame.mSound, 0x42F, 1.0f, 0.0f);
						mActorFlags.mFlags |= 0x200;
					}
					mKindStep = 20;
				} else if (IsUsingMug(&gGame.GetPlayer()->mLift)) {
					StunKind9Actor(this);
				}
			}
		}
		break;
	}
	case 20:
		if (--mFrameCount < 0) {
			mFrameCount = 2;
			Vec dustPos = mMoveTranslate;
			EmitKind9Dust(this, &dustPos);
		}
		if (Kind9AnimFinished(this, frames)) {
			mStatus.mFlags |= 0x100000;
			SetAnim(13, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mFrameCount = (3 * gRefreshRate) / frames;
			mKindStep   = 21;
			if (mActorFlags.Check(0x80)) {
				mActorFlags.mFlags &= ~0x80;
			}
			if (mActorFlags.mFlags & 0x200) {
				p.x                    = mMoveTranslate.x;
				p.y                    = 12.0f + mMoveTranslate.y;
				p.z                    = 12.0f + mMoveTranslate.z;
				scale.x                = 1.0f;
				scale.y                = 1.0f;
				scale.z                = 1.0f;
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 5, 0);
				if (effect != NULL) {
					effect->pos   = p;
					effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
					effect->scale = scale;
					UpdateEffectMtx(effect);
				}
				mGens[0] = effect;
				mActorFlags.mFlags &= ~0x200;
			}
		}
		break;
	case 21: {
		if (gGame.mEvt.mData.mScript) {
			break;
		}
		f32 distance = fabs(pos.x - mMoveTranslate.x);
		if (--mFrameCount < 0 && distance >= 12.0f) {
			mFrameCount = 0;
			SetAnim(12, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 22;
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
			mStatus.mFlags &= ~0x100000ULL;
		}
		break;
	}
	case 22:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			mFrameCount = 0;
			mKindStep   = 1;
		}
		break;
	case 100:
		if (mFrameCount & 1) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			++mKindStep;
		}
		break;
	case 101:
		break;
	}
}

static inline XObjSpiderBoss* GetSpiderBoss()
{
	return (XObjSpiderBoss*)gGame.mXObjs.mPools[19].mObjects;
}

static inline void PlaceBossProjectile(XObjActor3* actor, XObjSpiderBoss* boss, int spread)
{
	actor->mMoveTranslate.x = boss->GetPosX();
	actor->mMoveTranslate.y = 16.0f + boss->GetPosY() + rand() % spread;
	if (gSpiderBossVolleySlot == 0) {
		actor->mMoveTranslate.y = boss->GetPosY();
	}
	actor->mMoveTranslate.z = boss->GetPosZ();
	actor->SetPos(actor->mMoveTranslate);
	actor->mStatus.mFlags |= 0x10000;
	actor->mStatus.mFlags |= 4;
}

static inline void AimBossVolley(XObjActor3* actor, const Vec& center, f32 radius)
{
	actor->mStatus.mFlags |= 0x10000;
	actor->mTimerFadeMode = gSpiderBossVolleySlot;
	if (gSpiderBossVolleySlot == 0) {
		actor->mOrbitX      = center.x;
		actor->mOrbitHeight = center.y;
		actor->mOrbitZ      = center.z;
	} else {
		actor->mOrbitX      = center.x + radius * (f32)sin(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
		actor->mOrbitHeight = center.y;
		actor->mOrbitZ      = center.z + radius * (f32)cos(TWO_PI * (gSpiderBossVolleySlot - 1) / 9);
	}
	++gSpiderBossVolleySlot;
	++actor->mKindStep;
}

static inline bool BossProjectileLanded(XObjActor3* actor, u32 frames)
{
	actor->mKindTimer -= 0.09f * frames;
	actor->mMoveTranslate.y += actor->mKindTimer;
	return actor->mMoveTranslate.y <= -1.0f;
}

static inline void HideBossProjectile(XObjActor3* actor, bool resetPitch)
{
	actor->mStatus.mFlags &= ~0x10000ULL;
	actor->mStatus.mFlags &= ~4ULL;
	actor->mActorFlags.mFlags |= 4;
	actor->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	if (resetPitch) {
		actor->mMoveRotate.x = 0.0f;
	}
	actor->mKindStep = 0;
}

// NONMATCHING: the projectile/effect aggregate homes and FP register schedule still differ;
// boss position calls, effect handles, sound ids and all state transitions follow retail.
void XObjActor3::Kind13(u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	Vec pos                = *player->GetPos();
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				PlaceBossProjectile(this, boss, 64);
				mKindTimer = 16.0f;
				f32 dx = pos.x - mMoveTranslate.x, dy = 0.0f, dz = pos.z - mMoveTranslate.z;
				mMoveSpeed = 0.01f * FSqrt(dx * dx + dy * dy + dz * dz);
				if (mMoveSpeed < 1.2f) {
					mMoveSpeed = 1.2f;
				}
				if (mMoveSpeed > 24.0f) {
					mMoveSpeed = 24.0f;
				}
				AimBossVolley(this, pos, 24.0f);
			}
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		bool blocked         = false;
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000) && IsSpiderBossProjectileHitSuppressed(boss)) {
			blocked = true;
		}
		f32 dx = pos.x - mMoveTranslate.x, dy = pos.y - mMoveTranslate.y, dz = pos.z - mMoveTranslate.z;
		if (FSqrt(dx * dx + dy * dy + dz * dz) < 8.0f && !IsPlayerFlashingFromHit(player) && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)
		    && !blocked) {
			player->OnHitFromPosition(pos, 6.0f);
			mRumble     = 1;
			mFrameCount = 30;
			if (mGens[0] != NULL) {
				RetireEffect(&mGens[0]->handle, FALSE);
			}
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB1, 0);
			if (effect != NULL) {
				effect->pos   = mMoveTranslate;
				effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
				effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
				UpdateEffectMtx(effect);
			}
			mGens[0] = effect;
			PlayWiiSoundEffect(&gGame.mSound, 0x194, 1.0f, 0.0f);
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 2;
			break;
		}
		CVec velocity;
		if (BossProjectileLanded(this, frames)) {
			mKindTimer       = 0.0f;
			mMoveTranslate.y = 0.0f;
			mFrameCount      = 30;
			if (mTimerFadeMode == 0) {
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB1, 0);
				if (effect != NULL) {
					effect->pos   = mMoveTranslate;
					effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
					effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
					UpdateEffectMtx(effect);
				}
				mGens[0] = effect;
				PlayWiiSoundEffect(&gGame.mSound, 0x194, 1.0f, 0.0f);
			}
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 2;
			break;
		}
		if (mKindTimer >= -3.4499998f) {
			mKindTimer -= 0.36f * frames;
		} else {
			mKindTimer = -3.4499998f;
		}
		mHeading   = atan2(mOrbitX - mMoveTranslate.x, mOrbitZ - mMoveTranslate.z);
		velocity.x = mMoveSpeed * (f32)sin(mHeading);
		velocity.z = mMoveSpeed * (f32)cos(mHeading);
		velocity.y = 0.0f;
		mMoveTranslate.Set(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate);
		break;
	}
	case 2:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			HideBossProjectile(this, false);
		}
		break;
	case 10:
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				mMoveTranslate.x = boss->GetPosX();
				mMoveTranslate.y = 10.0f + boss->GetPosY();
				mMoveTranslate.z = boss->GetPosZ();
				SetPos(mMoveTranslate);
				mStatus.mFlags |= 0x10000;
				mStatus.mFlags |= 4;
				mMoveSpeed    = 2.0f;
				mHeading      = atan2(pos.x - mMoveTranslate.x, pos.z - mMoveTranslate.z);
				mMoveRotate.x = -HALF_PI;
				mStatus.mFlags |= 0x10000;
				mFrameCount = 120;
				++mKindStep;
			}
		}
		break;
	case 11: {
		f32 dx = pos.x - mMoveTranslate.x, dy = 0.0f, dz = pos.z - mMoveTranslate.z;
		if (FSqrt(dx * dx + dy * dy + dz * dz) < 8.0f && !IsPlayerFlashingFromHit(player)
		    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
			player->OnHitFromPosition(pos, 6.0f);
			mRumble                = 1;
			mFrameCount            = 30;
			EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB2, 0);
			if (effect != NULL) {
				effect->pos   = mMoveTranslate;
				effect->rot   = (Vec) { 0.0f, mHeading, 0.0f };
				effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
				UpdateEffectMtx(effect);
			}
			mGens[0] = effect;
			PlayWiiSoundEffect(&gGame.mSound, 0x194, 1.0f, 0.0f);
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 12;
		} else if (--mFrameCount < 0) {
			HideBossProjectile(this, true);
		} else {
			CVec velocity;
			velocity.x = mMoveSpeed * (f32)sin(mHeading);
			velocity.z = mMoveSpeed * (f32)cos(mHeading);
			velocity.y = 0.0f;
			mMoveTranslate.Set(mMoveTranslate + velocity * (f32)frames);
			SetPos(mMoveTranslate);
		}
		break;
	}
	case 12:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			HideBossProjectile(this, true);
		}
		break;
	case 20:
		if (!(mActorFlags.mFlags & 4)) {
			XObjSpiderBoss* boss = GetSpiderBoss();
			Vec center           = { 0.0f, 0.0f, 0.0f };
			if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
				PlaceBossProjectile(this, boss, 16);
				mKindTimer = 10.0f;
				center.x   = boss->GetPosX();
				center.y   = boss->GetPosY();
				center.z   = boss->GetPosZ();
				f32 dx = center.x - mMoveTranslate.x, dy = 0.0f, dz = center.z - mMoveTranslate.z;
				f32 distance = FSqrt(dx * dx + dy * dy + dz * dz);
				mMoveSpeed   = 2.4f;
				AimBossVolley(this, center, 36.0f);
			}
		}
		break;
	case 21: {
		CVec velocity;
		if (BossProjectileLanded(this, frames)) {
			mKindTimer       = 0.0f;
			mMoveTranslate.y = 0.0f;
			mFrameCount      = 30;
			if (mTimerFadeMode == 0) {
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB1, 0);
				if (effect != NULL) {
					effect->pos   = mMoveTranslate;
					effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
					effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
					UpdateEffectMtx(effect);
				}
				mGens[0] = effect;
				mRumble  = 1;
			}
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
			mKindStep = 22;
			break;
		}
		if (mKindTimer >= -3.4499998f) {
			mKindTimer -= 0.36f * frames;
		} else {
			mKindTimer = -3.4499998f;
		}
		mHeading   = atan2(mOrbitX - mMoveTranslate.x, mOrbitZ - mMoveTranslate.z);
		velocity.x = mMoveSpeed * (f32)sin(mHeading);
		velocity.z = mMoveSpeed * (f32)cos(mHeading);
		velocity.y = 0.0f;
		mMoveTranslate.Set(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate);
		break;
	}
	case 22:
		if (GetAnimEndFrame() - frames <= GetAnimFrame()) {
			HideBossProjectile(this, false);
		}
		break;
	}
}

static inline void BounceBossPart(XObjActor3* part, f32 speed)
{
	part->mKindTimer = speed;
	++part->mFrameCount;
}

// NONMATCHING: boss attachment/effect aggregate lifetimes and register scheduling differ;
// joint transforms, bounce states, sounds and visibility transitions follow retail.
void XObjActor3::Kind11(u32 frames)
{
	Vec playerPos = *gGame.GetPlayer()->GetPos();
	CPad pad(0);
	switch (mKindStep) {
	case 0: {
		if (mActorFlags.mFlags & 4) {
			break;
		}
		Vec origin           = { 0.0f, 0.0f, 0.0f };
		Vec rotation         = { 0.0f, 0.0f, 0.0f };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x   = boss->GetPosX();
			origin.y   = boss->GetPosY();
			origin.z   = boss->GetPosZ();
			rotation.x = boss->GetRotX();
			rotation.y = boss->GetRotY();
			rotation.z = boss->GetRotZ();
		}
		switch (GetKind()) {
		case 0x3CD:
			mOrbitDistance = 16.0f;
			mOrbitAngle    = -PI / 4;
			mOrbitHeight   = 10.0f;
			break;
		case 0x3CE:
			mOrbitDistance = 16.0f;
			mOrbitAngle    = PI / 4;
			mOrbitHeight   = 10.0f;
			break;
		case 0x3CF:
			mOrbitDistance = 10.0f;
			mOrbitAngle    = PI;
			mOrbitHeight   = 16.0f;
			break;
		case 0x3D0:
			mOrbitDistance = 0.0f;
			mOrbitAngle    = HALF_PI;
			mOrbitHeight   = 30.0f;
			break;
		case 0x3D1:
			mOrbitDistance = 30.0f;
			mOrbitAngle    = PI;
			mOrbitHeight   = 10.0f;
			break;
		case 0x3D2:
			mOrbitDistance = 40.0f;
			mOrbitAngle    = 3 * HALF_PI;
			mOrbitHeight   = 24.0f;
			break;
		case 0x3D3:
			mOrbitDistance = 30.0f;
			mOrbitAngle    = 3 * PI / 4;
			mOrbitHeight   = 14.0f;
			break;
		case 0x3D4:
			mOrbitDistance = 20.0f;
			mOrbitAngle    = 0.0f;
			mOrbitHeight   = 10.0f;
			break;
		case 0x3D5:
			mOrbitDistance = 16.0f;
			mOrbitAngle    = 3 * HALF_PI;
			mOrbitHeight   = 10.0f;
			break;
		default:
			mOrbitDistance = 32.0f;
			mOrbitAngle    = DEG2RAD * (rand() % 360);
			mOrbitHeight   = (48.0f - (rand() % 1800) / 100) / 2;
			break;
		}
		mOrbitX  = mOrbitDistance * (f32)sin(mOrbitAngle);
		mOrbitZ  = mOrbitDistance * (f32)cos(mOrbitAngle);
		mHeading = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		mStatus.mFlags |= 4;
		mFrameCount = 0;
		++mKindStep;
		break;
	}
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			f32 x = mOrbitX, y = mOrbitHeight, z = mOrbitZ;
			SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = boss->FindResNode("obj26");
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(boss->GetWorldMtx(joint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, x, y, z);
			PSMTXConcat(result, translation, result);
			SetBaseMtx(result);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15) {
					SetColor(1, 128, 0, 0, 0.0f);
				} else {
					SetColor(1, 0, 0, 0, 0.0f);
				}
			}
		}
		break;
	}
	case 2: {
		SetColor(1, 0, 0, 0, 0.0f);
		CVec velocity;
		mKindTimer -= 0.09f * frames;
		mMoveTranslate.y += mKindTimer;
		f32 floor = 0.0f;
		switch (GetKind()) {
		case 0x3D2:
			floor = 8.0f;
			break;
		case 0x3D3:
			floor = 16.0f;
			break;
		}
		if (mMoveTranslate.y <= floor) {
			mMoveTranslate.y = floor;
			switch (mFrameCount) {
			case 0: {
				++mFrameCount;
				mKindTimer = 8.0f;
				PlayWiiSoundEffect(&gGame.mSound, 0x477, 1.0f, 0.0f);
				Vec p;
				p.x                    = mMoveTranslate.x;
				p.y                    = mMoveTranslate.y - floor;
				p.z                    = mMoveTranslate.z;
				EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0xB5, 0);
				if (effect != NULL) {
					effect->pos   = p;
					effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
					effect->scale = (Vec) { 1.0f, 1.0f, 1.0f };
					UpdateEffectMtx(effect);
				}
				mGens[0] = effect;
				break;
			}
			case 1:
				mKindTimer = 7.0f;
				++mFrameCount;
				PlayWiiSoundEffect(&gGame.mSound, 0x477, 1.0f, 0.0f);
				if (mGens[0] != NULL) {
					RetireEffect(&mGens[0]->handle, FALSE);
				}
				break;
			case 2:
				BounceBossPart(this, 6.0f);
				break;
			case 3:
				BounceBossPart(this, 6.0f);
				break;
			case 4:
				BounceBossPart(this, 5.0f);
				break;
			case 5:
				BounceBossPart(this, 5.0f);
				break;
			case 6:
				BounceBossPart(this, 4.0f);
				break;
			case 7:
				BounceBossPart(this, 4.0f);
				break;
			case 8:
				BounceBossPart(this, 4.0f);
				break;
			case 9:
				mKindTimer  = 0.0f;
				mKindStep   = 3;
				mFrameCount = 30;
				break;
			}
		}
		if (mKindTimer >= -3.4499998f) {
			mKindTimer -= 0.36f * frames;
		} else {
			mKindTimer = -3.4499998f;
		}
		velocity.x = 0.6f * (f32)sin(mHeading);
		velocity.z = 0.6f * (f32)cos(mHeading);
		velocity.y = 0.0f;
		mMoveTranslate.Set(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate);
		break;
	}
	case 3:
		if ((mFrameCount & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			mKindStep = 0;
		}
		break;
	case 5: {
		CVec velocity;
		Vec origin           = { 0.0f, 0.0f, 0.0f };
		Vec rotation         = { 0.0f, 0.0f, 0.0f };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x            = boss->GetPosX();
			origin.y            = boss->GetPosY();
			origin.z            = boss->GetPosZ();
			rotation.x          = boss->GetRotX();
			rotation.y          = boss->GetRotY();
			rotation.z          = boss->GetRotZ();
			const CVec* bossPos = boss->GetPos();
			mHeading            = atan2(bossPos->x - mMoveTranslate.x, bossPos->z - mMoveTranslate.z);
		}
		f32 dx = origin.x - mMoveTranslate.x, dy = origin.y - mMoveTranslate.y, dz = origin.z - mMoveTranslate.z;
		if (FSqrt(dx * dx + dy * dy + dz * dz) < 16.0f) {
			mKindStep = 6;
		}
		velocity.x = 0.6f * (f32)sin(mHeading);
		velocity.z = 0.6f * (f32)cos(mHeading);
		velocity.y = 0.0f;
		mMoveTranslate.Set(mMoveTranslate + velocity * (f32)frames);
		SetPos(mMoveTranslate);
		break;
	}
	case 6: {
		Vec origin           = { 0.0f, 0.0f, 0.0f };
		Vec rotation         = { 0.0f, 0.0f, 0.0f };
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			origin.x   = boss->GetPosX();
			origin.y   = boss->GetPosY();
			origin.z   = boss->GetPosZ();
			rotation.x = boss->GetRotX();
			rotation.y = boss->GetRotY();
			rotation.z = boss->GetRotZ();
		}
		mOrbitAngle  = DEG2RAD * (rand() % 360);
		mOrbitX      = 32.0f * (f32)sin(mOrbitAngle);
		mOrbitHeight = (48.0f - (rand() % 1800) / 100) / 2;
		mOrbitZ      = 32.0f * (f32)cos(mOrbitAngle);
		mHeading     = atan2(origin.x - mMoveTranslate.x, origin.z - mMoveTranslate.z);
		mStatus.mFlags |= 0x100000;
		mKindStep = 1;
		if (pad.mPad->mTrigger & 0x0010000000001000ULL) {
			mKindStep = 5;
			mStatus.mFlags |= 0x10000;
			mActorFlags.mFlags &= ~4;
		}
		break;
	}
	}
}

extern "C" void EnableSpiderBossPartByIndex(s32 n)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	s32 count      = 0;
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1)) {
				if (obj->mKindStep == 0) {
					obj->mActorFlags.mFlags &= ~4;
				}
				return;
			}
			++count;
		}
	}
}

extern "C" void LaunchSpiderBossPartByIndex(s32 n)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	s32 count      = 0;
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0xB) {
			if (count == n && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
				LaunchSpiderBossPart(obj);
				return;
			}
			++count;
		}
	}
}

extern "C" void LaunchSpiderBossPart(XObjActor3* actor)
{
	XObj* holder = gGame.mXObjs.mPools[19].Get(0);
	if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && actor->mKindStep == 1) {
		Vec offset        = { actor->mOrbitDistance * (f32)sin(actor->mOrbitAngle + holder->GetRotY()), actor->mOrbitHeight,
			                  actor->mOrbitDistance * (f32)cos(actor->mOrbitAngle + holder->GetRotY()) };
		actor->mKindStep  = 2;
		actor->mKindTimer = 6.0f;
		actor->ClearBaseMtx();
		actor->mMoveTranslate.x = offset.x + holder->GetPosX();
		actor->mMoveTranslate.y = 10.0f + (8.0f + offset.y);
		actor->mMoveTranslate.z = offset.z + holder->GetPosZ();
		actor->SetPos(actor->mMoveTranslate);
		const CVec* holderPos = holder->GetPos();
		actor->mHeading       = (f32)atan2(holderPos->x - actor->mMoveTranslate.x, holderPos->z - actor->mMoveTranslate.z) + PI;
		PlayWiiSoundEffect(&gGame.mSound, 0x478, 1.0f, 0.0f);
	}
}

// NONMATCHING: retail also emits an unused mOrbitHeight comparison before reading the boss position;
// the remaining differences are that comparison, zero-vector store spelling and literal scheduling.
void XObjActor3::Kind12(u32 frames)
{
	gGame.GetPlayer()->GetPos(); // retail discards the player position before dispatching the state
	switch (mKindStep) {
	case 0:
		if (!(mActorFlags.mFlags & 4)) {
			mOrbitAngle  = 0.0f;
			mOrbitX      = 0.0f;
			mOrbitHeight = 0.0f;
			mOrbitZ      = 0.0f;
			mStatus.mFlags |= 0x100000;
			mStatus.mFlags |= 4;
			++mKindStep;
		}
		break;
	case 1: {
		XObjSpiderBoss* boss = GetSpiderBoss();
		if ((boss->mStatus.mFlags & 1) && (boss->mStatus.mFlags & 0x10000)) {
			Vec offset          = { 0.0f, 0.0f, 0.0f };
			const CVec* bossPos = boss->GetPos();
			mHeading            = atan2(bossPos->x - mMoveTranslate.x, bossPos->z - mMoveTranslate.z);
			SetScale(1.0f, 1.0f, 1.0f);
			nw4r::g3d::ResNode joint = boss->FindResNode("obj26");
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(boss->GetWorldMtx(joint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, offset.x, offset.y, offset.z);
			PSMTXConcat(result, translation, result);
			SetBaseMtx(result);
			if (boss->mBlinkTimer) {
				if (boss->mBlinkTimer % 30 < 15) {
					SetColor(1, 128, 0, 0, 0.0f);
				} else {
					SetColor(1, 0, 0, 0, 0.0f);
				}
			}
		}
		break;
	}
	case 2: {
		SetColor(1, 0, 0, 0, 0.0f);
		f32 current = GetAnimFrame();
		if (GetAnimEndFrame() - frames <= current) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			PlayWiiSoundEffect(&gGame.mSound, 0x477, 1.0f, 0.0f);
			mKindStep = 0;
		}
		break;
	}
	case 3:
		if ((mFrameCount & 3) < 2) {
			mStatus.mFlags |= 0x10000;
		} else {
			mStatus.mFlags &= ~0x10000ULL;
		}
		if (--mFrameCount < 0) {
			mStatus.mFlags &= ~0x10000ULL;
			mActorFlags.mFlags |= 4;
			mStatus.mFlags &= ~4ULL;
			mKindStep = 0;
			Release();
		}
		break;
	}
}

extern "C" void EnableSpiderBossCenterPart(void)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0xC && (obj->mStatus.mFlags & 1)) {
			if (obj->mKindStep == 0) {
				obj->mActorFlags.mFlags &= ~4;
			}
			return;
		}
	}
}

extern "C" void LaunchSpiderBossCenterPart(XObjActor3* obj);

extern "C" void fn_8013D0C8(void)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0xC && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			LaunchSpiderBossCenterPart(obj);
			return;
		}
	}
}

// NONMATCHING: retail has an unused mOrbitHeight comparison; the build also keeps gGame in
// an extra callee-saved register and schedules the zero-offset additions differently.
extern "C" void LaunchSpiderBossCenterPart(XObjActor3* obj)
{
	XObjSpiderBoss* holder = GetSpiderBoss();
	if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && obj->mKindStep == 1) {
		Vec offset;
		offset.x        = 0.0f;
		offset.y        = 0.0f;
		offset.z        = 0.0f;
		obj->mKindStep  = 2;
		obj->mKindTimer = 6.0f;
		obj->ClearBaseMtx();
		obj->mMoveTranslate.x = offset.x + holder->GetPosX();
		obj->mMoveTranslate.y = 10.0f + (8.0f + offset.y);
		obj->mMoveTranslate.z = offset.z + holder->GetPosZ();
		obj->SetPos(obj->mMoveTranslate);
		const CVec* holderPos = holder->GetPos();
		obj->mHeading         = (f32)atan2(holderPos->x - obj->mMoveTranslate.x, holderPos->z - obj->mMoveTranslate.z) + PI;
		obj->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
		PlayWiiSoundEffect(&gGame.mSound, 0x478, 1.0f, 0.0f);
	}
}

extern "C" void CommandSpiderBossProjectiles(s32 mode)
{
	if (mode == 0) {
		XObjPool& pool = gGame.mXObjs.mPools[5];
		for (s32 i = 0; i < 48; ++i) {
			XObjActor3* obj = (XObjActor3*)pool.Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mKindStep        = 0;
				gSpiderBossVolleySlot = 0;
			}
		}
	}
	if (mode == 2) {
		XObjPool& pool = gGame.mXObjs.mPools[5];
		for (s32 i = 0; i < 48; ++i) {
			XObjActor3* obj = (XObjActor3*)pool.Get(i);
			if (obj->GetKind() == 0x3E0 && (obj->mStatus.mFlags & 1) && obj->mActorFlags.Check(4)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mKindStep        = 20;
				gSpiderBossVolleySlot = 0;
			}
		}
	} else {
		XObjPool& pool = gGame.mXObjs.mPools[5];
		for (s32 i = 0; i < 48; ++i) {
			XObjActor3* obj = (XObjActor3*)pool.Get(i);
			if (obj->GetKind() == 0x3E0) {
				if (obj->mStatus.mFlags & 1) {
					if (obj->mActorFlags.Check(4)) {
						obj->mActorFlags.mFlags &= ~4;
						obj->mKindStep = 10;
						return;
					}
				} else {
					return;
				}
			}
		}
	}
}

void XObjActor3::UpdateSpiderCounterPanel(u32 frames)
{
	switch (mTimerFadeMode) {
	case 0:
		mTimerFadeMode   = 4;
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
	for (s32 i = 0; i < 16; ++i) {
		XObj* obj = gGame.mXObjs.mPools[17].Get(i);
		if ((obj->mStatus.mFlags & 0x10000) && !HasSpiderFinishedDeathAnimation(static_cast<XObjSpider*>(obj))) {
			++count;
		}
	}
	mSpiderCount = count;
}

void XObjActor3::Kind15(u32 frames)
{
	switch (mKindStep) {
	case 0:
		if (mActorFlags.mFlags & 4) {
			break;
		}
		++mKindStep;
		SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		if (++gActor3Kind15ActivationCounter >= 10) {
			gActor3Kind15ActivationCounter = 0;
		}
		if (!IsWiiSfxPlaying(&gGame.mSound, 0x194)) {
			PlayWiiSoundEffect(&gGame.mSound, 0x194, 1.0f, 0.0f);
		}
		break;
	case 1: {
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
			++mKindStep;
			SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		}
		break;
	}
	case 2:
		mActorFlags.mFlags |= 4;
		mKindStep = 0;
		break;
	}
}

extern "C" void ActivateWaitingKind43AActorsAtPosition(f32 x, f32 y, f32 z)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->GetKind() == 0x43A) {
			if ((obj->mStatus.mFlags & 1) && obj->mActorFlags.Check(4)) {
				obj->mActorFlags.mFlags &= ~4;
				obj->mMoveTranslate.x = x;
				obj->mMoveTranslate.y = y;
				obj->mMoveTranslate.z = z;
			}
		}
	}
}

static void SetSpiderBossAttachmentAnim(XObjActor3* obj, s32 n);

// puts a kind 0x10 actor back to its rest pose, small
static inline void ResetBossAttachment(XObjActor3* obj)
{
	obj->mKindStep = 0;
	obj->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	obj->mActorFlags.mFlags |= 4;
	obj->mActorFlags.mFlags |= 0x10;
	SetSpiderBossAttachmentAnim(obj, 0);
}

// NONMATCHING: the case-2 heading rounding and x/y stores schedule differently around sin;
// the shared position, joint return buffers, calls and animation-frame order match.
void XObjActor3::Kind16(u32 frames)
{
	Vec playerPos = *gGame.GetPlayer()->GetPos();
	Vec pos;
	XObj* boss = (XObj*)gGame.mXObjs.mPools[19].mObjects;
	if (mActorFlags.Check(0x10)) {
		return;
	}
	switch (mKindStep) {
	case 0:
		mActorFlags.mFlags |= 4;
		mActorFlags.mFlags |= 0x10;
		SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
		break;
	case 1: {
		pos = boss->GetJointPos(boss->FindResNode("obj24_3"));
		mMoveTranslate.Set(pos);
		mHeading = boss->GetRotY();
		break;
	}
	case 2: {
		pos              = boss->GetJointPos(boss->FindResNode("cyl3"));
		mHeading         = boss->GetRotY();
		mMoveTranslate.x = 2.0f * (f32)sin(mHeading) + pos.x;
		mMoveTranslate.y = pos.y;
		mMoveTranslate.z = 2.0f * (f32)cos(mHeading) + pos.z;
		break;
	}
	case 4: {
		pos = boss->GetJointPos(boss->FindResNode("obj24_3"));
		mMoveTranslate.Set(pos);
		mHeading    = boss->GetRotY();
		f32 current = GetAnimFrame();
		if (GetAnimEndFrame() - frames <= current) {
			ResetBossAttachment(this);
			Release();
		}
		break;
	}
	}
}

// plays anim n on a kind 0x10 actor and sizes it for it
static void SetSpiderBossAttachmentAnim(XObjActor3* obj, s32 n)
{
	obj->SetAnim(n, TRUE, 0.0f, 1.0f, 0.0f, 15.0f);
	obj->mActorFlags.mFlags &= ~4;
	obj->mActorFlags.mFlags &= ~0x10;
	switch (n) {
	case 0:
		obj->SetScale(0.1f, 0.1f, 0.1f);
		obj->mKindStep = 0;
		break;
	case 1:
		obj->SetScale(1.4f, 1.4f, 1.4f);
		obj->mKindStep = 1;
		break;
	case 2:
		obj->SetScale(1.0f, 1.0f, 1.0f);
		obj->mKindStep = 2;
		break;
	case 3:
		obj->SetScale(1.0f, 1.0f, 1.0f);
		obj->mKindStep = 1;
		break;
	case 4:
		obj->SetScale(1.0f, 1.0f, 1.0f);
		obj->mKindStep = 4;
		break;
	}
}

// plays anim n on the first kind 0x10 actor
extern "C" void PlaySpiderBossAttachmentAnimation(s32 n)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0x10 && (obj->mStatus.mFlags & 1)) {
			SetSpiderBossAttachmentAnim(obj, n);
			return;
		}
	}
}

// puts the first kind 0x10 actor back to its rest pose
extern "C" void ResetSpiderBossAttachment(void)
{
	XObjPool& pool = gGame.mXObjs.mPools[5];
	for (s32 i = 0; i < 48; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->mAttr.mObjData->GetItem() == 0x10 && (obj->mStatus.mFlags & 1)) {
			ResetBossAttachment(obj);
			return;
		}
	}
}

void XObjActor3::Kind17(u32 frames)
{
}

void XObjActor3::Kind18(u32 frames)
{
	f32 time = gGame.mTimeState.mTime;
	switch (mKindStep) {
	case 0:
		if (time >= 0.42f && time < 0.5f) {
			++mKindStep;
		}
		break;
	case 1:
		mMoveTranslate.x -= 0.2f * frames;
		if (mMoveTranslate.x <= PlacementPosition(mCoord).x - 10.0f) {
			mMoveTranslate.x = PlacementPosition(mCoord).x - 10.0f;
			++mKindStep;
		}
		break;
	case 2:
		if (time >= 0.5f) {
			++mKindStep;
		}
		break;
	case 3:
		mMoveTranslate.x += 0.2f * frames;
		if (mMoveTranslate.x >= PlacementPosition(mCoord).x) {
			mMoveTranslate.x = PlacementPosition(mCoord).x;
			mKindStep        = 0;
		}
		break;
	}
}

void XObjActor3::Kind19(u32 frames)
{
}

extern "C" void CarryRiderAtActorJoint(XObjActor3* actor, u32 frames)
{
	WiiPlayerState* player = gGame.GetPlayer();
	if ((actor->mActorFlags.mFlags & 0x40) && (player->mPlayerFlags & 0x40000000)) {
		Mtx scaleMtx, rotMtx, transMtx, mtx, m;
		nw4r::g3d::ResNode joint = actor->GetResNodeOrFirst(2);
		PSMTXCopy(actor->GetWorldMtx(joint), m);
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
		player->SetBaseMtx(mtx);
	}
}

extern "C" void HandleKind9ActorCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->mActorFlags.mFlags &= ~4;
			actor->mWindowLeft  = actor->mEventArg2 / 100.0f;
			actor->mWindowRight = actor->mEventArg3 / 100.0f;
		} else {
			actor->mActorFlags.mFlags |= 4;
			actor->mWindowLeft  = 0.0f;
			actor->mWindowRight = 0.0f;
			actor->mKindStep    = 0;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 2:
		if ((actor->mActorFlags.mFlags & 0x80) && !(actor->mActorFlags.mFlags & 4)) {
			gEventVars.mVars[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 3:
		actor->mActorFlags.mFlags &= ~0x80;
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	case 4: {
		Vec playerPos = *gGame.GetPlayer()->GetPos();
		if (!IsBusyAnim(gGame.GetPlayer()->GetAnimId()) && gGame.GetPlayer()->mAttachmentRequest < 0) {
			gEventVars.mVars[actor->mEventArg1] = 1;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
	}
}

extern "C" void HandleSpiderCounterCommand(XObjActor3* actor)
{
	switch (actor->mEventCommand) {
	case 0:
		break;
	case 1:
		if (actor->mEventArg1 != 0) {
			actor->mTimerFadeMode = 3;
		} else {
			actor->mTimerFadeMode = 4;
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
		actor->mCountdownFrames = (actor->mEventArg1 / 100.0f) * gRefreshRate;
		actor->mEventCommand    = 0;
		actor->mEventArg1       = 0;
		break;
	case 4:
		gEventVars.mVars[actor->mEventArg1] = TimerFramesToHundredths(actor->mCountdownFrames);
		actor->mEventCommand                = 0;
		actor->mEventArg1                   = 0;
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
		gEventVars.mVars[actor->mEventArg1] = actor->mSpiderCount;
		actor->mEventCommand                = 0;
		actor->mEventArg1                   = 0;
		break;
	}
}

extern "C" void HandleActorRiderCommand(XObjActor3* actor)
{
	WiiPlayerState* player = gGame.GetPlayer();
	switch (actor->mEventCommand) {
	case 1:
		switch (actor->mEventArg1) {
		case 1:
			actor->mActorFlags.mFlags |= 0x40;
			player->LockControl();
			break;
		default:
			actor->mActorFlags.mFlags &= ~0x40;
			CVec pos = actor->GetJointPos(actor->GetResNodeOrFirst(2));
			player->SetPos(pos);
			player->RestoreControl();
			player->ClearBaseMtx();
			break;
		}
		actor->mEventCommand = 0;
		actor->mEventArg1    = 0;
		break;
	}
}

extern "C" void HandleItem8ActorCommand(XObjActor3* actor)
{
	if (actor->mEventCommand != 1) {
		return;
	}
	s32 kind = -1;
	if (actor->mActorFlags.Check(0x100)) {
		kind = 0x40B;
		actor->mActorFlags.mFlags &= ~0x100;
	}
	if (kind >= 0) {
		switch (kind) {
		case 0x40B:
			for (s32 i = 0; i < 400; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					obj->mStatus.mFlags &= ~0x10000ULL;
				}
			}
			break;
		}
	}
	switch (actor->mEventArg1) {
	case 0x40B:
		for (s32 i = 0; i < 400; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (actor->mEventArg1 == obj->GetKind()) {
				actor->mHeld = obj;
				obj->mStatus.mFlags |= 0x10000ULL;
				switch (actor->mEventArg1) {
				case 0x40B:
					actor->mActorFlags.mFlags |= 0x100;
					obj->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					break;
				}
			}
		}
		break;
	default:
		if (actor->mHeld != NULL) {
			actor->mHeld->ClearBaseMtx();
			actor->mHeld = NULL;
		}
		break;
	}
	actor->mEventCommand = 0;
}

extern "C" void ResetActor3Fields(XObjActor3* actor)
{
	actor->_BA0               = 0.0f;
	actor->mMoveSpeed         = 0.0f;
	actor->mKindTimer         = 0.0f;
	actor->mHeading           = 0.0f;
	actor->mKindStep          = 0;
	actor->mFrameCount        = 0;
	actor->mActorFlags.mFlags = 0;
	actor->mActorFlags.mFlags |= 4;
	actor->mEventCommand      = 0;
	actor->mEventArg1         = 0;
	actor->mEventArg2         = 0;
	actor->mEventArg3         = 0;
	actor->mExtraEventArgs[0] = 0;
	actor->mExtraEventArgs[1] = 0;
	actor->mExtraEventArgs[2] = 0;
	actor->mExtraEventArgs[3] = 0;
	actor->mExtraEventArgs[4] = 0;
	actor->mExtraEventArgs[5] = 0;
	actor->mExtraEventArgs[6] = 0;
	actor->mWindowLeft        = 0.0f;
	actor->mWindowRight       = 0.0f;
	actor->mOrbitAngle        = 0.0f;
	Vec zero                  = { 0.0f, 0.0f, 0.0f };
	actor->mOrbitX            = zero.x;
	actor->mOrbitHeight       = zero.y;
	actor->mOrbitZ            = zero.z;
	for (int i = 0; i < 8; i++) {
		actor->mParts[i].Clear();
	}
	if (actor->mPoolIndex == 0) {
		gSpiderBossVolleySlot          = 0;
		gActor3Kind15ActivationCounter = 0;
	}
	actor->SetColor(1, 0, 0, 0, 0.0f);
	actor->_C0C.x             = 0.0f;
	actor->_C0C.y             = 0.0f;
	actor->_C0C.z             = 0.0f;
	actor->mTimerFadeMode     = 0;
	actor->mTimerPanelAlpha   = 0.0f;
	actor->mCountdownFrames   = 0.0f;
	actor->mCountdownRunning  = 0;
	actor->mCounterFadeMode   = 0;
	actor->mCounterPanelAlpha = 0.0f;
	actor->mSpiderCount       = 0;
	actor->_C38               = 0;
	if (actor->GetKind() == 0x31B) {
		InitParticlePool(&actor->mDustPool.mPtcl, actor->mDustPool.mPtcls, 0x50, 0x32);
		actor->mSfx.Init((WiiPlayerState*)actor, gHaisuiNezumiAnimSfxEvents);
	}
	actor->mGens[0]     = NULL;
	actor->mGens[1]     = NULL;
	actor->mGens[2]     = NULL;
	actor->mGens[3]     = NULL;
	actor->mGens[4]     = NULL;
	actor->mGens[5]     = NULL;
	actor->mRumble      = 0;
	actor->mRumbleTimer = 0;
	actor->mHeld        = NULL;
	actor->_1E20        = 0;
	actor->mSe          = 0;
}

void XObjActor3::InitializeObject()
{
	XObj::InitializeObject();
	ResetActor3Fields(this);
}

void XObjActor3::ResetObjectState()
{
	XObj::ResetObjectState();
	ResetActor3Fields(this);
}

void XObjActor3::ResetForNewGame()
{
	ResetActor3Fields(this);
}

BOOL XObjActor3::LoadFromPlacement(const CoordInfo& info)
{
	if (XObj::LoadFromPlacement(info)) {
		bool ready = false;
		mMoveSpeed = 0.0f;
		mKindTimer = 0.0f;

		mKindStep          = 0;
		mFrameCount        = 0;
		mActorFlags.mFlags = 0;
		mActorFlags.mFlags |= 4;
		mEventCommand      = 0;
		mEventArg1         = 0;
		mEventArg2         = 0;
		mEventArg3         = 0;
		mExtraEventArgs[0] = 0;
		mExtraEventArgs[1] = 0;
		mExtraEventArgs[2] = 0;
		mExtraEventArgs[3] = 0;
		mExtraEventArgs[4] = 0;
		mExtraEventArgs[5] = 0;
		mExtraEventArgs[6] = 0;
		mWindowLeft        = 0.0f;
		mWindowRight       = 0.0f;
		mOrbitAngle        = 0.0f;
		Vec zero           = { 0.0f, 0.0f, 0.0f };
		mOrbitX            = zero.x;
		mOrbitHeight       = zero.y;
		mOrbitZ            = zero.z;

		mMoveTranslate.x = PlacementPosition(mCoord).x;
		mMoveTranslate.y = PlacementPosition(mCoord).y;
		mMoveTranslate.z = PlacementPosition(mCoord).z;
		mMoveRotate.x    = PlacementRotation(mCoord).x;
		mMoveRotate.y    = PlacementRotation(mCoord).y;
		mMoveRotate.z    = PlacementRotation(mCoord).z;
		mHeading         = PlacementRotation(mCoord).y;
		for (int i = 0; i < 8; ++i) {
			mParts[i].Clear();
		}
		if (mPoolIndex == 0) {
			gSpiderBossVolleySlot          = 0;
			gActor3Kind15ActivationCounter = 0;
		}
		switch (GetKind()) {
		case 0x42B:
		case 0x149:
			if (mPanelTpl.ReadEx("tpl/stage02.tpl", 0, 0)) {
				ready = true;
			}
			return ready;
		case 0x31B:
			(&gGame.mTpl)->mTex.Init(&(&gGame.mTpl)->mPalette->descriptorArray[123], 0);
			mTex[0] = gGame.mTpl.mTex;
			(&gGame.mTpl)->mTex.Init(&(&gGame.mTpl)->mPalette->descriptorArray[124], 0);
			mTex[1] = gGame.mTpl.mTex;
			return TRUE;
		default:
			return TRUE;
		}
	}
	return FALSE;
}

void XObjActor3::Release()
{
	switch (GetKind()) {
	case 0x149:
	case 0x42B:
		mPanelTpl.Close();
		break;
	}
	XObj::Release();
}

void XObjActor3::UpdateObject(u32 frames)
{
	switch (GetItem()) {
	case 8:
		mActorFlags.mFlags |= 4;
		break;
	case 9:
		UpdateAnimationSounds();
		if (gGame.GetStatusFlags(0x400)) {
			return;
		}
		break;
	case 10:
		if (gGame.GetStatusFlags(0x400)) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 11:
	case 12:
		if (gGame.GetStatusFlags(0x400)) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			if (mEventArg1 != 0) {
				mActorFlags.mFlags &= ~4;
			} else {
				mActorFlags.mFlags |= 4;
				mKindStep = 0;
			}
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		case 2:
			mKindStep     = 2;
			mKindTimer    = mEventArg2 / 100.0f;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 13:
		switch (mRumble) {
		case 0:
			break;
		case 1:
			if (gGame.GetStatusFlags(0x800000000ULL)) {
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
		if (gGame.GetStatusFlags(0x400)) {
			return;
		}
		switch (mEventCommand) {
		case 1:
			mActorFlags.mFlags &= ~4;
			mEventCommand = 0;
			mEventArg1    = 0;
			break;
		}
		break;
	case 14:
		if (gGame.GetStatusFlags(0x400)) {
			return;
		}
		HandleSpiderCounterCommand(this);
		break;
	}
	if (mActorFlags.mFlags & 0x10) {
		return;
	}
	(this->*gActor3UpdateHandlers[GetItem()])(frames);
	if (!(mActorFlags.mFlags & 4)) {
		SetRotateY(mHeading, 0.0f);
		SetPos(mMoveTranslate);
	}
	XObj::UpdateObject(frames);
}

void XObjActor3::vf16C(u32 frames)
{
	if (mActorFlags.mFlags & 0x10) {
		return;
	}
	if (GetKind() == 0x31B) {
		UpdateParticlePool(&mDustPool.mPtcl, frames);
	}
	if (GetKind() == 0x2C3) {
		CarryRiderAtActorJoint(this, frames);
	}
	if (mActorFlags.mFlags & 2) {
		for (u32 i = 0; i < 8; i++) {
			if (mParts[i].mObj != NULL) {
				Mtx mtx;
				f32 z = mParts[i].mObj->GetPos()->z;
				f32 y = mParts[i].mObj->GetPos()->y;
				f32 x = mParts[i].mObj->GetPos()->x;
				PSMTXTrans(mtx, x, y, z);
				PSMTXConcat(GetWorldMtx(mParts[i].mJoint), mtx, mParts[i].mMtx);
				mParts[i].mObj->SetBaseMtx(mParts[i].mMtx);
			}
		}
	}
}

void XObjActor3::DrawObject()
{
	if (!(mActorFlags.mFlags & 0x10)) {
		DrawWithView(NULL);
	}
}

void XObjActor3::DrawScreenOverlay()
{
	if (GetItem() == 14) {
		DrawStage02TimerPanel(this);
		DrawStage02ActorCounterPanel(this);
	}
}

static inline void DrawStage02PanelLabel(s32 textId, f32 scale, f32 x, f32 y)
{
	CTexObj labelTex;
	const f32 size = 20.0f;
	f32 width      = scale * (MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(textId), TRUE, size) - 4.0f);
	DrawGsQuad(&gGame.mGs, 0xFF, 0xFF, 0xFF, 0xFF, x, y, 0.0f, width, 12.0f, 0.0f, 0.0f, 0.0f);
	f32 left = x - MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(textId), TRUE, size) / 2.0f;
	f32 top  = y - size / 2.0f - 3.0f;
	for (int i = 0; i < 4; ++i) {
		DrawGsProportionalTextFormat(&gGame.mGs, 0xFF, GetLocalizedInterfaceText(textId), left + gStage02TimerOutlineX[i],
		                             top + gStage02TimerOutlineY[i], 0.0f, size, size);
	}
	DrawGsProportionalTextFormat(&gGame.mGs, -1, GetLocalizedInterfaceText(textId), left, top, 0.0f, size, size);
}

extern "C" void DrawStage02TimerPanel(XObjActor3* actor)
{
	f32 alpha = actor->mTimerPanelAlpha;
	if (alpha <= 0.0f || !IsStage("stage02") || !actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	actor->mPanelTpl.mTex.Init(&actor->mPanelTpl.mPalette->descriptorArray[4], 0);
	tex         = actor->mPanelTpl.mTex;
	f32 scale   = 1.0f;
	f32 w       = scale * GXGetTexObjWidth(&tex);
	u16 h       = GXGetTexObjHeight(&tex);
	f32 centerX = gPointerScreenWidth / 2;
	f32 top     = gScreenSafeTop;
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, centerX, top + 10.0f, 0.0f, w, scale * h, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	DrawStage02PanelLabel(0x5C, scale, centerX, top + 10.0f);

	actor->mPanelTpl.mTex.Init(&actor->mPanelTpl.mPalette->descriptorArray[1], 0);
	tex       = actor->mPanelTpl.mTex;
	s32 tw    = GXGetTexObjWidth(&tex) / 11;
	f32 dw    = 1.4f * (scale * tw);
	u16 th    = GXGetTexObjHeight(&tex);
	f32 dh    = 1.4f * (scale * th);
	f32 x     = centerX - 144.0f - 45.0f - 16.0f - 5.0f + 11.0f;
	f32 y     = top + 36.0f;
	f32 count = TimerFramesToHundredths(actor->mCountdownFrames) / 100.0f;
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.09090909f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	u32 c0, c1;
	for (s32 i = 0; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
			c0 = (u32)(255.0f * alpha) | 0xFFFFFF00;
			c1 = (u32)(255.0f * alpha) | 0x1F33FF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, y, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
		}
	}

	tw = GXGetTexObjWidth(&tex) / 11;
	dw = 1.4f * (scale * tw);
	th = GXGetTexObjHeight(&tex);
	dh = 1.4f * (scale * th);
	PSMTXTrans(tex.mTexMtx.mMtx[0], 10.0f / 11.0f, 0.0f, 0.0f);
	tex.mTexMtx.mDirty = TRUE;
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
	s32 digits = 2;
	if (count < 10.0f) {
		digits = 1;
	}
	c0 = (u32)(255.0f * alpha) | 0xFFFFFF00;
	c1 = (u32)(255.0f * alpha) | 0x1F33FF00;
	DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - digits * (dw - 8.0f)) - 32.0f, y, 0.0f, dw, dh, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
}

extern "C" void DrawStage02ActorCounterPanel(XObjActor3* actor)
{
	if (gGame.mMotionControlCamera.IsAction(4)) {
		return;
	}
	f32 alpha = actor->mCounterPanelAlpha;
	if (alpha <= 0.0f || !IsStage("stage02") || !actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	actor->mPanelTpl.mTex.Init(&actor->mPanelTpl.mPalette->descriptorArray[9], 0);
	tex         = actor->mPanelTpl.mTex;
	f32 scale   = 1.0f;
	f32 w       = scale * GXGetTexObjWidth(&tex);
	u16 h       = GXGetTexObjHeight(&tex);
	f32 centerX = gPointerScreenWidth / 2;
	f32 top     = gScreenSafeTop;
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, centerX, top + 10.0f, 0.0f, w, scale * h, 0.0f, 0.0f, 0.0f);
	gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
	DrawStage02PanelLabel(0x5E, scale, centerX, top + 10.0f);

	actor->mPanelTpl.mTex.Init(&actor->mPanelTpl.mPalette->descriptorArray[1], 0);
	tex       = actor->mPanelTpl.mTex;
	s32 tw    = GXGetTexObjWidth(&tex) / 11;
	f32 dw    = 1.4f * (scale * tw);
	u16 th    = GXGetTexObjHeight(&tex);
	f32 dh    = 1.4f * (scale * th);
	f32 x     = centerX - 144.0f - 45.0f - 16.0f - 5.0f - (1 - actor->mSpiderCount / 10) * 12;
	f32 y     = top + 36.0f;
	f32 count = actor->mSpiderCount;
	PSMTXScale(tex.mTexMtx.mMtx[2], 0.09090909f, 1.0f, 1.0f);
	tex.mTexMtx.mDirty = TRUE;
	for (s32 i = 0; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mTexMtx.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mTexMtx.mDirty = TRUE;
			gGame.mGs.mCurTevStage->SetMode((GXTevMode)0);
			u32 c0 = (u32)(255.0f * alpha) | 0xFFFFFF00;
			u32 c1 = (u32)(255.0f * alpha) | 0x1F33FF00;
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, y, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			gGame.mGs.mCurTevStage->SetMode((GXTevMode)4);
		}
	}
}

void XObjActor3::DrawObjectEffects()
{
	if (GetKind() == 0x31B) {
		mDustPool.vf08(&gGame.mGs);
		DrawParticlePool(&mDustPool.mPtcl, &gGame.mGs);
	}
}

void XObjActor3::DrawDebugStatus()
{
}

s32 XObjActor3::HandleEventCommand(s32 count, s32* args)
{
	switch (args[0]) {
	case 100:
		if (args[1] == 0) {
			mActorFlags.mFlags &= ~2;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					mParts[i].mObj->ClearBaseMtx();
					mParts[i].mObj = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { }
			if (i < 8) {
				mActorFlags.mFlags |= 2;
				if (args[1] >= 10000) {
					mParts[i].mObj = gGame.mEvt.mData.mScript != NULL ? ResolveEventObjectId(&gGame.mEvt.mData, args[1]) : NULL;
				} else {
					mParts[i].mObj = gGame.mXObjs.mObjects[(s16)args[1]];
				}
				s32 path[8];
				s32 depth = args[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = args[3 + j];
				}
				mParts[i].mJoint = vf284(depth, path);
				Mtx mtx;
				f32 z = mParts[i].mObj->GetPos()->z;
				f32 y = mParts[i].mObj->GetPos()->y;
				f32 x = mParts[i].mObj->GetPos()->x;
				PSMTXTrans(mtx, x, y, z);
				PSMTXConcat(GetWorldMtx(mParts[i].mJoint), mtx, mParts[i].mMtx);
				mParts[i].mObj->SetBaseMtx(mParts[i].mMtx);
			}
		}
		break;
	default:
		mEventCommand      = args[0];
		mEventArg1         = args[1];
		mEventArg2         = args[2];
		mEventArg3         = args[3];
		mExtraEventArgs[0] = args[4];
		mExtraEventArgs[1] = args[5];
		mExtraEventArgs[2] = args[6];
		mExtraEventArgs[3] = args[7];
		mExtraEventArgs[4] = args[8];
		mExtraEventArgs[5] = args[9];
		mExtraEventArgs[6] = 0;
		switch (GetItem()) {
		case 9:
			HandleKind9ActorCommand(this);
			break;
		case 0x13:
			HandleActorRiderCommand(this);
			break;
		case 8:
			HandleItem8ActorCommand(this);
			break;
		}
		break;
	}
	return 0;
}

void XObjActor3::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (GetItem() == 1) {
		XObj::OnHitFromPosition(position, strength);
	} else if (!mActorFlags.Check(1)) {
		mActorFlags.mFlags |= 1;
	}
}

static void EmitKind9Dust(XObjActor3* actor, Vec* pos)
{
	f32 angle  = DEG2RAD * (rand() % 360);
	f32 sine   = std::sinf(angle);
	f32 vx     = sine * (rand() % 100) / 1000.0f;
	f32 cosine = std::cosf(angle);
	f32 vz     = cosine * (rand() % 100) / 1000.0f;
	f32 vy     = (rand() % 100) / 800.0f;
	Vec dustPos;
	dustPos.x = pos->x + 16.0f * (vx * (rand() % 100) / 100.0f);
	dustPos.y = pos->y;
	dustPos.z = pos->z + 16.0f * (vz * (rand() % 100) / 100.0f);
	Vec scale;
	scale.x = scale.y = 1.0f + (rand() % 200) / 200.0f;
	scale.z           = 1.0f;
	Vec vel           = { vx, vy, vz };
	CPtclOne* dust    = SpawnPoolParticle(&actor->mDustPool.mPtcl, &dustPos, &scale, &vel, 0xffffffa0);
	if (dust) {
		dust->mAngle      = DEG2RAD * (rand() % 360);
		dust->mAngleSpeed = DEG2RAD * (20 - rand() % 40);
		dust->mColor      = 0xffffffa0;
		dust->mTex        = &actor->mTex[0];
	}
}

void XObjActor3::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, frame, rate, blend, blendRate);
	if (GetKind() == 0x31B) {
		if (GetAnimId() == 7) {
			mSe = PlayWiiSoundEffect(&gGame.mSound, 0x42B, 1.0f, 0.0f);
		} else if (mSe != 0) {
			nw4r::snd::detail::BasicSound* sound = *(nw4r::snd::detail::BasicSound**)mSe;
			if (sound != NULL) {
				sound->Stop(0);
			}
			mSe = 0;
		}
	}
}

void XObjActor3::ResumeSounds(f32 fade)
{
	mSfx.Resume(fade);
}

void XObjActor3::PauseSounds(f32 fade)
{
	mSfx.Pause(fade);
}

void XObjActor3::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && mPoolIndex == 3) {
		f32 volume = GetSoundVolumeAtPosition(&gGame.mSound, GetPos(), 0);
		mSfx.Update(GetAnimId(), GetAnimFrame(), volume);
	}
}
