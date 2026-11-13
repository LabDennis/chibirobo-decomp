#include "game/hsd_jobj.h"
#include "game/title.h"

extern "C" {
extern void* __vt__6Cursor[];
XObj* __ct__4XObjFv(XObj* obj);
}

// todo: weak copy
/**
 * @note Address: 0x8001AF70
 * @note Size: 0x3C
 */
extern "C" Cursor* __ct__6CursorFv(Cursor* cursor)
{
	__ct__4XObjFv(cursor);
	*(void***)((u8*)cursor + 0x34) = __vt__6Cursor; // _34 vtable
	return cursor;
}
