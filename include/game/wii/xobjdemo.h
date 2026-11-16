#ifndef GAME_WII_XOBJDEMO_H
#define GAME_WII_XOBJDEMO_H

#include "game/wii/cursor.h"

// the object the title plays its demo scene through, its own camera and lights; the Wii's XObjDemo,
// an XObj2D (class name and bases from RTTI, the three objects at Title _1388). The members follow the
// GameCube's; the Wii camera and light types are not reconstructed, so their storage is a gap
class XObjDemo : public XObj2D {
public:
	virtual ~XObjDemo() { }
	virtual void Release();                                // _40
	virtual void ResetObjectState();                       // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void UpdateObject(u32 frames);                 // _164
	virtual void DrawObject();                             // _174

	s32 mState;             // _9A8, 2 waits for the fade to finish
	BOOL mLoop;             // _9AC, starts the anim over once it ends
	u8 _9B0[0xBA8 - 0x9B0]; // the camera
	s32 mCameraAnim;        // _BA8, -1 for none
	s32 mLightCount;        // _BAC
	u8 _BB0[0xF10 - 0xBB0]; // the eight lights
	s32 mLightAnims[8];     // _F10
};
STATIC_ASSERT(sizeof(XObjDemo) == 0xF30);

#endif
