#include "game/wii/cgame.h"
#include "game/wii/infoscrn.h"
#include "game/wii/cpad.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
BOOL IsPlayerFlashingFromHit(WiiPlayerState* player);
BOOL IsUsingMug(XAction* action);
f32 GetPlayerCollisionRadius(WiiPlayerState* player);
CVec GetStagePlacementScale(const XObj* obj);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
void fn_801426C0(XObjActor3* actor, Vec* pos);
bool IsSpiderBossProjectileHitSuppressed(XObjSpiderBoss* boss);
void LaunchSpiderBossPart(XObjActor3* actor);
extern s32 gRefreshRate;
s32 gSpiderBossVolleySlot;
}

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
	actor->SetAnim(11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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

// NONMATCHING: the distance expression keeps a second Vec temporary in retail;
// calls, constants and state transitions are otherwise reconstructed.
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
		SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
		SetAnim(3, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		if (mHeading == -HALF_PI) {
			mMoveTranslate.x -= 0.1f * frames;
		} else {
			mMoveTranslate.x += 0.1f * frames;
		}
		if (mMoveTranslate.x < mWindowLeft) {
			mMoveTranslate.x = mWindowLeft;
			_BA0             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		} else if (mMoveTranslate.x > mWindowRight) {
			mMoveTranslate.x = mWindowRight;
			_BA0             = mHeading;
			mKindStep        = 3;
			SetAnim(8, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
		SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
		SetAnim(5, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(7, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(4, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
				SetAnim(9, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				mKindStep = 9;
			} else if (--mFrameCount < 0) {
				mFrameCount = 2;
				Vec dustPos = mMoveTranslate;
				fn_801426C0(this, &dustPos);
			}
		}
		break;
	case 9:
		if (Kind9AnimFinished(this, frames)) {
			SetAnim(10, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
					SetAnim(11, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					if (!IsBusyAnim(gGame.GetPlayer()->GetAnimId()) && gGame.GetPlayer()->mAttachmentRequest < 0
					    && !InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
						mActorFlags.mFlags |= 0x80;
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
			fn_801426C0(this, &dustPos);
		}
		if (Kind9AnimFinished(this, frames)) {
			mStatus.mFlags |= 0x100000;
			SetAnim(13, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(12, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
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
	actor->SetAnim(0, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
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
			SetScale(1.0f);
			nw4r::g3d::ResNode joint = boss->GetResNodeOrFirst(0);
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
		if (pad.mPad->mTrigger & 0x10) {
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
		if (obj->GetKind() == 0xB) {
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
		if (obj->GetKind() == 0xB) {
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
		f32 angle               = actor->mOrbitAngle + holder->GetRotY();
		f32 x                   = actor->mOrbitDistance * (f32)sin(angle);
		f32 y                   = actor->mOrbitHeight;
		f32 z                   = actor->mOrbitDistance * (f32)cos(angle);
		actor->mKindStep        = 2;
		actor->mKindTimer       = 6.0f;
		actor->mMoveTranslate.x = x + holder->GetPosX();
		actor->mMoveTranslate.y = y + 18.0f;
		actor->mMoveTranslate.z = z + holder->GetPosZ();
		actor->SetPos(actor->mMoveTranslate);
		const CVec* holderPos = holder->GetPos();
		actor->mHeading       = (f32)atan2(holderPos->x - actor->mMoveTranslate.x, holderPos->z - actor->mMoveTranslate.z) + PI;
		PlayWiiSoundEffect(&gGame.mSound, 0x478, 1.0f, 0.0f);
	}
}

void XObjActor3::Kind12(u32 frames)
{
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
			mHeading = atan2(boss->mMoveTranslate.x - mMoveTranslate.x, boss->mMoveTranslate.z - mMoveTranslate.z);
			SetScale(1.0f);
			nw4r::g3d::ResNode joint = boss->GetResNodeOrFirst(0);
			Mtx scaleMtx, translation, result, mount;
			PSMTXCopy(boss->GetWorldMtx(joint), mount);
			Vec scale;
			HSD_MtxGetScale(mount, &scale);
			scale.x = GetStagePlacementScale(this).x / scale.x;
			scale.y = GetStagePlacementScale(this).y / scale.y;
			scale.z = GetStagePlacementScale(this).z / scale.z;
			PSMTXScale(scaleMtx, scale.x, scale.y, scale.z);
			PSMTXConcat(mount, scaleMtx, result);
			PSMTXTrans(translation, 0.0f, 0.0f, 0.0f);
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
		f32 end = GetAnimEndFrame();
		if (end - frames <= GetAnimFrame()) {
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
		if (obj->GetKind() == 0xC && (obj->mStatus.mFlags & 1)) {
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
	for (s32 i = 0; i < 16; ++i) {
		XObjActor3* obj = (XObjActor3*)pool.Get(i);
		if (obj->GetKind() == 0xC && (obj->mStatus.mFlags & 1) && (obj->mStatus.mFlags & 0x10000)) {
			LaunchSpiderBossCenterPart(obj);
		}
	}
}

extern "C" void LaunchSpiderBossCenterPart(XObjActor3* obj)
{
	XObj* holder = (XObj*)gGame.mXObjs.mPools[19].mObjects;
	if ((holder->mStatus.mFlags & 1) && (holder->mStatus.mFlags & 0x10000) && obj->mKindStep == 1) {
		obj->mKindStep        = 2;
		obj->mKindTimer       = 6.0f;
		obj->mMoveTranslate.x = holder->GetPosX();
		obj->mMoveTranslate.y = 18.0f;
		obj->mMoveTranslate.z = holder->GetPosZ();
		obj->SetPos(obj->mMoveTranslate);
		const CVec* holderPos = holder->GetPos();
		obj->mHeading         = (f32)atan2(holderPos->x - obj->mMoveTranslate.x, holderPos->z - obj->mMoveTranslate.z) + PI;
		obj->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
		PlayWiiSoundEffect(&gGame.mSound, 0x478, 1.0f, 0.0f);
	}
}
