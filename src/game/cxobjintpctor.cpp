#include "game/cobj.h"

extern "C" {
extern void* __vt__9CXObjIntp[];
extern void* __vt__5CJObj[];
extern void* __vt__4CVec[];
extern void* __vt__5CXObj[];
extern void* __vt__9CJArchive[];
extern void* __vt__7CObjVec[];
CXObj* __ct__5CXObjFv(CXObj* obj);
}

// todo: weak copy
/**
 * @note Address: 0x8001B150
 * @note Size: 0x8C
 */
extern "C" CXObjIntp* __ct__9CXObjIntpFv(CXObjIntp* obj)
{
	__ct__5CXObjFv(obj);
	*(void***)((u8*)obj + 0x34)                   = __vt__9CXObjIntp; // _34 vtable
	*(void***)((u8*)&obj->mIntp + 0x34)           = __vt__5CJObj;
	*(void***)((u8*)&obj->mIntp.mTranslate + 0xC) = __vt__4CVec;
	*(void***)((u8*)&obj->mIntp.mRotate + 0xC)    = __vt__4CVec;
	*(void***)((u8*)&obj->mIntp.mScale + 0xC)     = __vt__4CVec;
	obj->mIntp.mJObj                              = NULL;
	const f32 zero                                = 0.0f;
	obj->mIntp.mTranslate.x                       = zero;
	obj->mIntp.mTranslate.y                       = zero;
	obj->mIntp.mTranslate.z                       = zero;
	obj->mIntp.mRotate.x                          = zero;
	obj->mIntp.mRotate.y                          = zero;
	obj->mIntp.mRotate.z                          = zero;
	obj->mIntp.mScale.x                           = zero;
	obj->mIntp.mScale.y                           = zero;
	obj->mIntp.mScale.z                           = zero;
	return obj;
}

// todo: weak copy
/**
 * @note Address: 0x8001B1DC
 * @note Size: 0x110
 */
extern "C" CXObj* __ct__5CXObjFv(CXObj* obj)
{
	*(void***)((u8*)obj + 0x34)                             = __vt__5CJObj; // _34 vtable
	*(void***)((u8*)&obj->mTranslate + 0xC)                 = __vt__4CVec;
	*(void***)((u8*)&obj->mRotate + 0xC)                    = __vt__4CVec;
	*(void***)((u8*)&obj->mScale + 0xC)                     = __vt__4CVec;
	obj->mJObj                                              = NULL;
	const f32 zero                                          = 0.0f;
	obj->mTranslate.x                                       = zero;
	obj->mTranslate.y                                       = zero;
	obj->mTranslate.z                                       = zero;
	obj->mRotate.x                                          = zero;
	obj->mRotate.y                                          = zero;
	obj->mRotate.z                                          = zero;
	obj->mScale.x                                           = zero;
	obj->mScale.y                                           = zero;
	obj->mScale.z                                           = zero;
	*(void***)((u8*)obj + 0x34)                             = __vt__5CXObj;
	*(void***)((u8*)&obj->mArchive + sizeof(CJArchive) - 4) = __vt__9CJArchive;
	*(void***)((u8*)&obj->mMoveTranslate + 0xC)             = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveTranslate + 0xC)             = __vt__7CObjVec;
	*(void***)((u8*)&obj->mMoveTranslate.mTarget + 0xC)     = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveTranslate.mSpeed + 0xC)      = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveRotate + 0xC)                = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveRotate + 0xC)                = __vt__7CObjVec;
	*(void***)((u8*)&obj->mMoveRotate.mTarget + 0xC)        = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveRotate.mSpeed + 0xC)         = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveScale + 0xC)                 = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveScale + 0xC)                 = __vt__7CObjVec;
	*(void***)((u8*)&obj->mMoveScale.mTarget + 0xC)         = __vt__4CVec;
	*(void***)((u8*)&obj->mMoveScale.mSpeed + 0xC)          = __vt__4CVec;
	obj->mMoveTranslate.x                                   = zero;
	obj->mMoveTranslate.y                                   = zero;
	obj->mMoveTranslate.z                                   = zero;
	obj->mMoveTranslate.mTarget.x                           = zero;
	obj->mMoveTranslate.mTarget.y                           = zero;
	obj->mMoveTranslate.mTarget.z                           = zero;
	obj->mMoveTranslate.mSpeed.x                            = zero;
	obj->mMoveTranslate.mSpeed.y                            = zero;
	obj->mMoveTranslate.mSpeed.z                            = zero;
	obj->mMoveRotate.x                                      = zero;
	obj->mMoveRotate.y                                      = zero;
	obj->mMoveRotate.z                                      = zero;
	obj->mMoveRotate.mTarget.x                              = zero;
	obj->mMoveRotate.mTarget.y                              = zero;
	obj->mMoveRotate.mTarget.z                              = zero;
	obj->mMoveRotate.mSpeed.x                               = zero;
	obj->mMoveRotate.mSpeed.y                               = zero;
	obj->mMoveRotate.mSpeed.z                               = zero;
	obj->mMoveScale.x                                       = zero;
	obj->mMoveScale.y                                       = zero;
	obj->mMoveScale.z                                       = zero;
	obj->mMoveScale.mTarget.x                               = zero;
	obj->mMoveScale.mTarget.y                               = zero;
	obj->mMoveScale.mTarget.z                               = zero;
	obj->mMoveScale.mSpeed.x                                = zero;
	obj->mMoveScale.mSpeed.y                                = zero;
	obj->mMoveScale.mSpeed.z                                = zero;
	return obj;
}
