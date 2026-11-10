#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/animframe.h"
#include <stdlib.h>

// XObjActor3's destructor, its anim sounds and its script commands

extern "C" {
BOOL HSD_AudioSFXCheck(s32 id);
void HSD_AudioSFXKeyOff(s32 id);
CVec GetStagePlacementScale(XObj* obj);
void HSD_MtxGetScale(Mtx mat, Vec* vec);
void OSReport(const char* fmt, ...);
void HandleKind9ActorCommand(XObjActor3* actor);
void HandleActorRiderCommand(XObjActor3* actor);
}

// hides the items of the kind, the 0x40B ones only
static inline void HideItems(s32 kind)
{
	if (kind >= 0) {
		switch (kind) {
		case 0x40B:
			for (s32 i = 0; i < 0x120; i++) {
				XObj* obj = gGame.mXObjs.mPools[22].Get(i);
				if (kind == obj->GetKind()) {
					obj->mStatus.mFlags &= ~0x10000ULL;
				}
			}
			break;
		}
	}
}

/**
 * @note Address: 0x80108794
 * @note Size: 0x870
 */
// the event commands: 100 carries another object at one of its joints, or lets go of all
// of them, anything else is kept for the kind's function to read
// NONMATCHING: argv[1] and the 0x40B item loop take other registers, as in game/xobjspider
extern "C" s32 HandleEventCommand__10XObjActor3FlPl(XObjActor3* actor, s32 argc, s32* argv)
{
	switch (argv[0]) {
	case 100:
		if (argv[1] == 0) {
			actor->_B8C.mFlags &= ~2;
			for (u32 i = 0; i < 8; i++) {
				if (actor->mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(actor->mParts[i].mObj->mJObj);
					actor->mParts[i].mObj   = NULL;
					actor->mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; actor->mParts[i].mObj != NULL && i < 8; i++) { }
			if (i < 8) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i);
#endif
				actor->_B8C.mFlags |= 2;
				if (argv[1] >= 10000) {
					actor->mParts[i].mObj = FindObj(argv[1]);
				} else {
					actor->mParts[i].mObj = gGame.GetObject((s16)argv[1]);
				}
#if !defined(VERSION_PAL)
				if (actor->mParts[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = argv[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = argv[3 + j];
				}
				actor->mParts[i].mJoint = GetJointByPath(actor->mJObj, path, depth);
				Mtx t;
				XObj* obj = actor->mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(actor->mParts[i].mJoint), actor->mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(actor->mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(actor->mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(actor->mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(actor->mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(actor->mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = actor->mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
		break;
	default:
		actor->mEventCommand = argv[0];
		actor->mEventArg1    = argv[1];
		actor->mEventArg2    = argv[2];
		actor->mEventArg3    = argv[3];
		actor->_BA4[0]       = argv[4];
		actor->_BA4[1]       = argv[5];
		actor->_BA4[2]       = argv[6];
		actor->_BA4[3]       = argv[7];
		actor->_BA4[4]       = argv[8];
		actor->_BA4[5]       = argv[9];
		actor->_BA4[6]       = 0;
		switch (actor->GetItem()) {
		case 9:
			HandleKind9ActorCommand(actor);
			break;
		case 0x13:
			HandleActorRiderCommand(actor);
			break;
		case 8:
			switch (actor->mEventCommand) {
			case 0:
				break;
			case 1:
				s32 kind = -1;
				if (actor->_B8C.Check(0x100)) {
					kind = 0x40B;
					actor->_B8C.mFlags &= ~0x100;
				}
				HideItems(kind);
				switch (actor->mEventArg1) {
				case 0x40B:
					for (s32 i = 0; i < 0x120; i++) {
						XObj* obj = gGame.mXObjs.mPools[22].Get(i);
						if (actor->mEventArg1 == obj->GetKind()) {
							actor->mHeld = obj;
							obj->mStatus.mFlags |= 0x10000ULL;
							switch (actor->mEventArg1) {
							case 0x40B:
								actor->_B8C.mFlags |= 0x100;
								obj->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
								break;
							}
						}
					}
					break;
				default:
					if (actor->mHeld != NULL) {
						HSD_JObjClearMtxFlags(actor->mHeld->mJObj);
						actor->mHeld = NULL;
					}
					break;
				}
				actor->mEventCommand = 0;
				break;
			}
			break;
		}
		break;
	}
	return 0;
}

static inline void UpdateActorAnimationSound(XObjActor3* actor, s32 id)
{
	f32 frame = GetAnimFrame(actor->mJObj);
	actor->mAnimSfx.Update(id, frame, GetSoundVolumeAtPosition(&gGame.mSound, &actor->mMoveTranslate, 0));
}

/**
 * @note Address: 0x80108424
 * @note Size: 0x11C
 */
XObjActor3::~XObjActor3()
{
}

/**
 * @note Address: 0x80108540
 * @note Size: 0x154
 */

void XObjActor3::UpdateAnimationSounds()
{
	if ((mStatus.mFlags & 1) && (mStatus.mFlags & 0x10000) && mPoolIndex == 3) {
		UpdateActorAnimationSound(this, mAnimId);
	}
}

/**
 * @note Address: 0x80108694
 * @note Size: 0x94
 */
void XObjActor3::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	if (GetKind() == 0x31B) {
		if (mAnimId == 7) {
			mSe = PlayGameSound(&gGame.mSound, 0xB0000, 0xFF);
		} else if (HSD_AudioSFXCheck(mSe)) {
			HSD_AudioSFXKeyOff(mSe);
		}
	}
}

/**
 * @note Address: 0x80108728
 * @note Size: 0x6C
 */
void XObjActor3::OnHitFromPosition(const Vec& position, f32 strength)
{
	if (GetItem() == 1) {
		XObj::OnHitFromPosition(position, strength);
	} else if (!_B8C.Check(1)) {
		_B8C.mFlags |= 1;
	}
}
