#include "game/wii/cgame.h"
#include <Dolphin/gx.h>
#include "Dolphin/OS/OSFastCast.h"

// One function of a larger file: Start's 1.0f and 0.0f (0x805A2B6C, 0x805A2B70) are also used by
// fn_80035DC8 through fn_80036314, so the unit cannot be linked until those are written too.

extern "C" {
u32 SCCheckStatus();
u8 SCGetLanguage();
void DVDInit();
void DVDSetAutoFatalMessaging(BOOL enable);
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

// NONMATCHING: retail computes &mPad for mPad.Init before it stores mLanguage = 0, this build after
void CBase::Start()
{
	while (SCCheckStatus() != 0) { }
	OSInitFastCast();
	DVDInit();
	DVDSetAutoFatalMessaging(TRUE);
	DVDChangeDir("/cb/");
	fn_80199AE4();
	fn_800355F4(this);
	fn_8003568C(this);
	fn_80035AD0(this);
	fn_802AAF98(gGame._0000 + 8);
	mLanguage = 0;
	mPad.Init(1);
	KPADSetPosParam(gWiiPads[0].mPort, 0.07f, 0.8f);
	KPADSetHoriParam(gWiiPads[0].mPort, 0.05f, 1.0f);
	KPADSetDistParam(gWiiPads[0].mPort, 0.0f, 1.0f);
	fn_80036694(&mSound, 0xA00000);
	srand(OSGetTick());
	GXSetMisc((GXMiscToken)1, 8);
}
