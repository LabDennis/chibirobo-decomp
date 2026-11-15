#ifndef GAME_WII_XOBJ_H
#define GAME_WII_XOBJ_H

#include "Dolphin/mtx.h"
#include "game/ut.h"

// The Wii's object family (RTTI: XObj, XObjActor and the rest). The GameCube's XObj wrapped
// sysdolphin joints and an HSD archive in its head; the Wii renders through nw4r::g3d callbacks
// (ICalcWorldCallback's vtable sits at +0 of the complete object, the CXObj chain's 155-slot
// vtable at +4), which drops the joint/archive head and shifts every member. Members are
// declared only where the ported code reaches them, offsets verified against the retail dol.
// The chain vtable's order is not yet reconstructed; virtuals arrive with the functions that
// need them.

// a pose channel of a Wii XObj: current value, target and step rate, the GameCube's CObjVec
class CObjVec : public CVec {
public:
	CVec mTarget; // _10
	CVec mSpeed;  // _20
};
STATIC_ASSERT(sizeof(CObjVec) == 0x30);

// the chain subobject view: what a pointer passed to an XObj method sees (the complete object
// carries the nw4r callback base four bytes in front)
class XObj {
public:
	u8 _000[0x030];
	CObjVec mMoveTranslate; // _030, the object's pose position
	CObjVec mMoveRotate;    // _060
	u8 _090[0x9A8 - 0x090];
};
STATIC_ASSERT(sizeof(XObj) == 0x9A8);

// the base of the people and toys, the GameCube's members shifted by +0x28
class XObjActor : public XObj {
public:
	f32 mHeading;   // _9A8, the way it walks
	f32 mMoveSpeed; // _9AC, move speed
	u8 _9B0[0x9B8 - 0x9B0];
	s32 mKindStep;    // _9B8, the kind's own step
	u32 mKindCounter; // _9BC, cleared with mKindStep

	void OnPushDisplacement(const Vec& displacement);
	Vec GetCameraTarget();
};

#endif
