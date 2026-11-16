#ifndef GAME_WII_CURSOR_H
#define GAME_WII_CURSOR_H

#include "game/wii/xobj.h"

// the base of the Wii's screen-space objects (Cursor and XObjDemo derive from it). class name
// and bases from RTTI; it adds no members, its one override sits at 0x800BD4B8
class XObj2D : public XObj {
public:
	virtual ~XObj2D() { }
	virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj); // _0C
};

// the menus' pointer, an XObj2D with a little on top: it spins. class name from RTTI
class Cursor : public XObj2D {
public:
	virtual ~Cursor() { }
	virtual void InitializeObject();                       // _14C
	virtual void ResetObjectState();                       // _154
	virtual BOOL LoadFromPlacement(const CoordInfo& info); // _158
	virtual void UpdateObject(u32 frames);                 // _164
	virtual void DrawObject();                             // _174

	f32 mAngle;      // _9A8
	f32 mAngleSpeed; // _9AC
};
STATIC_ASSERT(sizeof(Cursor) == 0x9B0);

#endif
