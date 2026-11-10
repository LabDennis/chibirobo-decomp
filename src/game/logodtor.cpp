#include "game/hsd_jobj.h"
#include "game/logo.h"
#include "game/cscrn.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__4Logo[];
extern void* __vt__9CScrnFade[];
}

// todo: weak copy
/**
 * @note Address: 0x80018D28
 * @note Size: 0x90
 */
extern "C" Logo* __dt__4LogoFv(Logo* logo, s32 flags)
{
	if (logo != NULL) {
		*(void***)((u8*)logo + 0x200) = __vt__4Logo; // _200 vtable
		logo->mTpl.CTpl::~CTpl();
		if ((s16)flags > 0) {
			::operator delete(logo);
		}
	}
	return logo;
}

// todo: weak copy
/**
 * @note Address: 0x80018DB8
 * @note Size: 0x5C
 */
extern "C" CScrnFade* __dt__9CScrnFadeFv(CScrnFade* fade, s32 flags)
{
	if (fade != NULL) {
		*(void***)((u8*)fade + 0x30) = __vt__9CScrnFade; // _30 vtable
		fade->CScrnColor::~CScrnColor();
		if ((s16)flags > 0) {
			::operator delete(fade);
		}
	}
	return fade;
}
