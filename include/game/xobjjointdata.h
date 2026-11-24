#ifndef _GAME_XOBJJOINTDATA_H
#define _GAME_XOBJJOINTDATA_H

#include "Version.h"
#include "game/xobjdata.h"
#if !defined(VERSION_R24J01)
#include "game/hsd_jobj.h"
#endif

// an object an event tied to one of the spider's joints
struct SpiderTie {
#if defined(VERSION_R24J01)
	nw4r::g3d::ResNode mJoint; // _00
#else
	HSD_JObj* mJoint; // _00
#endif
	XObj* mObj; // _04
	Mtx mMtx;   // _08
};
STATIC_ASSERT(sizeof(SpiderTie) == 0x38);

struct XObjJointPart {
#if defined(VERSION_R24J01)
	nw4r::g3d::ResNode mJoint; // _00
#else
	HSD_JObj* mJoint; // _00
#endif
	XObj* mObj; // _04
	Mtx mMtx;   // _08

#if !defined(VERSION_R24J01)
	template <class Owner>
	static inline void UpdateMatrix(Owner& owner, u32 i)
	{
		Mtx translate;
		const CVec& pos = owner.mParts[i].mObj->mMoveTranslate;
		PSMTXTrans(translate, pos.x, pos.y, pos.z);
		PSMTXConcat(HSD_JObjGetMtxPtr(owner.mParts[i].mJoint), translate, owner.mParts[i].mMtx);
		HSD_JObjCopyMtx(owner.mParts[i].mObj->mJObj, owner.mParts[i].mMtx);
	}
#endif
};
STATIC_ASSERT(sizeof(XObjJointPart) == 0x38);

#endif
