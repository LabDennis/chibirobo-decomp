#include "game/xobjactor.h"
#include "game/actortoyanimation.h"
#include "game/cjobjset.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/hsd_dobj.h"

// the sound tables the toys play along their anims
ActorToyAnimationData gActorToyAnimationData = {
	{ 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 3, 0, 0, 3, 0 },
	{
	    { 4, 15.0f, -1.0f, 1, 0x000D000D, 0xFF, 0, 0, 0x00 },      { 4, 60.0f, -1.0f, 1, 0x00000145, 0xFF, 0, 0, 0x00 },
	    { 6, 22.0f, -1.0f, 1, 0x000D000F, 0xFF, 0, 0, 0x00 },      { 13, 0.0f, -1.0f, 1, 0x000D000B, 0xFF, 0, 0, 0x00 },
	    { 14, 0.0f, -1.0f, 1, 0x000D000C, 0xFF, 0, 0, 0x00 },      { 14, 52.0f, -1.0f, 1, 0x000D000E, 0xFF, 0, 0, 0x00 },
	    { 15, 0.0f, -1.0f, 1, 0x000D0011, 0xFF, 0, 0, 0x00 },      { 2, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { 1030, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },   { 36, 28.0f, 56.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { 1037, 30.0f, 60.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },   { 1037, 90.0f, 120.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { 1037, 150.0f, 160.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 }, { 1037, 180.0f, 190.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { 1037, 200.0f, 210.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 }, { 1037, 240.0f, 250.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { 1037, 260.0f, 270.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 }, { 1037, 280.0f, 300.0f, 1, 0x000D0010, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{ 0.0f,  0.1f,  0.1f,  0.1f,  0.2f,  0.2f,  0.3f,  0.3f,  0.35f, 0.35f, 0.4f,  0.4f,  0.4f,  0.43f, 0.43f,
	  0.47f, 0.48f, 0.45f, 0.44f, 0.43f, 0.4f,  0.4f,  0.4f,  0.4f,  0.37f, 0.36f, 0.31f, 0.3f,  0.3f,  0.3f,
	  0.3f,  0.3f,  0.3f,  0.3f,  0.3f,  0.35f, 0.35f, 0.43f, 0.43f, 0.48f, 0.48f, 0.49f, 0.49f, 0.49f, 0.47f,
	  0.47f, 0.46f, 0.43f, 0.41f, 0.38f, 0.36f, 0.36f, 0.36f, 0.36f, 0.36f, 0.3f,  0.24f, 0.2f,  0.12f, 0.0f },
	{
	    { -203.97f, 0.0f, -29.19f },
	    { -258.79f, 0.0f, -28.54f },
	    { -203.97f, 0.0f, -29.19f },
	    { -205.69f, 0.0f, -199.83f },
	    { -68.85f, 0.0f, -316.37f },
	    { -46.55f, 0.0f, -280.86f },
	    { -66.58f, 0.0f, 89.06f },
	},
	{
	    { 2, 33.0f, 66.0f, 1, 0x000C0008, 0xFF, 0, 0, 0x00 },
	    { 4, 40.0f, 80.0f, 1, 0x000C0006, 0xFF, 0, 0, 0x00 },
	    { 1005, 40.0f, -1.0f, 1, 0x000C0007, 0xFF, 0, 0, 0x00 },
	    { 1005, 126.0f, -1.0f, 1, 0x000C0005, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 1006, 73.0f, -1.0f, 1, 0x00100012, 0xFF, 0, 0, 0x00 },
	    { 2, 39.0f, 78.0f, 1, 0x00100013, 0xFF, 0, 0, 0x00 },
	    { 2, 117.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0x00 },
	    { 1012, 37.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0x00 },
	    { 1013, 37.0f, -1.0f, 1, 0x00100013, 0xFF, 0, 0, 0x00 },
	    { 1014, 1.0f, 20.0f, 1, 0x00100014, 0xFF, 0, 0, 0x00 },
	    { 16, 1.0f, -1.0f, 1, 0x0010001F, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 2, 37.0f, 76.0f, 1, 0x0000015C, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 7, 2.0f, -1.0f, 1, 0x000E0008, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 1026, 5.0f, -1.0f, 1, 0x000F001D, 0xFF, 0, 0, 0x00 },
	    { 2, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0x00 },
	    { 2, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0x00 },
	    { 10, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0x00 },
	    { 10, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0x00 },
	    { 17, 26.0f, 85.0f, 1, 0x000F001F, 0xFF, 0, 0, 0x00 },
	    { 17, 55.0f, 115.0f, 1, 0x000F001E, 0xFF, 0, 0, 0x00 },
	    { 1039, 26.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0x00 },
	    { 1039, 55.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0x00 },
	    { 13, 49.0f, -1.0f, 1, 0x000F001F, 0xFF, 0, 0, 0x00 },
	    { 13, 95.0f, -1.0f, 1, 0x000F001E, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 2, 30.0f, 70.0f, 1, 0x0007001F, 0xFF, 0, 0, 0x00 },
	    { 2, 110.0f, -1.0f, 1, 0x0007001F, 0xFF, 0, 0, 0x00 },
	    { 43, 65.0f, -1.0f, 1, 0x0007001F, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 2, 17.0f, 35.0f, 1, 0x0006000B, 0xFF, 0, 0, 0x00 },
	    { 4, 115.0f, -1.0f, 1, 0x0006000C, 0xFF, 0, 0, 0x00 },
	    { 4, 220.0f, -1.0f, 1, 0x0006000D, 0xFF, 0, 0, 0x00 },
	    { 11, 50.0f, -1.0f, 1, 0x0006000E, 0xFF, 0, 0, 0x00 },
	    { 5, 55.0f, -1.0f, 1, 0x0006000F, 0xFF, 0, 0, 0x00 },
	    { 8, 29.0f, -1.0f, 1, 0x0006000B, 0xFF, 0, 0, 0x00 },
	    { 13, 99.0f, 160.0f, 1, 0x00000019, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
	{
	    { 10, 1.0f, 50.0f, 1, 0x0012001C, 0xFF, 9, 0, 0x00 },
	    { 7, 15.0f, -1.0f, 1, 0x00120028, 0xFF, 0, 0, 0x00 },
	    { 4, 40.0f, -1.0f, 1, 0x00120029, 0xFF, 0, 0, 0x00 },
	    { 5, 30.0f, -1.0f, 1, 0x00120027, 0xFF, 0, 0, 0x00 },
	    { -1, 0.0f, 0.0f, 0, 0x00000000, 0x00, 0, 0, 0x00 },
	},
};

/**
 * @note Address: 0x8012214C
 * @note Size: 0x82C
 */
void XObjActorToy::vfAC()
{
	XObjActor::vfAC();
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
	XObj::Release();
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
	u8* data = (u8*)&gActorToyAnimationData;
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
	XObjActor::vf94();
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
