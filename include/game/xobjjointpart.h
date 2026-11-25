#ifndef _GAME_XOBJJOINTPART_H
#define _GAME_XOBJJOINTPART_H

#include "game/xobj.h"
#include "game/cprimitive.h"

struct XObjJointPart {
	HSD_JObj* mJoint;
	XObj* mObj;
	Mtx mMtx;

	template <class Owner>
	static inline void UpdateMatrix(Owner& owner, u32 i)
	{
		Mtx translate;
		const CVec& pos = owner.mParts[i].mObj->mMoveTranslate;
		PSMTXTrans(translate, pos.x, pos.y, pos.z);
		PSMTXConcat(HSD_JObjGetMtxPtr(owner.mParts[i].mJoint), translate, owner.mParts[i].mMtx);
		HSD_JObjCopyMtx(owner.mParts[i].mObj->mJObj, owner.mParts[i].mMtx);
	}
};
STATIC_ASSERT(sizeof(XObjJointPart) == 0x38);

#endif
