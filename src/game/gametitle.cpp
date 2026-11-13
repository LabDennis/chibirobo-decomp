#include "game/hsd_jobj.h"
#include "game/title.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__5Title[];
extern void* __vt__6Cursor[];
extern void* __vt__4XObj[];
extern void* __vt__9CXObjIntp[];
extern void* __vt__5CXObj[];
extern void* __vt__8XObjDemo[];
extern void* __vt__7CTexObj[];
CLight* __dt__6CLightFv(CLight* light, s32 flags);
void* __dt__7CTexObjFv(void* tex, s32 flags);
XObjDemo* __dt__8XObjDemoFv(XObjDemo* demo, s32 flags);
Cursor* __dt__6CursorFv(Cursor* cursor, s32 flags);
CXObjIntp* __dt__9CXObjIntpFv(CXObjIntp* object, s32 flags);
CXObj* __dt__5CXObjFv(CXObj* object, s32 flags);
}

/**
 * @note Address: 0x80008650
 * @note Size: 0xCC
 */
// todo: weak copy
extern "C" Title* __dt__5TitleFv(Title* title, s32 flags)
{
	if (title != NULL) {
#if defined(VERSION_JP)
		*(void***)((u8*)title + 0x3C24) = __vt__5Title; // _3C24 vtable
#else
		*(void***)((u8*)title + 0x3C48) = __vt__5Title; // _3C48 vtable
#endif
		__destroy_arr(title->mTex, (ConstructorDestructor*)__dt__7CTexObjFv, sizeof(CTexObj), 6);
		title->mTpl.CTpl::~CTpl();
		__dt__8XObjDemoFv(&title->mDemo, -1);
		__dt__6CursorFv(&title->mCursor, -1);
		if ((s16)flags > 0)
			::operator delete(title);
	}
	return title;
}

/**
 * @note Address: 0x8000871C
 * @note Size: 0x60
 */
// todo: weak copy
extern "C" Cursor* __dt__6CursorFv(Cursor* cursor, s32 flags)
{
	if (cursor != NULL) {
		*(void***)((u8*)cursor + 0x34) = __vt__6Cursor;
		__dt__4XObjFv(cursor, 0);
		if ((s16)flags > 0)
			::operator delete(cursor);
	}
	return cursor;
}

/**
 * @note Address: 0x8000877C
 * @note Size: 0x168
 */
// todo: weak copy
extern "C" XObj* __dt__4XObjFv(XObj* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x34) = __vt__4XObj;
		object->mAnim.CJAnim::~CJAnim();
		object->mAnmQueue.CAnmQueue::~CAnmQueue();
		object->mEvt.CEvt::~CEvt();
		object->_794.CVec::~CVec();
		object->mLastPos.CVec::~CVec();
		object->mAttr.AttrInfo::~AttrInfo();
		object->mCoord.CoordInfo::~CoordInfo();
		object->mStatus.CStatus64::~CStatus64();
		__dt__9CXObjIntpFv(object, 0);
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

/**
 * @note Address: 0x800088E4
 * @note Size: 0xB4
 */
// todo: weak copy
extern "C" CXObjIntp* __dt__9CXObjIntpFv(CXObjIntp* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x34) = __vt__9CXObjIntp;
		object->mIntp.CJObj::~CJObj();
		__dt__5CXObjFv(object, 0);
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

/**
 * @note Address: 0x80008998
 * @note Size: 0x1A0
 */
// todo: weak copy
extern "C" CXObj* __dt__5CXObjFv(CXObj* object, s32 flags)
{
	if (object != NULL) {
		*(void***)((u8*)object + 0x34) = __vt__5CXObj;
		object->mMoveScale.CObjVec::~CObjVec();
		object->mMoveRotate.CObjVec::~CObjVec();
		object->mMoveTranslate.CObjVec::~CObjVec();
		object->mArchive.CJArchive::~CJArchive();
		object->CJObj::~CJObj();
		if ((s16)flags > 0)
			::operator delete(object);
	}
	return object;
}

/**
 * @note Address: 0x80008B38
 * @note Size: 0xF4
 */
// todo: weak copy
extern "C" XObjDemo* __dt__8XObjDemoFv(XObjDemo* demo, s32 flags)
{
	if (demo != NULL) {
		*(void***)((u8*)demo + 0x34) = __vt__8XObjDemo;
		__destroy_arr(demo->mLights, (ConstructorDestructor*)__dt__6CLightFv, sizeof(CLight), 8);
		demo->mCamera.CCamera::~CCamera();
		__dt__4XObjFv(demo, 0);
		if ((s16)flags > 0)
			::operator delete(demo);
	}
	return demo;
}

/**
 * @note Address: 0x80008C2C
 * @note Size: 0x48
 */
// todo: weak copy
// void*, the c++ ~CTexObj the inlined dtors above use has the same name
extern "C" void* __dt__7CTexObjFv(void* tex, s32 flags)
{
	if (tex != NULL) {
		*(void***)((u8*)tex + 0xFC) = __vt__7CTexObj;
		if ((s16)flags > 0)
			::operator delete(tex);
	}
	return tex;
}
