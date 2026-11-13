#ifndef _GAME_XOBJDEMO_H
#define _GAME_XOBJDEMO_H

#include "Version.h"
#include "game/xobj.h"
#include "game/clight.h"

// the object the title plays its demo scene through, its own camera and lights. class name from RTTI
class XObjDemo : public XObj {
public:
	XObjDemo();

	// _00-_980 = XObj
	s32 mState;         // _980, _970 in japan, 2 waits for the fade to finish
	BOOL mLoop;         // _984, starts the anim over once it ends
	CCamera mCamera;    // _988
	s32 mCameraAnim;    // _B1C, -1 for none
	s32 mLightCount;    // _B20
	CLight mLights[8];  // _B24
	s32 mLightAnims[8]; // _25E4

	virtual ~XObjDemo() { } // _08
	virtual void Release();
	virtual void ResetObjectState();
	virtual BOOL LoadFromPlacement(const CoordInfo& info);
	virtual void UpdateObject(u32 frames);
	virtual void DrawObject();
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjDemo) == 0x25F8);
#else
STATIC_ASSERT(sizeof(XObjDemo) == 0x2608);
#endif

#endif
