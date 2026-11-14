#ifndef GAME_WII_CGS_H
#define GAME_WII_CGS_H

#include "game/ctpl.h"
#include <Dolphin/gx.h>

// The Wii graphics state. The GameCube CGs goes through HSD; this one keeps the settings itself and hands
// them to GX. Only the members the reconstructed code reaches are here, the names follow the GameCube ones.

// the pixel engine settings
class WiiGsRenderMode {
public:
	GXBlendMode mBlendType;   // _00
	GXBlendFactor mSrcFactor; // _04
	GXBlendFactor mDstFactor; // _08
	GXLogicOp mLogicOp;       // _0C
	u8 _10[4];
	GXBool mZEnable;  // _14
	GXCompare mZFunc; // _18
	GXBool mZUpdate;  // _1C

	void SetBlendMode(GXBlendMode type, GXBlendFactor src, GXBlendFactor dst, GXLogicOp op)
	{
		mBlendType = type;
		mSrcFactor = src;
		mDstFactor = dst;
		mLogicOp   = op;
		GXSetBlendMode(type, src, dst, op);
	}
	void SetZMode(GXBool enable, GXCompare func, GXBool update)
	{
		mZEnable = enable;
		mZFunc   = func;
		mZUpdate = update;
		GXSetZMode(enable, func, update);
	}
};

// in GXSetTevColorIn order
struct WiiGsTevIn {
	u32 mA; // _00
	u32 mB; // _04
	u32 mC; // _08
	u32 mD; // _0C
};

// in GXSetTevColorOp order, as the GameCube GsTevOp. Every known call passes 0 for bias and scale, so
// which of the two words is which is not confirmed
struct WiiGsTevOp {
	u32 mOp;    // _00
	u32 mBias;  // _04
	u32 mScale; // _08
	u8 mClamp;  // _0C
	u32 mReg;   // _10
};

// one tev stage: either one of the GXSetTevOp modes, or the color and alpha blocks
class WiiGsTevStage {
public:
	u8 _00[4];
	u32 mCoord; // _04
	u32 mMap;   // _08
	u32 mColor; // _0C
	u8 _10[4];
	BOOL mCustom;    // _14, set up from the blocks below instead of mMode
	GXTevMode mMode; // _18
	u8 _1C[4];
	WiiGsTevOp mColorOp; // _20
	u8 _34[4];
	WiiGsTevIn mColorIn; // _38
	u8 _48[4];
	WiiGsTevOp mAlphaOp; // _4C
	u8 _60[4];
	WiiGsTevIn mAlphaIn; // _64

	void SetOrder(u32 coord, u32 map, u32 color)
	{
		mCoord = coord;
		mMap   = map;
		mColor = color;
	}
	void SetMode(GXTevMode mode)
	{
		mMode   = mode;
		mCustom = FALSE;
	}
	void SetColorIn(u32 a, u32 b, u32 c, u32 d)
	{
		mColorIn.mA = a;
		mColorIn.mB = b;
		mColorIn.mC = c;
		mColorIn.mD = d;
		mCustom     = TRUE;
	}
	void SetColorOp(u32 op, u32 bias, u32 scale, u8 clamp, u32 reg)
	{
		mColorOp.mOp    = op;
		mColorOp.mScale = scale;
		mColorOp.mBias  = bias;
		mColorOp.mClamp = clamp;
		mColorOp.mReg   = reg;
		mCustom         = TRUE;
	}
	void SetAlphaIn(u32 a, u32 b, u32 c, u32 d)
	{
		mAlphaIn.mA = a;
		mAlphaIn.mB = b;
		mAlphaIn.mC = c;
		mAlphaIn.mD = d;
		mCustom     = TRUE;
	}
	void SetAlphaOp(u32 op, u32 bias, u32 scale, u8 clamp, u32 reg)
	{
		mAlphaOp.mOp    = op;
		mAlphaOp.mScale = scale;
		mAlphaOp.mBias  = bias;
		mAlphaOp.mClamp = clamp;
		mAlphaOp.mReg   = reg;
		mCustom         = TRUE;
	}
};

class WiiGs {
public:
	u8 _000[8];
	GXRenderModeObj mScreen; // _008
	u8 _044[0x380 - 0x44];
	WiiGsRenderMode* mCurRenderMode; // _380
	WiiGsTevStage* mCurTevStage;     // _384
};
STATIC_ASSERT(sizeof(WiiGs) == 0x388);

// a frame buffer copy and the texture that shows it; the GameCube FbTexture with the Wii CTexObj and a
// flag for who owns the image
struct WiiFbTexture {
	void* mImage;     // _00
	u16 mWidth;       // _04
	u16 mHeight;      // _06
	GXTexFmt mFormat; // _08
	u32 _0C;          // _0C
	f32 mX;           // _10
	f32 mY;           // _14
	u8 mMipmap;       // _18, copies the efb at twice the size
	CTexObj mTex;     // _1C
	BOOL mOwnsImage;  // _120, mImage came from the heap and is freed with the texture
};
STATIC_ASSERT(sizeof(WiiFbTexture) == 0x124);

extern "C" {
// image NULL allocates one under the given name
void CreateFbTexture(WiiFbTexture* texture, u16 width, u16 height, GXTexFmt format, BOOL mipmap, void* image, const char* name);
void FreeFbTexture(WiiFbTexture* texture);
void CopyFbTexture(WiiFbTexture* texture, GXTexFmt format, BOOL clear, BOOL flush);
void DrawFbTexture(WiiFbTexture* texture, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsTexturedQuad(WiiGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx,
                        f32 ry, f32 rz);
}

#endif
