#ifndef _GAME_XOBJJOINTPART_H
#define _GAME_XOBJJOINTPART_H

#include "game/xobj.h"
#include "game/cprimitive.h"

struct XObjJointPart {
	HSD_JObj* mJoint;
	XObj* mObj;
	Mtx mMtx;
};
STATIC_ASSERT(sizeof(XObjJointPart) == 0x38);

#endif
