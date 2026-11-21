#ifndef _GAME_WII_XOBJCAMERA_H
#define _GAME_WII_XOBJCAMERA_H

#include "types.h"
#include "Dolphin/mtx.h"
#include "game/ut.h"

class XObj;
class CCamera;

// the small scene-camera record the Wii player keeps twice (the eye and the head aim,
// WiiPlayerState _9810/_987C): the game's init fills it with its defaults; the class
// name and its other members are not recovered. its retail ctor (0x800BF9A4) stores
// CVec vtables at _1C/_58/_68 and the TVec vtable at _48
struct XObjScnCamInitRec {
	u16 m_00;
	s16 mSnap[2]; // _02, fn_800C6CF0's per-axis snap counters
	u16 m_06;
	XObj* m_08;
	XObj* m_0C;    // the object the aim tracks when set
	CVec mTarget;  // _10, a fixed aim position; x stays NAN when unused
	CCamera* m_20; // the camera the aim falls back to
	f32 m_24;
	f32 m_28;
	f32 m_2C;
	f32 m_30;
	f32 m_34;
	f32 m_38;
	TVec m_3C; // _3C, the aim's rotation: PSMTXRotRad reads .x as the z angle and .y as the y
	CVec m_4C; // _4C, cleared together with the target
	CVec m_5C; // _5C

	XObjScnCamInitRec();
	~XObjScnCamInitRec();
};
STATIC_ASSERT(sizeof(XObjScnCamInitRec) == 0x6C);

#endif
