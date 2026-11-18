// chibi robo's action base and what follows it, .text 0x800C6428-0x800C6584 (wii_units.md Nr. 57), the
// GameCube's xaction.cpp: XAction::Reset leads it, a static initializer (0x800C64E4) ends it. The static
// initializer registers the unit's two reader globals (the GameCube keeps them in xobjs.cpp); the
// XObjReadAnm reader does not exist on the Wii.
// NONMATCHING: vf08__Q27XBullet8CPtclSetFv is XBullet::CPtclSet's first virtual (RTTI
// "XBullet::CPtclSet"; its home is the multi-inheritance vtable 0x80303240, an abstract root
// delegating through a member at _10) - XBullet is not declared yet. The __sinit does the same
// work as retail but our compiler doesn't fold the two reader vptrs onto one base register.
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

// the GameCube kept the readers' out-of-line destructors in actor_pool_4.cpp; on the Wii they
// live here, which is why the two reader vtables are emitted in this unit's .data
XObjReadAll::~XObjReadAll()
{
}

XObjReadOne::~XObjReadOne()
{
}
