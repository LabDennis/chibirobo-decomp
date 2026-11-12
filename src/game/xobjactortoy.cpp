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
extern XObjActorToy::StateFunc gToyInitialState;
extern XObjActorToy::StateFunc gToyHitRecoveryState;
extern XObjActorToy::StateFunc gToy0FPickUpState;
extern XObjActorToy::StateFunc gToy156PickUpState;
extern XObjActorToy::StateFunc gToy159PickUpState;
extern XObjActorToy::StateFunc gToy25EPickUpState;
extern XObjActorToy::StateFunc gToyPostHitState;
extern XObjActorToy::StateFunc gToyPostPickUpState;
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

static inline void EmitToyBurst(XObjActorToy* toy)
{
	PlayGameSound(&gGame.mSound, 0xe0007, 255);
	Vec scale = { 1, 1, 1 }, rotation = { 0, 0, 0 };
	gGame.mParticle.CreateSRT(0x5b, 1, toy->mMoveTranslate, scale, rotation, TRUE);
}

static inline void SetToyBlinkVisibility(XObjActorToy* toy, CJObj& joint)
{
	if ((s32)(toy->_994 & 3) < 2)
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	else
		HSD_JObjSetFlags(joint.mJObj, 0x10);
}

#define BLINK_TOY_JOINT(depth, ...)                                              \
	do {                                                                         \
		joint.Attach(GetObjectJointByPath(toy, depth, (s32[]) { __VA_ARGS__ })); \
		SetToyBlinkVisibility(toy, joint);                                       \
	} while (0)

// NONMATCHING: retail calls CJObj's destructor at every return where the build inlines it, gives each burst a scale
// of its own and copies the burst's rotation from the six Vecs its .rodata starts with
void XObjActorToy::ProcessPendingEvents(u32 frames)
{
	XObjActorToy* toy = this;
	if (toy->_998.Check(2)) {
		toy->_998.mFlags &= ~2;
		toy->_9A4 = 5 * frames;
		switch (toy->GetKind()) {
		case 0xF:
			toy->_9AC = 20.0f;
			if (!toy->_998.Check(0x40) && !toy->_2098.Check(2)) {
				toy->mState = gToyHitRecoveryState;
				ResetActorMovementStep(toy);
				toy->_998.mFlags |= 0x40;
			}
			break;
		case 0x6F:
			if (!toy->_990)
				toy->_990 = 1;
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
			if (!toy->_990)
				toy->_990 = 1;
			break;
		case 0x25e:
			break;
		case 0x26C:
		case 0x485:
			toy->_9A4 = 30 * frames;
			toy->_9AC = 4.0f;
			break;
		case 0x26D:
			if (!toy->_990)
				toy->_990 = 1;
			break;
		}
	}
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
		switch (toy->_1F90) {
		case 0:
			if (anim == 0x13 || anim == 0x3fb) {
				toy->mFirst.SetPos(toy->mMoveTranslate, 0);
				toy->mFirst.SetRotationY(toy->GetRotationY(), 0);
				toy->mFirst.mStatus.mFlags |= 0x10000;
				toy->mFirst.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->_1F90 = 1;
			} else if (anim == 0xe || anim == 0x14 || anim == 0x3f6 || anim == 0x3fc) {
				toy->mSecond.SetPos(toy->mMoveTranslate, 0);
				toy->mSecond.SetRotationY(toy->GetRotationY(), 0);
				toy->mSecond.SetAnim(1, TRUE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->mSecond.mStatus.mFlags |= 0x10000;
				toy->_1F90 = 2;
			}
			break;
		case 1:
			toy->mFirst.SetPos(toy->mMoveTranslate, 0);
			toy->mFirst.SetRotationY(toy->GetRotationY(), 0);
			if (anim != 0x13 && anim != 0x3fb) {
				toy->mFirst.mStatus.mFlags &= ~0x10000ULL;
				toy->_1F90 = 0;
			}
			break;
		case 2: {
			toy->mSecond.SetPos(toy->mMoveTranslate, 0);
			toy->mSecond.SetRotationY(toy->GetRotationY(), 0);
			f32 frame = GetJointAnimationCurrentFrame(toy);
			if (GetJointAnimationEndFrame(toy) - frames <= frame) {
				toy->mSecond.mStatus.mFlags &= ~0x10000ULL;
				toy->_1F90 = 0;
			}
			break;
		}
		}
		break;
	}
	case 0x110: {
		s32 path[] = { 0, 3, 0, 0, 0 };
		joint.Attach(GetObjectJointByPath(toy, 5, path));
		if ((toy->_9A4 & 3) < 2)
			HSD_JObjClearFlags(joint.mJObj, 0x10);
		else
			HSD_JObjSetFlags(joint.mJObj, 0x10);
		if (!(toy->_2098.mFlags & 1))
			break;
		switch (toy->_990) {
		case 0:
			break;
		case 1:
			++toy->_990;
			toy->mMovePhase = 0;
			toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
		// fall through
		case 2: {
			if (toy->_990 == 2)
				toy->mMovePhase += 2;
			toy->_984 = gActorToyAnimationData.movementSpeeds[toy->mMovePhase % 60];
			if (PSVECDistance(&toy->mMoveTranslate, &CVec(toy->_9C8)) < 0.1f) {
				toy->_984 = 0;
				toy->_990 = 0;
				toy->_2098.mFlags &= ~1;
				toy->SetPos(toy->_9C8.x, toy->_9C8.y, toy->_9C8.z, 0);
				toy->SetMoveDir(&CVec(toy->mMoveTranslate), 0);
			} else {
				toy->SetMoveDir(&CVec(toy->_9C8), toy->_984);
				CVec destination(toy->_9C8);
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
		if ((toy->_2098.mFlags & 1) && toy->_990 == 0 && toy->mMoveTranslate.y <= 2.0f) {
			++toy->_990;
			toy->mStatus.mFlags &= ~0x10000ULL;
		}
		break;
	}
	case 0x157:
		if (toy->_990 == 0 && (toy->mStatus.mFlags & 0x1000000000000000ULL)) {
			toy->_998.mFlags |= 4;
			toy->_998.mFlags |= 8;
			toy->_984   = 0.5f;
			toy->_9C8.x = 14.0f * (f32)sin(toy->_980) + toy->mMoveTranslate.x;
			toy->_9C8.y = toy->mMoveTranslate.y;
			toy->_9C8.z = 14.0f * (f32)cos(toy->_980) + toy->mMoveTranslate.z;
			++toy->_990;
			PlayGameSound(&gGame.mSound, 0x10017, 255);
			for (s32 i = 0; i < 15; ++i) {
				XObjActorToy* other = GetToy(i);
				if (other->GetKind() == 0x156) {
					other->_2098.mFlags |= 0x10;
					break;
				}
			}
		} else if (toy->_990 == 1) {
			if (GetToyHorizontalDistance(&toy->_9C8, &toy->mMoveTranslate) < 1.0f) {
				toy->_998.mFlags &= ~4;
				toy->_998.mFlags &= ~8;
				toy->_984 = 0;
			}
			if (toy->mMoveTranslate.y <= 2.0f) {
				++toy->_990;
				toy->_994 = 0;
				toy->mStatus.mFlags &= ~0x10000ULL;
				for (s32 i = 0; i < 15; ++i) {
					XObjActorToy* other = GetToy(i);
					if (other->GetKind() == 0x16c) {
						other->mStatus.mFlags |= 0x10000;
						other->SetTranslateX(toy->mMoveTranslate.x, 0);
						other->SetTranslateY(toy->mMoveTranslate.y, 0);
						other->SetTranslateZ(toy->mMoveTranslate.z, 0);
						other->_2098.mFlags |= 4;
						PlayGameSound(&gGame.mSound, 0x10016, 255);
						break;
					}
				}
			}
		}
		break;
	case 0x16c:
		switch (toy->_990) {
		case 0:
			if (toy->_2098.Check(4)) {
				toy->_2098.mFlags &= ~4;
				toy->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				++toy->_990;
			}
			break;
		case 1: {
			f32 frame = GetJointAnimationCurrentFrame(toy);
			if (GetJointAnimationEndFrame(toy) - frames <= frame) {
				toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				++toy->_990;
				toy->_994 = 0;
			}
			break;
		}
		case 2:
			if (toy->_2098.Check(8)) {
				toy->_2098.mFlags &= ~8;
				toy->SetAnim(2, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				toy->_990 = 10;
				toy->_994 = 0;
				EmitToyBurst(toy);
			}
			break;
		case 10:
			BLINK_TOY_JOINT(1, 1);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 11:
			BLINK_TOY_JOINT(1, 2);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 12:
			BLINK_TOY_JOINT(1, 3);
			BLINK_TOY_JOINT(1, 0);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 13:
			BLINK_TOY_JOINT(1, 4);
			BLINK_TOY_JOINT(2, 4, 0);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
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
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 15:
			BLINK_TOY_JOINT(1, 6);
			BLINK_TOY_JOINT(2, 6, 0);
			BLINK_TOY_JOINT(2, 6, 1);
			BLINK_TOY_JOINT(3, 6, 0, 0);
			BLINK_TOY_JOINT(4, 6, 0, 0, 0);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 16:
			BLINK_TOY_JOINT(1, 7);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 17:
			BLINK_TOY_JOINT(1, 8);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
				return;
			}
			break;
		case 18:
			BLINK_TOY_JOINT(1, 9);
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
				HSD_JObjSetFlags(joint.mJObj, 0x10);
				EmitToyBurst(toy);
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
			++toy->_994;
			if ((s32)toy->_994 > 4) {
				++toy->_990;
				toy->_994 = 0;
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
		switch (toy->mCmd[0]) {
		case 0:
			break;
		case 1:
			switch (toy->mCmd[1]) {
			case 0:
				toy->_2098.mFlags |= 1;
				toy->mCmd[0] = 0;
				break;
			case 1:
				toy->_2098.mFlags &= ~1;
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
				toy->_2098.mFlags |= 8;
				break;
			default:
				toy->_2098.mFlags &= ~8;
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
			if (toy->_2098.mFlags & 2)
				gEventVariables[toy->mCmd[1]] = 1;
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
	toy->_A50->SetAnim(1, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
	Mtx mount, rot, trans, result;
	PSMTXCopy(joint.GetMtx(), mount);
	RotateHeldProp(mount, result, rot, 'z', angle);
	OffsetHeldProp(result, trans, x, y, z);
	SetHeldPropMatrix(toy->_A50, result);
}

void XObjActorToy::StateHitRecovery(u32 frames)
{
	if (_A4C >= 0 && _A50) {
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
			_A50->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			break;
		}
		case 0x25e: {
			s32 path[2] = { 0, 4 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			Vec pos = joint.GetPos();
			_A50->SetPos(pos.x, 2.0f + pos.y, 2.5f + pos.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			break;
		}
		}
		CopyHeldPropVisibility(this, _A50);
	}
	switch (_990) {
	case 0: {
		Vec& player = gGame.GetPlayer()->mMoveTranslate;
		_980        = PI + (f32)atan2(player.x - mMoveTranslate.x, player.z - mMoveTranslate.z);
		_9BC        = mMoveTranslate;
		_998.mFlags |= 8;
		_998.mFlags |= 0x10;
		_998.mFlags |= 4;
		if (mHitStrength >= 10.0f) {
			_984 = 0;
			_9D8 = 6.8f;
		} else {
			_984 = 0;
			_9D8 = 4.8f;
		}
		_994 = 0;
		++_990;
		break;
	}
	case 1:
		++_994;
		if (_998.Check(0x20)) {
			_9D8 = 0;
			++_990;
		}
		break;
	case 2:
		_984 -= 0.04f * frames;
		if (_984 <= 0) {
			_984 = 0;
			mMoveTranslate.Set(_9BC);
			_990 = 4;
		}
		break;
	case 3: {
		_984 = 0.5f;
		_980 = atan2(_9BC.x - mMoveTranslate.x, _9BC.z - mMoveTranslate.z);
		_988 = _980;
		CVec home(_9BC);
		f32 dx = home.x - mMoveTranslate.x, dz = home.z - mMoveTranslate.z;
		if (sqrtf(dz * dz + (dx * dx + 0.0f)) < 1.0f) {
			mMoveTranslate.Set(_9BC);
			_984 = 0;
			++_990;
		}
		break;
	}
	case 4:
		_998.mFlags &= ~4;
		_998.mFlags &= ~0x40;
		mState = gToyPostHitState;
		ResetActorMovementStep(this);
		break;
	}
}

extern "C" CVec GetStagePlacementScale(XObj* obj);
extern "C" void HSD_MtxGetScale(Mtx matrix, Vec* scale);

static inline void FinishToyMount(XObjActorToy* toy)
{
	CopyHeldPropVisibility(toy, toy->_A50);
	toy->_A50->mStatus.mFlags &= ~0x0200000000000000ULL;
}

void XObjActorToy::StatePickUp(u32 frames)
{
	switch (_990) {
	case 0: {
		XObjActorTao* tao = (XObjActorTao*)gGame.mXObjs.mPools[14].First();
		if ((tao->mStatus.mFlags & 1) && !tao->IsFree())
			return;
		XObjActorPapa* papa = (XObjActorPapa*)gGame.mXObjs.mPools[13].First();
		if ((papa->mStatus.mFlags & 1) && !papa->IsFree())
			return;
		if (_A50)
			_A50->Release();
		if (_A4C >= 0)
			_A50 = ReadObj(_A4C);
		else
			_A50 = NULL;
		_998.mFlags |= 0x2000;
		++_990;
	}
	case 1:
		if (_A50) {
			if (_A50->mStatus.mFlags & 0x0040000000000000ULL)
				return;
			gGame.mStatus.mFlags |= 0x1000;
		}
		++_990;
	case 2:
		if (_A4C >= 0 && _A50) {
			_A50->SetScale(1, 1, 1, 0);
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
				_A50->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
				MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
				break;
			}
			case 0x159: {
				s32 path[2] = { 0, 7 };
				joint.Attach(FindJointByPath(mJObj, path, 2));
				position = joint.GetPos();
				_A50->SetPos(position.x, 12.0f + position.y, position.z, 0);
				PSMTXCopy(joint.GetMtx(), _A54);
				HSD_MtxGetScale(_A54, &position);
				position.x = GetStagePlacementScale(_A50).x / position.x;
				position.y = GetStagePlacementScale(_A50).y / position.y;
				position.z = GetStagePlacementScale(_A50).z / position.z;
				Mtx scale, translation, rotation, result;
				PSMTXScale(scale, position.x, position.y, position.z);
				PSMTXConcat(_A54, scale, result);
				RotateHeldProp(result, result, rotation, 'y', -PI);
				RotateHeldProp(result, result, rotation, 'x', -HALF_PI);
				OffsetHeldProp(result, translation, 5, 12, 2);
				SetHeldPropMatrix(_A50, result);
				break;
			}
			case 0x25e: {
				s32 path[2] = { 0, 4 };
				joint.Attach(FindJointByPath(mJObj, path, 2));
				position = joint.GetPos();
				_A50->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
				MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
				break;
			}
			}
			FinishToyMount(this);
		}
		_998.mFlags &= ~0x2000;
		mState = gToyPostPickUpState;
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
		if (_A4C >= 0 && _A50) {
			CJObj joint;
			s32 path[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 9));
			MountAnimatedToyProp(this, joint, HALF_PI, -1.2f, -5.5f, -0.5f);
			FinishToyMount(this);
		}
		break;
	case 0x156:
		if (_A4C >= 0 && _A50) {
			CJObj joint;
			s32 path[4] = { 0, 2, 4, 0 };
			joint.Attach(FindJointByPath(mJObj, path, 4));
			position = joint.GetPos();
			_A50->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
			MountAnimatedToyProp(this, joint, 0, 0, 2, 2);
			FinishToyMount(this);
		}
		break;
	case 0x159:
		if (_A4C >= 0 && _A50) {
			CJObj joint;
			s32 path[2] = { 0, 7 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			position = joint.GetPos();
			_A50->SetPos(position.x, 12.0f + position.y, position.z, 0);
			PSMTXCopy(joint.GetMtx(), _A54);
			HSD_MtxGetScale(_A54, &position);
			position.x = GetStagePlacementScale(_A50).x / position.x;
			position.y = GetStagePlacementScale(_A50).y / position.y;
			position.z = GetStagePlacementScale(_A50).z / position.z;
			Mtx scale, translation, rotation, result;
			PSMTXScale(scale, position.x, position.y, position.z);
			PSMTXConcat(_A54, scale, result);
			RotateHeldProp(result, result, rotation, 'y', -PI);
			RotateHeldProp(result, result, rotation, 'x', -HALF_PI);
			OffsetHeldProp(result, translation, 5, 12, 2);
			SetHeldPropMatrix(_A50, result);
			FinishToyMount(this);
		}
		break;
	case 0x25e:
		if (_A4C >= 0 && _A50) {
			CJObj joint;
			s32 path[2] = { 0, 4 };
			joint.Attach(FindJointByPath(mJObj, path, 2));
			position = joint.GetPos();
			_A50->SetPos(position.x, 2.0f + position.y, 2.5f + position.z, 0);
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
	RotateHeldProp(result, result, rotation, 'y', .19634955f);
	SetHeldPropMatrix(prop, result);
}

extern "C" void UpdateToy0FCarriedObjects(XObjActorToy* toy, u32 frames)
{
	CJObj joint;
	if (toy->_2098.Check(1)) {
		Vec& player = gGame.GetPlayer()->mMoveTranslate;
		toy->_988   = atan2(player.x - toy->mMoveTranslate.x, player.z - toy->mMoveTranslate.z);
		toy->SetRotateY(toy->_988, 0);
	}
	toy->mMoveTranslate.x += .1f * (toy->_1F94.x * frames);
	toy->mMoveTranslate.z += .1f * (toy->_1F94.z * frames);
	toy->_1F94.Set(0, 0, 0);
	if (toy->_C84.mFlags & 0x9f) {
		s32 hand[9] = { 0, 3, 0, 0, 0, 0, 0, 5, 0 };
		joint.Attach(FindJointByPath(toy->mJObj, hand, 9));
		CVec position;
		position = joint.GetPos();
		s32 kind = -1;
		if (toy->_C84.Check(1))
			kind = 0x3d;
		else if (toy->_C84.Check(2))
			kind = 0x3e;
		else if (toy->_C84.Check(4))
			kind = 0x3f;
		else if (toy->_C84.Check(8))
			kind = 0x3b;
		else if (toy->_C84.Check(0x80))
			kind = 0x21d;
		else if (toy->_C84.Check(0x10))
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
					MountToyPiece(joint, prop, toy->_A54);
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
				CopyHeldPropVisibility(toy, prop);
			}
		}
	}
	for (u32 i = 0; i < 4; ++i) {
		XObjActorToyPiece& piece = toy->mPieces[i];
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
				CopyHeldPropVisibility(toy, piece.mObj);
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
	if (object->_2098.mFlags & 8) {
		s32 path[] = { 0, 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 3, path));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	} else {
		s32 path[] = { 0, 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 3, path));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	}
}

void UpdateToy6FJointSpin(XObjActorToy* object, u32)
{
	CJObj joint;
	if (object->_2098.mFlags & 1) {
		s32 path[] = { 0, 3, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	} else {
		s32 path[] = { 0, 3, 2, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	}

	static f32 sAngle = 0.0f;
	if (object->_990 == 1) {
		s32 path[] = { 0, 3, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path));
		sAngle += 0.4f;
		if (sAngle >= TWO_PI) {
			object->_990 = 0;
			sAngle       = 0.0f;
		}
		joint.SetRotationZ(sAngle, 0);
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
	if (object->_C84.mFlags & 0x40) {
		s32 path[1] = { 0 };
		CVec pos;
		HSD_JObj* hand = GetJointByPath(object->mJObj, path, 1);
		joint.Attach(hand);
		pos    = joint.GetPos();
		s32 id = -1;
		if (object->_C84.mFlags & 0x40) {
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
	switch (object->_990) {
	case 0:
		break;
	case 1:
		object->_990++;
		object->_988 = 0.0f;
		break;
	case 2: {
		object->_988 += 0.2f;
		s32 path[2] = { 0, 3 };
		joint.Attach(GetJointFromRoot(object->mJObj, 2, path));
		if (object->_988 >= TWO_PI) {
			object->_990 = 0;
			object->_988 = 0.0f;
		}
		joint.SetRotationY(object->_988 + HSD_JObjGetRotationY(object->mJObj), 0);
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
	switch (object->_990) {
	case 0:
		if (object->_9A4 > 0) {
			object->_990++;
		}
		break;
	case 1: {
		s32 path[4] = { 0, 2, 4, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path));
		joint.SetTranslateY(-object->_9A8, 0);
		if (1.0f == object->_9A8) {
			object->_990 = 0;
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
	switch (object->_990) {
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
		object->_990++;
		break;
	}
	case 2: {
		sTime += 0.16f;
		if (sTime >= 1.0f) {
			object->_990++;
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
			object->_990 = 0;
			s32 path2[4] = { 0, 2, 0, 1 };
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
	if (object->_2098.mFlags & 1) {
		s32 path0[4] = { 0, 0, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
		s32 path1[5] = { 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
		s32 path2[6] = { 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path2));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
		s32 path3[7] = { 0, 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 7, path3));
		HSD_JObjSetFlags(joint.mJObj, 0x10);
	} else {
		s32 path0[4] = { 0, 0, 0, 1 };
		joint.Attach(GetJointFromRoot(object->mJObj, 4, path0));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
		s32 path1[5] = { 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 5, path1));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
		s32 path2[6] = { 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 6, path2));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
		s32 path3[7] = { 0, 0, 0, 0, 0, 0, 0 };
		joint.Attach(GetJointFromRoot(object->mJObj, 7, path3));
		HSD_JObjClearFlags(joint.mJObj, 0x10);
	}
}

/**
 * @note Address: 0x8011E3A4
 * @note Size: 0x2E4
 */
// hides the item the toy dropped, the _C84 bits say which
extern "C" void HideToyDroppedItem(XObjActorToy* object)
{
	s32 id = -1;
	if (object->_C84.Check(1)) {
		id = 0x3D;
		object->_C84.mFlags &= ~1;
	} else if (object->_C84.Check(2)) {
		id = 0x3E;
		object->_C84.mFlags &= ~2;
	} else if (object->_C84.Check(4)) {
		id = 0x3F;
		object->_C84.mFlags &= ~4;
	} else if (object->_C84.Check(8)) {
		id = 0x3B;
		object->_C84.mFlags &= ~8;
	} else if (object->_C84.Check(0x80)) {
		id = 0x21D;
		object->_C84.mFlags &= ~0x80;
	} else if (object->_C84.Check(0x10)) {
		id = 0x21E;
		object->_C84.mFlags &= ~0x10;
	} else if (object->_C84.Check(0x40)) {
		id = 0x341;
		object->_C84.mFlags &= ~0x40;
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
		object->_A4C   = object->mCmd[1];
		object->mState = gToy25EPickUpState;
		ResetActorMovementStep(object);
		object->mCmd[0] = 0;
		break;
	case 2:
		object->_A4C = -1;
		if (object->_A50 != NULL) {
			object->_A50->Release();
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
						object->_C84.mFlags |= 0x40;
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
			object->_2098.mFlags |= 1;
			object->mCmd[0] = 0;
			break;
		case 1:
			object->_2098.mFlags &= ~1;
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
		object->_A4C   = object->mCmd[1];
		object->mState = gToy159PickUpState;
		ResetActorMovementStep(object);
		object->mCmd[0] = 0;
		break;
	case 2:
		object->_A4C = -1;
		if (object->_A50 != NULL) {
			object->_A50->Release();
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
	case 1:
		object->_2098.mFlags |= 8;
		object->mCmd[0] = 0;
		break;
	case 2:
		if (object->mStatus.mFlags & 0x10000) {
			gEventVariables[object->mCmd[1]] = 1;
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
			gEventVariables[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 3:
		if (object->_2098.mFlags & 0x10) {
			gEventVariables[object->mCmd[1]] = 1;
		}
		object->mCmd[0] = 0;
		break;
	case 4:
		object->_A4C   = object->mCmd[1];
		object->mState = gToy156PickUpState;
		ResetActorMovementStep(object);
		object->mCmd[0] = 0;
		break;
	case 5:
		object->_A4C = -1;
		if (object->_A50 != NULL) {
			object->_A50->Release();
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
			object->_2098.mFlags &= ~1;
			object->_984 = 0.0f;
			object->_990 = 0;
			CVec pos(object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z);
			object->SetMoveDir(&pos, 0.0f);
			break;
		}
		case 1:
			object->_2098.mFlags |= 1;
			object->_9C8.x = object->mCmd[2] / 100.0f;
			object->_9C8.y = object->mCmd[3] / 100.0f;
			object->_9C8.z = object->mCmd[4] / 100.0f;
			object->_990   = 1;
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
		case 0:
			object->_2098.mFlags &= ~1;
			break;
		case 1:
			object->_2098.mFlags |= 1;
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
						object->_C84.mFlags |= 8;
						break;
					case 0x3D:
						object->_C84.mFlags |= 1;
						break;
					case 0x3E:
						object->_C84.mFlags |= 2;
						break;
					case 0x3F:
						object->_C84.mFlags |= 4;
						break;
					case 0x21D:
						object->_C84.mFlags |= 0x80;
						break;
					case 0x21E:
						object->_C84.mFlags |= 0x10;
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
		gEventVariables[object->mCmd[1]] = CountSmallSpidersGrabbingPlayer();
		object->mCmd[0]                  = 0;
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
		gGame.mEvt._10  = count;
		object->mCmd[0] = 0;
		break;
	case 5:
		object->_A4C   = object->mCmd[1];
		object->mState = gToy0FPickUpState;
		ResetActorMovementStep(object);
		object->mCmd[0] = 0;
		break;
	case 6:
		object->_A4C = -1;
		if (object->_A50 != NULL) {
			object->_A50->Release();
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
		gEventVariables[object->mCmd[1]] = CountEmergedSmallSpiders();
		object->mCmd[0]                  = 0;
		break;
	}
}

/**
 * @note Address: 0x8011F29C
 * @note Size: 0x220
 */
// NONMATCHING: regalloc
void XObjActorToy::vf148(u32 frames)
{
	vf148__9XObjActorFUl(this, frames);
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
		if (_2098.mFlags & 1) {
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
	pos.x   = place._00->mPos[0] / 100.0f;
	pos.y   = place._00->mPos[1] / 100.0f;
	pos.z   = place._00->mPos[2] / 100.0f;
	return Vec(pos);
}

static inline Vec ToyPlacementRotation(const CoordInfo& place)
{
	Vec rot = gToyPlacementRotation;
	rot.x   = DEG2RAD * place._00->_10[0];
	rot.y   = DEG2RAD * place._00->_10[1];
	rot.z   = DEG2RAD * place._00->_10[2];
	return Vec(rot);
}

/**
 * @note Address: 0x80121B08
 * @note Size: 0x5A0
 */
void XObjActorToy::ResetActorState()
{
	_998.mFlags  = 0;
	_2098.mFlags = 0;
	_C84.mFlags  = 0;
	_980         = 0.0f;
	_984         = 0.0f;
	_988         = 0.0f;
	mState       = gToyInitialState;
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
		_2098.mFlags |= 1;
		break;
	case 0x117:
		_2098.mFlags &= ~8;
		break;
	case 0x156:
		_980 = ToyPlacementRotation(mCoord).y;
		break;
	case 0x157:
		_980 = ToyPlacementRotation(mCoord).y;
		break;
	case 0x16c:
		_980 = ToyPlacementRotation(mCoord).y;
		break;
	}
	_1F94.Set(0.0f, 0.0f, 0.0f);
	_1FA4 = 0;
}
