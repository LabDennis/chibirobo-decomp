#include "game/xobj.h"

extern "C" {
extern void* __vt__4XObj[];
extern void* __vt__9CStatus64[];
extern void* __vt__9CoordInfo[];
extern void* __vt__8AttrInfo[];
extern void* __vt__4CVec[];
extern void* __vt__4CEvt[];
extern void* __vt__8CEvtData[];
extern void* __vt__6CStack[];
extern void* __vt__9CAnmQueue[];
extern void* __vt__9CJArchive[];
extern void* __vt__6CJAnim[];
CXObjIntp* __ct__9CXObjIntpFv(CXObjIntp* obj);
}

// todo: weak copy, CEvt's call data and stacks are still untyped in evt.h
/**
 * @note Address: 0x8001AFAC
 * @note Size: 0x100
 */
extern "C" XObj* __ct__4XObjFv(XObj* obj)
{
	__ct__9CXObjIntpFv(obj);
	*(void***)((u8*)obj + 0x34)                          = __vt__4XObj; // _34 vtable
	*(void***)((u8*)&obj->mStatus + 0x8)                 = __vt__9CStatus64;
	*(u32*)((u8*)&obj->mStatus + 0x4)                    = 0;
	*(u32*)&obj->mStatus                                 = 0;
	*(void***)((u8*)&obj->mCoord + 0x4)                  = __vt__9CoordInfo;
	obj->mCoord._00                                      = NULL;
	*(void***)((u8*)&obj->mAttr + 0x8)                   = __vt__8AttrInfo;
	obj->mAttr._00                                       = NULL;
	*(void***)((u8*)&obj->mLastPos + 0xC)                = __vt__4CVec;
	*(void***)((u8*)&obj->_794 + 0xC)                    = __vt__4CVec;
	*(void***)((u8*)&obj->mEvt + 0x64)                   = __vt__4CEvt;
	*(void***)((u8*)&obj->mEvt + 0xC)                    = __vt__8CEvtData;
	obj->mEvt.mData.mScript                              = NULL;
	obj->mEvt.mData._04                                  = 0;
	*(void***)((u8*)&obj->mEvt + 0x30)                   = __vt__6CStack;
	obj->mEvt.mCalls.mBuf                                = NULL;
	obj->mEvt.mCalls.mSize                               = 0;
	obj->mEvt.mCalls.mCount                              = 0;
	*(void***)((u8*)&obj->mEvt + 0x40)                   = __vt__6CStack;
	obj->mEvt.mFrames.mBuf                               = NULL;
	obj->mEvt.mFrames.mSize                              = 0;
	obj->mEvt.mFrames.mCount                             = 0;
	*(void***)((u8*)&obj->mEvt + 0x50)                   = __vt__6CStack;
	obj->mEvt.mValues.mBuf                               = NULL;
	obj->mEvt.mValues.mSize                              = 0;
	obj->mEvt.mValues.mCount                             = 0;
	*(void***)((u8*)&obj->mAnmQueue + 0x84)              = __vt__9CAnmQueue;
	*(void***)((u8*)&obj->mAnim + sizeof(CJArchive) - 4) = __vt__9CJArchive;
	*(void***)((u8*)&obj->mAnim + sizeof(CJArchive) - 4) = __vt__6CJAnim;
	return obj;
}
