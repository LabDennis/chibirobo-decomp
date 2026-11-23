#include "game/wii/cgame.h"
#include "game/wii/ceffect.h"
#include "game/wii/xobj.h"
#include "game/gmath.h"
#include "game/wii/spiderplacement.h"
#include "arithmetic.h"
#include <math.h>
#include <stdlib.h>

extern "C" {
void ApproachHudValue(f32* value, f32* target, f32 step);
EffectInstance* fn_800A11A4(EffectInstance* effects, u32 id, s32 group);
CVec GetStagePlacementScale(const XObj* obj);
void HSD_MtxGetScale(Mtx matrix, Vec* scale);
StagePlacement* GetStageObjectPlacementByIndex(s32 index);
StagePlacement* GetSpawnData(s32 index);
XObj* SpawnObjectFromPlacement(XOBJS* list, CoordInfo* info, s32 direct, BOOL async);
s32 CountSmallSpidersGrabbingPlayer();
s32 CountEmergedSmallSpiders();
u16 GetItemFlagsMasked(s16 item, u16 mask);
void HandleToyKind0FCommand(XObjActorToy* toy, u32 frames);
void HandleToyKind110Command(XObjActorToy* toy, u32 frames);
void HandleToyKind156Command(XObjActorToy* toy, u32 frames);
void HandleToyKind16CCommand(XObjActorToy* toy, u32 frames);
void HandleToyKind159Command(XObjActorToy* toy, u32 frames);
void HandleToyKind1FCCommand(XObjActorToy* toy, u32 frames);
void UpdateToy0FCarriedObjects(XObjActorToy* toy, u32 frames);
void UpdateToyLinkedObject(XObjActorToy* toy, u32 frames);
void UpdateToy25EHeldObjects(XObjActorToy* toy, u32 frames);
extern s32 gRefreshRate;
}

static inline void HideToyDroppedItem(XObjActorToy* toy);

static f32 sToyMoveSpeeds[60] = {
	0.0f,  0.1f,  0.1f,  0.1f,  0.2f,  0.2f,  0.3f,  0.3f,  0.35f, 0.35f, 0.4f,  0.4f,  0.4f,  0.43f, 0.43f,
	0.47f, 0.48f, 0.45f, 0.44f, 0.43f, 0.4f,  0.4f,  0.4f,  0.4f,  0.37f, 0.36f, 0.31f, 0.3f,  0.3f,  0.3f,
	0.3f,  0.3f,  0.3f,  0.3f,  0.3f,  0.35f, 0.35f, 0.43f, 0.43f, 0.48f, 0.48f, 0.49f, 0.49f, 0.49f, 0.47f,
	0.47f, 0.46f, 0.43f, 0.41f, 0.38f, 0.36f, 0.36f, 0.36f, 0.36f, 0.36f, 0.3f,  0.24f, 0.2f,  0.12f, 0.0f,
};

static const Vec sToyBurstScales[10] = {
	{ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f },
};

static AnimSfxEvent sToy110Sounds[19] = {
	{ 4, 15.0f, -1.0f, 1, 0x000D000D, 0xFF, 0, 0, 0 },
	{ 4, 60.0f, -1.0f, 1, 0x00000145, 0xFF, 0, 0, 0 },
	{ 6, 22.0f, -1.0f, 1, 0x000D000F, 0xFF, 0, 0, 0 },
	{ 13, 0.0f, -1.0f, 1, 0x000D000B, 0xFF, 0, 0, 0 },
	{ 14, 0.0f, -1.0f, 1, 0x000D000C, 0xFF, 0, 0, 0 },
	{ 14, 52.0f, -1.0f, 1, 0x000D000E, 0xFF, 0, 0, 0 },
	{ 15, 0.0f, -1.0f, 1, 0x000D0011, 0xFF, 0, 0, 0 },
	{ 2, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1030, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 36, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 30.0f, 60.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 90.0f, 120.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 150.0f, 160.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 180.0f, 190.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 200.0f, 210.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 240.0f, 250.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 260.0f, 270.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ 1037, 280.0f, 300.0f, 1, 0x000D0010, 0xFF, 0, 0, 0 },
	{ -1 },
};

static AnimSfxEvent sToy6FSounds[5] = {
	{ 2, 33.0f, 66.0f, 1, 0x000C0008, 0xFF, 0, 0, 0 },
	{ 4, 40.0f, 80.0f, 1, 0x000C0006, 0xFF, 0, 0, 0 },
	{ 1005, 40.0f, -1.0f, 1, 0x000C0007, 0xFF, 0, 0, 0 },
	{ 1005, 126.0f, -1.0f, 1, 0x000C0005, 0xFF, 0, 0, 0 },
	{ -1 },
};

static AnimSfxEvent sToy1FCSounds[8] = {
	{ 1006, 73.0f, -1.0f, 1, 0x00100012, 0xFF, 0, 0, 0 }, { 2, 39.0f, 78.0f, 1, 0x00100013, 0xFF, 0, 0, 0 },
	{ 2, 117.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0 },   { 1012, 37.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0 },
	{ 1013, 37.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0 }, { 1014, 1.0f, 20.0f, 1, 0x00100014, 0xFF, 0, 0, 0 },
	{ 16, 1.0f, -1.0f, 1, 0x0010001F, 0xFF, 0, 0, 0 },    { -1 },
};

static AnimSfxEvent sToy26CAnd485Sounds[2] = {
	{ 2, 37.0f, 76.0f, 1, 0x0000015C, 0xFF, 0, 0, 0 },
	{ -1 },
};

static AnimSfxEvent sToy159Sounds[2] = {
	{ 7, 2.0f, -1.0f, 1, 0x000E0008, 0xFF, 0, 0, 0 },
	{ -1 },
};

static AnimSfxEvent sToy117Sounds[12] = {
	{ 1026, 5.0f, -1.0f, 1, 0x000F001D, 0xFF, 0, 0, 0 },  { 2, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0 },
	{ 2, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0 },    { 10, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0 },
	{ 10, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0 },   { 17, 26.0f, 85.0f, 1, 0x000F001F, 0xFF, 0, 0, 0 },
	{ 17, 55.0f, 115.0f, 1, 0x000F001E, 0xFF, 0, 0, 0 },  { 1039, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0 },
	{ 1039, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0 }, { 13, 49.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0 },
	{ 13, 95.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0 },   { -1 },
};

static AnimSfxEvent sToy1FBSounds[4] = {
	{ 2, 30.0f, 70.0f, 1, 0x0007001F, 0xFF, 0, 0, 0 },
	{ 2, 110.0f, -1.0f, 1, 0x0007001F, 0xFF, 0, 0, 0 },
	{ 43, 65.0f, -1.0f, 1, 0x0007001F, 0xFF, 0, 0, 0 },
	{ -1 },
};

static AnimSfxEvent sToy317Sounds[8] = {
	{ 2, 17.0f, 35.0f, 1, 0x0006000B, 0xFF, 0, 0, 0 },   { 4, 115.0f, -1.0f, 1, 0x0006000C, 0xFF, 0, 0, 0 },
	{ 4, 220.0f, -1.0f, 1, 0x0006000D, 0xFF, 0, 0, 0 },  { 11, 50.0f, -1.0f, 1, 0x0006000E, 0xFF, 0, 0, 0 },
	{ 5, 55.0f, -1.0f, 1, 0x0006000F, 0xFF, 0, 0, 0 },   { 8, 29.0f, -1.0f, 1, 0x0006000B, 0xFF, 0, 0, 0 },
	{ 13, 99.0f, 160.0f, 1, 0x00000019, 0xFF, 0, 0, 0 }, { -1 },
};

static AnimSfxEvent sToy25ESounds[5] = {
	{ 10, 1.0f, 50.0f, 1, 0x0012001C, 0xFF, 9, 0, 0 },
	{ 7, 15.0f, -1.0f, 1, 0x00120028, 0xFF, 0, 0, 0 },
	{ 4, 40.0f, -1.0f, 1, 0x00120029, 0xFF, 0, 0, 0 },
	{ 5, 30.0f, -1.0f, 1, 0x00120027, 0xFF, 0, 0, 0 },
	{ -1 },
};

static inline XObjActorToy* GetToy(s32 index)
{
	return (XObjActorToy*)gGame.mXObjs.mPools[10].Get(index);
}

static inline XObj* ReadObj(s16 id)
{
	return SpawnObjectFromPlacement(&gGame.mXObjs, &CoordInfo(GetStageObjectPlacementByIndex(id)), 0, TRUE);
}

static inline f32 GetToyHorizontalDistance(Vec* first, Vec* second)
{
	f32 dx = first->x - second->x;
	f32 dy = 0.0f;
	f32 dz = first->z - second->z;
	return FSqrt(dx * dx + dy * dy + dz * dz);
}

void XObjActorToy::InitializeObject()
{
	XObjActor::InitializeObject();
	mLoad      = 0;
	mShownPart = 0;
	mFirst.InitializeObject();
	mSecond.InitializeObject();
}

void XObjActorToy::Release()
{
	mFirst.Release();
	mSecond.Release();
	XObj::Release();
	mLoad      = 0;
	mShownPart = 0;
}

void XObjActorToy::DrawObject()
{
	XObjActor::DrawObject();
	if (mFirst.mStatus.mFlags & 0x10000) {
		mFirst.SetPos(GetPosX(), GetPosY(), GetPosZ());
		mFirst.SetScale(GetScaleX(), GetScaleY(), GetScaleZ());
		mFirst.SetRotationY(GetRotY());
		mFirst.DrawObject();
	}
	if (mSecond.mStatus.mFlags & 0x10000) {
		mSecond.SetPos(GetPosX(), GetPosY(), GetPosZ());
		mSecond.SetScale(GetScaleX(), GetScaleY(), GetScaleZ());
		mSecond.SetRotationY(GetRotY());
		mSecond.DrawObject();
	}
}

void XObjActorToy::DrawScreenOverlay()
{
	XObjActor::DrawScreenOverlay();
	if (mFirst.mStatus.mFlags & 0x10000) {
		mFirst.DrawScreenOverlay();
	}
	if (mSecond.mStatus.mFlags & 0x10000) {
		mSecond.DrawScreenOverlay();
	}
}

// NONMATCHING: toy 0x0F's StateHitRecovery pointer needs a Toy-only mState declaration;
// putting it in the shared XObjActor regresses the other actor constructors. The remaining
// differences are ResNode/temporary layout. Wii sound ids and the first three-object search
// come from retail.
void XObjActorToy::ProcessPendingEvents(u32 frames)
{
	XObjActorToy* toy = this;
	if (toy->mActorStatus.Check(2)) {
		toy->mActorStatus.mFlags &= ~2;
		toy->_9CC = 5 * frames;
		switch (toy->GetKind()) {
		case 0xF:
			toy->_9D4 = 20.0f;
			if (!toy->mActorStatus.Check(0x40) && !toy->mKindFlags.Check(2)) {
				toy->ResetMovementStep();
				toy->mActorStatus.mFlags |= 0x40;
				toy->OnMoveRequest();
			}
			break;
		case 0x6F:
			if (!toy->mKindStep) {
				toy->mKindStep = 1;
			}
			break;
		case 0x110:
			toy->_9CC = 30 * frames;
			break;
		case 0x117:
			toy->_9D4 = 3.0f;
			break;
		case 0x156:
			toy->_9D4 = 2.0f;
			break;
		case 0x158:
			break;
		case 0x1D3:
			PlayWiiSoundEffect(&gGame.mSound, 0xD8 + rand() % 12, 1.0f, 0.0f);
			break;
		case 0x159:
			toy->_9D4 = 3.0f;
			break;
		case 0x16C:
			break;
		}
		if (--toy->_9CC < 0) {
			toy->_9D4 = 1.0f;
			toy->_9CC = 0;
		}
		f32 step = Abs((toy->_9D4 - toy->_9D0) / 4.0f);
		if (step < 0.01f) {
			step = 0.01f;
		}
		ApproachHudValue(&toy->_9D0, &toy->_9D4, step);

		switch (toy->GetKind()) {
		case 0x6F:
		case 0x117:
		case 0x159:
		case 0x1D3:
		case 0x1FB:
		case 0x1FC:
		case 0x25E:
		case 0x26C:
		case 0x26D:
		case 0x485:
			break;
		case 0xF: {
			if (toy->mSfxActors[0].GetHandle() == NULL && (toy->mStatus.mFlags & 0x10000) && gGame.mMovie.mState == 0) {
				CSound3DActor* actor = toy->mSfxActors[0].mActor;
				if (actor != NULL && actor->StartSound(&actor->mHandle, 0x165)) {
					actor->mHandle.SetVolume(1.0f, 0);
				}
			}
			if ((toy->mSfxActors[0].GetHandle() == NULL || !(toy->mStatus.mFlags & 0x10000)) && gGame.mMovie.mState != 0
			    && toy->mSfxActors[0].mActor != NULL) {
				toy->mSfxActors[0].mActor->mHandle.Stop((s32)(gRefreshRate * 0.0f));
			}
			if (toy->mFirst.mStatus.mFlags & 0x10000) {
				toy->mFirst.UpdateObject(frames);
			}
			if (toy->mSecond.mStatus.mFlags & 0x10000) {
				toy->mSecond.UpdateObject(frames);
			}
			s32 anim = toy->GetAnimId();
			switch (toy->mShownPart) {
			case 0:
				if (anim == 0x13 || anim == 0x3FB) {
					toy->mFirst.SetPos(*toy->GetPos());
					toy->mFirst.SetRotationY(toy->GetRotY());
					toy->mFirst.mStatus.mFlags |= 0x10000;
					toy->mFirst.SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
					toy->mShownPart = 1;
				} else if (anim == 0xE || anim == 0x14 || anim == 0x3F6 || anim == 0x3FC) {
					toy->mSecond.SetPos(*toy->GetPos());
					toy->mSecond.SetRotationY(toy->GetRotY());
					toy->mSecond.SetAnim(1, TRUE, 0.0f, 1.0f, 0.0f, 10.0f);
					toy->mSecond.mStatus.mFlags |= 0x10000;
					toy->mShownPart = 2;
				}
				break;
			case 1:
				toy->mFirst.SetPos(*toy->GetPos());
				toy->mFirst.SetRotationY(toy->GetRotY());
				if (anim != 0x13 && anim != 0x3FB) {
					toy->mFirst.mStatus.mFlags &= ~0x10000ULL;
					toy->mShownPart = 0;
				}
				break;
			case 2:
				toy->mSecond.SetPos(*toy->GetPos());
				toy->mSecond.SetRotationY(toy->GetRotY());
				if (toy->GetAnimEndFrame() - frames <= toy->GetAnimFrame()) {
					toy->mSecond.mStatus.mFlags &= ~0x10000ULL;
					toy->mShownPart = 0;
				}
				break;
			}
			break;
		}
		case 0x110:
			if (!(toy->mKindFlags.mFlags & 1)) {
				break;
			}
			switch (toy->mKindStep) {
			case 0:
				break;
			case 1:
				++toy->mKindStep;
				toy->mMovePhase = 0;
				toy->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			// fall through
			case 2:
				if (toy->mKindStep == 2) {
					toy->mMovePhase += 2;
				}
				toy->mMoveSpeed = sToyMoveSpeeds[toy->mMovePhase % 60];
				if (PSVECDistance(&toy->mMoveTranslate, &CVec(toy->mVelocity)) < 0.1f) {
					toy->mMoveSpeed = 0.0f;
					toy->mKindStep  = 0;
					toy->mKindFlags.mFlags &= ~1;
					toy->SetPos(toy->mVelocity.x, toy->mVelocity.y, toy->mVelocity.z);
					toy->SetMoveDir(&CVec(toy->mMoveTranslate), 0.0f);
				} else {
					toy->SetMoveDir(&CVec(toy->mVelocity), toy->mMoveSpeed);
					CVec destination(toy->mVelocity);
					toy->SetRotationY((f32)atan2(destination.x - toy->GetPos()->x, destination.z - toy->GetPos()->z));
				}
				break;
			}
			break;
		case 0x156:
			if (toy->mKindFlags.mFlags & 1) {
				switch (toy->mKindStep) {
				case 0:
					if (toy->mMoveTranslate.y <= 2.0f) {
						++toy->mKindStep;
						toy->mStatus.mFlags &= ~0x10000ULL;
					}
					break;
				case 1:
				case 2:
					break;
				}
			}
			break;
		case 0x157:
			switch (toy->mKindStep) {
			case 2:
				break;
			case 0:
				if (toy->mStatus.mFlags & 0x1000000000000000ULL) {
					toy->mActorStatus.mFlags |= 0xC;
					toy->mMoveSpeed  = 0.5f;
					toy->mVelocity.x = 14.0f * (f32)sin(toy->mHeading) + toy->mMoveTranslate.x;
					toy->mVelocity.y = toy->mMoveTranslate.y;
					toy->mVelocity.z = 14.0f * (f32)cos(toy->mHeading) + toy->mMoveTranslate.z;
					++toy->mKindStep;
					PlayWiiSoundEffect(&gGame.mSound, 0x3D5, 1.0f, 0.0f);
					for (s32 i = 0; i < 3; ++i) {
						XObjActorToy* other = GetToy(i);
						if (other->GetKind() == 0x156) {
							other->mKindFlags.mFlags |= 0x10;
							break;
						}
					}
				}
				break;
			case 1:
				if (GetToyHorizontalDistance(&toy->mVelocity, &toy->mMoveTranslate) < 1.0f) {
					toy->mActorStatus.mFlags &= ~0xC;
					toy->mMoveSpeed = 0.0f;
				}
				if (toy->mMoveTranslate.y <= 2.0f) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->mStatus.mFlags &= ~0x10000ULL;
					for (s32 i = 0; i < 15; ++i) {
						XObjActorToy* other = GetToy(i);
						if (other->GetKind() == 0x16C) {
							other->mStatus.mFlags |= 0x10000;
							other->SetTranslateX(toy->mMoveTranslate.x);
							other->SetTranslateY(toy->mMoveTranslate.y);
							other->SetTranslateZ(toy->mMoveTranslate.z);
							other->mKindFlags.mFlags |= 4;
							PlayWiiSoundEffect(&gGame.mSound, 0x3D4, 1.0f, 0.0f);
							break;
						}
					}
				}
				break;
			}
			break;
		case 0x16C:
			switch (toy->mKindStep) {
			case 0:
				if (toy->mKindFlags.Check(4)) {
					toy->mKindFlags.mFlags &= ~4;
					toy->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					++toy->mKindStep;
				}
				break;
			case 1:
				if (toy->GetAnimEndFrame() - frames <= toy->GetAnimFrame()) {
					toy->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					++toy->mKindStep;
					toy->mKindCounter = 0;
				}
				break;
			case 2:
				if (toy->mKindFlags.Check(8)) {
					toy->mKindFlags.mFlags &= ~8;
					toy->SetAnim(2, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
					toy->mKindStep    = 10;
					toy->mKindCounter = 0;
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[0];
						UpdateEffectMtx(effect);
					}
				}
				break;
			case 10:
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[1];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 11:
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[2];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 12:
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[3];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 13:
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[4];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 14:
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[5];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 15:
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[6];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 16:
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[7];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 17:
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[8];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 18:
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					PlayWiiSoundEffect(&gGame.mSound, 0x4B2, 1.0f, 0.0f);
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 0x5B, 0);
					if (effect != NULL) {
						effect->pos   = toy->mMoveTranslate;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = sToyBurstScales[9];
						UpdateEffectMtx(effect);
					}
					return;
				}
				break;
			case 19:
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				toy->GetResNodeOrFirst(0);
				++toy->mKindCounter;
				if ((s32)toy->mKindCounter > 4) {
					++toy->mKindStep;
					toy->mKindCounter = 0;
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->GetResNodeOrFirst(0);
					toy->mStatus.mFlags &= ~0x10000ULL;
					return;
				}
				break;
			case 20:
				break;
			}
			break;
		}

		switch (toy->GetKind()) {
		case 0x158:
		case 0x1D3:
		case 0x1FB:
		case 0x26D:
			break;
		case 0xF:
			HandleToyKind0FCommand(toy, frames);
			break;
		case 0x6F:
			switch (toy->mCmd[0]) {
			case 0:
				break;
			case 1:
				switch (toy->mCmd[1]) {
				case 0:
					toy->mKindFlags.mFlags |= 1;
					toy->mCmd[0] = 0;
					break;
				case 1:
					toy->mKindFlags.mFlags &= ~1;
					toy->mCmd[0] = 0;
					break;
				}
				break;
			}
			break;
		case 0x110:
			HandleToyKind110Command(toy, frames);
			break;
		case 0x117:
			switch (toy->mCmd[0]) {
			case 0:
				break;
			case 1:
				switch (toy->mCmd[1]) {
				case 8:
					toy->mKindFlags.mFlags |= 8;
					break;
				default:
					toy->mKindFlags.mFlags &= ~8;
					break;
				}
				toy->mCmd[0] = 0;
				break;
			}
			break;
		case 0x156:
			HandleToyKind156Command(toy, frames);
			break;
		case 0x157:
			switch (toy->mCmd[0]) {
			case 0:
				break;
			case 1:
				if (toy->mKindFlags.mFlags & 2) {
					gEventVars.mVars[toy->mCmd[1]] = 1;
				}
				toy->mCmd[0] = 0;
				break;
			}
			break;
		case 0x16C:
			HandleToyKind16CCommand(toy, frames);
			break;
		case 0x159:
			HandleToyKind159Command(toy, frames);
			break;
		case 0x1FC:
			StartScreenShake(&gGame.mQuake, toy->mCmd[1], toy->mCmd[2], toy->mCmd[3], 0.0f, 0.0f, 0.0f, toy->mCmd[4] / 100.0f);
			toy->mCmd[0] = 0;
			break;
		case 0x26C:
		case 0x485:
		case 0x25E:
			break;
		}
	}
}

static inline void ScaleToyCallbackResult(nw4r::g3d::ChrAnmResult* result, f32 scale)
{
	Mtx mtx;
	Mtx scaling;
	PSMTXCopy(result->mpMtx, mtx);
	PSMTXScale(scaling, scale, scale, scale);
	PSMTXConcat(mtx, scaling, mtx);
	PSMTXCopy(mtx, result->mpMtx);
}

static inline void RotateToyCallbackResult(nw4r::g3d::ChrAnmResult* result, char axis, f32 angle)
{
	Mtx mtx;
	Mtx rotation;
	PSMTXCopy(result->mpMtx, mtx);
	PSMTXRotRad(rotation, axis, angle);
	PSMTXConcat(mtx, rotation, mtx);
	PSMTXCopy(mtx, result->mpMtx);
}

static inline void TranslateToyCallbackResult(nw4r::g3d::ChrAnmResult* result, f32 y, f32 z)
{
	Mtx mtx;
	Mtx translation;
	PSMTXCopy(result->mpMtx, mtx);
	PSMTXTrans(translation, 0.0f, y, z);
	PSMTXConcat(mtx, translation, mtx);
	PSMTXCopy(mtx, result->mpMtx);
}

static f32 sToy6FAngle;
static f32 sToy1FCTime;

// NONMATCHING: matrix temporary lifetimes and node-id assertion ordering still differ.
void XObjActorToy::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	XObj::ExecCallbackB(pResult, mdl, pFuncObj);
	const u16 nodeId = pFuncObj->GetCallbackNodeID();
	switch (GetKind()) {
	case 0xF:
		if (nodeId == 0) {
			if (mActorStatus.mFlags & 0x10) {
				Mtx mtx;
				PSMTXCopy(pResult->mpMtx, mtx);
				_2204 += mFallSpeed;
				if (mMoveTranslate.y + _2204 <= mGroundHeight) {
					mActorStatus.mFlags &= ~0x18;
					mStatus.mFlags &= ~0x30000ULL;
					mFallSpeed = 0.0f;
				} else {
					Mtx translation;
					PSMTXTrans(translation, 0.0f, 0.0f, 0.0f);
					PSMTXConcat(mtx, translation, mtx);
					PSMTXCopy(mtx, pResult->mpMtx);
				}
			}
			if (_21F0 != 0.0f) {
				RotateToyCallbackResult(pResult, 'y', _21F4);
			}
			if (_9D0 != 1.0f) {
				nw4r::g3d::ResNode node = GetResNode("r_eye_a");
				if (node.IsValid()) {
					pFuncObj->SetCallbackNodeID(node.GetID());
				}
			}
		} else {
			ScaleToyCallbackResult(pResult, _9D0);
		}
		break;
	case 0x6F:
		if (nodeId == 0) {
			if (mKindStep == 1) {
				nw4r::g3d::ResNode node = GetResNode("cube212");
				if (node.IsValid()) {
					pFuncObj->SetCallbackNodeID(node.GetID());
				}
			}
		} else {
			sToy6FAngle += 0.4f;
			if (sToy6FAngle >= TWO_PI) {
				mKindStep   = 0;
				sToy6FAngle = 0.0f;
			}
			RotateToyCallbackResult(pResult, 'z', sToy6FAngle);
		}
		break;
	case 0x117:
		if (nodeId == 0) {
			if (_9D0 != 1.0f) {
				nw4r::g3d::ResNode node = GetResNode("_02tai");
				if (node.IsValid()) {
					pFuncObj->SetCallbackNodeID(node.GetID());
				}
			}
		} else {
			ScaleToyCallbackResult(pResult, _9D0);
		}
		break;
	case 0x156: {
		nw4r::g3d::ResNode first  = GetResNode("cube21");
		nw4r::g3d::ResNode second = GetResNode("cube3");
		if (nodeId == 0) {
			if (_9D0 != 1.0f && first.IsValid()) {
				pFuncObj->SetCallbackNodeID(first.GetID());
			}
		} else if (nodeId == first.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
			if (second.IsValid()) {
				pFuncObj->SetCallbackNodeID(second.GetID());
			}
		} else if (nodeId == second.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
		}
		break;
	}
	case 0x159: {
		nw4r::g3d::ResNode first  = GetResNode("L_tuno");
		nw4r::g3d::ResNode second = GetResNode("R_tuno");
		if (nodeId == 0) {
			if (_9D0 != 1.0f && second.IsValid()) {
				pFuncObj->SetCallbackNodeID(second.GetID());
			}
		} else if (nodeId == second.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
			if (first.IsValid()) {
				pFuncObj->SetCallbackNodeID(first.GetID());
			}
		} else if (nodeId == first.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
		}
		break;
	}
	case 0x1FB: {
		nw4r::g3d::ResNode first  = GetResNode("hand3");
		nw4r::g3d::ResNode second = GetResNode("hand5");
		if (nodeId == 0) {
			if (_9D0 != 1.0f && first.IsValid()) {
				pFuncObj->SetCallbackNodeID(first.GetID());
			}
		} else if (nodeId == first.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
			if (second.IsValid()) {
				pFuncObj->SetCallbackNodeID(second.GetID());
			}
		} else if (nodeId == second.GetID()) {
			ScaleToyCallbackResult(pResult, _9D0);
		}
		break;
	}
	case 0x1FC: {
		nw4r::g3d::ResNode first  = GetResNode("obj6");
		nw4r::g3d::ResNode second = GetResNode("obj7");
		if (nodeId == 0) {
			switch (mKindStep) {
			case 1:
				++mKindStep;
				break;
			case 2:
				sToy1FCTime += 0.16f;
				if (sToy1FCTime >= 1.0f) {
					++mKindStep;
				}
				break;
			case 3:
				sToy1FCTime -= 0.16f;
				if (sToy1FCTime <= 0.0f) {
					mKindStep = 0;
				}
				break;
			}
			if (first.IsValid()) {
				pFuncObj->SetCallbackNodeID(first.GetID());
			}
		} else if (nodeId == first.GetID()) {
			if (mKindStep >= 2 && mKindStep < 4) {
				TranslateToyCallbackResult(pResult, 0.0f, 16.0f * sToy1FCTime);
			}
			if (second.IsValid()) {
				pFuncObj->SetCallbackNodeID(second.GetID());
			}
		} else if (nodeId == second.GetID()) {
			if (mKindStep >= 2 && mKindStep < 4) {
				TranslateToyCallbackResult(pResult, 0.0f, 16.0f * sToy1FCTime);
			}
		}
		break;
	}
	case 0x26C:
	case 0x485: {
		nw4r::g3d::ResNode node = GetResNode("obj2");
		if (nodeId == 0) {
			if (node.IsValid()) {
				pFuncObj->SetCallbackNodeID(node.GetID());
			}
		} else if (nodeId == node.GetID()) {
			switch (mKindStep) {
			case 0:
				if (_9CC > 0) {
					++mKindStep;
				}
				break;
			case 1:
				TranslateToyCallbackResult(pResult, -_9D0, 0.0f);
				if (_9D0 == 1.0f) {
					mKindStep = 0;
				}
				break;
			}
		}
		break;
	}
	case 0x26D: {
		nw4r::g3d::ResNode node = GetResNode("obj27");
		if (nodeId == 0) {
			if (node.IsValid()) {
				pFuncObj->SetCallbackNodeID(node.GetID());
			}
		} else if (nodeId == node.GetID()) {
			switch (mKindStep) {
			case 1:
				++mKindStep;
				mFacing = 0.0f;
				break;
			case 2:
				mFacing += 0.2f;
				if (mFacing >= TWO_PI) {
					mKindStep = 0;
					mFacing   = 0.0f;
				}
				RotateToyCallbackResult(pResult, 'y', mFacing);
				break;
			}
		}
		break;
	}
	}
}

void XObjActorToy::UpdateAfterAnim(u32 frames)
{
	XObjActor::UpdateAfterAnim(frames);
	switch (GetKind()) {
	case 0xF:
		UpdateToy0FCarriedObjects(this, frames);
		break;
	case 0x156:
		if (mKindFlags.mFlags & 1) {
			for (s32 i = 0; i < 15; ++i) {
				XObjActorToy* toy = GetToy(i);
				if (toy->GetKind() == 0x157) {
					CVec position    = *toy->GetPos();
					mMoveTranslate.x = position.x;
					mMoveTranslate.y = position.y;
					mMoveTranslate.z = position.z;
				}
			}
		}
		break;
	case 0x26C:
	case 0x485:
		UpdateToyLinkedObject(this, frames);
		break;
	case 0x25E:
		UpdateToy25EHeldObjects(this, frames);
		break;
	}
}

static Vec sToyLinkedObjectOrigin;

extern "C" void UpdateToyLinkedObject(XObjActorToy* toy, u32 frames)
{
	if (toy->_21E0 == NULL) {
		toy->_21E0 = (XObj*)1;
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
			XObj* obj = gGame.mXObjs.mObjects[i];
			if ((obj->mStatus.mFlags & 0x100000000ULL) && obj->GetKind() == 0x6A && obj->mPlacementParam0 == 1) {
				toy->_21E0 = obj;
				break;
			}
		}
	} else if (toy->_21E0 != (XObj*)1 && toy->_21E0 != NULL
	           && (toy->_21E0->mStatus.mFlags & 0x0001000100000000ULL) == 0x0001000100000000ULL) {
		Mtx translation, result;
		Vec position;
		PSMTXTrans(translation, 0.0f, 5.5f, -5.5f);
		PSMTXConcat(toy->GetWorldMtx("obj6"), translation, translation);
		PSMTXMultVec(translation, &sToyLinkedObjectOrigin, &position);
		toy->_21E0->SetPos(position);
		toy->_21E0->SetRotationY(toy->mMoveRotate.y);
	}
}

extern "C" void UpdateToy25EHeldObjects(XObjActorToy* toy, u32 frames)
{
	if (toy->mHideKinds.mFlags & 0x40) {
		nw4r::g3d::ResNode joint = toy->FindResNode("cube1");
		CVec position;
		position = toy->GetJointPos(joint);
		s32 id   = -1;
		if (toy->mHideKinds.mFlags & 0x40) {
			id = 0x341;
		}
		if (id >= 0) {
			for (s32 i = 0; i < 400; ++i) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (obj->GetKind() == id) {
					obj->SetBaseMtx(toy->GetWorldMtx(joint));
					if (toy->mStatus.mFlags & 0x10000) {
						obj->mStatus.mFlags |= 0x10000ULL;
					} else {
						obj->mStatus.mFlags &= ~0x10000ULL;
					}
				}
			}
		}
	}
}

void XObjActorToy::_unk_00C(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	if (timing != 4) {
		return;
	}
	switch (GetKind()) {
	case 0xF:
	case 0x156:
	case 0x157:
	case 0x158:
	case 0x159:
	case 0x16C:
	case 0x1D3:
	case 0x1FB:
	case 0x25E:
	case 0x26C:
	case 0x26D:
	case 0x485:
		break;
	case 0x6F: {
		bool visible            = !(mKindFlags.mFlags & 1);
		nw4r::g3d::ResNode node = GetResNode("cube212");
		nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
		if (access.IsValid()) {
			access.SetVisibilityEx(visible);
		} else {
			node.SetVisibility(visible);
		}
		break;
	}
	case 0x110: {
		bool visible            = !(_9CC & 2);
		nw4r::g3d::ResNode node = GetResNode("_02tai");
		nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
		if (access.IsValid()) {
			access.SetVisibilityEx(visible);
		} else {
			node.SetVisibility(visible);
		}
		break;
	}
	case 0x117: {
		bool visible            = (mKindFlags.mFlags & 8) != 0;
		nw4r::g3d::ResNode node = GetResNode("cube21");
		nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
		if (access.IsValid()) {
			access.SetVisibilityEx(visible);
		} else {
			node.SetVisibility(visible);
		}
		break;
	}
	case 0x1FC: {
		static const char* nodeNames[] = { "obj1", "cube87", "cube88", "cube89", NULL };
		bool visible                   = !(mKindFlags.mFlags & 1);
		for (const char** name = nodeNames; *name != NULL; ++name) {
			nw4r::g3d::ResNode node = GetResNode(*name);
			nw4r::g3d::ScnMdl::CopiedVisAccess access(mObject.m_pScnMdl, node.GetID());
			if (access.IsValid()) {
				access.SetVisibilityEx(visible);
			} else {
				node.SetVisibility(visible);
			}
		}
		break;
	}
	}
}

void XObjActorToy::StateCarry(u32 frames)
{
	Vec position;
	switch (GetKind()) {
	case 0x6F:
	case 0x110:
	case 0x117:
	case 0x158:
	case 0x16C:
	case 0x1D3:
	case 0x1FB:
	case 0x1FC:
	case 0x26C:
	case 0x26D:
	case 0x485:
		break;
	case 0xF:
		if (mAttachedId >= 0 && mAttached) {
			nw4r::g3d::ResNode joint = FindResNode("r_hand");
			Mtx mount, translation, rotation, result;
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', HALF_PI);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, -1.2f, -5.5f, -0.5f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			if (mStatus.mFlags & 0x10000) {
				mAttached->mStatus.mFlags |= 0x10000ULL;
			} else {
				mAttached->mStatus.mFlags &= ~0x10000ULL;
			}
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	case 0x156:
		if (mAttachedId >= 0 && mAttached) {
			nw4r::g3d::ResNode joint = FindResNode("sphere2");
			position                 = GetJointPos(joint);
			mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z);
			Mtx mount, translation, rotation, result;
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', 0.0f);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			if (mStatus.mFlags & 0x10000) {
				mAttached->mStatus.mFlags |= 0x10000ULL;
			} else {
				mAttached->mStatus.mFlags &= ~0x10000ULL;
			}
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	case 0x159:
		if (mAttachedId >= 0 && mAttached) {
			nw4r::g3d::ResNode joint = FindResNode("obj7");
			position                 = GetJointPos(joint);
			mAttached->SetPos(position.x, 12.0f + position.y, position.z);
			PSMTXCopy(GetWorldMtx(joint), mAttachedMtx);
			HSD_MtxGetScale(mAttachedMtx, &position);
			position.x = GetStagePlacementScale(mAttached).x / position.x;
			position.y = GetStagePlacementScale(mAttached).y / position.y;
			position.z = GetStagePlacementScale(mAttached).z / position.z;
			Mtx scale, translation, rotation, result;
			PSMTXScale(scale, position.x, position.y, position.z);
			PSMTXConcat(mAttachedMtx, scale, result);
			PSMTXRotRad(rotation, 'y', -PI);
			PSMTXConcat(result, rotation, result);
			PSMTXRotRad(rotation, 'x', -HALF_PI);
			PSMTXConcat(result, rotation, result);
			PSMTXTrans(translation, 5.0f, 12.0f, 2.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			if (mStatus.mFlags & 0x10000) {
				mAttached->mStatus.mFlags |= 0x10000ULL;
			} else {
				mAttached->mStatus.mFlags &= ~0x10000ULL;
			}
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	case 0x25E:
		if (mAttachedId >= 0 && mAttached) {
			nw4r::g3d::ResNode joint = FindResNode("obj1_1");
			position                 = GetJointPos(joint);
			mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z);
			Mtx mount, translation, rotation, result;
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', 0.0f);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, 0.0f, 1.1f, 1.4f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			if (mStatus.mFlags & 0x10000) {
				mAttached->mStatus.mFlags |= 0x10000ULL;
			} else {
				mAttached->mStatus.mFlags &= ~0x10000ULL;
			}
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		break;
	}
}

void XObjActorToy::StateHitRecovery(u32 frames)
{
	if (mAttachedId >= 0 && mAttached) {
		Mtx mount, translation, rotation, result;
		switch (GetKind()) {
		case 0xF: {
			nw4r::g3d::ResNode joint = FindResNode("r_hand");
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', HALF_PI);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, -1.2f, -5.5f, -0.5f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			break;
		}
		case 0x156: {
			nw4r::g3d::ResNode joint = FindResNode("sphere2");
			Vec pos                  = GetJointPos(joint);
			mAttached->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z);
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', 0.0f);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			break;
		}
		case 0x25E: {
			nw4r::g3d::ResNode joint = FindResNode("obj1_1");
			Vec pos                  = GetJointPos(joint);
			mAttached->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z);
			mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
			PSMTXCopy(GetWorldMtx(joint), mount);
			PSMTXRotRad(rotation, 'z', 0.0f);
			PSMTXConcat(mount, rotation, result);
			PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
			PSMTXConcat(result, translation, result);
			mAttached->SetBaseMtx(result);
			break;
		}
		}
		if (mStatus.mFlags & 0x10000) {
			mAttached->mStatus.mFlags |= 0x10000ULL;
		} else {
			mAttached->mStatus.mFlags &= ~0x10000ULL;
		}
	}
	switch (mKindStep) {
	case 0: {
		Vec& player = gGame.GetPlayer()->mMoveTranslate;
		mHeading    = PI + (f32)atan2(player.x - mMoveTranslate.x, player.z - mMoveTranslate.z);
		Vec pos     = { 0.0f, 0.0f, 0.0f };
		pos.x       = mMoveTranslate.x;
		pos.y       = mMoveTranslate.y;
		pos.z       = mMoveTranslate.z;
		mHomePos.x  = pos.x;
		mHomePos.y  = pos.y;
		mHomePos.z  = pos.z;
		mActorStatus.mFlags |= 8;
		mActorStatus.mFlags |= 0x10;
		mActorStatus.mFlags |= 4;
		if (mHitStrength >= 10.0f) {
			mMoveSpeed = 0.0f;
			mFallSpeed = 6.8f;
		} else {
			mMoveSpeed = 0.0f;
			mFallSpeed = 4.8f;
		}
		mKindCounter = 0;
		++mKindStep;
		break;
	}
	case 1:
		++mKindCounter;
		if (mActorStatus.Check(0x20)) {
			mFallSpeed = 0.0f;
			++mKindStep;
		}
		break;
	case 2:
		mMoveSpeed -= 0.04f * frames;
		if (mMoveSpeed <= 0.0f) {
			mMoveSpeed       = 0.0f;
			mMoveTranslate.x = mHomePos.x;
			mMoveTranslate.y = mHomePos.y;
			mMoveTranslate.z = mHomePos.z;
			mKindStep        = 4;
		}
		break;
	case 3: {
		mMoveSpeed = 0.5f;
		mHeading   = atan2(mHomePos.x - mMoveTranslate.x, mHomePos.z - mMoveTranslate.z);
		mFacing    = mHeading;
		CVec home(mHomePos);
		if (GetToyHorizontalDistance(&home, &mMoveTranslate) < 1.0f) {
			mMoveTranslate.x = mHomePos.x;
			mMoveTranslate.y = mHomePos.y;
			mMoveTranslate.z = mHomePos.z;
			mMoveSpeed       = 0.0f;
			++mKindStep;
		}
		break;
	}
	case 4:
		mActorStatus.mFlags &= ~4;
		mActorStatus.mFlags &= ~0x40;
		mState = (StateFunc)&XObjActorToy::StateCarry;
		ResetMovementStep();
		break;
	}
}

void XObjActorToy::StatePickUp(u32 frames)
{
	switch (mKindStep) {
	case 0: {
		XObjActorTao* tao = (XObjActorTao*)gGame.mXObjs.mPools[14].First();
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree()) {
			return;
		}
		XObjActorPapa* papa = (XObjActorPapa*)gGame.mXObjs.mPools[13].First();
		if ((papa->mStatus.mFlags & 1) && !((XObjActorTao*)papa)->IsFree()) {
			return;
		}
		if (mAttached) {
			mAttached->Release();
		}
		if (mAttachedId >= 0) {
			mAttached = ReadObj(mAttachedId);
		} else {
			mAttached = NULL;
		}
		mActorStatus.mFlags |= 0x2000;
		++mKindStep;
	}
	case 1:
		if (mAttached) {
			if (mAttached->mStatus.mFlags & 0x0040000000000000ULL) {
				return;
			}
			gGame.mStatus.mFlags |= 0x1000;
		}
		++mKindStep;
	case 2:
		if (mAttachedId >= 0 && mAttached) {
			mAttached->SetScale(1.0f, 1.0f, 1.0f);
			Mtx mount, translation, rotation, result;
			switch (GetKind()) {
			case 0xF: {
				nw4r::g3d::ResNode joint = FindResNode("r_hand");
				mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				PSMTXCopy(GetWorldMtx(joint), mount);
				PSMTXRotRad(rotation, 'z', HALF_PI);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translation, -1.2f, -5.5f, -0.5f);
				PSMTXConcat(result, translation, result);
				mAttached->SetBaseMtx(result);
				break;
			}
			case 0x156: {
				nw4r::g3d::ResNode joint = FindResNode("sphere2");
				Vec position             = GetJointPos(joint);
				mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z);
				mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				PSMTXCopy(GetWorldMtx(joint), mount);
				PSMTXRotRad(rotation, 'z', 0.0f);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
				PSMTXConcat(result, translation, result);
				mAttached->SetBaseMtx(result);
				break;
			}
			case 0x159: {
				nw4r::g3d::ResNode joint = FindResNode("obj7");
				Vec position             = GetJointPos(joint);
				mAttached->SetPos(position.x, 12.0f + position.y, position.z);
				PSMTXCopy(GetWorldMtx(joint), mAttachedMtx);
				HSD_MtxGetScale(mAttachedMtx, &position);
				position.x = GetStagePlacementScale(mAttached).x / position.x;
				position.y = GetStagePlacementScale(mAttached).y / position.y;
				position.z = GetStagePlacementScale(mAttached).z / position.z;
				PSMTXScale(mount, position.x, position.y, position.z);
				PSMTXConcat(mAttachedMtx, mount, result);
				PSMTXRotRad(rotation, 'y', -PI);
				PSMTXConcat(result, rotation, result);
				PSMTXRotRad(rotation, 'x', -HALF_PI);
				PSMTXConcat(result, rotation, result);
				PSMTXTrans(translation, 5.0f, 12.0f, 2.0f);
				PSMTXConcat(result, translation, result);
				mAttached->SetBaseMtx(result);
				break;
			}
			case 0x25E: {
				nw4r::g3d::ResNode joint = FindResNode("obj1_1");
				Vec position             = GetJointPos(joint);
				mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z);
				mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 10.0f);
				PSMTXCopy(GetWorldMtx(joint), mount);
				PSMTXRotRad(rotation, 'z', 0.0f);
				PSMTXConcat(mount, rotation, result);
				PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
				PSMTXConcat(result, translation, result);
				mAttached->SetBaseMtx(result);
				break;
			}
			}
			if (mStatus.mFlags & 0x10000) {
				mAttached->mStatus.mFlags |= 0x10000ULL;
			} else {
				mAttached->mStatus.mFlags &= ~0x10000ULL;
			}
			mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
		}
		mActorStatus.mFlags &= ~0x2000;
		mState = (StateFunc)&XObjActorToy::StateCarry;
		ResetMovementStep();
		break;
	}
}

extern "C" void HandleToyKind110Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		switch (object->mCmd[1]) {
		case 0: {
			object->mKindFlags.mFlags &= ~1;
			object->mMoveSpeed = 0.0f;
			object->mKindStep  = 0;
			CVec pos(object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			object->SetMoveDir(&pos, 0.0f);
			break;
		}
		case 1:
			object->mKindFlags.mFlags |= 1;
			object->mVelocity.x = object->mCmd[2] / 100.0f;
			object->mVelocity.y = object->mCmd[3] / 100.0f;
			object->mVelocity.z = object->mCmd[4] / 100.0f;
			object->mKindStep   = 1;
			break;
		}
		object->mCmd[0] = 0;
		break;
	}
}

extern "C" void HandleToyKind156Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 1:
		object->mCmd[0] = 0;
		break;
	case 2:
		if (object->mMoveTranslate.y <= 2.0f) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		if (object->mKindFlags.mFlags & 0x10) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 4:
		object->mAttachedId = object->mCmd[1];
		object->mState      = (XObjActor::StateFunc)&XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 5:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	}
}

extern "C" void HandleToyKind16CCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		object->mCmd[0] = 0;
		break;
	case 1:
		StartScreenShake(&gGame.mQuake, object->mCmd[1], object->mCmd[2], object->mCmd[3], 0.0f, 0.0f, 0.0f, object->mCmd[4] / 100.0f);
		PlayWiiSoundEffect(&gGame.mSound, 0x4F7, 1.0f, 0.0f);
		object->mCmd[0] = 0;
		break;
	case 2:
		switch (object->mCmd[1]) {
		case 0:
			object->mKindFlags.mFlags |= 1;
			object->mCmd[0] = 0;
			break;
		case 1:
			object->mKindFlags.mFlags &= ~1;
			object->mCmd[0] = 0;
			break;
		}
		break;
	}
}

extern "C" void HandleToyKind159Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		object->mAttachedId = object->mCmd[1];
		object->mState      = (XObjActor::StateFunc)&XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 2:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		HideToyDroppedItem(object);
		switch (object->mCmd[1]) {
		case 0x341:
			for (s32 i = 0; i < 200; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (object->mCmd[1] == obj->GetKind()) {
					object->mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					object->mHideKinds.mFlags |= 0x40;
				}
			}
			break;
		default:
			if (object->mHeldObjects[0] != NULL) {
				object->mHeldObjects[0]->ClearBaseMtx();
				object->mHeldObjects[0] = NULL;
			}
			break;
		}
		object->mCmd[0] = 0;
		break;
	}
}

extern "C" void HandleToyKind1FCCommand(XObjActorToy* object, u32 frames)
{
	s32 id = -1;
	if (object->mHideKinds.Check(1)) {
		id = 0x3D;
		object->mHideKinds.mFlags &= ~1;
	} else if (object->mHideKinds.Check(2)) {
		id = 0x3E;
		object->mHideKinds.mFlags &= ~2;
	} else if (object->mHideKinds.Check(4)) {
		id = 0x3F;
		object->mHideKinds.mFlags &= ~4;
	} else if (object->mHideKinds.Check(8)) {
		id = 0x3B;
		object->mHideKinds.mFlags &= ~8;
	} else if (object->mHideKinds.Check(0x80)) {
		id = 0x21D;
		object->mHideKinds.mFlags &= ~0x80;
	} else if (object->mHideKinds.Check(0x10)) {
		id = 0x21E;
		object->mHideKinds.mFlags &= ~0x10;
	} else if (object->mHideKinds.Check(0x40)) {
		id = 0x341;
		object->mHideKinds.mFlags &= ~0x40;
	}
	if (id < 0) {
		return;
	}
	switch (id) {
	case 0x3B:
	case 0x3D:
	case 0x3E:
	case 0x3F:
	case 0x21D:
	case 0x21E:
		for (s32 i = 0; i < 32; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x341:
		for (s32 i = 0; i < 200; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

void XObjActorToy::OnMoveRequest()
{
	switch (GetKind()) {
	case 0xF:
		if (_21F0 > 0.0f) {
			_21F0 = 0.0f;
			_21F4 = 0.0f;
			_21FC = 0;
		}
		if (_21F8 >= 0) {
			SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
			_21F8 = -1;
		}
		if (_2200 != NULL) {
			if (!_2200->IsFinished()) {
				RetireEffect(&_2200->handle, TRUE);
			}
			_2200 = NULL;
		}
		break;
	}
}

extern "C" void UpdateToy0FCarriedObjects(XObjActorToy* toy, u32 frames)
{
	if (toy->mKindFlags.Check(1)) {
		if (toy->_21F0 == 0.0f) {
			if (toy->_21F8 >= 0 && toy->IsAnimDone()) {
				toy->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				toy->_21F8 = -1;
			}
		} else {
			if (toy->_21F0 > 0.0f) {
				toy->_21F0 -= DEG2RAD * 0.3f * frames;
				if (toy->_21F0 < DEG2RAD * 6.0f) {
					toy->_21F0 = DEG2RAD * 6.0f;
				}
			}
			f32 previous = toy->_21F4;
			if (toy->_21EC >= 0.0f) {
				toy->_21F4 += toy->_21F0 * frames;
				if (toy->_21F0 <= DEG2RAD * 6.0f && fmod(previous, TWO_PI) >= fmod(toy->_21F4, TWO_PI)) {
					toy->_21F0 = 0.0f;
				}
			} else {
				toy->_21F4 -= toy->_21F0 * frames;
				if (toy->_21F0 <= DEG2RAD * 6.0f && fmod(previous, TWO_PI) <= fmod(toy->_21F4, TWO_PI)) {
					toy->_21F0 = 0.0f;
				}
			}
			if (toy->_21F0 == 0.0f && toy->_21F8 >= 0) {
				toy->SetRotateY(PI + gGame.mMotionControlCamera.mView.mAngle.y, DEG2RAD * 6.0f);
				toy->SetAnim(toy->_21F8, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				if (toy->_2200 != NULL && !toy->_2200->IsFinished()) {
					RetireEffect(&toy->_2200->handle, TRUE);
				}
				if (toy->_21FC) {
					toy->_21FC             = 0;
					CVec position          = toy->GetJointWorldPos("body");
					EffectInstance* effect = fn_800A11A4((EffectInstance*)&gGame.mEvtPtcl, 5, 1);
					if (effect != NULL) {
						effect->pos   = position;
						effect->rot   = (Vec) { 0.0f, 0.0f, 0.0f };
						effect->scale = *toy->GetScale();
						UpdateEffectMtx(effect);
					}
					toy->_2200 = effect;
				}
			}
		}
		Vec& player  = gGame.GetPlayer()->mMoveTranslate;
		toy->mFacing = (f32)atan2(player.x - toy->mMoveTranslate.x, player.z - toy->mMoveTranslate.z);
		toy->SetRotateY(toy->mFacing, DEG2RAD * 6.0f);
	} else if (toy->_21F0 > 0.0f) {
		toy->_21F0 = 0.0f;
		toy->_21F8 = -1;
		toy->_21FC = 0;
	}

	toy->mMoveTranslate.x += 0.1f * (toy->mPendingMove.x * frames);
	toy->mMoveTranslate.z += 0.1f * (toy->mPendingMove.z * frames);
	toy->mPendingMove.Set(0.0f, 0.0f, 0.0f);
	if (toy->mHideKinds.mFlags & 0x9F) {
		nw4r::g3d::ResNode joint = toy->FindResNode("r_hand");
		CVec position;
		position = toy->GetJointPos(joint);
		s32 kind = -1;
		if (toy->mHideKinds.Check(1)) {
			kind = 0x3D;
		} else if (toy->mHideKinds.Check(2)) {
			kind = 0x3E;
		} else if (toy->mHideKinds.Check(4)) {
			kind = 0x3F;
		} else if (toy->mHideKinds.Check(8)) {
			kind = 0x3B;
		} else if (toy->mHideKinds.Check(0x80)) {
			kind = 0x21D;
		} else if (toy->mHideKinds.Check(0x10)) {
			kind = 0x21E;
		}
		if (kind >= 0) {
			for (s32 i = 0; i < 64; ++i) {
				XObj* prop = gGame.mXObjs.mPools[8].Get(i);
				if (kind != prop->GetKind()) {
					continue;
				}
				Mtx mount, translation, rotation, result;
				switch (kind) {
				case 0x3D:
				case 0x3E:
				case 0x3F:
					PSMTXCopy(toy->GetWorldMtx(joint), mount);
					PSMTXRotRad(rotation, 'y', -HALF_PI);
					PSMTXConcat(mount, rotation, result);
					PSMTXTrans(translation, 0.0f, -2.0f, 8.0f);
					PSMTXConcat(result, translation, result);
					prop->SetBaseMtx(result);
					break;
				case 0x3B: {
					MtxP source = toy->GetWorldMtx(joint);
					PSMTXCopy(source, mount);
					PSMTXTrans(translation, -2.0f, -2.0f, -10.0f);
					PSMTXConcat(source, translation, result);
					prop->SetBaseMtx(result);
					break;
				}
				case 0x21D: {
					joint = toy->FindResNode("body");
					Vec scale;
					PSMTXCopy(toy->GetWorldMtx(joint), toy->mAttachedMtx);
					HSD_MtxGetScale(toy->mAttachedMtx, &scale);
					scale.x = GetStagePlacementScale(prop).x / scale.x;
					scale.y = GetStagePlacementScale(prop).y / scale.y;
					scale.z = GetStagePlacementScale(prop).z / scale.z;
					PSMTXScale(mount, scale.x, scale.y, scale.z);
					PSMTXConcat(toy->mAttachedMtx, mount, result);
					PSMTXTrans(translation, 0.0f, -6.5f, 0.0f);
					PSMTXConcat(result, translation, result);
					PSMTXRotRad(rotation, 'y', PI / 16);
					PSMTXConcat(result, rotation, result);
					prop->SetBaseMtx(result);
					break;
				}
				case 0x21E:
					prop->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					PSMTXCopy(toy->GetWorldMtx(joint), mount);
					PSMTXRotRad(rotation, 'z', HALF_PI);
					PSMTXConcat(mount, rotation, result);
					PSMTXTrans(translation, -1.2f, -4.5f, -0.5f);
					PSMTXConcat(result, translation, result);
					prop->SetBaseMtx(result);
					break;
				default:
					prop->SetBaseMtx(toy->GetWorldMtx(joint));
					break;
				}
				if (toy->mStatus.mFlags & 0x10000) {
					prop->mStatus.mFlags |= 0x10000ULL;
				} else {
					prop->mStatus.mFlags &= ~0x10000ULL;
				}
			}
		}
	}

	for (u32 i = 0; i < 4; ++i) {
		XObjActorToyPiece& piece = toy->mPieces[i];
		switch (piece.mState) {
		case 1:
			if (piece.mObj != NULL) {
				piece.mObj->Release();
			}
			if (piece.mId >= 0) {
				piece.mObj = ReadObj(piece.mId);
			} else {
				piece.mObj = NULL;
			}
			++piece.mState;
		case 2:
			if (piece.mObj != NULL) {
				if (piece.mObj->mStatus.mFlags & 0x0040000000000000ULL) {
					break;
				}
				gGame.mStatus.mFlags |= 0x1000;
			}
			++piece.mState;
		case 3:
			if (piece.mId >= 0 && piece.mObj != NULL) {
				piece.mObj->SetScale(1.0f, 1.0f, 1.0f);
				nw4r::g3d::ResNode joint = toy->FindResNode("body");
				Vec scale;
				Mtx scaling, result, translation, rotation;
				PSMTXCopy(toy->GetWorldMtx(joint), piece.mMount);
				HSD_MtxGetScale(piece.mMount, &scale);
				scale.x = GetStagePlacementScale(piece.mObj).x / scale.x;
				scale.y = GetStagePlacementScale(piece.mObj).y / scale.y;
				scale.z = GetStagePlacementScale(piece.mObj).z / scale.z;
				PSMTXScale(scaling, scale.x, scale.y, scale.z);
				PSMTXConcat(piece.mMount, scaling, result);
				PSMTXTrans(translation, 0.0f, -6.5f, 0.0f);
				PSMTXConcat(result, translation, result);
				PSMTXRotRad(rotation, 'y', PI / 16);
				PSMTXConcat(result, rotation, result);
				piece.mObj->SetBaseMtx(result);
				if (toy->mStatus.mFlags & 0x10000) {
					piece.mObj->mStatus.mFlags |= 0x10000ULL;
				} else {
					piece.mObj->mStatus.mFlags &= ~0x10000ULL;
				}
				piece.mObj->mStatus.mFlags &= ~0x0200000000000000ULL;
			}
			break;
		}
	}
	toy->mFirst.vf160();
	toy->mSecond.vf160();
	if (gGame.mStage == 22) {
		CVec horizontal(toy->mMoveTranslate.x, 0.0f, toy->mMoveTranslate.z);
		if (PSVECMag(&horizontal) > 215.0f) {
			PSVECNormalize(&horizontal, &horizontal);
			horizontal            = horizontal * 215.0f;
			toy->mMoveTranslate.x = horizontal.x;
			toy->mMoveTranslate.z = horizontal.z;
		}
	}
}

extern "C" void HandleToyKind0FCommand(XObjActorToy* object, u32 frames)
{
	s32 count;
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		switch (object->mCmd[1]) {
		case 0:
			object->mKindFlags.mFlags &= ~1;
			break;
		case 1:
			object->mKindFlags.mFlags |= 1;
			break;
		}
		object->mCmd[0] = 0;
		break;
	case 2:
		HideToyDroppedItem(object);
		switch (object->mCmd[1]) {
		case 0x3B:
		case 0x3D:
		case 0x3E:
		case 0x3F:
		case 0x21D:
		case 0x21E:
			for (s32 i = 0; i < 32; i++) {
				XObj* obj = gGame.mXObjs.mPools[8].Get(i);
				if (object->mCmd[1] == obj->GetKind()) {
					obj->mStatus.mFlags |= 0x10000ULL;
					object->mHeldObjects[0] = obj;
					switch (object->mCmd[1]) {
					case 0x3B:
						object->mHideKinds.mFlags |= 8;
						break;
					case 0x3D:
						object->mHideKinds.mFlags |= 1;
						break;
					case 0x3E:
						object->mHideKinds.mFlags |= 2;
						break;
					case 0x3F:
						object->mHideKinds.mFlags |= 4;
						break;
					case 0x21D:
						object->mHideKinds.mFlags |= 0x80;
						break;
					case 0x21E:
						object->mHideKinds.mFlags |= 0x10;
						break;
					}
				}
			}
			break;
		default:
			if (object->mHeldObjects[0] != NULL) {
				object->mHeldObjects[0]->ClearBaseMtx();
				object->mHeldObjects[0] = NULL;
			}
			break;
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		gEventVars.mVars[object->mCmd[1]] = CountSmallSpidersGrabbingPlayer();
		object->mCmd[0]                   = 0;
		break;
	case 4:
		count = 0;
		for (s32 i = 0; i < 0x100; i++) {
			InventoryItem* items = gGame.GetPlayer()->mParams.mInventory.mItems;
			s16 num              = items[i].GetCount();
			s16 id               = items[i].GetId();
			if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
				count++;
			}
		}
		gGame.mEvt.mSelectResult = count;
		object->mCmd[0]          = 0;
		break;
	case 5:
		object->mAttachedId = object->mCmd[1];
		object->mState      = (XObjActor::StateFunc)&XObjActorToy::StatePickUp;
		object->ResetMovementStep();
		object->mCmd[0] = 0;
		break;
	case 6:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	case 7:
		if (object->mCmd[1]) {
			object->mPieces[0].mId    = object->mCmd[2];
			object->mPieces[0].mState = 1;
		} else {
			for (u32 j = 0; j < 4; j++) {
				if (object->mPieces[j].mObj != NULL) {
					object->mPieces[j].mObj->Release();
				}
				object->mPieces[j].mId    = -1;
				object->mPieces[j].mState = 0;
				object->mPieces[j].mObj   = NULL;
			}
		}
		object->mCmd[0] = 0;
		break;
	case 8:
		gEventVars.mVars[object->mCmd[1]] = CountEmergedSmallSpiders();
		object->mCmd[0]                   = 0;
		break;
	}
}

static inline void HideToyDroppedItem(XObjActorToy* toy)
{
	HandleToyKind1FCCommand(toy, 0);
}

void XObjActorToy::ResetActorState()
{
	mActorStatus.mFlags = 0;
	mKindFlags.mFlags   = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	mState              = (StateFunc)&XObjActorToy::StateCarry;
	ResetMovementStep();

	mMoveTranslate.x = PlacementPosition(mCoord).x;
	mMoveTranslate.y = PlacementPosition(mCoord).y;
	mMoveTranslate.z = PlacementPosition(mCoord).z;
	SetPos(mMoveTranslate);
	for (u32 i = 0; i < 4; ++i) {
		mPieces[i].mId    = -1;
		mPieces[i].mState = 0;
		mPieces[i].mObj   = NULL;
	}
	switch (GetKind()) {
	case 0x6F:
	case 0x110:
	case 0x159:
	case 0x1D3:
	case 0x1FB:
	case 0x1FC:
	case 0x25E:
	case 0x26C:
	case 0x26D:
	case 0x485:
		break;
	case 0xF:
		mKindFlags.mFlags |= 1;
		break;
	case 0x117:
		mKindFlags.mFlags &= ~8;
		break;
	case 0x156:
	case 0x157:
	case 0x16C:
		mHeading = PlacementRotation(mCoord).y;
		break;
	}
	mPendingMove.Set(0.0f, 0.0f, 0.0f);
}

void XObjActorToy::ResetForNewGame()
{
	XObjActor::ResetForNewGame();
	mLoad      = 0;
	mShownPart = 0;
	mFirst.ResetForNewGame();
	mSecond.ResetForNewGame();
}

void XObjActorToy::ResetObjectState()
{
	XObjActor::ResetObjectState();
	mLoad      = 0;
	mShownPart = 0;
	mFirst.ResetObjectState();
	mSecond.ResetObjectState();
	_21F8 = -1;
	_21EC = 0.0f;
	_21F0 = 0.0f;
	_21F4 = 0.0f;
	_21FC = 0;
	_2200 = NULL;
	switch (GetKind()) {
	case 0x6F:
		mSfx.Init((WiiPlayerState*)this, sToy6FSounds);
		break;
	case 0x110:
		mSfx.Init((WiiPlayerState*)this, sToy110Sounds);
		break;
	case 0x117:
		mSfx.Init((WiiPlayerState*)this, sToy117Sounds);
		break;
	case 0x159:
		mSfx.Init((WiiPlayerState*)this, sToy159Sounds);
		break;
	case 0x1FB:
		mSfx.Init((WiiPlayerState*)this, sToy1FBSounds);
		break;
	case 0x1FC:
		mSfx.Init((WiiPlayerState*)this, sToy1FCSounds);
		break;
	case 0x25E:
		mSfx.Init((WiiPlayerState*)this, sToy25ESounds);
		break;
	case 0x317:
		mSfx.Init((WiiPlayerState*)this, sToy317Sounds);
		break;
	case 0x26C:
	case 0x485:
		mSfx.Init((WiiPlayerState*)this, sToy26CAnd485Sounds);
		break;
	default:
		mSfx.Init((WiiPlayerState*)this, NULL);
		break;
	}
	_21E0 = 0;
}

BOOL XObjActorToy::LoadFromPlacement(const CoordInfo& info)
{
	if (GetKind() == 0xF) {
		switch (mLoad) {
		case 0:
			if (!XObjActor::LoadFromPlacement(info)) {
				break;
			}
			mLoad++;
		case 1:
			if (!mFirst.LoadFromPlacement(CoordInfo(GetSpawnData(0x2C)))) {
				break;
			}
			mFirst.mStatus.mFlags &= ~0x10000ULL;
			mFirst.SetPos(mMoveTranslate);
			mLoad++;
		case 2:
			if (!mSecond.LoadFromPlacement(CoordInfo(GetSpawnData(0x2D)))) {
				break;
			}
			mSecond.mStatus.mFlags &= ~0x10000ULL;
			mSecond.SetPos(mMoveTranslate);
			mLoad++;
		}
		if (mLoad > 2) {
			mLoad = 0;
			return TRUE;
		}
		return FALSE;
	}
	return XObjActor::LoadFromPlacement(info) != FALSE;
}

s32 XObjActorToy::HandleEventCommand(s32 count, s32* command)
{
	XObjActor::HandleEventCommand(count, command);
	if (count < 1) {
		return 0;
	}
	mCmd[0] = command[0];
	if (count < 2) {
		mCmd[1] = 0;
	} else {
		mCmd[1] = command[1];
	}
	mCmd[2] = command[2];
	mCmd[3] = command[3];
	mCmd[4] = command[4];
	mCmd[5] = command[5];
	mCmd[6] = command[6];
	mCmd[7] = command[7];
	mCmd[8] = command[8];
	mCmd[9] = command[9];
	return 0;
}

void XObjActorToy::OnPushDisplacement(const Vec& displacement)
{
	switch (GetKind()) {
	case 0xF:
		break;
	case 0x6F:
		break;
	case 0x110:
		break;
	case 0x117:
		break;
	case 0x156:
		break;
	case 0x158:
		break;
	case 0x159:
		break;
	case 0x1D3:
		break;
	case 0x1FB:
		break;
	case 0x1FC:
		break;
	case 0x25E:
		break;
	case 0x26C:
		break;
	case 0x26D:
		break;
	case 0x485:
		break;
	case 0x157:
		mKindFlags.mFlags |= 2;
		for (s32 i = 0; i < 15; i++) {
			XObjActorToy* toy = GetToy(i);
			if (toy->GetKind() == 0x156 && (toy->mKindFlags.mFlags & 1)) {
				toy->SetTranslateX(GetPos()->x);
				toy->SetTranslateZ(GetPos()->z);
				if (toy->mAttachedId >= 0 && toy->mAttached != NULL) {
					nw4r::g3d::ResNode joint = toy->FindResNode("sphere2");
					Mtx mount, translation, rotation, result;
					toy->mAttached->SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
					PSMTXCopy(GetWorldMtx(joint), mount);
					PSMTXRotRad(rotation, 'z', 0.0f);
					PSMTXConcat(mount, rotation, result);
					PSMTXTrans(translation, 0.0f, 2.0f, 2.0f);
					PSMTXConcat(result, translation, result);
					toy->mAttached->SetBaseMtx(result);
					if (mStatus.mFlags & 0x10000) {
						toy->mStatus.mFlags |= 0x10000ULL;
					} else {
						toy->mStatus.mFlags &= ~0x10000ULL;
					}
				}
			}
		}
		break;
	}
}

s32 XObjActorToy::vf208(const Vec* pointer, const Vec* motion, u8 over, f32 speed)
{
	if (GetKind() == 0xF && !gGame.GetStatusFlags(0x100) && gGame.GetPlayer()->GetAttachment() != 2
	    && !(gGame.mMotionControlCamera.IsAction(1) && ((CCameraActionArea*)gGame.mCameraActions.mTable[1])->_2D0)
	    && !(gGame.mMotionControlCamera.IsAction(7) && ((CCameraActionMap*)gGame.mCameraActions.mTable[7])->mPointerHeld)
	    && gGame.mStage != 22 && _6DC != 0) {
		f32 absX = __fabsf(motion->x);
		if (absX >= 0.005f) {
			if (absX > 0.05f) {
				absX = 0.05f;
			}
			f32 previous = _21EC;
			if (motion->x >= 0.0f) {
				_21EC = 1.5707964f;
			} else {
				_21EC = -4.712389f;
			}
			f32 strength = DEG2RAD * (400.0f * absX + 4.0f);
			if (previous * _21EC < 0.0f || strength > _21F0) {
				_21F0 = strength;
				SetAnim(1, FALSE, 0.0f, 1.0f, 0.0f, 15.0f);
				if (absX >= 0.05f) {
					_21F8 = 0x410;
					_21FC = 1;
					if (mSfxActors[1].GetHandle() == NULL) {
						CSound3DActor* actor = mSfxActors[1].mActor;
						if (actor != NULL && actor->StartSound(&actor->mHandle, 0x213)) {
							actor->mHandle.SetVolume(1.0f, 0);
						}
					}
				} else if (_21F8 < 0) {
					_21F8 = 0x3F5;
				}
			}
		}
	}
	return XObj::vf208(pointer, motion, over, speed);
}

void XObjActorToy::RunStateHandler(u32 frames)
{
	if (mState != NULL) {
		(this->*mState)(frames);
	}
}

void XObjActorToy::ClearStateHandler()
{
	mState = NULL;
}

void XObjActorToy::UpdateAnimationSounds(f32 frame)
{
	switch (GetKind()) {
	case 0x1FC:
		if (mSfx.mActors[5].GetHandle() != NULL && GetAnimId() != 0x10) {
			mSfx.mActors[5].mActor->mHandle.Stop((s32)(gRefreshRate * 0.0f));
		}
		break;
	case 0x159:
		if (mSfx.mActors[5].GetHandle() != NULL && GetAnimId() != 7) {
			mSfx.mActors[5].mActor->mHandle.Stop((s32)(gRefreshRate * 0.0f));
		}
		break;
	}
	XObjActor::UpdateAnimationSounds(frame);
}
