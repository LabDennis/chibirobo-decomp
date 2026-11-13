#ifndef _GAME_CSCRN_H
#define _GAME_CSCRN_H

#include "game/ctpl.h"
#include "game/ut.h"

class CGs;

// full screen color, fade and wipe effects, CGame holds one of each. class names from
// RTTI, layouts are partial

// a color in floats so it can step a fraction per frame
struct ScrnColorF {
	f32 r; // _00
	f32 g; // _04
	f32 b; // _08
	f32 a; // _0C
};

// a flat color over the whole screen that fades to its target
class CScrnColor {
public:
	ScrnColorF mColor;  // _00
	ScrnColorF mTarget; // _10
	ScrnColorF mSpeed;  // _20, per frame, all zero once it's done
	// _30 vtable

	virtual ~CScrnColor() { } // _08 (weak)

	CScrnColor() { ResetChannels(0.0f); }

private:
	void ResetChannels(f32 value)
	{
		mColor.r  = value;
		mColor.g  = value;
		mColor.b  = value;
		mColor.a  = value;
		mTarget.r = value;
		mTarget.g = value;
		mTarget.b = value;
		mTarget.a = value;
		mSpeed.r  = value;
		mSpeed.g  = value;
		mSpeed.b  = value;
		mSpeed.a  = value;
	}
};
typedef char CScrnColor_size_check[sizeof(CScrnColor) == 0x34 ? 1 : -1];

class CScrnFade : public CScrnColor {
public:
	virtual ~CScrnFade() { } // _08 (weak)
	virtual void Reset();    // _0C
};

// a color with a hole that opens or closes, the subclasses pick the shape
class CScrnWipe : public CScrnColor {
public:
	virtual ~CScrnWipe() { }              // _08 (weak)
	virtual void Reset();                 // _0C, Reset__9CScrnWipeFv
	virtual void Update(u32 frames);      // _10, Update__9CScrnWipeFUl
	virtual void Draw(CGs* gs, s32 mode); // _14, Draw__9CScrnWipeFP3CGsl

	BOOL mActive;       // _34
	BOOL mHideWhenDone; // _38, turns itself off once the size stops
	f32 mX;             // _3C
	f32 mY;             // _40
	f32 mWidth;         // _44
	f32 mHeight;        // _48
	f32 mTargetWidth;   // _4C
	f32 mTargetHeight;  // _50
	f32 mSpeedWidth;    // _54, zero once the wipe is done
	f32 mSpeedHeight;   // _58
	f32 mRotation; // _5C, angle of the hole in radians, advanced by mSpeedRotation and wrapped at 2 pi in Update; Tex and Tile draws rotate
	               // by it; the eye camera writes its yaw into gGame.mRoboeyes
	f32 mSpeedRotation; // _60, radians per frame Update adds to mRotation, skipped when 0; nothing in the decompiled code sets it to
	                    // anything but 0
};
typedef char CScrnWipe_size_check[sizeof(CScrnWipe) == 0x64 ? 1 : -1];

class CScrnWipeCircle : public CScrnWipe {
public:
	virtual ~CScrnWipeCircle() { } // _08 (weak)
};

class CScrnWipeTex : public CScrnWipe {
public:
	virtual ~CScrnWipeTex() { } // _08 (weak)
	void Draw(CGs* gs, s32 mode);

	CTexObj mTex; // _64
};
typedef char CScrnWipeTex_size_check[sizeof(CScrnWipeTex) == 0x164 ? 1 : -1];

class CScrnWipeCustom : public CScrnWipe {
public:
	virtual ~CScrnWipeCustom() { } // _08 (weak)
	void Draw(CGs* gs, s32 mode);
};

class CScrnWipeTile : public CScrnWipe {
public:
	virtual ~CScrnWipeTile() { } // _08 (weak)
	void Draw(CGs* gs, s32 mode);
};

// chibi robo's eyes closing over the screen
class CScrnRoboeyes : public CScrnWipe {
public:
	virtual ~CScrnRoboeyes() { }          // _08 (weak)
	virtual void Reset();                 // _0C, Reset__13CScrnRoboeyesFv
	virtual void Update(u32 frames);      // _10, Update__13CScrnRoboeyesFUl
	virtual void Draw(CGs* gs, s32 mode); // _14, Draw__13CScrnRoboeyesFP3CGsl

	f32 mAngle; // _64
	f32 mPhase; // _68
};
typedef char CScrnRoboeyes_size_check[sizeof(CScrnRoboeyes) == 0x6C ? 1 : -1];

// shakes the screen, the amount steps to the target and flips sign every frame
class CScrnQuake {
public:
	CVec mShake[3]; // _00, the amount, its target and its per frame speed
	s32 mSign;      // _30
	// _34 vtable

	virtual ~CScrnQuake() { } // _08 (weak)
};
typedef char CScrnQuake_size_check[sizeof(CScrnQuake) == 0x38 ? 1 : -1];

extern "C" {
// CScrnColor
void DrawScreenColorOverlay(CScrnColor* color, CGs* gs);
void UpdateScreenFade(CScrnColor* color, u32 frames);
void SetScreenFadeTargetColor(CScrnColor* color, u8 r, u8 g, u8 b, u8 a, f32 time);
void StartScreenFade(CScrnColor* color, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, u8 a1, f32 time);
// CScrnWipe
void StartScreenWipeColorTransition(CScrnWipe* wipe, u8 r0, u8 g0, u8 b0, u8 a0, u8 r1, u8 g1, u8 b1, f32 x, f32 y, f32 width0, f32 height0,
                                    f32 width1, f32 height1, f32 time, u8 a1, BOOL hideWhenDone);
void StartScreenWipe(CScrnWipe* wipe, u8 r, u8 g, u8 b, u8 a, BOOL hideWhenDone, f32 x, f32 y, f32 width0, f32 height0, f32 width1,
                     f32 height1, f32 time);
// CScrnRoboeyes
// CScrnQuake
void UpdateScreenShake(CScrnQuake* quake, u32 frames);
void StartScreenShake(CScrnQuake* quake, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, f32 time);
void ResetScreenShake(CScrnQuake* quake);
}

#endif
