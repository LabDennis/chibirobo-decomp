#include "game/xobjmap.h"
#include "game/clight.h"
#include "game/ctrigon.h"

// XObjMap, the room model. its children are the floor collision, one CTrigon each, and it
// can carry the room's lights

// who owns the stage lights, 1 the map and 2 the light objects
static inline s32 GetLightMode()
{
	return GetEnvData()[4];
}

static inline u32 MakeRGB(s32 r, s32 g, s32 b)
{
	s32 cr = r;
	if (cr >= 256) {
		cr = 255;
	}
	s32 cg = g;
	if (cg >= 256) {
		cg = 255;
	}
	s32 cb = b;
	if (cb >= 256) {
		cb = 255;
	}
	return (cr << 16) | (cg << 8) | cb;
}

// the floor kind of a triangle, from its vertex color
/**
 * @note Address: 0x80093210
 * @note Size: 0x48C
 */
extern "C" s32 GetMapTriangleFloorType(XObjMap* map, s32 trigon, s32 tri)
{
	static u32 sColors[13] = {
		MakeRGB(0, 0, 0),       MakeRGB(256, 256, 128), MakeRGB(256, 0, 0), MakeRGB(0, 0, 0), MakeRGB(256, 256, 0),
		MakeRGB(256, 256, 256), MakeRGB(0, 0, 0),       MakeRGB(0, 0, 0),   MakeRGB(0, 0, 0), MakeRGB(0, 256, 256),
		MakeRGB(256, 0, 256),   MakeRGB(256, 0, 128),   MakeRGB(0, 0, 0),
	};
	u32 color = map->mTrigon[trigon].mTri[tri].mColor;
	for (s32 i = 1; i < 13; i++) {
		if (color == sColors[i]) {
			return i;
		}
	}
	return 0;
}

/**
 * @note Address: 0x8009369C
 * @note Size: 0xCC
 */
void XObjMap::UpdateObject(u32 frames)
{
	// NONMATCHING: induction pointer difference, see XObjLight::Update
	UpdateObject__4XObjFUl(this, frames);
	if (GetLightMode() == 1) {
		for (int i = 0; i < mLightNum; i++) {
			if (GetLightDesc(i) != NULL && GetLightAnimNum(i) > 0) {
				mLights[i].SetAnimRate(frames);
				mLights[i].Anim();
			}
		}
	}
}

/**
 * @note Address: 0x80093768
 * @note Size: 0xC0
 */
void XObjMap::Release()
{
	for (int i = 0; i < mTrigonCount; i++) {
		mTrigon[i].Release();
	}
	if (GetLightMode() == 1) {
		for (int i = 0; i < mLightNum; i++) {
			if (GetLightDesc(i) != NULL) {
				mLights[i].Release();
			}
		}
	}
	Release__4XObjFv(this);
}

// a floor for every child of the root joint that has triangles, sorted from the highest down
/**
 * @note Address: 0x80093828
 * @note Size: 0x298
 */
BOOL XObjMap::LoadFromPlacement(const CoordInfo& info)
{
	// NONMATCHING: the dol copies a zero path template before storing the index
	if (LoadFromPlacement__4XObjFRC9CoordInfo(this, &info)) {
		int i;
		int j;
		int n = 0;
		for (i = 0;; i++) {
			s32 path[]     = { 0 };
			path[0]        = i;
			HSD_JObj* jobj = GetJointByPath(mJObj, path, 1);
			if (jobj == NULL) {
				break;
			}
			if (mTrigon[n].Create(jobj) != NULL) {
				mTrigonOrder[n] = n;
				n++;
			}
		}
		mTrigonCount = n;
		for (i = 0; i < mTrigonCount - 1; i++) {
			for (j = i + 1; j < mTrigonCount; j++) {
				s32 a = mTrigonOrder[i];
				s32 b = mTrigonOrder[j];
				if (mTrigon[a].mMinY < mTrigon[b].mMinY) {
					mTrigonOrder[i] = b;
					mTrigonOrder[j] = a;
				}
			}
		}
		for (i = 0; i < mTrigonCount; i++) { }
		if (GetLightMode() == 1) {
			CLight* prev = NULL;
			for (i = 0; i < 8; i++) {
				if (GetLightDesc(i) != NULL) {
					mLights[i].Init(GetLightDesc(i)->mDesc, GetEnvData()[5] & (1 << (7 - i)));
					if (GetLightAnimNum(i) > 0) {
						mLightAnims[i] = 0;
						mLights[i].SetAnim(GetLightDesc(i)->mAnims[mLightAnims[i]], 0.0f);
					}
					mLightNum++;
					if (prev != NULL) {
						prev->next = &mLights[i];
					}
					prev = &mLights[i];
				}
			}
			if (prev != NULL) {
				prev->next = NULL;
			}
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80093AC0
 * @note Size: 0x9C
 */
void XObjMap::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
	mTrigonCount = 0;
	for (int i = 0; i < 16; i++) {
		mTrigonOrder[i] = 0;
	}
	mLightNum = 0;
	for (int i = 0; i < 8; i++) {
		mLightAnims[i] = -1;
	}
}

/**
 * @note Address: 0x80093B5C
 * @note Size: 0x20
 */
void XObjMap::InitializeObject(u32 id)
{
	InitializeObject__4XObjFUl(this, id);
}
