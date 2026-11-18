#ifndef GAME_WII_XOBJFREE_H
#define GAME_WII_XOBJFREE_H

#include "game/wii/xobj.h"

// the plain stage objects, name from RTTI. The Wii class keeps the GameCube's fields
// shifted 0x28 behind the bigger XObj; the retail FREE pool stride is 0x9B8
class XObjFree : public XObj {
public:
	XObjFree();
	virtual ~XObjFree();                                                                  // _08
	virtual void Release();                                                               // _40
	virtual void SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate); // _108, shares the base body
	virtual void ResetObjectState();                                                       // _154
	virtual void UpdateObject(u32 frames);                                                 // _164
	virtual void DrawScreenOverlay();                                                      // _190
	virtual void OnHitFromPosition(const Vec& position, f32 strength);                     // _1FC

	f32 mLife;      // _9A8, the item index it was placed with, counted down by hits
	f32 mFlash;     // _9AC, the ring DrawScreenOverlay draws while it counts down
	void* mMatAnim; // _9B0, kept across anim changes like the GameCube's
	u32 _9B4;       // _9B4, never accessed
};
STATIC_ASSERT(sizeof(XObjFree) == 0x9B8);
STATIC_ASSERT(offsetof(XObjFree, mLife) == 0x9A8);

#endif
