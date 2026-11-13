#include "game/xobjactor.h"
#include "game/actortoyanimation.h"
#include "game/cjobjset.h"
#include "game/xchr.h"
#include "game/cscrn.h"
#include "game/globals.h"
#include "game/gmath.h"
#include "game/hsd_dobj.h"
#include "game/heldpropmatrix.h"
#include <math.h>
#include <stdlib.h>
extern "C" {
#include <sysdolphin/tobj.h>
}

extern "C" {

// the kinds' own updates
void UpdateToy1FBJointScales(XObjActorToy* object, u32 frames);
void UpdateToy159JointScales(XObjActorToy* object, u32 frames);
void UpdateToy117JointVisibility(XObjActorToy* object, u32 frames);
void UpdateToy6FJointSpin(XObjActorToy* object, u32 frames);
}

extern "C" {
f32 GetToyHorizontalDistance(Vec*, Vec*);
void ApproachHudValue(f32*, f32*, f32);
void HandleToyKind0FCommand(XObjActorToy*, u32 frames);
void HandleToyKind110Command(XObjActorToy*, u32 frames);
void HandleToyKind156Command(XObjActorToy*, u32 frames);
void HandleToyKind16CCommand(XObjActorToy*, u32 frames);
void HandleToyKind159Command(XObjActorToy*, u32 frames);
void HandleToyKind1FCCommand(XObjActorToy*, u32 frames);
void HandleToyScreenShakeCommand(XObjActorToy*, u32 frames);
void HandleToyKind25ECommand(XObjActorToy*, u32 frames);
}

extern "C" HSD_JObj* GetObjectJointByPath(const CJObj*, s32, s32*);
extern "C" f32 GetJointAnimationCurrentFrame(CJObj*);
extern "C" f32 GetJointAnimationEndFrame(CJObj*);

static inline void SetToyBlinkVisibility(XObjActorToy* toy, CJObj& joint)
{
	if ((s32)(toy->mKindCounter & 3) < 2)
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	else
		HSD_JObjSetFlags(joint.mJObj, 0x10);
}

#define BLINK_TOY_JOINT(depth, ...)                                              \
	do {                                                                         \
		joint.Attach(GetObjectJointByPath(toy, depth, (s32[]) { __VA_ARGS__ })); \
		SetToyBlinkVisibility(toy, joint);                                       \
	} while (0)

// NONMATCHING: retail calls CJObj's destructor at every return where the build inlines it, and copies each burst's
// rotation from the six Vecs its .rodata starts with
void XObjActorToy::ProcessPendingEvents(u32 frames)
{
	XObjActorToy* toy = this;
	// a hit is pending (OnHitFromPosition): most kinds set the value their joint effect eases to (_9AC) and how long the hit
	// lasts (_9A4)
	if (toy->mActorStatus.Check(2)) {
		toy->mActorStatus.mFlags &= ~2;
		toy->_9A4 = 5 * frames;
		switch (toy->GetKind()) {
		case 0xF:
			toy->_9AC = 20.0f;
			if (!toy->mActorStatus.Check(0x40) && !toy->mKindFlags.Check(2)) {
				toy->mState = &XObjActorToy::StateHitRecovery;
				ResetActorMovementStep(toy);
				toy->mActorStatus.mFlags |= 0x40;
			}
			break;
		case 0x6F:
			if (!toy->mKindStep)
				toy->mKindStep = 1;
			break;
		case 0x110:
			toy->_9A4 = 30 * frames;
			break;
		case 0x117:
			toy->_9AC = 3.0f;
			break;
		case 0x156:
			toy->_9AC = 2.0f;
			break;
		case 0x158:
			break;
		case 0x159:
			toy->_9AC = 3.0f;
			break;
		case 0x16c:
			break;
		case 0x1D3:
			PlayGameSound(&gGame.mSound, 0x6B + rand() % 12, 255);
			break;
		case 0x1FB:
			toy->_9AC = 2.5f;
			break;
		case 0x1FC:
			if (!toy->mKindStep)
				toy->mKindStep = 1;
			break;
		case 0x25e:
			break;
		case 0x26C:
		case 0x485:
			toy->_9A4 = 30 * frames;
			toy->_9AC = 4.0f;
			break;
		case 0x26D:
			if (!toy->mKindStep)
				toy->mKindStep = 1;
			break;
		}
	}
	// when the hit's time is up the target falls back to 1; _9A8 eases a quarter of the way to _9AC each tick
	if (--toy->_9A4 < 0) {
		toy->_9AC = 1.0f;
		toy->_9A4 = 0;
	}
	f32 step = Abs((toy->_9AC - toy->_9A8) * 0.25f);
	if (step < 0.01f)
		step = 0.01f;
	ApproachHudValue(&toy->_9A8, &toy->_9AC, step);

	CJObj joint;
	switch (toy->GetKind()) {
	case 0x6f:
	case 0x159:
	case 0x1d3:
	case 0x1fb:
	case 0x1fc:
	case 0x25e:
	case 0x26c:
	case 0x26d:
	case 0x485:
		break;

	case 0xF: {
		s32 path[] = { 0, 3, 0, 0, 0, 0, 0, 8 };
		joint.Attach(GetObjectJointByPath(toy, 8, path));
		joint.SetScale(1, 1, toy->_9A8, 0);
		XSound* sound = &gGame.mSound;
		// sound 0xF8 loops while the toy is drawn (0x10000), at the volume for where it stands
		if (!IsGameSoundPlaying(sound, 0xf8) && (toy->mStatus.mFlags & 0x10000))
			PlayGameSound(sound, 0xf8, 255);
		SetGameSfxVolume(sound, 0xf8, GetSoundVolumeAtPosition(sound, &toy->mMoveTranslate, 0), 0);
		if (IsGameSoundPlaying(sound, 0xf8) && !(toy->mStatus.mFlags & 0x10000))
			StopGameSfx(sound, 0xf8);
		if (toy->mFirst.mStatus.mFlags & 0x10000)
			toy->mFirst.UpdateObject(frames);
		if (toy->mSecond.mStatus.mFlags & 0x10000)
			toy->mSecond.UpdateObject(frames);
		s32 anim = toy->mAnimId;
		// mShownPart: 1 mFirst is out while anim 0x13 or 0x3FB plays, 2 mSecond until that anim (0xE, 0x14, 0x3F6 or 0x3FC) ends
		switch (toy->mShownPart) {
		case 0:
			if (anim == 0x13 || anim == 0x3fb) {
				toy->mFirst.SetPos(toy->mMoveTranslate, 0);
				toy->mFirst.SetRotationY(toy->GetRotationY(), 0);
				toy->mFirst.mStatus.mFlags |= 0x10000;
				toy->mFirst.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->mShownPart = 1;
			} else if (anim == 0xe || anim == 0x14 || anim == 0x3f6 || anim == 0x3fc) {
				toy->mSecond.SetPos(toy->mMoveTranslate, 0);
				toy->mSecond.SetRotationY(toy->GetRotationY(), 0);
				toy->mSecond.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->mSecond.mStatus.mFlags |= 0x10000;
				toy->mShownPart = 2;
			}
			break;
		case 1:
			toy->mFirst.SetPos(toy->mMoveTranslate, 0);
			toy->mFirst.SetRotationY(toy->GetRotationY(), 0);
			if (anim != 0x13 && anim != 0x3fb) {
				toy->mFirst.mStatus.mFlags &= ~0x10000ULL;
				toy->mShownPart = 0;
			}
			break;
		case 2: {
			toy->mSecond.SetPos(toy->mMoveTranslate, 0);
			toy->mSecond.SetRotationY(toy->GetRotationY(), 0);
			f32 frame = GetJointAnimationCurrentFrame(toy);
			if (GetJointAnimationEndFrame(toy) - frames <= frame) {
				toy->mSecond.mStatus.mFlags &= ~0x10000ULL;
				toy->mShownPart = 0;
			}
			break;
		}
		}
		break;
	}
	case 0x110: {
		s32 path[] = { 0, 3, 0, 0, 0 };
		joint.Attach(GetObjectJointByPath(toy, 5, path));
		// the joint blinks while a hit lasts
		if ((toy->_9A4 & 3) < 2)
			HSD_JObjClearFlags(joint.mJObj, 0x10);
		else
			HSD_JObjSetFlags(joint.mJObj, 0x10);
		// flag 1: walking to the spot command 1 left in mVelocity; step 1 starts the walk anim, step 2 walks and stops on the spot
		if (!(toy->mKindFlags.mFlags & 1))
			break;
		switch (toy->mKindStep) {
		case 0:
			break;
		case 1:
			++toy->mKindStep;
			toy->mMovePhase = 0;
			toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		// fall through
		case 2: {
			if (toy->mKindStep == 2)
				toy->mMovePhase += 2;
			toy->mMoveSpeed = gActorToyAnimationData.movementSpeeds[toy->mMovePhase % 60];
			if (PSVECDistance(&toy->mMoveTranslate, &CVec(toy->mVelocity)) < 0.1f) {
				toy->mMoveSpeed = 0;
				toy->mKindStep  = 0;
				toy->mKindFlags.mFlags &= ~1;
				toy->SetPos(toy->mVelocity.x, toy->mVelocity.y, toy->mVelocity.z, 0);
				toy->SetMoveDir(&CVec(toy->mMoveTranslate), 0);
			} else {
				toy->SetMoveDir(&CVec(toy->mVelocity), toy->mMoveSpeed);
				CVec destination(toy->mVelocity);
				toy->SetRotationY((f32)atan2(destination.x - toy->mMoveTranslate.x, destination.z - toy->mMoveTranslate.z), 0);
			}
			break;
		}
		}
		break;
	}
	case 0x117: {
		s32 path[] = { 0, 1 };
		joint.Attach(GetObjectJointByPath(toy, 2, path));
		joint.SetScale(toy->_9A8, toy->_9A8, toy->_9A8, 0);
		break;
	}
	case 0x156: {
		s32 first[] = { 0, 2, 4, 0, 2 };
		joint.Attach(GetObjectJointByPath(toy, 5, first));
		joint.SetScale(toy->_9A8, toy->_9A8, toy->_9A8, 0);
		s32 second[] = { 0, 2, 4, 0, 3 };
		joint.Attach(GetObjectJointByPath(toy, 5, second));
		joint.SetScale(toy->_9A8, toy->_9A8, toy->_9A8, 0);
		// rode toy 0x157 down to the floor: it hides
		if ((toy->mKindFlags.mFlags & 1) && toy->mKindStep == 0 && toy->mMoveTranslate.y <= 2.0f) {
			++toy->mKindStep;
			toy->mStatus.mFlags &= ~0x10000ULL;
		}
		break;
	}
	case 0x157:
		// step 0: once pushed off the edge (XObj's falling flag) it moves 14 ahead and tells toy 0x156 (flag 0x10); step 1: down on
		// the floor it hides and toy 0x16C appears in its place
		if (toy->mKindStep == 0 && (toy->mStatus.mFlags & 0x1000000000000000ULL)) {
			toy->mActorStatus.mFlags |= 4;
			toy->mActorStatus.mFlags |= 8;
			toy->mMoveSpeed  = 0.5f;
			toy->mVelocity.x = 14.0f * (f32)sin(toy->mHeading) + toy->mMoveTranslate.x;
			toy->mVelocity.y = toy->mMoveTranslate.y;
			toy->mVelocity.z = 14.0f * (f32)cos(toy->mHeading) + toy->mMoveTranslate.z;
			++toy->mKindStep;
			PlayGameSound(&gGame.mSound, 0x10017, 255);
			for (s32 i = 0; i < 15; ++i) {
				XObjActorToy* other = GetToy(i);
				if (other->GetKind() == 0x156) {
					other->mKindFlags.mFlags |= 0x10;
					break;
				}
			}
		} else if (toy->mKindStep == 1) {
			if (GetToyHorizontalDistance(&toy->mVelocity, &toy->mMoveTranslate) < 1.0f) {
				toy->mActorStatus.mFlags &= ~4;
				toy->mActorStatus.mFlags &= ~8;
				toy->mMoveSpeed = 0;
			}
			if (toy->mMoveTranslate.y <= 2.0f) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				toy->mStatus.mFlags &= ~0x10000ULL;
				for (s32 i = 0; i < 15; ++i) {
					XObjActorToy* other = GetToy(i);
					if (other->GetKind() == 0x16c) {
						other->mStatus.mFlags |= 0x10000;
						other->SetTranslateX(toy->mMoveTranslate.x, 0);
						other->SetTranslateY(toy->mMoveTranslate.y, 0);
						other->SetTranslateZ(toy->mMoveTranslate.z, 0);
						other->mKindFlags.mFlags |= 4;
						PlayGameSound(&gGame.mSound, 0x10016, 255);
						break;
					}
				}
			}
		}
		break;
	case 0x16c:
		// 0 waits for toy 0x157 to land (flag 4), 1 plays anim 1 once, 2 waits for command 1 (flag 8); 10-18 each blink a group of
		// joints for five ticks, then hide it with a burst of particle 0x5B; 19 hides the last group and the toy; 20 is done
		switch (toy->mKindStep) {
		case 0:
			if (toy->mKindFlags.Check(4)) {
				toy->mKindFlags.mFlags &= ~4;
				toy->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				++toy->mKindStep;
			}
			break;
		case 1: {
			f32 frame = GetJointAnimationCurrentFrame(toy);
			if (GetJointAnimationEndFrame(toy) - frames <= frame) {
				toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				++toy->mKindStep;
				toy->mKindCounter = 0;
			}
			break;
		}
		case 2:
			if (toy->mKindFlags.Check(8)) {
				toy->mKindFlags.mFlags &= ~8;
				toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->mKindStep    = 10;
				toy->mKindCounter = 0;
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
			}
			break;
		case 10:
			BLINK_TOY_JOINT(1, 1);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 11:
			BLINK_TOY_JOINT(1, 2);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 12:
			BLINK_TOY_JOINT(1, 3);
			BLINK_TOY_JOINT(1, 0);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 13:
			BLINK_TOY_JOINT(1, 4);
			BLINK_TOY_JOINT(2, 4, 0);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 14:
			BLINK_TOY_JOINT(1, 5);
			BLINK_TOY_JOINT(2, 5, 0);
			BLINK_TOY_JOINT(2, 5, 1);
			BLINK_TOY_JOINT(2, 5, 2);
			BLINK_TOY_JOINT(3, 5, 0, 0);
			BLINK_TOY_JOINT(4, 5, 0, 0, 0);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 15:
			BLINK_TOY_JOINT(1, 6);
			BLINK_TOY_JOINT(2, 6, 0);
			BLINK_TOY_JOINT(2, 6, 1);
			BLINK_TOY_JOINT(3, 6, 0, 0);
			BLINK_TOY_JOINT(4, 6, 0, 0, 0);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 16:
			BLINK_TOY_JOINT(1, 7);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 17:
			BLINK_TOY_JOINT(1, 8);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 18:
			BLINK_TOY_JOINT(1, 9);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				PlayGameSound(&gGame.mSound, 0xe0007, 255);
				Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
				gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
				return;
			}
			break;
		case 19:
			BLINK_TOY_JOINT(1, 10);
			BLINK_TOY_JOINT(2, 10, 0);
			BLINK_TOY_JOINT(2, 10, 1);
			BLINK_TOY_JOINT(2, 10, 2);
			BLINK_TOY_JOINT(2, 10, 3);
			BLINK_TOY_JOINT(2, 10, 4);
			++toy->mKindCounter;
			if ((s32)toy->mKindCounter > 4) {
				++toy->mKindStep;
				toy->mKindCounter = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
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
	case 0x1d3:
	case 0x1fb:
	case 0x26d:
		break;

	case 0xF:
		HandleToyKind0FCommand(toy, frames);
		break;
	case 0x6F:
		// command 1: arg 0 hides joint 0-3-2-0-0-0, arg 1 shows it again (UpdateToy6FJointSpin)
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
		// command 1: arg 8 shows joint 0-0-3, anything else hides it (UpdateToy117JointVisibility)
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
			if (toy->mKindFlags.mFlags & 2)
				gEventVars.mVars[toy->mCmd[1]] = 1;
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
		HandleToyKind1FCCommand(toy, frames);
		break;
	case 0x26C:
	case 0x485:
		HandleToyScreenShakeCommand(toy, frames);
		break;
	case 0x25E:
		HandleToyKind25ECommand(toy, frames);
		break;
	}
}

#undef BLINK_TOY_JOINT

static inline void MountAnimatedToyProp(XObjActorToy* toy, CJObj& joint, f32 angle, f32 x, f32 y, f32 z)
{
	toy->mAttached->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	Mtx mount, rot, trans, result;
	PSMTXCopy(joint.GetMtx(), mount);
	RotateHeldProp(mount, result, rot, 'z', angle);
	OffsetHeldProp(result, trans, x, y, z);
	SetHeldPropMatrix(toy->mAttached, result);
}

void XObjActorToy::StateHitRecovery(u32 frames)
{
	if (mAttachedId >= 0 && mAttached) {
		CJObj joint;
		switch (GetKind()) {
		case 0xf: {
			s32 path[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 9));
			MountAnimatedToyProp(this, joint, HALF_PI, -1.2f, -5.5f, -0.5f);
			break;
		}
		case 0x156: {
			s32 path[4] = { 0, 2, 4, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 4));
			Vec pos = joint.GetPos();
			mAttached->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			break;
		}
		case 0x25e: {
			s32 path[2] = { 0, 4 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			Vec pos = joint.GetPos();
			mAttached->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			break;
		}
		}
		if (mStatus.mFlags & 0x10000)
			mAttached->mStatus.mFlags |= 0x10000ULL;
		else
			mAttached->mStatus.mFlags &= ~0x10000ULL;
	}
	// 0 starts a hop, heading away from the player; 1 waits until it lands; 2 slows to a stop and puts it back on mHomePos;
	// 4 returns to StateCarry; nothing enters 3, a walk back home
	switch (mKindStep) {
	case 0: {
		Vec& player = gGame.GetPlayer()->mMoveTranslate;
		mHeading    = PI + (f32)atan2(player.x - mMoveTranslate.x, player.z - mMoveTranslate.z);
		mHomePos    = mMoveTranslate;
		// in the air, falling under gravity, moved by UpdateGroundMovement
		mActorStatus.mFlags |= 8;
		mActorStatus.mFlags |= 0x10;
		mActorStatus.mFlags |= 4;
		if (mHitStrength >= 10.0f) {
			mMoveSpeed = 0;
			mFallSpeed = 6.8f;
		} else {
			mMoveSpeed = 0;
			mFallSpeed = 4.8f;
		}
		mKindCounter = 0;
		++mKindStep;
		break;
	}
	case 1:
		++mKindCounter;
		if (mActorStatus.Check(0x20)) {
			mFallSpeed = 0;
			++mKindStep;
		}
		break;
	case 2:
		mMoveSpeed -= 0.04f * frames;
		if (mMoveSpeed <= 0) {
			mMoveSpeed = 0;
			mMoveTranslate.Set(mHomePos);
			mKindStep = 4;
		}
		break;
	case 3: {
		mMoveSpeed = 0.5f;
		mHeading   = atan2(mHomePos.x - mMoveTranslate.x, mHomePos.z - mMoveTranslate.z);
		mFacing    = mHeading;
		CVec home(mHomePos);
		f32 dx = home.x - mMoveTranslate.x, dz = home.z - mMoveTranslate.z;
		if (sqrtf(dz * dz + (dx * dx + 0.0f)) < 1.0f) {
			mMoveTranslate.Set(mHomePos);
			mMoveSpeed = 0;
			++mKindStep;
		}
		break;
	}
	case 4:
		mActorStatus.mFlags &= ~4;
		mActorStatus.mFlags &= ~0x40;
		mState = &XObjActorToy::StateCarry;
		ResetActorMovementStep(this);
		break;
	}
}

extern "C" CVec GetStagePlacementScale(XObj* obj);
extern "C" void HSD_MtxGetScale(Mtx matrix, Vec* scale);

static inline void FinishToyMount(XObjActorToy* toy)
{
	if (toy->mStatus.mFlags & 0x10000)
		toy->mAttached->mStatus.mFlags |= 0x10000ULL;
	else
		toy->mAttached->mStatus.mFlags &= ~0x10000ULL;
	toy->mAttached->mStatus.mFlags &= ~0x0200000000000000ULL;
}

void XObjActorToy::StatePickUp(u32 frames)
{
	// 0 waits until Tao and Papa are free and reads mAttachedId in, 1 waits for the read, 2 mounts it and goes to StateCarry
	switch (mKindStep) {
	case 0: {
		XObjActorTao* tao = (XObjActorTao*)gGame.mXObjs.mPools[14].First();
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree())
			return;
		XObjActorPapa* papa = (XObjActorPapa*)gGame.mXObjs.mPools[13].First();
		if ((papa->mStatus.mFlags & 1) && !papa->IsFree())
			return;
		if (mAttached)
			mAttached->Release();
		if (mAttachedId >= 0)
			mAttached = ReadObj(mAttachedId);
		else
			mAttached = NULL;
		mActorStatus.mFlags |= 0x2000;
		++mKindStep;
	}
	case 1:
		if (mAttached) {
			if (mAttached->mStatus.mFlags & 0x0040000000000000ULL)
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		++mKindStep;
	case 2:
		if (mAttachedId >= 0 && mAttached) {
			mAttached->SetScale(1, 1, 1, 0);
			CJObj joint;
			Vec position;
			switch (GetKind()) {
			case 0xf: {
				s32 path[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
				joint.Attach(FindJointByPath(mJObj, path, 9));
				MountAnimatedToyProp(this, joint, HALF_PI, -1.2f, -5.5f, -0.5f);
				break;
			}
			case 0x156: {
				s32 path[4] = { 0, 2, 4, 0 };
				joint.Attach(FindJointByPath(mJObj, path, 4));
				position = joint.GetPos();
				mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
				MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
				break;
			}
			case 0x159: {
				s32 path[2] = { 0, 7 };
				joint.Attach(FindJointByPath(mJObj, path, 2));
				position = joint.GetPos();
				mAttached->SetPos(position.x, 12.0f + position.y, position.z, 0);
				PSMTXCopy(joint.GetMtx(), mAttachedMtx);
				HSD_MtxGetScale(mAttachedMtx, &position);
				position.x = GetStagePlacementScale(mAttached).x / position.x;
				position.y = GetStagePlacementScale(mAttached).y / position.y;
				position.z = GetStagePlacementScale(mAttached).z / position.z;
				Mtx scale, translation, rotation, result;
				PSMTXScale(scale, position.x, position.y, position.z);
				PSMTXConcat(mAttachedMtx, scale, result);
				RotateHeldProp(result, result, rotation, 'y', -PI);
				RotateHeldProp(result, result, rotation, 'x', -HALF_PI);
				OffsetHeldProp(result, translation, 5, 12, 2);
				SetHeldPropMatrix(mAttached, result);
				break;
			}
			case 0x25e: {
				s32 path[2] = { 0, 4 };
				joint.Attach(FindJointByPath(mJObj, path, 2));
				position = joint.GetPos();
				mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
				MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
				break;
			}
			}
			FinishToyMount(this);
		}
		mActorStatus.mFlags &= ~0x2000;
		mState = &XObjActorToy::StateCarry;
		ResetActorMovementStep(this);
		break;
	}
}

void XObjActorToy::StateCarry(u32 frames)
{
	Vec position;
	switch (GetKind()) {
	case 0x6f:
	case 0x110:
	case 0x117:
	case 0x158:
	case 0x16c:
	case 0x1d3:
	case 0x1fb:
	case 0x1fc:
	case 0x26c:
	case 0x26d:
	case 0x485:
		break;
	case 0xf:
		if (mAttachedId >= 0 && mAttached) {
			CJObj joint;
			s32 path[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 9));
			MountAnimatedToyProp(this, joint, HALF_PI, -1.2f, -5.5f, -0.5f);
			FinishToyMount(this);
		}
		break;
	case 0x156:
		if (mAttachedId >= 0 && mAttached) {
			CJObj joint;
			s32 path[4] = { 0, 2, 4, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 4));
			position = joint.GetPos();
			mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			FinishToyMount(this);
		}
		break;
	case 0x159:
		if (mAttachedId >= 0 && mAttached) {
			CJObj joint;
			s32 path[2] = { 0, 7 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			position = joint.GetPos();
			mAttached->SetPos(position.x, 12.0f + position.y, position.z, 0);
			PSMTXCopy(joint.GetMtx(), mAttachedMtx);
			HSD_MtxGetScale(mAttachedMtx, &position);
			position.x = GetStagePlacementScale(mAttached).x / position.x;
			position.y = GetStagePlacementScale(mAttached).y / position.y;
			position.z = GetStagePlacementScale(mAttached).z / position.z;
			Mtx scale, translation, rotation, result;
			PSMTXScale(scale, position.x, position.y, position.z);
			PSMTXConcat(mAttachedMtx, scale, result);
			RotateHeldProp(result, result, rotation, 'y', -PI);
			RotateHeldProp(result, result, rotation, 'x', -HALF_PI);
			OffsetHeldProp(result, translation, 5, 12, 2);
			SetHeldPropMatrix(mAttached, result);
			FinishToyMount(this);
		}
		break;
	case 0x25e:
		if (mAttachedId >= 0 && mAttached) {
			CJObj joint;
			s32 path[2] = { 0, 4 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			position = joint.GetPos();
			mAttached->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 1.1f, 1.4f);
			FinishToyMount(this);
		}
		break;
	}
}

static inline void MountToyPiece(CJObj& joint, XObj* const& prop, Mtx mount)
{
	Vec scale;
	Mtx scaling, translation, rotation, result;
	PSMTXCopy(joint.GetMtx(), mount);
	HSD_MtxGetScale(mount, &scale);
	scale.x = GetStagePlacementScale(prop).x / scale.x;
	scale.y = GetStagePlacementScale(prop).y / scale.y;
	scale.z = GetStagePlacementScale(prop).z / scale.z;
	PSMTXScale(scaling, scale.x, scale.y, scale.z);
	PSMTXConcat(mount, scaling, result);
	OffsetHeldProp(result, translation, 0, -6.5f, 0);
	RotateHeldProp(result, result, rotation, 'y', PI / 16);
	SetHeldPropMatrix(prop, result);
}

extern "C" void UpdateToy0FCarriedObjects(XObjActorToy* toy, u32 frames)
{
	CJObj joint;
	if (toy->mKindFlags.Check(1)) {
		Vec& player  = gGame.GetPlayer()->mMoveTranslate;
		toy->mFacing = atan2(player.x - toy->mMoveTranslate.x, player.z - toy->mMoveTranslate.z);
		toy->SetRotateY(toy->mFacing, 0);
	}
	toy->mMoveTranslate.x += .1f * (toy->mPendingMove.x * frames);
	toy->mMoveTranslate.z += .1f * (toy->mPendingMove.z * frames);
	toy->mPendingMove.Set(0, 0, 0);
	// one of the pool 8 items: every mHideKinds bit but 0x40, which is toy 0x25E's 0x341
	if (toy->mHideKinds.mFlags & 0x9f) {
		s32 hand[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
		joint.Attach(FindJointByPath(toy->mJObj, hand, 9));
		CVec position;
		position = joint.GetPos();
		s32 kind = -1;
		if (toy->mHideKinds.Check(1))
			kind = 0x3d;
		else if (toy->mHideKinds.Check(2))
			kind = 0x3e;
		else if (toy->mHideKinds.Check(4))
			kind = 0x3f;
		else if (toy->mHideKinds.Check(8))
			kind = 0x3b;
		else if (toy->mHideKinds.Check(0x80))
			kind = 0x21d;
		else if (toy->mHideKinds.Check(0x10))
			kind = 0x21e;
		if (kind >= 0) {
			for (s32 i = 0; i < 64; ++i) {
				XObj* prop = gGame.mXObjs.mPools[8].Get(i);
				if (kind != prop->GetKind())
					continue;
				Mtx mount, translation, rotation, result;
				switch (kind) {
				case 0x3b: {
					MtxPtr source = joint.GetMtx();
					PSMTXCopy(source, mount);
					PSMTXTrans(translation, -2, -2, -10);
					PSMTXConcat(source, translation, result);
					SetHeldPropMatrix(prop, result);
					break;
				}
				case 0x3d:
				case 0x3e:
				case 0x3f:
					PSMTXCopy(joint.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'y', -HALF_PI);
					OffsetHeldProp(result, translation, 0, -2, 8);
					SetHeldPropMatrix(prop, result);
					break;
				case 0x21d: {
					s32 path[7] = { 0, 3, 0, 0, 0, 0, 0 };
					joint.Attach(FindJointByPath(toy->mJObj, path, 7));
					MountToyPiece(joint, prop, toy->mAttachedMtx);
					break;
				}
				case 0x21e:
					prop->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
					PSMTXCopy(joint.GetMtx(), mount);
					RotateHeldProp(mount, result, rotation, 'z', HALF_PI);
					OffsetHeldProp(result, translation, -1.2f, -4.5f, -.5f);
					SetHeldPropMatrix(prop, result);
					break;
				default:
					SetHeldPropMatrix(prop, joint.GetMtx());
					break;
				}
				if (toy->mStatus.mFlags & 0x10000)
					prop->mStatus.mFlags |= 0x10000ULL;
				else
					prop->mStatus.mFlags &= ~0x10000ULL;
			}
		}
	}
	for (u32 i = 0; i < 4; ++i) {
		XObjActorToyPiece& piece = toy->mPieces[i];
		// 1 reads the piece in by id, 2 waits for the read, 3 keeps it mounted on joint 0-3-0-0-0-0-0
		switch (piece.mState) {
		case 1:
			if (piece.mObj)
				piece.mObj->Release();
			if (piece.mId >= 0)
				piece.mObj = ReadObj(piece.mId);
			else
				piece.mObj = NULL;
			++piece.mState;
		case 2:
			if (piece.mObj) {
				if (piece.mObj->mStatus.mFlags & 0x0040000000000000ULL)
					break;
				gGame.mStatus.mFlags |= 0x1000;
			}
			++piece.mState;
		case 3:
			if (piece.mId >= 0 && piece.mObj) {
				piece.mObj->SetScale(1, 1, 1, 0);
				CJObj mount;
				s32 path[7] = { 0, 3, 0, 0, 0, 0, 0 };
				mount.Attach(FindJointByPath(toy->mJObj, path, 7));
				MountToyPiece(mount, piece.mObj, piece.mMount);
				if (toy->mStatus.mFlags & 0x10000)
					piece.mObj->mStatus.mFlags |= 0x10000ULL;
				else
					piece.mObj->mStatus.mFlags &= ~0x10000ULL;
				piece.mObj->mStatus.mFlags &= ~0x0200000000000000ULL;
			}
			break;
		}
	}
}

void UpdateToy1FBJointScales(XObjActorToy* object, u32)
{
	CJObj joint;
	s32 first[] = { 0, 0, 0, 0, 0, 0, 0 };
	joint.Attach(GetJointFromRoot(object->mJObj, 7, first));
	joint.SetScale(object->_9A8, object->_9A8, object->_9A8, 0);
	s32 second[] = { 0, 0, 1, 0, 0, 0, 0 };
	joint.Attach(GetJointFromRoot(object->mJObj, 7, second));
	joint.SetScale(object->_9A8, object->_9A8, object->_9A8, 0);
}

void UpdateToy159JointScales(XObjActorToy* object, u32)
{
	CJObj joint;
	s32 first[] = { 0, 2 };
	joint.Attach(GetJointFromRoot(object->mJObj, 2, first));
	joint.SetScale(object->_9A8, object->_9A8, object->_9A8, 0);
	s32 second[] = { 0, 7 };
	joint.Attach(GetJointFromRoot(object->mJObj, 2, second));
	joint.SetScale(object->_9A8, object->_9A8, object->_9A8, 0);
}

void UpdateToy117JointVisibility(XObjActorToy* object, u32)
{
	CJObj joint;
	if (object->mKindFlags.mFlags & 8) {
		s32 path[] = { 0, 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 3, path));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
	} else {
		s32 path[] = { 0, 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 3, path));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
	}
}

void UpdateToy6FJointSpin(XObjActorToy* object, u32)
{
	CJObj joint;
	if (object->mKindFlags.mFlags & 1) {
		s32 path[] = { 0, 3, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
	} else {
		s32 path[] = { 0, 3, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
	}

	static f32 sAngle = 0.0f;
	switch (object->mKindStep) {
	case 0:
		break;
	case 1: {
		s32 path[] = { 0, 3, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path));
		sAngle += 0.4f;
		if (sAngle >= TWO_PI) {
			object->mKindStep = 0;
			sAngle            = 0.0f;
		}
		joint.SetRotationZ(sAngle, 0);
		break;
	}
	}
}

/**
 * @note Address: 0x80119774
 * @note Size: 0x4B0
 */
// toy 0x25E, keeps the 0x341 objects it holds in its hand
// NONMATCHING: regalloc
extern "C" void UpdateToy25EHeldObjects(XObjActorToy* object, u32 frames)
{
	CJObj joint;
	if (object->mHideKinds.mFlags & 0x40) {
		s32 path[1] = { 0 };
		CVec pos;
		HSD_JObj* hand = GetJointByPath(object->mJObj, path, 1);
		joint.Attach(hand);
		pos    = joint.GetPos();
		s32 id = -1;
		if (object->mHideKinds.mFlags & 0x40) {
			id = 0x341;
		}
		if (id >= 0) {
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (id == obj->GetKind()) {
					HSD_JObjCopyMtx(obj->mJObj, HSD_JObjGetMtxPtr(joint.mJObj));
					if (object->mStatus.mFlags & 0x10000) {
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
 * @note Address: 0x80119C24
 * @note Size: 0x42C
 */
// toy 0x26D, spins a joint once round
extern "C" void UpdateToyJointSpin(XObjActorToy* object, u32 frames)
{
	CJObj joint;
	switch (object->mKindStep) {
	case 0:
		break;
	case 1:
		object->mKindStep++;
		object->mFacing = 0.0f;
		break;
	case 2: {
		object->mFacing += 0.2f;
		s32 path[2] = { 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 2, path));
		if (object->mFacing >= TWO_PI) {
			object->mKindStep = 0;
			object->mFacing   = 0.0f;
		}
		joint.SetRotationY(object->mFacing + HSD_JObjGetRotationY(object->mJObj), 0);
		break;
	}
	}
}

/**
 * @note Address: 0x8011A050
 * @note Size: 0x340
 */
// toys 0x26C and 0x485
extern "C" void UpdateToyJointVerticalOffset(XObjActorToy* object, u32 frames)
{
	CJObj joint;
	switch (object->mKindStep) {
	case 0:
		if (object->_9A4 > 0) {
			object->mKindStep++;
		}
		break;
	case 1: {
		s32 path[4] = { 0, 2, 4, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path));
		joint.SetTranslateY(-object->_9A8, 0);
		if (1.0f == object->_9A8) {
			object->mKindStep = 0;
		}
		break;
	}
	}
}

/**
 * @note Address: 0x8011A390
 * @note Size: 0x1338
 */
// toy 0x1FC, pushes two of its joints out and back
// NONMATCHING: the paths sit at other .rodata offsets, the CJObj ctor is inlined
extern "C" void UpdateToyJointExtension(XObjActorToy* object, u32 frames)
{
	CJObj joint;
	static f32 sTime = 0.0f;
	static Vec sPos0;
	static Vec sPos1;
	switch (object->mKindStep) {
	case 1: {
		s32 path0[4] = { 0, 2, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		sPos0.x      = HSD_JObjGetTranslationX(joint.mJObj);
		sPos0.y      = HSD_JObjGetTranslationY(joint.mJObj);
		sPos0.z      = HSD_JObjGetTranslationZ(joint.mJObj);
		s32 path1[5] = { 0, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		sPos1.x = HSD_JObjGetTranslationX(joint.mJObj);
		sPos1.y = HSD_JObjGetTranslationY(joint.mJObj);
		sPos1.z = HSD_JObjGetTranslationZ(joint.mJObj);
		object->mKindStep++;
		break;
	}
	case 2: {
		sTime += 0.16f;
		if (sTime >= 1.0f) {
			object->mKindStep++;
		}
		s32 path0[4] = { 0, 2, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		joint.SetTranslateZ(sPos0.z + 16.0f * sTime, 0);
		s32 path1[5] = { 0, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		joint.SetTranslateZ(sPos1.z + 16.0f * sTime, 0);
		break;
	}
	case 3: {
		s32 path0[4] = { 0, 2, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		joint.SetTranslateZ(sPos0.z + 16.0f * sTime, 0);
		s32 path1[5] = { 0, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		joint.SetTranslateZ(sPos1.z + 16.0f * sTime, 0);
		sTime -= 0.16f;
		if (sTime <= 0.0f) {
			object->mKindStep = 0;
			s32 path2[4]      = { 0, 2, 0, 1 };
			joint.Attach(GetJointFromRoot(object->mJObj, 4, path2));
			joint.SetTranslateX(sPos0.x, 0);
			joint.SetTranslateY(sPos0.y, 0);
			joint.SetTranslateZ(sPos0.z, 0);
			s32 path3[5] = { 0, 2, 0, 0, 0 };
			joint.Attach(GetJointFromRoot(object->mJObj, 5, path3));
			joint.SetTranslateX(sPos1.x, 0);
			joint.SetTranslateY(sPos1.y, 0);
			joint.SetTranslateZ(sPos1.z, 0);
			sTime = 0.0f;
		}
		break;
	}
	}
	if (object->mKindFlags.mFlags & 1) {
		s32 path0[4] = { 0, 0, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
		s32 path1[5] = { 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
		s32 path2[6] = { 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path2));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
		s32 path3[7] = { 0, 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 7, path3));
		HSD_JObjSetFlags(joint.mJObj, HIDDEN);
	} else {
		s32 path0[4] = { 0, 0, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
		s32 path1[5] = { 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
		s32 path2[6] = { 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path2));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
		s32 path3[7] = { 0, 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 7, path3));
		HSD_JObjClearFlags(joint.mJObj, HIDDEN);
	}
}

/**
 * @note Address: 0x8011E3A4
 * @note Size: 0x2E4
 */
// hides the item the toy dropped, the mHideKinds bits say which
extern "C" void HideToyDroppedItem(XObjActorToy* object)
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
		for (s32 i = 0; i < 64; i++) {
			XObj* obj = gGame.mXObjs.mPools[8].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	case 0x341:
		for (s32 i = 0; i < 288; i++) {
			XObj* obj = gGame.mXObjs.mPools[22].Get(i);
			if (id == obj->GetKind()) {
				obj->mStatus.mFlags &= ~0x10000ULL;
			}
		}
		break;
	}
}

/**
 * @note Address: 0x8011E688
 * @note Size: 0x254
 */
extern "C" void HandleToyKind25ECommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		ResetActorMovementStep(object);
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
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (object->mCmd[1] == obj->GetKind()) {
					object->mHeldObjects[0] = obj;
					obj->mStatus.mFlags |= 0x10000ULL;
					switch (object->mCmd[1]) {
					case 0x341:
						object->mHideKinds.mFlags |= 0x40;
						break;
					}
				}
			}
			break;
		default:
			if (object->mHeldObjects[0] != NULL) {
				HSD_JObjClearMtxFlags(object->mHeldObjects[0]->mJObj);
				object->mHeldObjects[0] = NULL;
			}
			break;
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011E8DC
 * @note Size: 0xD0
 */
extern "C" void HandleToyScreenShakeCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 1:
		StartScreenShake(&gGame.mQuake, object->mCmd[1], object->mCmd[2], object->mCmd[3], 0.0f, 0.0f, 0.0f, object->mCmd[4] / 100.0f);
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011E9AC
 * @note Size: 0x140
 */
extern "C" void HandleToyKind1FCCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		object->mCmd[0] = 0;
		break;
	case 1:
		StartScreenShake(&gGame.mQuake, object->mCmd[1], object->mCmd[2], object->mCmd[3], 0.0f, 0.0f, 0.0f, object->mCmd[4] / 100.0f);
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

/**
 * @note Address: 0x8011EAEC
 * @note Size: 0xAC
 */
extern "C" void HandleToyKind159Command(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		object->mAttachedId = object->mCmd[1];
		object->mState      = &XObjActorToy::StatePickUp;
		ResetActorMovementStep(object);
		object->mCmd[0] = 0;
		break;
	case 2:
		object->mAttachedId = -1;
		if (object->mAttached != NULL) {
			object->mAttached->Release();
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011EB98
 * @note Size: 0x80
 */
extern "C" void HandleToyKind16CCommand(XObjActorToy* object, u32 frames)
{
	switch (object->mCmd[0]) {
	case 0:
		break;
	// 1 starts its blinking away (step 2 in ProcessPendingEvents), 2 tells the script whether it is still drawn
	case 1:
		object->mKindFlags.mFlags |= 8;
		object->mCmd[0] = 0;
		break;
	case 2:
		if (object->mStatus.mFlags & 0x10000) {
			gEventVars.mVars[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	}
}

/**
 * @note Address: 0x8011EC18
 * @note Size: 0x130
 */
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
		object->mState      = &XObjActorToy::StatePickUp;
		ResetActorMovementStep(object);
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

/**
 * @note Address: 0x8011ED48
 * @note Size: 0x144
 */
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

/**
 * @note Address: 0x8011EE8C
 * @note Size: 0x410
 */
extern "C" void HandleToyKind0FCommand(XObjActorToy* object, u32 frames)
{
	s32 count;
	switch (object->mCmd[0]) {
	case 0:
		break;
	case 1:
		switch (object->mCmd[1]) {
		// flag 1 keeps it turned to the player (UpdateToy0FCarriedObjects)
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
			for (s32 i = 0; i < 64; i++) {
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
				HSD_JObjClearMtxFlags(object->mHeldObjects[0]->mJObj);
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
		object->mState      = &XObjActorToy::StatePickUp;
		ResetActorMovementStep(object);
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

/**
 * @note Address: 0x8011F29C
 * @note Size: 0x220
 */
// NONMATCHING: regalloc
void XObjActorToy::UpdateAfterAnim(u32 frames)
{
	UpdateAfterAnim__9XObjActorFUl(this, frames);
	switch (GetKind()) {
	case 0xF:
		UpdateToy0FCarriedObjects(this, frames);
		break;
	case 0x6F:
		UpdateToy6FJointSpin(this, frames);
		break;
	case 0x110:
		break;
	case 0x117:
		UpdateToy117JointVisibility(this, frames);
		break;
	case 0x156:
		// flag 1: it rides on toy 0x157 (OnPushDisplacement)
		if (mKindFlags.mFlags & 1) {
			for (s32 i = 0; i < 15; i++) {
				if (GetToy(i)->GetKind() == 0x157) {
					Vec pos          = GetToy(i)->mMoveTranslate;
					mMoveTranslate.x = pos.x;
					mMoveTranslate.y = pos.y;
					mMoveTranslate.z = pos.z;
				}
			}
		}
		break;
	case 0x158:
		break;
	case 0x159:
		UpdateToy159JointScales(this, frames);
		break;
	case 0x16C:
		break;
	case 0x1D3:
		break;
	case 0x1FB:
		UpdateToy1FBJointScales(this, frames);
		break;
	case 0x1FC:
		UpdateToyJointExtension(this, frames);
		break;
	case 0x26C:
	case 0x485:
		UpdateToyJointVerticalOffset(this, frames);
		break;
	case 0x26D:
		UpdateToyJointSpin(this, frames);
		break;
	case 0x25E:
		UpdateToy25EHeldObjects(this, frames);
		break;
	}
}

/**
 * @note Address: 0x801219B0
 * @note Size: 0x104
 */
extern "C" f32 GetToyHorizontalDistance(Vec* first, Vec* second)
{
	f32 dx = first->x - second->x;
	f32 dy = 0.0f;
	f32 dz = first->z - second->z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

extern "C" {
extern Vec gToyPlacementPosition, gToyPlacementRotation;
}

static inline Vec ToyPlacementPosition(const CoordInfo& place)
{
	Vec pos = gToyPlacementPosition;
	pos.x   = place.mPlacement->mPos[0] / 100.0f;
	pos.y   = place.mPlacement->mPos[1] / 100.0f;
	pos.z   = place.mPlacement->mPos[2] / 100.0f;
	return Vec(pos);
}

static inline Vec ToyPlacementRotation(const CoordInfo& place)
{
	Vec rot = gToyPlacementRotation;
	rot.x   = DEG2RAD * place.mPlacement->mRot[0];
	rot.y   = DEG2RAD * place.mPlacement->mRot[1];
	rot.z   = DEG2RAD * place.mPlacement->mRot[2];
	return Vec(rot);
}

/**
 * @note Address: 0x80121B08
 * @note Size: 0x5A0
 */
void XObjActorToy::ResetActorState()
{
	mActorStatus.mFlags = 0;
	mKindFlags.mFlags   = 0;
	mHideKinds.mFlags   = 0;
	mHeading            = 0.0f;
	mMoveSpeed          = 0.0f;
	mFacing             = 0.0f;
	mState              = &XObjActorToy::StateCarry;
	ResetActorMovementStep(this);

	mMoveTranslate.x = ToyPlacementPosition(mCoord).x;
	mMoveTranslate.y = ToyPlacementPosition(mCoord).y;
	mMoveTranslate.z = ToyPlacementPosition(mCoord).z;
	SetPos(mMoveTranslate, 0);
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
		mHeading = ToyPlacementRotation(mCoord).y;
		break;
	case 0x157:
		mHeading = ToyPlacementRotation(mCoord).y;
		break;
	case 0x16c:
		mHeading = ToyPlacementRotation(mCoord).y;
		break;
	}
	mPendingMove.Set(0.0f, 0.0f, 0.0f);
	_1FA4 = 0;
}
