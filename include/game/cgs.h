#ifndef _GAME_CGS_H
#define _GAME_CGS_H

#include "Version.h"

#include "game/hsd_cobj.h"
#include "game/cprimitive.h"
#include "game/cfont.h"
extern "C" {
#include <sysdolphin/tev.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/texp.h>

// hsd's gx headers don't have these two, the tev stage setup needs them
void GXSetNumTexGens(u8 nTexGens);
void GXSetTexCoordGen2(u32 dst_coord, u32 func, u32 src_param, u32 mtx, u8 normalize, u32 pt_texmtx);
}

// the game's graphics state. class names are RTTI, layout from the constructor at
// 0x8001B9B8. CBase derives from it, so every CGs call is on the game object gGame.
// the wii version splits the screen and channel parts out into cgs_scrn.cpp and cgs_chan.cpp

class CCamera;
class CLight;

// base classes are from RTTI

class CGsScreen : public GXRenderModeObj {
public:
	CGsScreen() { mMode = -1; }

	s32 mMode; // _3C, index into the mode table, -1 before the first set
#if defined(VERSION_PAL)
	f32 mHeightScale; // _40, europe only, scales by the xfb height
	                  // _44 vtable
#else
	// _40 vtable
#endif

	virtual ~CGsScreen() { } // _08
};
#if defined(VERSION_PAL)
STATIC_ASSERT(sizeof(CGsScreen) == 0x48);
#else
STATIC_ASSERT(sizeof(CGsScreen) == 0x44);
#endif

// CGsRenderMode::Init args
struct GsBlendMode {
	s32 mType;      // _00
	s32 mSrcFactor; // _04
	s32 mDstFactor; // _08
	s32 mLogicOp;   // _0C
};
struct GsAlphaCompare {
	s32 mComp0; // _00
	u8 mRef0;   // _04
	s32 mOp;    // _08
	s32 mComp1; // _0C
	u8 mRef1;   // _10
};
struct GsDstAlpha {
	u8 mEnable; // _00
	u8 mValue;  // _01
};
struct GsZMode {
	u8 mEnable; // _00
	s32 mFunc;  // _04
	u8 mUpdate; // _08
};
struct GsLineWidth {
	u8 mWidth;    // _00
	s32 mTexOffs; // _04
};

// pixel engine state, each part goes out to HSD when its dirty flag is set
class CGsRenderMode : public HSD_PEDesc {
public:
	BOOL mPEDirty;     // _0C
	BOOL mCullDirty;   // _10
	BOOL mLineDirty;   // _14
	BOOL mPointDirty;  // _18
	s32 mCullMode;     // _1C
	u8 mLineWidth;     // _20
	s32 mLineTexOffs;  // _24
	u8 mPointSize;     // _28
	s32 mPointTexOffs; // _2C
	// _30 vtable

	virtual ~CGsRenderMode() { } // _08

	void Invalidate()
	{
		if (!mPEDirty) {
			mPEDirty = TRUE;
		}
		if (!mCullDirty) {
			mCullDirty = TRUE;
		}
		if (!mLineDirty) {
			mLineDirty = TRUE;
		}
		if (!mPointDirty) {
			mPointDirty = TRUE;
		}
	}

	void Setup(BOOL force)
	{
		if (force || mPEDirty) {
			mPEDirty = FALSE;
			HSD_StateInvalidate(HSD_STATE_RENDER_MODE);
			HSD_SetupPEMode(0, this);
		}
		if (force || mCullDirty) {
			mCullDirty = FALSE;
			HSD_StateSetCullMode(mCullMode);
		}
		if (force || mLineDirty) {
			mLineDirty = FALSE;
			HSD_StateSetLineWidth(mLineWidth, mLineTexOffs);
		}
		if (force || mPointDirty) {
			mPointDirty = FALSE;
			HSD_StateSetPointSize(mPointSize, mPointTexOffs);
		}
	}
};
STATIC_ASSERT(sizeof(CGsRenderMode) == 0x34);

// CGsTevStage::Init args, in GXSetTevColorIn/GXSetTevColorOp order
struct GsTevOrder {
	u32 mCoord; // _00
	u32 mMap;   // _04
	u32 mColor; // _08
};
struct GsTevIn {
	u32 mA; // _00
	u32 mB; // _04
	u32 mC; // _08
	u32 mD; // _0C
};
struct GsTevOp {
	u32 mOp;    // _00
	u32 mBias;  // _04
	u32 mScale; // _08
	u8 mClamp;  // _0C
	u32 mReg;   // _10
};

// one tev stage with its texgen
class CGsTevStage : public HSD_TevDesc {
public:
	CGsTevStage();

	BOOL mStageDirty;   // _74
	BOOL mNumGensDirty; // _78
	BOOL mTexGenDirty;  // _7C
	u8 mNumTexGens;     // _80
	u32 mTexCoord;      // _84
	u32 mTexGenFunc;    // _88
	u32 mTexGenSrc;     // _8C
	u32 mTexMtx;        // _90
	// _94 vtable

	virtual ~CGsTevStage() { } // _08

	void Invalidate()
	{
		if (!mStageDirty) {
			mStageDirty = TRUE;
		}
		if (!mNumGensDirty) {
			mNumGensDirty = TRUE;
		}
		if (!mTexGenDirty) {
			mTexGenDirty = TRUE;
		}
	}

	void Setup(BOOL force)
	{
		if (force || mStageDirty) {
			mStageDirty = FALSE;
			HSD_StateInvalidate(HSD_STATE_TEV_STAGE);
			HSD_StateInvalidate(HSD_STATE_TEV_REGISTER);
			HSD_StateInvalidate(HSD_STATE_TEX_COORD_GEN);
			HSD_SetupTevStageAll(this);
		}
		if (force || mNumGensDirty) {
			HSD_StateInvalidate(HSD_STATE_TEX_COORD_GEN);
			GXSetNumTexGens(mNumTexGens);
		}
		if (force || mTexGenDirty) {
			HSD_StateInvalidate(HSD_STATE_TEV_REGISTER);
			HSD_StateInvalidate(HSD_STATE_TEX_COORD_GEN);
			GXSetTexCoordGen2(mTexCoord, mTexGenFunc, mTexGenSrc, mTexMtx, GX_FALSE, GX_PTIDENTITY);
		}
	}
	void SetOrder(s32 coord, s32 map, s32 color)
	{
		if (this->coord != coord) {
			this->coord = coord;
			Invalidate();
		}
		if (this->map != map) {
			this->map = map;
			Invalidate();
		}
		if (this->color != color) {
			this->color = color;
			Invalidate();
		}
	}
	void SetNumTexGens(u8 num)
	{
		mNumTexGens   = num;
		mNumGensDirty = TRUE;
	}
	void SetTexGen(u32 coord, u32 func, u32 src, u32 mtx)
	{
		mTexCoord    = coord;
		mTexGenFunc  = func;
		mTexGenSrc   = src;
		mTexMtx      = mtx;
		mTexGenDirty = TRUE;
	}
};
STATIC_ASSERT(sizeof(CGsTevStage) == 0x98);

// channel setup args, in HSD_Chan's order
struct GsChanCtrl {
	u8 mEnable;     // _00
	s32 mAmbSrc;    // _04
	s32 mMatSrc;    // _08
	s32 mLightMask; // _0C
	s32 mDiffFn;    // _10
	s32 mAttnFn;    // _14
};

class CGsChannel : public HSD_Chan {
public:
	CGsChannel();

	BOOL mDirty; // _30
	// _34 vtable

	virtual ~CGsChannel() { } // _08

	void Setup(BOOL force)
	{
		if (force || mDirty) {
			mDirty = FALSE;
			HSD_SetupChannelAll(this);
		}
	}
};
STATIC_ASSERT(sizeof(CGsChannel) == 0x38);

class CGs {
public:
	CGsScreen mScreen;             // _00, everything after it is 4 later in europe
	CCamera* mCamera;              // _44, made current by BeginGsCamera
	CLight* mLight;                // _48
	CGsRenderMode mRenderMode;     // _4C
	CGsTevStage mTevStages[2];     // _80
	CGsChannel mChannels[4];       // _1B0
	CGsRenderMode* mCurRenderMode; // _290, mRenderMode unless someone set their own
	CGsTevStage* mCurTevStage;     // _294
	CGsChannel* mCurChannel;       // _298
	BOOL mBillboardEnabled;        // _29C, compared signed
	CFont mFont;                   // _2A0
	CTexObj mTex;                  // _3E0
	CSprite mSprite;               // _4E0
	CLine mLine;                   // _668
	CTriangle mTriangle;           // _7D4
	CCircle mCircle;               // _940
	CTile mTile;                   // _ABC
	CSprite mSprite2;              // _C40
	CCube mCube;                   // _DC8
	COctahedron mOctahedron;       // _F30
	CDodeca mDodeca;               // _1098
	CIcosahedron mIcosahedron;     // _1200
	CSphere mSphere;               // _1368
	CCylinder mCylinder;           // _14D0
	CTorus mTorus;                 // _1638
	s32 mRotOrder;                 // _17A0, 5 after Init
#if defined(VERSION_JP)
	// _17A4 vtable, japan has no sjis switch or glyph widths
#else
	s32 mUseShiftJis;          // _17A4, Init's argument
	f32 mShiftJisWidthScale;   // _17A8
	f32 mSingleByteWidthScale; // _17AC
	                           // _17B0 vtable, _17B4 in europe
#endif

	CGs();
	virtual ~CGs() { } // _08

#if !defined(VERSION_JP)
	Vec mTextRot; // _17B4, _17B8 in europe, the text rotation, cleared by the light setup
#endif
};
#if defined(VERSION_JP)
STATIC_ASSERT(sizeof(CGs) == 0x17A8);
#elif defined(VERSION_PAL)
STATIC_ASSERT(sizeof(CGs) == 0x17C4);
#else
STATIC_ASSERT(sizeof(CGs) == 0x17C0);
#endif

// game/cgame.h, the game base holds a CGs first
class CBase;

extern "C" {
// text, plain and printf style
void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h);
void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...);
void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height);
void DrawGsFixedWidthTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...);
void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...);
// text widths
f32 MeasureGameTextHeight(CGs* gs, const char* text, f32 size);
f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size);

// shapes, sprites and vertex lists
void SetGsFixedGlyphWidthScale(CGs* gs, f32 width);
void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides);
void DrawGsCylinder(CGs* gs, u32 color, u8 sides, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor);
void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx,
                        f32 ry, f32 rz);
void DrawGsQuad(CGs* gs, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2, u32 color2,
                      f32 x3, f32 y3, f32 z3, u32 color3);
void DrawGsTriangleVertices(CGs* gs, GXPrimitive type, Mtx view, PrimVertex* vtx, u16 count);
void DrawGsColoredTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32* color, u16 count);
void DrawGsTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32 color, u16 count);
void DrawGsColoredLineStrip(CGs* gs, Vec* pos, u32* color, u16 count);
void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count);
void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1);

// sets the lighting channel mode and color
s32 SetGsChannelMaterialAndLighting(CGs* gs, s32 matSrc, u32 rgba);
void EndGsCamera(CGs* gs);
MtxPtr BeginGsCamera(CBase* gs);
void SetGsChannelEnabled(CGs* gs, s32 chan, u8 enable);
void ConfigureGsChannel(CGs* gs, CGsChannel* ch, s32 chan, GsChanCtrl ctrl, GXColor amb, GXColor mat);
void ConfigureGsTevStage(CGs* gs, CGsTevStage* stage, u32 id, GsTevOrder order, GsTevOp colorOp, GsTevIn colorIn, GsTevOp alphaOp,
                         GsTevIn alphaIn);
void ConfigureGsRenderMode(CGs* gs, CGsRenderMode* mode, u8 colorUpdate, u8 alphaUpdate, GsBlendMode blend, GsAlphaCompare compare,
                           GsDstAlpha dstAlpha, GsZMode zmode, u8 beforeTex, u8 dither, s32 cull, GsLineWidth line, GsLineWidth point);
#if defined(VERSION_JP)
void InitializeGsRenderDefaultsJP(CGs* gs);
#else
void InitializeGsRenderDefaults(CGs* gs, s32 arg);
#endif
void SetGsScreenModeAndRenderDefaults(CGs* gs, s32 mode, u16 hor, u16 ver, s32 arg);

// the channel, tev stage, render mode and screen setters
void ApplyGsChannel(CGsChannel* chan, BOOL force);
void ApplyGsTevStage(CGsTevStage* stage, BOOL force);
void SetGsTevMode(CGsTevStage* stage, s32 mode);
void SetGsTevOrder(CGsTevStage* stage, s32 coord, s32 map, s32 color);
void InitializeGsTevStageDescriptor(CGsTevStage* stage, u32 id, GsTevOrder order, GsTevOp colorOp, GsTevIn colorIn, GsTevOp alphaOp,
                                    GsTevIn alphaIn);
void ApplyGsRenderMode(CGsRenderMode* mode, BOOL force);
void SetGsLineWidth(CGsRenderMode* mode, u8 width, s32 texOffs);
void SetGsCullMode(CGsRenderMode* mode, s32 cull);
void SetGsDepthCompareBeforeTexture(CGsRenderMode* mode, u8 beforeTex);
void SetGsDestinationAlpha(CGsRenderMode* mode, u8 enable, u8 value);
void SetGsDepthMode(CGsRenderMode* mode, u8 enable, s32 func, u8 update);
void SetGsAlphaCompare(CGsRenderMode* mode, s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1);
void SetGsBlendMode(CGsRenderMode* mode, s32 type, s32 src, s32 dst, s32 op);
void SetGsAlphaUpdate(CGsRenderMode* mode, u8 enable);
void SetGsColorUpdate(CGsRenderMode* mode, u8 enable);
void InitializeGsRenderModeDescriptor(CGsRenderMode* mode, u8 colorUpdate, u8 alphaUpdate, GsBlendMode blend, GsAlphaCompare compare,
                                      GsDstAlpha dstAlpha, GsZMode zmode, u8 beforeTex, u8 dither, s32 cull, GsLineWidth line,
                                      GsLineWidth point);
void ConfigureGsScreenMode(CGsScreen* screen, s32 mode, u16 hor, u16 ver);
}

#endif
