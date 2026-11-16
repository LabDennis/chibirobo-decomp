#ifndef _GAME_WII_FBEFFECT_H
#define _GAME_WII_FBEFFECT_H

#include "game/ctpl.h"

// FbEffect::Draw's pass bits: bits 0-1 grab the efb (without one the buffer is reused), the rest add a pass
#define FBEFF_COPY_A8   0x01  // grab into the texture as alpha
#define FBEFF_COPY_I8   0x02  // grab into the texture as intensity
#define FBEFF_OUTLINE   0x04  // mThick != 0: outline the marked pixels mThick wide; 0: colour every pixel with id > 4
#define FBEFF_OUTLINE2  0x10  // mThick2 != 0: second outline in mColor2; 0: fill the id-2 pixels with mColor2, patterned by mPatternTex
#define FBEFF_OVERLAY   0x20  // blend the whole screen toward buffer intensity * mColor3, at mColor3's alpha
#define FBEFF_ZOOM_COPY 0x40  // draw the screen shrunk into the middle and grab it
#define FBEFF_ZOOM_FADE 0x80  // fade the zoomed copy over the screen
#define FBEFF_GHOSTS    0x100 // ghost copies circling the outline

// the Wii frame buffer effects CGame owns. They are the GameCube's CFbBlur and CFbOutline put on a
// common base that keeps the screen rectangle, its projection and the quad's matrix. Class names from
// RTTI (FbBase, FbBlur, FbEffect); the member function names are the GameCube's where there is one.
// fbeffect.cpp's string pool also names a haze, a mosaic and a depth-of-field effect whose code the
// linker dropped.
class FbBase {
public:
	u16 mWidth;  // _00
	u16 mHeight; // _02
	f32 mAspect; // _04, stretches the rectangle sideways
	f32 mX;      // _08
	f32 mY;      // _0C
	f32 mW;      // _10, mWidth * mAspect
	f32 mH;      // _14
	Mtx44 mProj; // _18
	Mtx mMtx;    // _58
	// _88 vtable

	virtual void Init(u16 w, u16 h, void* buf, f32 aspect);                     // _08
	virtual void Init(const GXRenderModeObj* rmode, void* buf, f32 aspect) = 0; // _0C
	virtual void Update(u32 frames)                                        = 0; // _10
	virtual void Draw()                                                    = 0; // _14
	virtual void Copy()                                                    = 0; // _18
	virtual void SetAspect(f32 aspect);                                         // _1C
	virtual void CalcMtx() = 0;                                                 // _20
	virtual ~FbBase() { }                                                       // _24 (weak)
};
STATIC_ASSERT(sizeof(FbBase) == 0x8C);

// fades the last frame over the current one
class FbBlur : public FbBase {
public:
	CTexObj mTex; // _8C, half the screen's size, RGB565
	f32 mAlpha;   // _190
	f32 mTarget;  // _194
	f32 mSpeed;   // _198
	f32 mOffsetX; // _19C
	f32 mOffsetY; // _1A0
	u8 mValid;    // _1A4

	virtual void Init(u16 w, u16 h, void* buf, f32 aspect); // _08
	// _0C, defined in game/wii/gameupdate.cpp
	virtual void Init(const GXRenderModeObj* rmode, void* buf, f32 aspect);
	virtual void Update(u32 frames); // _10
	virtual void Draw();             // _14
	virtual void Copy();             // _18
	virtual void CalcMtx();          // _20
	virtual ~FbBlur() { }            // _24 (weak)
};
STATIC_ASSERT(sizeof(FbBlur) == 0x1A8);

// the outline and its companions, drawn from the object ids in the frame buffer's alpha
class FbEffect : public FbBase {
public:
	CTexObj mTex;          // _8C, the screen's size, I8
	GXColor mColor0;       // _190, line colour, alpha is the id
	GXColor mColor2;       // _194, FBEFF_OUTLINE2
	GXColor mColor3;       // _198, FBEFF_OVERLAY, skipped if alpha is 0
	GXColor mColor4;       // _19C, FBEFF_ZOOM_FADE, skipped if alpha is 0
	u8 _1A0[4];            // _1A0
	f32 mThick;            // _1A4
	f32 mThick2;           // _1A8, sign picks the blend
	GXTexObj* mPatternTex; // _1AC
	f32 mPatternTransX;    // _1B0
	f32 mPatternTransY;    // _1B4
	f32 mPatternScaleX;    // _1B8
	f32 mPatternScaleY;    // _1BC
	s32 mFrame;            // _1C0, 0 to 3
	f32 mZoom;             // _1C4

	virtual void Init(u16 w, u16 h, void* buf, f32 aspect); // _08
	// _0C, defined in game/wii/gameupdate.cpp
	virtual void Init(const GXRenderModeObj* rmode, void* buf, f32 aspect);
	virtual void Update(u32 frames); // _10
	virtual void Draw() { }          // _14
	virtual void Copy() { }          // _18
	virtual void CalcMtx();          // _20
	virtual ~FbEffect() { }          // _24 (weak)

	void Draw(u32 flags);

	// inlined:
	void Copy(GXTexFmt format);
};
STATIC_ASSERT(sizeof(FbEffect) == 0x1C8);

#endif
