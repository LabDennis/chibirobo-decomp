#include "game/xobjactor.h"
#include "game/cjobjset.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"

extern "C" {
void PSMTXRotRad(Mtx mtx, char axis, f32 angle);
void PSMTXTrans(Mtx mtx, f32 x, f32 y, f32 z);
}

// the texture animation of the skip-th dobj's material
static inline HSD_AObj* GetTexAObj(HSD_JObj* jobj, s32 skip)
{
	if (jobj == NULL) {
		return NULL;
	}
	HSD_DObj* dobj = HSD_JObjGetDObj(jobj);
	for (s32 i = 0; i < skip; i++) {
		if (dobj == NULL) {
			return NULL;
		}
		dobj = HSD_DObjGetNext(dobj);
	}
	HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
	if (mobj == NULL) {
		return NULL;
	}
	HSD_TObj* tobj = HSD_MObjGetTObj(mobj);
	if (tobj == NULL) {
		return NULL;
	}
	return tobj->aobj;
}

/**
 * @note Address: 0x801155B4
 * @note Size: 0xB8
 */
XObjActorToy::~XObjActorToy()
{
}

/**
 * @note Address: 0x8011566C
 * @note Size: 0x44
 */
bool XObjActorToy::IsFree() const
{
	if (!(mStatus.mFlags & 1)) {
		return true;
	}
	return !mActorStatus.Check(0x2000);
}

/**
 * @note Address: 0x801156B0
 * @note Size: 0x2D0
 */
void XObjActorToy::UpdateAnimationSounds(f32 frame)
{
	switch (GetKind()) {
	case 0x1FC:
		if (mAnimId == 0x3F6) {
			s32 path[3]    = { 0, 2, 0 };
			HSD_AObj* aobj = GetTexAObj(GetJointByPath(mJObj, path, 3), 4);
			frame          = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		}
		if (IsGameSoundPlaying(&gGame.mSound, 0x10001F)) {
			if (mAnimId != 0x10) {
				StopGameSfx(&gGame.mSound, 0x10001F);
			} else {
				SetGameSfxVolume(&gGame.mSound, 0x10001F, GetSoundVolumeAtPosition(&gGame.mSound, &mMoveTranslate, 0), 0);
			}
		}
		break;
	case 0x159:
		if (IsGameSoundPlaying(&gGame.mSound, 0xE0008) && mAnimId != 7) {
			StopGameSfx(&gGame.mSound, 0xE0008);
		}
		break;
	}
	XObjActor::UpdateAnimationSounds(frame);
}

/**
 * @note Address: 0x80115980
 * @note Size: 0x598
 */
// the 0x157 toy carries the 0x156 ones along, with what they hold in their hand
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
	case 0x157:
		mKindFlags.mFlags |= 2;
		for (s32 i = 0; i < 15; i++) {
			XObjActorToy* toy = GetToy(i);
			if (toy->GetKind() == 0x156 && (toy->mKindFlags.mFlags & 1)) {
				toy->SetTranslateX(mMoveTranslate.x, 0);
				toy->SetTranslateZ(mMoveTranslate.z, 0);
				if (toy->mAttachedId >= 0 && toy->mAttached != NULL) {
					CJObj joint;
					s32 path[4] = { 0, 2, 4, 0 };
					Mtx hand;
					Mtx trans;
					Mtx rot;
					Mtx mtx;
					joint.Attach(GetJointFromRoot(toy->mJObj, 4, path));
					toy->mAttached->SetAnim(1, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
					PSMTXCopy(joint.GetMtx(), hand);
					PSMTXRotRad(rot, 'z', 0.0f);
					PSMTXConcat(hand, rot, mtx);
					PSMTXTrans(trans, 0.0f, 2.0f, 2.0f);
					PSMTXConcat(mtx, trans, mtx);
					HSD_JObj* jobj = toy->mAttached->mJObj;
					MtxPtr m       = mtx;
					HSD_JObjCopyMtx(jobj, m);
					if (mStatus.mFlags & 0x10000) {
						toy->mStatus.mFlags |= 0x10000ULL;
					} else {
						toy->mStatus.mFlags &= ~0x10000ULL;
					}
				}
			}
		}
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
	}
}
