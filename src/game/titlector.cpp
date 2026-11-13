#include "game/hsd_jobj.h"
#include "game/menuex.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

extern "C" {
extern void* __vt__10MenuCursor[];
extern void* __vt__5Title[];
extern void* __vt__5CFile[];
extern void* __vt__4CTpl[];
extern void* __vt__7CTexObj[];
extern void* __vt__8XObjDemo[];
extern void* __vt__7CCamera[];
extern void* __vt__4CVec[];
Cursor* __ct__6CursorFv(Cursor* cursor);
XObj* __ct__4XObjFv(XObj* obj);
XObjDemo* __ct__8XObjDemoFv(XObjDemo* demo);
CTexObj* __ct__7CTexObjFv(CTexObj* tex);
CLight* __ct__6CLightFv(CLight* light);
void* __dt__7CTexObjFv(void* tex, s32 flags);
void* __dt__6CLightFv(void* light, s32 flags);
}

// todo: weak copy
/**
 * @note Address: 0x8001B3CC
 * @note Size: 0x3C
 */
extern "C" MenuCursor* __ct__10MenuCursorFv(MenuCursor* cursor)
{
	__ct__6CursorFv(cursor);
	*(void***)((u8*)cursor + 0x34) = __vt__10MenuCursor; // _34 vtable
	return cursor;
}

// todo: weak copy
/**
 * @note Address: 0x8001B408
 * @note Size: 0xAC
 */
extern "C" Title* __ct__5TitleFv(Title* title)
{
#if defined(VERSION_JP)
	*(void***)((u8*)title + 0x3C24) = __vt__5Title; // _3C24 vtable
#else
	*(void***)((u8*)title + 0x3C48) = __vt__5Title; // _3C48 vtable
#endif
	__ct__6CursorFv(&title->mCursor);
	__ct__8XObjDemoFv(&title->mDemo);
	CTpl* tpl                          = &title->mTpl;
	*(void***)((u8*)tpl + 0xE8)        = __vt__5CFile; // the inlined CFile ctor
	tpl->mArchive                      = NULL;
	tpl->mEntrynum                     = -1;
	tpl->mBuffer                       = NULL;
	tpl->mResult                       = 0;
	tpl->mState                        = 1;
	*(void***)((u8*)tpl + 0xE8)        = __vt__4CTpl;
	*(void***)((u8*)&tpl->mTex + 0xFC) = __vt__7CTexObj;
	__construct_array(title->mTex, (ConstructorDestructor)__ct__7CTexObjFv, (ConstructorDestructor)__dt__7CTexObjFv, sizeof(CTexObj), 6);
	return title;
}

// todo: weak copy
/**
 * @note Address: 0x8001B4B4
 * @note Size: 0x84
 */
extern "C" XObjDemo* __ct__8XObjDemoFv(XObjDemo* demo)
{
	__ct__4XObjFv(demo);
	*(void***)((u8*)demo + 0x34)            = __vt__8XObjDemo; // _34 vtable
	*(void***)((u8*)&demo->mCamera + 0x190) = __vt__7CCamera;
	*(void***)((u8*)&demo->mCamera + 0xA0)  = __vt__4CVec;
	*(void***)((u8*)&demo->mCamera + 0xB0)  = __vt__4CVec;
	*(void***)((u8*)&demo->mCamera + 0xC0)  = __vt__4CVec;
	*(void***)((u8*)&demo->mCamera + 0xD0)  = __vt__4CVec;
	*(void***)((u8*)&demo->mCamera + 0x124) = __vt__4CVec;
	__construct_array(demo->mLights, (ConstructorDestructor)__ct__6CLightFv, (ConstructorDestructor)__dt__6CLightFv, sizeof(CLight), 8);
	return demo;
}
