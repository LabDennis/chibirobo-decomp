#ifndef _GAME_XOBJSOUND_H
#define _GAME_XOBJSOUND_H

#include "game/xobj.h"
#include "game/csound.h"

// an object that only plays its anim's sounds
class XObjSound : public XObj {
public:
	// _00-_980 = XObj
	AnimSfx mSfx;           // _980
	u32 _990;               // _990, written 0 in ResetObjectState, never read
	u32 _994;               // _994, written 0 in ResetObjectState, never read
	u8 _998[0x9A0 - 0x998]; // _998

	XObjSound();

	virtual ~XObjSound();                  // _08
	virtual void InitializeObject(u32 id); // _90
	virtual void ResetObjectState();       // _98
	virtual void UpdateObject(u32 frames); // _A4
	virtual void UpdateSfx(f32 frame);     // _148
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjSound) == 0x990);
#else
STATIC_ASSERT(sizeof(XObjSound) == 0x9A0);
#endif

#endif
