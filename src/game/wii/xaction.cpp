// chibi robo's action base and what follows it, .text 0x800C6428-0x800C6584 (wii_units.md Nr. 57), the
// GameCube's xaction.cpp: XAction::Reset leads it, a static initializer (0x800C64E4) ends it.
// NONMATCHING: only XAction::Reset and XAction::Init have source so far.
#include "game/wii/cgame.h"

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
