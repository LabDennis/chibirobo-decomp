#include "game/wii/cgame.h"
#include <Dolphin/gx.h>

extern "C" {
u32 SCCheckStatus();
u8 SCGetLanguage();
void DVDInit();
void DVDSetAutoFatalMessaging(BOOL enable);
BOOL fn_801905CC(const char* path);
void fn_80199AE4();
void fn_800355F4(CBase*);
void fn_8003568C(CBase*);
void fn_80035AD0(CBase*);
void fn_802AAF98(void*);
void fn_80036694(XSound*, u32);
void KPADSetPosParam(s32, f32, f32);
void KPADSetHoriParam(s32, f32, f32);
void KPADSetDistParam(s32, f32, f32);
u32 OSGetTick();
void srand(u32);
}

void CBase::Start()
{
	while (SCCheckStatus() != 0) { }
	asm {
		li r3, 4
		oris r3, r3, 4
		mtspr 914, r3
		li r3, 5
		oris r3, r3, 5
		mtspr 915, r3
		li r3, 6
		oris r3, r3, 6
		mtspr 916, r3
		li r3, 7
		oris r3, r3, 7
		mtspr 917, r3
	}
	DVDInit();
	DVDSetAutoFatalMessaging(TRUE);
	fn_801905CC("/cb/");
	fn_80199AE4();
#if defined(WII_LANGUAGE_SYSTEM)
	u8 language = SCGetLanguage();
	mLanguage   = language <= 5 ? language : 1;
#elif defined(WII_LANGUAGE)
	mLanguage = WII_LANGUAGE;
#else
	mLanguage = 0;
#endif
	fn_800355F4(this);
	fn_8003568C(this);
	fn_80035AD0(this);
	fn_802AAF98(gGame._0000 + 8);
	mPad.Init(1);
	KPADSetPosParam(gWiiPads[0].mPort, 0.07f, 0.8f);
	KPADSetHoriParam(gWiiPads[0].mPort, 0.05f, 1.0f);
	KPADSetDistParam(gWiiPads[0].mPort, 0.0f, 1.0f);
	fn_80036694(&mSound, 0xA00000);
	srand(OSGetTick());
	GXSetMisc((GXMiscToken)1, 8);
}
