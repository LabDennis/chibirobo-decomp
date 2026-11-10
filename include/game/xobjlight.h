#ifndef _GAME_XOBJLIGHT_H
#define _GAME_XOBJLIGHT_H

#include "game/xobj.h"
#include "game/clight.h"

// a light's desc list, the lobj desc and its anims
struct XObjLightDesc {
	HSD_LightDesc* mDesc;   // _00
	HSD_LightAnim** mAnims; // _04
};

// the stage's lights when the map doesn't own them, vtable at 0x80225D98
class XObjLight : public XObj {
public:
	// _00-_980 = XObj
	s32 mLightNum;      // _980
	CLight mLights[8];  // _984, from desc lists 1 to 8
	s32 mLightAnims[8]; // _2444

	// todo: cleanup
	XObjLightDesc* GetLightDesc(s32 i) { return (XObjLightDesc*)mArchive.mDesc[i + 1]; }
	s32 GetLightAnimNum(s32 i) { return mArchive.mNum[i + 2]; }
	CLight* GetLights() { return mLightNum > 0 ? mLights : NULL; }

	XObjLight();
	virtual void Release();
	virtual void InitializeObject(u32 id);
	virtual void ResetObjectState();
	virtual BOOL LoadFromPlacement(const CoordInfo& info);
	virtual void UpdateObject(u32 frames);
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjLight) == 0x2458);
#else
STATIC_ASSERT(sizeof(XObjLight) == 0x2468);
#endif

#endif
