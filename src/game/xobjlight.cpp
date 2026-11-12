#include "game/xobjmap.h"

// XObjLight::UpdateObject
/**
 * @note Address: 0x80092E38
 * @note Size: 0xC8
 */
void XObjLight::UpdateObject(u32 frames)
{
	if ((s32)GetEnvData()[4] == 2) {
		for (s32 i = 0; i < mLightNum; ++i) {
			if (GetLightDesc(i) && GetLightAnimNum(i) > 0) {
				mLights[i].SetAnimRate((f32)frames);
				mLights[i].Anim();
			}
		}
	}
}

// XObjLight::Release
/**
 * @note Address: 0x80092F00
 * @note Size: 0x98
 */
extern "C" void Release__9XObjLightFv(XObjLight* object)
{
	if ((s32)GetEnvData()[4] == 2) {
		for (s32 i = 0; i < object->mLightNum; ++i) {
			if (object->GetLightDesc(i))
				object->mLights[i].Release();
		}
	}
	Release__4XObjFv(object);
}

// XObjLight::LoadFromPlacement
/**
 * @note Address: 0x80092F98
 * @note Size: 0x14C
 */
// NONMATCHING: regalloc — retail keeps one more copy of the object pointer live.
BOOL XObjLight::LoadFromPlacement(const CoordInfo& info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		if ((s32)GetEnvData()[4] == 2) {
			HSD_LObj* previous = NULL;
			for (s32 i = 0; i < 8; ++i) {
				if (GetLightDesc(i)) {
					mLights[i].Init(GetLightDesc(i)->mDesc, GetEnvData()[5] & (1 << (7 - i)));
					if (GetLightAnimNum(i) > 0) {
						mLightAnims[i] = 0;
						mLights[i].SetAnim(GetLightDesc(i)->mAnims[mLightAnims[i]], 0.0f);
					}
					++mLightNum;
					if (previous)
						previous->next = &mLights[i];
					previous = &mLights[i];
				}
			}
			if (previous)
				previous->next = NULL;
		}
		mStatus.mFlags &= ~0x10000ULL;
		return TRUE;
	}
	return FALSE;
}

// XObjLight::ResetObjectState
/**
 * @note Address: 0x800930E4
 * @note Size: 0x30
 */
void XObjLight::ResetObjectState()
{
	mLightNum = 0;
	for (u32 i = 0; i < 8; ++i)
		mLightAnims[i] = -1;
}

// XObjLight::InitializeObject
/**
 * @note Address: 0x80093114
 * @note Size: 0x20
 */
void XObjLight::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
}
