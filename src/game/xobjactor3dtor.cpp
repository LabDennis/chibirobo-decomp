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
s32 XObjActor3::HandleEventCommand(s32 argc, s32* argv)
{
	switch (argv[0]) {
	case 100:
		if (argv[1] == 0) {
			mActorFlags.mFlags &= ~2;
			for (u32 i = 0; i < 8; i++) {
				if (mParts[i].mObj != NULL) {
					HSD_JObjClearMtxFlags(mParts[i].mObj->mJObj);
					mParts[i].mObj   = NULL;
					mParts[i].mJoint = NULL;
				}
			}
		} else {
			u32 i;
			for (i = 0; mParts[i].mObj != NULL && i < 8; i++) { } // first free part slot
			if (i < 8) {
#if !defined(VERSION_PAL)
				OSReport("id = %d, argv[1]=%d\n", i);
#endif
				mActorFlags.mFlags |= 2;
				if (argv[1] >= 10000) { // a placement id vs a slot index
					mParts[i].mObj = FindObj(argv[1]);
				} else {
					mParts[i].mObj = gGame.GetObject((s16)argv[1]);
				}
#if !defined(VERSION_PAL)
				if (mParts[i].mObj == NULL) {
					OSReport("NULL===============\n");
				}
#endif
				s32 path[8];
				s32 depth = argv[2];
				for (s32 j = 0; j < depth; j++) {
					path[j] = argv[3 + j];
				}
				mParts[i].mJoint = GetJointByPath(mJObj, path, depth);
				Mtx t;
				XObj* obj = mParts[i].mObj;
				PSMTXTrans(t, obj->mMoveTranslate.x, obj->mMoveTranslate.y, obj->mMoveTranslate.z);
				PSMTXCopy(HSD_JObjGetMtxPtr(mParts[i].mJoint), mParts[i].mMtx);
				Vec scale;
				HSD_MtxGetScale(mParts[i].mMtx, &scale);
				scale.x = GetStagePlacementScale(mParts[i].mObj).x / scale.x;
				scale.y = GetStagePlacementScale(mParts[i].mObj).y / scale.y;
				scale.z = GetStagePlacementScale(mParts[i].mObj).z / scale.z;
				Mtx s;
				PSMTXScale(s, scale.x, scale.y, scale.z);
				Mtx m;
				PSMTXConcat(mParts[i].mMtx, s, m);
				PSMTXConcat(m, t, m);
				HSD_JObj* joint = mParts[i].mObj->mJObj;
				MtxPtr mp       = m;
				HSD_JObjCopyMtx(joint, mp);
			}
		}
		break;
	default:
		mEventCommand      = argv[0];
		mEventArg1         = argv[1];
		mEventArg2         = argv[2];
		mEventArg3         = argv[3];
		mExtraEventArgs[0] = argv[4];
		mExtraEventArgs[1] = argv[5];
		mExtraEventArgs[2] = argv[6];
		mExtraEventArgs[3] = argv[7];
		mExtraEventArgs[4] = argv[8];
		mExtraEventArgs[5] = argv[9];
		mExtraEventArgs[6] = 0;
		switch (GetItem()) {
		case 9:
			HandleKind9ActorCommand(this);
			break;
		case 0x13:
			HandleActorRiderCommand(this);
			break;
		case 8:
			switch (mEventCommand) {
			case 0:
				break;
			case 1:
				s32 kind = -1;
				if (mActorFlags.Check(0x100)) { // a 0x40B item is on show
					kind = 0x40B;
					mActorFlags.mFlags &= ~0x100;
				}
				HideItems(kind);
				switch (mEventArg1) {
				case 0x40B:
					for (s32 i = 0; i < 0x120; i++) {
						XObj* obj = gGame.mXObjs.mPools[22].Get(i);
						if (mEventArg1 == obj->GetKind()) {
							mHeld = obj;
							obj->mStatus.mFlags |= 0x10000ULL;
							switch (mEventArg1) {
							case 0x40B:
								mActorFlags.mFlags |= 0x100;
								obj->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
								break;
							}
						}
					}
					break;
				default:
					if (mHeld != NULL) {
						HSD_JObjClearMtxFlags(mHeld->mJObj);
						mHeld = NULL;
					}
					break;
				}
				mEventCommand = 0;
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
		} else if (HSD_AudioSFXCheck(mSe)) { // leaving anim 7: key the sound off
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
	if (GetItem() == 1) { // item 1's hits go to the base class
		XObj::OnHitFromPosition(position, strength);
	} else if (!mActorFlags.Check(1)) {
		mActorFlags.mFlags |= 1;
	}
}
