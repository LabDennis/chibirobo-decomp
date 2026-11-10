#ifndef _GAME_LOGO_H
#define _GAME_LOGO_H

#include "game/ctpl.h"

// the boot logos, CGame holds one. they come from one tpl, each fades in, waits and fades
// out, the first can be skipped with a button. class name from RTTI, layout is partial
class Logo {
public:
	s32 mState; // _00, 0 off, 15 done
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
	bool IsActive() const { return mState >= 1 && mState < 15; }
	TPLDescriptor* GetDesc(s32 i) { return &((TPLPalette*)mTpl.mBuffer)->descriptorArray[i]; }
};
STATIC_ASSERT(sizeof(Logo) == 0x204);

extern "C" {
void DrawCurrentLogo(Logo* logo);
void UpdateBootLogos(Logo* logo, u32 frames);
void ResetLogoFadeState(Logo* logo);
}

#endif
