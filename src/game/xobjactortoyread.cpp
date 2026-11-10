#include "game/xobjactor.h"
#include "game/cjobjset.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"

extern "C" {
// the sound tables the toys play along their anims
extern u8 gActorToyAnimationData[];
}

/**
 * @note Address: 0x8012214C
 * @note Size: 0x82C
 */
void XObjActorToy::vfAC()
{
	vfAC__9XObjActorFv(this);
	if (mFirst.mStatus.mFlags & 0x10000) {
		mFirst.CJObj::SetTranslate(GetTranslateX(), GetTranslateY(), GetTranslateZ(), 1);
		mFirst.CJObj::SetScale(GetScaleX(), GetScaleY(), GetScaleZ(), 1);
		mFirst.CJObj::SetRotationY(GetRotationY(), 1);
		if (mJObj->flags & 8) {
			HSD_JObjSetFlags(mFirst.mJObj, 8);
		} else {
			HSD_JObjClearFlags(mFirst.mJObj, 8);
		}
		mFirst.vfAC();
	}
	if (mSecond.mStatus.mFlags & 0x10000) {
		mSecond.CJObj::SetTranslate(GetTranslateX(), GetTranslateY(), GetTranslateZ(), 1);
		mSecond.CJObj::SetScale(GetScaleX(), GetScaleY(), GetScaleZ(), 1);
		mSecond.CJObj::SetRotationY(GetRotationY(), 1);
		if (mJObj->flags & 8) {
			HSD_JObjSetFlags(mSecond.mJObj, 8);
		} else {
			HSD_JObjClearFlags(mSecond.mJObj, 8);
		}
		mSecond.vfAC();
	}
}

/**
 * @note Address: 0x80122978
 * @note Size: 0x64
 */
void XObjActorToy::Release()
{
	mFirst.Release();
	mSecond.Release();
	Release__4XObjFv(this);
	mLoad = 0;
	_1F90 = 0;
}

/**
 * @note Address: 0x801229DC
 * @note Size: 0x1E8
 */
BOOL XObjActorToy::LoadFromPlacement(const CoordInfo& info)
{
	switch (GetKind()) {
	case 0xF:
		switch (mLoad) {
		case 0:
			if (!LoadFromPlacement__9XObjActorFRC9CoordInfo(this, info)) {
				break;
			}
			mLoad++;
		case 1:
			if (!mFirst.LoadFromPlacement(CoordInfo(GetSpawnData(0x2A)))) {
				break;
			}
			mFirst.mStatus.mFlags &= ~0x10000ULL;
			mFirst.SetPos(mMoveTranslate, 0);
			mLoad++;
		case 2:
			if (!mSecond.LoadFromPlacement(CoordInfo(GetSpawnData(0x2B)))) {
				break;
			}
			mSecond.mStatus.mFlags &= ~0x10000ULL;
			mSecond.SetPos(mMoveTranslate, 0);
			mLoad++;
		}
		if (mLoad > 2) {
			mLoad = 0;
			return TRUE;
		}
		return FALSE;
	}
	return LoadFromPlacement__9XObjActorFRC9CoordInfo(this, info) != FALSE;
}

/**
 * @note Address: 0x80122BC4
 * @note Size: 0x174
 */
void XObjActorToy::ResetObjectState()
{
	u8* data = gActorToyAnimationData;
	ResetObjectState__9XObjActorFv(this);
	mLoad = 0;
	_1F90 = 0;
	mFirst.ResetObjectState();
	mSecond.ResetObjectState();
	switch (GetKind()) {
	case 0x6F:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x374);
		break;
	case 0x110:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x68);
		break;
	case 0x117:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x50C);
		break;
	case 0x159:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x4DC);
		break;
	case 0x1FB:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x62C);
		break;
	case 0x1FC:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x3EC);
		break;
	case 0x25E:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x74C);
		break;
	case 0x317:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x68C);
		break;
	case 0x26C:
	case 0x485:
		mAnimSfx.mEvents = (AnimSfxEvent*)(data + 0x4AC);
		break;
	default:
		mAnimSfx.mEvents = NULL;
		break;
	}
}

/**
 * @note Address: 0x80122D38
 * @note Size: 0x60
 */
void XObjActorToy::vf94()
{
	vf94__9XObjActorFv(this);
	mLoad = 0;
	_1F90 = 0;
	mFirst.vf94();
	mSecond.vf94();
}

/**
 * @note Address: 0x80122D98
 * @note Size: 0x74
 */
void XObjActorToy::InitializeObject(u32 id)
{
	InitializeObject__9XObjActorFUl(this, id);
	mLoad = 0;
	_1F90 = 0;
	mFirst.InitializeObject(id);
	mSecond.InitializeObject(id);
}
