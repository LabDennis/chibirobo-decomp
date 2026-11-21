#ifndef _GAME_WII_XOBJCAMERA_H
#define _GAME_WII_XOBJCAMERA_H

#include "types.h"

class XObj;
class CCamera;

// the small scene-camera record the Wii player keeps twice (the eye and the head aim,
// WiiPlayerState _9810/_987C): the game's init fills it with its defaults; the class
// name and its other members are not recovered
struct XObjScnCamInitRec {
	u16 m_00;
	s16 mSnap[2]; // _02, fn_800C6CF0's per-axis snap counters
	u16 m_06;
	XObj* m_08;
	XObj* m_0C;  // the object the aim tracks when set
	Vec mTarget; // _10, a fixed aim position; x stays NAN when unused
	u32 m_1C;
	CCamera* m_20; // the camera the aim falls back to
	f32 m_24;
	f32 m_28;
	f32 m_2C;
	f32 m_30;
	f32 m_34;
	f32 m_38;
	f32 m_3C;
	f32 m_40;
	f32 m_44;
	u32 m_48;
	f32 m_4C;
	f32 m_50;
	f32 m_54;
	u32 m_58;
	f32 m_5C;
	f32 m_60;
	f32 m_64;
};
STATIC_ASSERT(sizeof(XObjScnCamInitRec) == 0x68);

#endif
