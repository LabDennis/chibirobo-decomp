#ifndef _GAME_XOBJFREE_H
#define _GAME_XOBJFREE_H

#include "game/xobj.h"

// the plain stage objects, name from RTTI
class XObjFree : public XObj {
public:
	// _00-_980 = XObj
	f32 mLife;                  // _980, counts down from the object's flags once hit
	f32 mFlash;                 // _984, the ring drawn around it
	HSD_MatAnimJoint* mMatAnim; // _988, kept across anim changes
	u32 _98C;                   // _98C, never accessed

	XObjFree();

	virtual ~XObjFree() { } // _08 (weak)
	virtual void Release(); // _54
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);                               // _80
	virtual void ResetObjectState();                                   // _98
	virtual void UpdateObject(u32 frames);                             // _A4
	virtual void DrawScreenOverlay();                                  // _C8
	virtual void OnHitFromPosition(const Vec& position, f32 strength); // _138
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(XObjFree) == 0x980);
#else
STATIC_ASSERT(sizeof(XObjFree) == 0x990);
#endif

#endif
