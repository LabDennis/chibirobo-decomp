#ifndef GAME_WII_CGS_H
#define GAME_WII_CGS_H

#include "game/ctpl.h"
#include "game/ut.h"
#include <Dolphin/gx.h>

class CCamera;
struct TextChar;

// The Wii graphics state layer. Class names from RTTI; the GS* classes each hold the arguments of
// one GX call and are handed around by value. Their vtable sits behind the data.

class GSBlendMode {
public:
	GXBlendMode mType;  // _00
	GXBlendFactor mSrc; // _04
	GXBlendFactor mDst; // _08
	GXLogicOp mOp;      // _0C
	// _10 vtable

	GSBlendMode(GXBlendMode type, GXBlendFactor src, GXBlendFactor dst, GXLogicOp op)
	{
		mType = type;
		mSrc  = src;
		mDst  = dst;
		mOp   = op;
		GXSetBlendMode(type, src, dst, op);
	}
	virtual ~GSBlendMode() { } // _08
};

class GSZMode {
public:
	GXBool mEnable;  // _00
	GXCompare mFunc; // _04
	GXBool mUpdate;  // _08
	// _0C vtable

	GSZMode(GXBool enable, GXCompare func, GXBool update)
	{
		mEnable = enable;
		mFunc   = func;
		mUpdate = update;
		GXSetZMode(enable, func, update);
	}
	virtual ~GSZMode() { } // _08
};

class GSAlphaCompare {
public:
	GXCompare mComp0; // _00
	u8 mRef0;         // _04
	GXAlphaOp mOp;    // _08
	GXCompare mComp1; // _0C
	u8 mRef1;         // _10
	// _14 vtable

	GSAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1)
	{
		mComp0 = comp0;
		mRef0  = ref0;
		mOp    = op;
		mComp1 = comp1;
		mRef1  = ref1;
		GXSetAlphaCompare(comp0, ref0, op, comp1, ref1);
	}
	virtual ~GSAlphaCompare() { } // _08
};

class GSColorUpdate {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSColorUpdate(GXBool enable)
	{
		mEnable = enable;
		GXSetColorUpdate(enable);
	}
	virtual ~GSColorUpdate() { } // _08
};

class GSAlphaUpdate {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSAlphaUpdate(GXBool enable)
	{
		mEnable = enable;
		GXSetAlphaUpdate(enable);
	}
	virtual ~GSAlphaUpdate() { } // _08
};

class GSDstAlpha {
public:
	GXBool mEnable; // _00
	u8 mAlpha;      // _01
	// _04 vtable

	GSDstAlpha(GXBool enable, u8 alpha)
	{
		mEnable = enable;
		mAlpha  = alpha;
		GXSetDstAlpha(enable, alpha);
	}
	virtual ~GSDstAlpha() { } // _08
};

class GSZCompLoc {
public:
	GXBool mBeforeTex; // _00
	// _04 vtable

	GSZCompLoc(GXBool beforeTex)
	{
		mBeforeTex = beforeTex;
		GXSetZCompLoc(beforeTex);
	}
	virtual ~GSZCompLoc() { } // _08
};

class GSDither {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSDither(GXBool enable)
	{
		mEnable = enable;
		GXSetDither(enable);
	}
	virtual ~GSDither() { } // _08
};

class GSLineWidth {
public:
	u8 mWidth;           // _00
	GXTexOffset mOffset; // _04
	// _08 vtable

	GSLineWidth(u8 width, GXTexOffset offset)
	{
		mWidth  = width;
		mOffset = offset;
		GXSetLineWidth(width, offset);
	}
	virtual ~GSLineWidth() { } // _08
};

class GSPointSize {
public:
	u8 mSize;            // _00
	GXTexOffset mOffset; // _04
	// _08 vtable

	GSPointSize(u8 size, GXTexOffset offset)
	{
		mSize   = size;
		mOffset = offset;
		GXSetPointSize(size, offset);
	}
	virtual ~GSPointSize() { } // _08
};

class GSCullMode {
public:
	GXCullMode mMode; // _00
	// _04 vtable

	GSCullMode(GXCullMode mode)
	{
		mMode = mode;
		GXSetCullMode(mode);
	}
	virtual ~GSCullMode() { } // _08
};

class GSClipMode {
public:
	GXClipMode mMode; // _00
	// _04 vtable

	GSClipMode(GXClipMode mode)
	{
		mMode = mode;
		GXSetClipMode(mode);
	}
	virtual ~GSClipMode() { } // _08
};

// the tev classes carry no stage, so they are applied later, by CGsTevStage
class GSTevOrder {
public:
	GXTexCoordID mCoord; // _00
	GXTexMapID mMap;     // _04
	GXChannelID mColor;  // _08
	// _0C vtable

	GSTevOrder(GXTexCoordID coord, GXTexMapID map, GXChannelID color)
	{
		mCoord = coord;
		mMap   = map;
		mColor = color;
	}
	virtual ~GSTevOrder() { } // _08
};

class GSTevColorOp {
public:
	GXTevOp mOp;       // _00
	GXTevBias mBias;   // _04
	GXTevScale mScale; // _08
	GXBool mClamp;     // _0C
	GXTevRegID mReg;   // _10
	// _14 vtable

	GSTevColorOp(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg)
	{
		mOp    = op;
		mScale = scale;
		mBias  = bias;
		mClamp = clamp;
		mReg   = reg;
	}
	virtual ~GSTevColorOp() { } // _08
};

class GSTevColorIn {
public:
	GXTevColorArg mA; // _00
	GXTevColorArg mB; // _04
	GXTevColorArg mC; // _08
	GXTevColorArg mD; // _0C
	// _10 vtable

	GSTevColorIn(GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d)
	{
		mA = a;
		mB = b;
		mC = c;
		mD = d;
	}
	virtual ~GSTevColorIn() { } // _08
};

class GSTevAlphaOp {
public:
	GXTevOp mOp;       // _00
	GXTevBias mBias;   // _04
	GXTevScale mScale; // _08
	GXBool mClamp;     // _0C
	GXTevRegID mReg;   // _10
	// _14 vtable

	GSTevAlphaOp(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg)
	{
		mOp    = op;
		mScale = scale;
		mBias  = bias;
		mClamp = clamp;
		mReg   = reg;
	}
	virtual ~GSTevAlphaOp() { } // _08
};

class GSTevAlphaIn {
public:
	GXTevAlphaArg mA; // _00
	GXTevAlphaArg mB; // _04
	GXTevAlphaArg mC; // _08
	GXTevAlphaArg mD; // _0C
	// _10 vtable

	GSTevAlphaIn(GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d)
	{
		mA = a;
		mB = b;
		mC = c;
		mD = d;
	}
	virtual ~GSTevAlphaIn() { } // _08
};

class GSChanCtrl {
public:
	GXBool mEnable;      // _00
	GXColorSrc mAmbSrc;  // _04
	GXColorSrc mMatSrc;  // _08
	u32 mLightMask;      // _0C
	GXDiffuseFn mDiffFn; // _10
	GXAttnFn mAttnFn;    // _14
	// _18 vtable

	GSChanCtrl(GXBool enable, GXColorSrc ambSrc, GXColorSrc matSrc, u32 lightMask, GXDiffuseFn diffFn, GXAttnFn attnFn)
	{
		mEnable    = enable;
		mAmbSrc    = ambSrc;
		mMatSrc    = matSrc;
		mLightMask = lightMask;
		mDiffFn    = diffFn;
		mAttnFn    = attnFn;
	}
	virtual ~GSChanCtrl() { } // _08
};

// The three kinds of state a CGs keeps a current one of. Their members are the GS* objects above, one after
// the other; they are written out as plain fields here, with the GS* vtable words as gaps.

// the pixel engine settings
class CGsRenderState {
public:
	GXBlendMode mBlendType;   // _00
	GXBlendFactor mSrcFactor; // _04
	GXBlendFactor mDstFactor; // _08
	GXLogicOp mLogicOp;       // _0C
	u8 _10[4];
	GXBool mZEnable;  // _14
	GXCompare mZFunc; // _18
	GXBool mZUpdate;  // _1C
	u8 _1D[3];
	u8 _20[4];
	GXCompare mComp0; // _24
	u8 mRef0;         // _28
	u8 _29[3];
	GXAlphaOp mAop;   // _2C
	GXCompare mComp1; // _30
	u8 mRef1;         // _34
	u8 _35[0x3C - 0x35];
	GXBool mColorUpdate; // _3C
	u8 _3D[0x44 - 0x3D];
	GXBool mAlphaUpdate; // _44
	u8 _45[0x64 - 0x45];
	u8 mLineWidth;           // _64
	GXTexOffset mLineOffset; // _68
	u8 _6C[4];
	u8 mPointSize;            // _70
	GXTexOffset mPointOffset; // _74
	u8 _78[4];
	GXCullMode mCullMode; // _7C
	u8 _80[4];
	GXClipMode mClipMode; // _84
	u8 _88[0x90 - 0x88];

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
	void SetColorUpdate(GXBool enable)
	{
		mColorUpdate = enable;
		GXSetColorUpdate(enable);
	}
	void SetAlphaUpdate(GXBool enable)
	{
		mAlphaUpdate = enable;
		GXSetAlphaUpdate(enable);
	}
	void SetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp aop, GXCompare comp1, u8 ref1)
	{
		mComp0 = comp0;
		mRef0  = ref0;
		mAop   = aop;
		mComp1 = comp1;
		mRef1  = ref1;
		GXSetAlphaCompare(comp0, ref0, aop, comp1, ref1);
	}
	void SetLineWidth(u8 width, GXTexOffset offset)
	{
		mLineWidth  = width;
		mLineOffset = offset;
		GXSetLineWidth(width, offset);
	}
	void SetPointSize(u8 pointSize, GXTexOffset offset)
	{
		mPointSize   = pointSize;
		mPointOffset = offset;
		GXSetPointSize(pointSize, offset);
	}
	void SetCullMode(GXCullMode mode)
	{
		mCullMode = mode;
		GXSetCullMode(mode);
	}
	void SetClipMode(GXClipMode mode)
	{
		mClipMode = mode;
		GXSetClipMode(mode);
	}
	void Apply(); // 0x802A9CE4
};

// in GXSetTevColorIn order
struct WiiGsTevIn {
	u32 mA; // _00
	u32 mB; // _04
	u32 mC; // _08
	u32 mD; // _0C
};

// one texgen row of a tev stage, the SetTexGen arguments in order
struct WiiGsTexGen {
	u32 mCoord; // _00
	u32 mFunc;  // _04
	u32 mSrc;   // _08
	u32 mMtx;   // _0C
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
class CGsTevStage {
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
	u8 _74[0xB8 - 0x74];
	u8 mNumTexGens; // _B8
	u8 _B9[0xC0 - 0xB9];
	WiiGsTexGen mTexGen; // _C0
	u8 _D0[0xE0 - 0xD0];

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
	void SetNumTexGens(u8 num) { mNumTexGens = num; }
	void SetTexGen(u32 coord, u32 func, u32 src, u32 mtx)
	{
		mTexGen.mCoord = coord;
		mTexGen.mFunc  = func;
		mTexGen.mSrc   = src;
		mTexGen.mMtx   = mtx;
	}
	void Apply(); // 0x802AA0DC
};
STATIC_ASSERT(sizeof(CGsTevStage) == 0xE0);

class CGsChannel {
public:
	u32 mId;    // _00
	u8 mEnable; // _04
	u8 _05[0x30 - 0x05];
	CGsChannel* mNext; // _30
	u8 _34[4];

	void Apply(); // 0x802AA4CC
};
STATIC_ASSERT(sizeof(CGsChannel) == 0x38);

// the render mode in use, RTTI gives the base as "_GXRenderModeObj"
class CGsScreen : public GXRenderModeObj {
public:
	// _3C vtable

	virtual ~CGsScreen(); // _08

	// the SDK render mode with that index in the screen mode table
	GXRenderModeObj* GetRenderMode(s32 mode); // 0x802A977C
};
STATIC_ASSERT(sizeof(CGsScreen) == 0x40);

// one entry of the scene draw list the CGs keeps at _1BA4; vf038 pops a queued draw
class WiiGsSceneObj {
public:
	virtual void vf008();
	virtual void vf00C();
	virtual void vf010();
	virtual void vf014();
	virtual void vf018();
	virtual void vf01C();
	virtual void vf020();
	virtual void vf024();
	virtual void vf028();
	virtual void vf02C();
	virtual void vf030();
	virtual void vf034();
	virtual void vf038(s32 index);

	u8 _004[0xE4 - 0x004];
	s32 mPending; // _E4, the queued draws still waiting
};

class CGs {
public:
	void* mFifoBuffer; // _00
	GXFifoObj* mFifo;  // _04
	CGsScreen mScreen; // _08
	CCamera* mCamera;  // _48, the camera the 2D draws take their view matrix from
	u8 _4C[4];
	CGsRenderState mRenderState;     // _50, the CGs's own state, current unless another one was set
	CGsTevStage mTevStages[2];       // _E0
	CGsChannel mChannels[4];         // _2A0
	CGsRenderState* mCurRenderState; // _380
	CGsTevStage* mCurTevStage;       // _384
	CGsChannel* mCurChannel;         // _388
	BOOL mBillboardEnabled;          // _38C
	u8 _390[0x1B80 - 0x390];
	TextChar* mTextChars;      // _1B80, 256 entries allocated by Init
	s32 mUseShiftJis;          // _1B84
	f32 mShiftJisWidthScale;   // _1B88
	f32 mSingleByteWidthScale; // _1B8C
	// _1B90 vtable

	virtual ~CGs(); // _08

	Vec mTextRot;              // _1B94, the text rotation, cleared by the light setup
	u32 mTextFont;             // _1BA0, the font argument handed to the glyph lookups
	WiiGsSceneObj* mSceneHead; // _1BA4, the list the scene pass drains
	u8 _1BA8[0x2290 - 0x1BA8];

	void Init(u32 renderStates, u32 tevStages, u32 channels, u32 texGens); // 0x802AA610
	void SetScreenMode(s32 mode, s32 arg2, s32 arg3);                      // 0x802AAF6C
	void InitFrameBuffers();                                               // 0x802AAF98
	// each takes the object to fill, or NULL for the CGs's own one, and makes it the current one
	void SetRenderState(CGsRenderState* state, GSBlendMode blend, GSZMode zMode, GSAlphaCompare alphaCompare, GSColorUpdate colorUpdate,
	                    GSAlphaUpdate alphaUpdate, GSDstAlpha dstAlpha, GSZCompLoc zCompLoc, GSDither dither, GSLineWidth lineWidth,
	                    GSPointSize pointSize, GSCullMode cullMode, GSClipMode clipMode); // 0x802AAFC4
	void SetTevStage(CGsTevStage* stage, s32 index, GSTevOrder order, GSTevColorOp colorOp, GSTevColorIn colorIn, GSTevAlphaOp alphaOp,
	                 GSTevAlphaIn alphaIn);                                                                   // 0x802AB1DC
	void SetChannel(CGsChannel* channel, GXChannelID id, GSChanCtrl ctrl, GXColor ambient, GXColor material); // 0x802AB344
};

// GXSetFog's arguments, kept so the fog can be put back
class CFog {
public:
	GXFogType mType; // _00
	f32 mStartZ;     // _04
	f32 mEndZ;       // _08
	f32 mNearZ;      // _0C
	f32 mFarZ;       // _10
	CColor mColor;   // _14
	// _1C vtable

	virtual ~CFog(); // _08

	void Init(GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ); // 0x802A8FF8
};
STATIC_ASSERT(sizeof(CFog) == 0x20);

// a GX light with its place in a list. The Init* names follow the GameCube light types
class CLight : public GXLightObj {
public:
	u32 mFlags;     // _40, 1 ambient, 2 infinite, 4 point, 8 spot
	CVec mPosition; // _44
	CVec mInterest; // _54
	// _64 vtable

	virtual ~CLight();                                                                       // _08
	virtual void InitAmbient(u8 r, u8 g, u8 b, u8 a);                                        // _0C
	virtual void InitInfinite(const Vec& position, u8 r, u8 g, u8 b, u8 a);                  // _10
	virtual void InitPoint(const Vec& position, u8 r, u8 g, u8 b, u8 a);                     // _14
	virtual void InitSpot(const Vec& position, const Vec& interest, u8 r, u8 g, u8 b, u8 a); // _18
	virtual void InitAmbient(u32 color);                                                     // _1C
	virtual void InitInfinite(const Vec& position, u32 color);                               // _20
	virtual void InitPoint(const Vec& position, u32 color);                                  // _24
	virtual void InitSpot(const Vec& position, const Vec& interest, u32 color);              // _28
	virtual void vf2C();                                                                     // _2C, sets flag 0x8000
	virtual void vf30() { mFlags &= ~0x8000; }                                               // _30
	virtual void SetPosition(f32 x, f32 y, f32 z)                                            // _34
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z); // _38
	virtual void SetPosition(const Vec& position); // _3C
	virtual void SetInterest(const Vec& interest); // _40
	virtual void SetColor(u8 r, u8 g, u8 b, u8 a); // _44

	CLight* mNext; // _68
};
STATIC_ASSERT(sizeof(CLight) == 0x6C);

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
void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx,
                        f32 ry, f32 rz);
void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2, u32 color2,
                      f32 x3, f32 y3, f32 z3, u32 color3);
void DrawGsTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32 color, u16 count);
void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count);
void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1);
void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height);
void DrawGsFixedWidthTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...);
void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...);
void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h);
void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...);
// text extents
f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size);
f32 MeasureGameTextHeight(CGs* gs, const char* text, f32 size);
// applies the channel material and lighting setup for the primitive draws
s32 SetGsChannelMaterialAndLighting(CGs* gs, s32 matSrc, u32 rgba);
void SetGsChannelEnabled(CGs* gs, s32 chan, u8 enable);
}

#endif
