// chibi robo's action base and what follows it, .text 0x800C6428-0x800C6584 (wii_units.md Nr. 57), the
// GameCube's xaction.cpp: XAction::Reset leads it, a static initializer (0x800C64E4) ends it. The static
// initializer registers the unit's two reader globals (the GameCube keeps them in xobjs.cpp); the
// XObjReadAnm reader does not exist on the Wii.
// NONMATCHING: fn_800C6454 is XBullet::CPtclSet's first virtual (RTTI "XBullet::CPtclSet", vtable
// 0x80303240; an abstract root class with pure virtuals after it) and tail-calls slot _0C of a
// polymorphic member at _10 - XBullet is not declared yet. The __sinit writes fields the Wii XObjRead
// stand-in does not model yet (+0x318, the queue's vptr at +0x348).
#include "game/wii/cgame.h"
#include "game/wii/xobj.h"

XObjReadAll gAllObjectsReader;
XObjReadOne gSingleObjectReader;

void XAction::Reset()
{
	mState         = 0;
	mMoveDirection = 0;
	mTarget        = NULL;
	mTargetState   = 0;
}

void XAction::Init(WiiPlayerState* owner)
{
	mOwner = owner;
	Reset();
}
