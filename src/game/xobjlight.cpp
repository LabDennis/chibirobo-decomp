#include "game/xobjmap.h"

// XObjLight::UpdateObject
/**
 * @note Address: 0x80092E38
 * @note Size: 0xC8
 */
extern "C" void UpdateObject__9XObjLightFUl(XObjLight* object, u32 frames)
{
	if ((s32)GetEnvData()[4] == 2) {
		for (s32 i = 0; i < object->mLightNum; ++i) {
			if (object->GetLightDesc(i) && object->GetLightAnimNum(i) > 0) {
				object->mLights[i].SetAnimRate((f32)frames);
				object->mLights[i].Anim();
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
extern "C" BOOL LoadFromPlacement__9XObjLightFRC9CoordInfo(XObjLight* object, CoordInfo* info)
{
	if (LoadFromPlacement__4XObjFRC9CoordInfo(object, info)) {
		if ((s32)GetEnvData()[4] == 2) {
			HSD_LObj* previous = NULL;
			for (s32 i = 0; i < 8; ++i) {
				if (object->GetLightDesc(i)) {
					object->mLights[i].Init(object->GetLightDesc(i)->mDesc, GetEnvData()[5] & (1 << (7 - i)));
					if (object->GetLightAnimNum(i) > 0) {
						object->mLightAnims[i] = 0;
						object->mLights[i].SetAnim(object->GetLightDesc(i)->mAnims[object->mLightAnims[i]], 0.0f);
					}
					++object->mLightNum;
					if (previous)
						previous->next = &object->mLights[i];
					previous = &object->mLights[i];
				}
			}
			if (previous)
				previous->next = NULL;
		}
		object->mStatus.mFlags &= ~0x10000ULL;
		return TRUE;
	}
	return FALSE;
}

// XObjLight::ResetObjectState
/**
 * @note Address: 0x800930E4
 * @note Size: 0x30
 */
extern "C" void ResetObjectState__9XObjLightFv(XObjLight* object)
{
	object->mLightNum = 0;
	for (u32 i = 0; i < 8; ++i)
		object->mLightAnims[i] = -1;
}

// XObjLight::InitializeObject
/**
 * @note Address: 0x80093114
 * @note Size: 0x20
 */
extern "C" void InitializeObject__9XObjLightFUl(XObjLight* object, u32 id)
{
	InitializeObject__4XObjFUl(object, id);
}
