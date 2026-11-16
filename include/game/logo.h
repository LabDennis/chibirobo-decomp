#ifndef _GAME_LOGO_H
#define _GAME_LOGO_H

#include "game/ctpl.h"

// the boot logos, CGame holds one. they come from one tpl, each fades in, waits and fades
// out, the first can be skipped with a button. class name from RTTI, layout is partial
#if defined(VERSION_R24J01)
// the file the Wii logos are read through, class name and base from RTTI
class CFileLogo : public CFile {
public:
	virtual ~CFileLogo() { }       // _08 (weak)
	virtual void* Alloc(u32 size); // _0C, the top of the MEM1 arena, still unused while the logos show
	virtual void Free();           // _10, so there is nothing to give back
};
#endif

class Logo {
public:
	s32 mState; // _00, 0 off, 15 done
#if defined(VERSION_R24J01)
	CFileLogo mFile; // _04, the Wii class has it ahead of the tpl, which moves everything behind it by 0xEC
#endif
	CTpl mTpl;  // _04
	f32 mAlpha; // _1F4
	f32 mAngle; // _1F8, turns the first logo's shine
	f32 mTimer; // _1FC, frames the current logo has shown
	// _200 vtable

	virtual ~Logo() { } // _08 (weak)

	void ResetFade()
	{
		mAlpha = 0.0f;
		mAngle = 0.0f;
		mTimer = 0.0f;
	}
#if defined(VERSION_R24J01)
	bool IsActive() const { return mState >= 1 && mState < 12; } // the Wii is done at 12
	TPLDescriptor* GetDesc(s32 i) { return &mTpl.mPalette->descriptorArray[i]; }
#else
	bool IsActive() const { return mState >= 1 && mState < 15; }
	TPLDescriptor* GetDesc(s32 i) { return &((TPLPalette*)mTpl.mBuffer)->descriptorArray[i]; }
#endif
};
#if defined(VERSION_R24J01)
STATIC_ASSERT(sizeof(Logo) == 0x2F8);
#else
STATIC_ASSERT(sizeof(Logo) == 0x204);
#endif

extern "C" {
void DrawCurrentLogo(Logo* logo);
void UpdateBootLogos(Logo* logo, u32 frames);
void ResetLogoFadeState(Logo* logo);
}

#endif
