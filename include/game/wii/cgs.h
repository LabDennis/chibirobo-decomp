#ifndef GAME_WII_CGS_H
#define GAME_WII_CGS_H

#include "game/cprimitive.h"
#include "game/ctpl.h"
#include "game/cfont.h"
#include "game/ut.h"
#include <Dolphin/gx.h>

class CCamera;
class CLight;
struct TextChar;
struct PrimVertex;

// The Wii graphics state layer. Class names from RTTI; the GS* classes each hold the arguments of
// one GX call and are handed around by value. Their vtable sits behind the data.

class GSBlendMode {
public:
	GXBlendMode mType;  // _00
	GXBlendFactor mSrc; // _04
	GXBlendFactor mDst; // _08
	GXLogicOp mOp;      // _0C
	// _10 vtable

	GSBlendMode() { }
	GSBlendMode(GXBlendMode type, GXBlendFactor src, GXBlendFactor dst, GXLogicOp op) { Set(type, src, dst, op); }
	void Set(GXBlendMode type, GXBlendFactor src, GXBlendFactor dst, GXLogicOp op)
	{
		mType = type;
		mSrc  = src;
		mDst  = dst;
		mOp   = op;
		GXSetBlendMode(type, src, dst, op);
	}
	virtual ~GSBlendMode() { } // _08

	void Set(GSBlendMode src); // 0x802A9BAC
};

class GSZMode {
public:
	GXBool mEnable;  // _00
	GXCompare mFunc; // _04
	GXBool mUpdate;  // _08
	// _0C vtable

	GSZMode() { }
	GSZMode(GXBool enable, GXCompare func, GXBool update) { Set(enable, func, update); }
	void Set(GXBool enable, GXCompare func, GXBool update)
	{
		mEnable = enable;
		mFunc   = func;
		mUpdate = update;
		GXSetZMode(enable, func, update);
	}
	virtual ~GSZMode() { } // _08

	void Set(GSZMode src); // 0x802A9BD8
};

class GSAlphaCompare {
public:
	GXCompare mComp0; // _00
	u8 mRef0;         // _04
	GXAlphaOp mOp;    // _08
	GXCompare mComp1; // _0C
	u8 mRef1;         // _10
	// _14 vtable

	GSAlphaCompare() { }
	GSAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1) { Set(comp0, ref0, op, comp1, ref1); }
	void Set(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1)
	{
		mComp0 = comp0;
		mRef0  = ref0;
		mOp    = op;
		mComp1 = comp1;
		mRef1  = ref1;
		GXSetAlphaCompare(comp0, ref0, op, comp1, ref1);
	}
	virtual ~GSAlphaCompare() { } // _08

	void Set(GSAlphaCompare src); // 0x802A9BFC
};

class GSColorUpdate {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSColorUpdate() { }
	GSColorUpdate(GXBool enable) { Set(enable); }
	void Set(GXBool enable)
	{
		mEnable = enable;
		GXSetColorUpdate(enable);
	}
	virtual ~GSColorUpdate() { } // _08

	void Set(GSColorUpdate src); // 0x802A9C30
};

class GSAlphaUpdate {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSAlphaUpdate() { }
	GSAlphaUpdate(GXBool enable) { Set(enable); }
	void Set(GXBool enable)
	{
		mEnable = enable;
		GXSetAlphaUpdate(enable);
	}
	virtual ~GSAlphaUpdate() { } // _08

	void Set(GSAlphaUpdate src); // 0x802A9C40
};

class GSDstAlpha {
public:
	GXBool mEnable; // _00
	u8 mAlpha;      // _01
	// _04 vtable

	GSDstAlpha() { }
	GSDstAlpha(GXBool enable, u8 alpha) { Set(enable, alpha); }
	void Set(GXBool enable, u8 alpha)
	{
		mEnable = enable;
		mAlpha  = alpha;
		GXSetDstAlpha(enable, alpha);
	}
	virtual ~GSDstAlpha() { } // _08

	void Set(GSDstAlpha src); // 0x802A9C50
};

class GSZCompLoc {
public:
	GXBool mBeforeTex; // _00
	// _04 vtable

	GSZCompLoc() { }
	GSZCompLoc(GXBool beforeTex) { Set(beforeTex); }
	void Set(GXBool beforeTex)
	{
		mBeforeTex = beforeTex;
		GXSetZCompLoc(beforeTex);
	}
	virtual ~GSZCompLoc() { } // _08

	void Set(GSZCompLoc src); // 0x802A9C6C
};

class GSDither {
public:
	GXBool mEnable; // _00
	// _04 vtable

	GSDither() { }
	GSDither(GXBool enable) { Set(enable); }
	void Set(GXBool enable)
	{
		mEnable = enable;
		GXSetDither(enable);
	}
	virtual ~GSDither() { } // _08

	void Set(GSDither src); // 0x802A9C7C
};

class GSLineWidth {
public:
	u8 mWidth;           // _00
	GXTexOffset mOffset; // _04
	// _08 vtable

	GSLineWidth() { }
	GSLineWidth(u8 width, GXTexOffset offset) { Set(width, offset); }
	void Set(u8 width, GXTexOffset offset)
	{
		mWidth  = width;
		mOffset = offset;
		GXSetLineWidth(width, offset);
	}
	virtual ~GSLineWidth() { } // _08

	void Set(GSLineWidth src); // 0x802A9C8C
};

class GSPointSize {
public:
	u8 mSize;            // _00
	GXTexOffset mOffset; // _04
	// _08 vtable

	GSPointSize() { }
	GSPointSize(u8 size, GXTexOffset offset) { Set(size, offset); }
	void Set(u8 size, GXTexOffset offset)
	{
		mSize   = size;
		mOffset = offset;
		GXSetPointSize(size, offset);
	}
	virtual ~GSPointSize() { } // _08

	void Set(GSPointSize src); // 0x802A9CA8
};

class GSCullMode {
public:
	GXCullMode mMode; // _00
	// _04 vtable

	GSCullMode() { }
	GSCullMode(GXCullMode mode) { Set(mode); }
	void Set(GXCullMode mode)
	{
		mMode = mode;
		GXSetCullMode(mode);
	}
	virtual ~GSCullMode() { } // _08

	void Set(GSCullMode src); // 0x802A9CC4
};

class GSClipMode {
public:
	GXClipMode mMode; // _00
	// _04 vtable

	GSClipMode() { }
	GSClipMode(GXClipMode mode) { Set(mode); }
	void Set(GXClipMode mode)
	{
		mMode = mode;
		GXSetClipMode(mode);
	}
	virtual ~GSClipMode() { } // _08

	void Set(GSClipMode src); // 0x802A9CD4
};

// the tev classes carry no stage, so they are applied later, by CGsTevStage
class GSTevOrder {
public:
	GXTexCoordID mCoord; // _00
	GXTexMapID mMap;     // _04
	GXChannelID mColor;  // _08
	// _0C vtable

	GSTevOrder() { }
	GSTevOrder(GXTexCoordID coord, GXTexMapID map, GXChannelID color) { Set(coord, map, color); }
	void Set(GXTexCoordID coord, GXTexMapID map, GXChannelID color)
	{
		mCoord = coord;
		mMap   = map;
		mColor = color;
	}
	virtual ~GSTevOrder() { } // _08

	void Set(GSTevOrder src); // 0x802A9D9C
};

class GSTevColorOp {
public:
	GXTevOp mOp;       // _00
	GXTevBias mBias;   // _04
	GXTevScale mScale; // _08
	GXBool mClamp;     // _0C
	GXTevRegID mReg;   // _10
	// _14 vtable

	GSTevColorOp() { }
	GSTevColorOp(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg) { Set(op, bias, scale, clamp, reg); }
	void Set(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg)
	{
		mOp    = op;
		mScale = scale;
		mBias  = bias;
		mClamp = clamp;
		mReg   = reg;
	}
	virtual ~GSTevColorOp() { } // _08

	void Set(GSTevColorOp src); // 0x802A9F7C
};

class GSTevColorIn {
public:
	GXTevColorArg mA; // _00
	GXTevColorArg mB; // _04
	GXTevColorArg mC; // _08
	GXTevColorArg mD; // _0C
	// _10 vtable

	GSTevColorIn() { }
	GSTevColorIn(GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d) { Set(a, b, c, d); }
	void Set(GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d)
	{
		mA = a;
		mB = b;
		mC = c;
		mD = d;
	}
	virtual ~GSTevColorIn() { } // _08

	void Set(GSTevColorIn src); // 0x802A9FA8
};

class GSTevAlphaOp {
public:
	GXTevOp mOp;       // _00
	GXTevBias mBias;   // _04
	GXTevScale mScale; // _08
	GXBool mClamp;     // _0C
	GXTevRegID mReg;   // _10
	// _14 vtable

	GSTevAlphaOp() { }
	GSTevAlphaOp(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg) { Set(op, bias, scale, clamp, reg); }
	void Set(GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID reg)
	{
		mOp    = op;
		mScale = scale;
		mBias  = bias;
		mClamp = clamp;
		mReg   = reg;
	}
	virtual ~GSTevAlphaOp() { } // _08

	void Set(GSTevAlphaOp src); // 0x802A9FCC
};

class GSTevAlphaIn {
public:
	GXTevAlphaArg mA; // _00
	GXTevAlphaArg mB; // _04
	GXTevAlphaArg mC; // _08
	GXTevAlphaArg mD; // _0C
	// _10 vtable

	GSTevAlphaIn() { }
	GSTevAlphaIn(GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d) { Set(a, b, c, d); }
	void Set(GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d)
	{
		mA = a;
		mB = b;
		mC = c;
		mD = d;
	}
	virtual ~GSTevAlphaIn() { } // _08

	void Set(GSTevAlphaIn src); // 0x802A9FF8
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

	GSChanCtrl() { }
	GSChanCtrl(GXBool enable, GXColorSrc ambSrc, GXColorSrc matSrc, u32 lightMask, GXDiffuseFn diffFn, GXAttnFn attnFn)
	{
		Set(enable, ambSrc, matSrc, lightMask, diffFn, attnFn);
	}
	void Set(GXBool enable, GXColorSrc ambSrc, GXColorSrc matSrc, u32 lightMask, GXDiffuseFn diffFn, GXAttnFn attnFn)
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

// The tev settings only CGsTevStage holds; nothing hands them around, so they have no constructor here.
// Class names from RTTI.
class GSTevMode {
public:
	GXTevMode mMode; // _00
	// _04 vtable

	virtual ~GSTevMode() { } // _08
};

class GSTevKColorSel {
public:
	GXTevKColorSel mSel; // _00
	// _04 vtable

	virtual ~GSTevKColorSel() { } // _08
};

class GSTevKAlphaSel {
public:
	GXTevKAlphaSel mSel; // _00
	// _04 vtable

	virtual ~GSTevKAlphaSel() { } // _08
};

class GSTevSwapMode {
public:
	GXTevSwapSel mRas; // _00
	GXTevSwapSel mTex; // _04
	// _08 vtable

	virtual ~GSTevSwapMode() { } // _08
};

class GSTevSwapModeTable {
public:
	GXTevSwapSel mTable;   // _00
	GXTevColorChan mRed;   // _04
	GXTevColorChan mGreen; // _08
	GXTevColorChan mBlue;  // _0C
	GXTevColorChan mAlpha; // _10
	// _14 vtable

	virtual ~GSTevSwapModeTable() { } // _08
};

// the stage's constant colors, class names from RTTI; CGsTevStage's constructor (0x80033A74) stores
// GSTevColor's vtable at _B0 and GSTevKColor's at _B4, which leaves the second no room for data
class GSTevColor {
public:
	GXColor mColor; // _00, by its name; no reader checked yet
	// _04 vtable

	virtual ~GSTevColor() { } // _08
};

class GSTevKColor {
public:
	// _00 vtable

	virtual ~GSTevKColor() { } // _08
};

class GSTexGens {
public:
	u8 mNum; // _00
	// _04 vtable

	virtual ~GSTexGens() { } // _08
};

class GSTexCoordGen {
public:
	GXTexCoordID mCoord; // _00
	GXTexGenType mFunc;  // _04
	GXTexGenSrc mSrc;    // _08
	u32 mMtx;            // _0C
	// _10 vtable

	virtual ~GSTexCoordGen() { } // _08
};

// The three kinds of state a CGs keeps a current one of. Their members are the GS* objects above, one after
// the other.

// the pixel engine settings
class CGsRenderState {
public:
	GSBlendMode mBlendMode;       // _00
	GSZMode mZMode;               // _14
	GSAlphaCompare mAlphaCompare; // _24
	GSColorUpdate mColorUpdate;   // _3C
	GSAlphaUpdate mAlphaUpdate;   // _44
	GSDstAlpha mDstAlpha;         // _4C
	GSZCompLoc mZCompLoc;         // _54
	GSDither mDither;             // _5C
	GSLineWidth mLineWidth;       // _64
	GSPointSize mPointSize;       // _70
	GSCullMode mCullMode;         // _7C
	GSClipMode mClipMode;         // _84
	// _8C vtable

	virtual ~CGsRenderState() { } // _08

	void SetBlendMode(GXBlendMode type, GXBlendFactor src, GXBlendFactor dst, GXLogicOp op)
	{
		mBlendMode.mType = type;
		mBlendMode.mSrc  = src;
		mBlendMode.mDst  = dst;
		mBlendMode.mOp   = op;
		GXSetBlendMode(type, src, dst, op);
	}
	void SetZMode(GXBool enable, GXCompare func, GXBool update)
	{
		mZMode.mEnable = enable;
		mZMode.mFunc   = func;
		mZMode.mUpdate = update;
		GXSetZMode(enable, func, update);
	}
	void SetColorUpdate(GXBool enable)
	{
		mColorUpdate.mEnable = enable;
		GXSetColorUpdate(enable);
	}
	void SetAlphaUpdate(GXBool enable)
	{
		mAlphaUpdate.mEnable = enable;
		GXSetAlphaUpdate(enable);
	}
	void SetDstAlpha(GXBool enable, u8 alpha)
	{
		mDstAlpha.mEnable = enable;
		mDstAlpha.mAlpha  = alpha;
		GXSetDstAlpha(enable, alpha);
	}
	void SetZCompLoc(GXBool beforeTex)
	{
		mZCompLoc.mBeforeTex = beforeTex;
		GXSetZCompLoc(beforeTex);
	}
	void SetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp aop, GXCompare comp1, u8 ref1)
	{
		mAlphaCompare.mComp0 = comp0;
		mAlphaCompare.mRef0  = ref0;
		mAlphaCompare.mOp    = aop;
		mAlphaCompare.mComp1 = comp1;
		mAlphaCompare.mRef1  = ref1;
		GXSetAlphaCompare(comp0, ref0, aop, comp1, ref1);
	}
	void SetLineWidth(u8 width, GXTexOffset offset)
	{
		mLineWidth.mWidth  = width;
		mLineWidth.mOffset = offset;
		GXSetLineWidth(width, offset);
	}
	void SetPointSize(u8 pointSize, GXTexOffset offset)
	{
		mPointSize.mSize   = pointSize;
		mPointSize.mOffset = offset;
		GXSetPointSize(pointSize, offset);
	}
	void SetCullMode(GXCullMode mode)
	{
		mCullMode.mMode = mode;
		GXSetCullMode(mode);
	}
	void SetClipMode(GXClipMode mode)
	{
		mClipMode.mMode = mode;
		GXSetClipMode(mode);
	}
	// takes over all twelve settings and sends each to GX
	void Set(GSBlendMode blendMode, GSZMode zMode, GSAlphaCompare alphaCompare, GSColorUpdate colorUpdate, GSAlphaUpdate alphaUpdate,
	         GSDstAlpha dstAlpha, GSZCompLoc zCompLoc, GSDither dither, GSLineWidth lineWidth, GSPointSize pointSize, GSCullMode cullMode,
	         GSClipMode clipMode); // 0x802A997C
	void Apply();                  // 0x802A9CE4
};
STATIC_ASSERT(sizeof(CGsRenderState) == 0x90);

// one tev stage: either one of the GXSetTevOp modes, or the color and alpha blocks. Stages chain
// through mNext, and the first one of a chain also carries the swap table and the texgen
class CGsTevStage {
public:
	GXTevStageID mId;                  // _00
	GSTevOrder mOrder;                 // _04
	BOOL mCustom;                      // _14, set up from the blocks below instead of mMode
	GSTevMode mMode;                   // _18
	GSTevColorOp mColorOp;             // _20
	GSTevColorIn mColorIn;             // _38
	GSTevAlphaOp mAlphaOp;             // _4C
	GSTevAlphaIn mAlphaIn;             // _64
	GSTevKColorSel mKColorSel;         // _78
	GSTevKAlphaSel mKAlphaSel;         // _80
	GSTevSwapMode mSwapMode;           // _88
	GSTevSwapModeTable mSwapModeTable; // _94
	GSTevColor mColor;                 // _AC
	GSTevKColor mKColor;               // _B4
	GSTexGens mTexGens;                // _B8
	GSTexCoordGen mTexGen;             // _C0
	CGsTevStage* mNext;                // _D4
	BOOL mIndirect;                    // _D8, the stage keeps its indirect setup; otherwise Setup sets it direct
	// _DC vtable

	virtual ~CGsTevStage() { } // _08

	void SetOrder(u32 coord, u32 map, u32 color)
	{
		mOrder.mCoord = (GXTexCoordID)coord;
		mOrder.mMap   = (GXTexMapID)map;
		mOrder.mColor = (GXChannelID)color;
	}
	void SetMode(GXTevMode mode)
	{
		mMode.mMode = mode;
		mCustom     = FALSE;
	}
	void SetColorIn(u32 a, u32 b, u32 c, u32 d)
	{
		mColorIn.mA = (GXTevColorArg)a;
		mColorIn.mB = (GXTevColorArg)b;
		mColorIn.mC = (GXTevColorArg)c;
		mColorIn.mD = (GXTevColorArg)d;
		mCustom     = TRUE;
	}
	void SetColorOp(u32 op, u32 bias, u32 scale, u8 clamp, u32 reg)
	{
		mColorOp.mOp    = (GXTevOp)op;
		mColorOp.mScale = (GXTevScale)scale;
		mColorOp.mBias  = (GXTevBias)bias;
		mColorOp.mClamp = clamp;
		mColorOp.mReg   = (GXTevRegID)reg;
		mCustom         = TRUE;
	}
	void SetAlphaIn(u32 a, u32 b, u32 c, u32 d)
	{
		mAlphaIn.mA = (GXTevAlphaArg)a;
		mAlphaIn.mB = (GXTevAlphaArg)b;
		mAlphaIn.mC = (GXTevAlphaArg)c;
		mAlphaIn.mD = (GXTevAlphaArg)d;
		mCustom     = TRUE;
	}
	void SetAlphaOp(u32 op, u32 bias, u32 scale, u8 clamp, u32 reg)
	{
		mAlphaOp.mOp    = (GXTevOp)op;
		mAlphaOp.mScale = (GXTevScale)scale;
		mAlphaOp.mBias  = (GXTevBias)bias;
		mAlphaOp.mClamp = clamp;
		mAlphaOp.mReg   = (GXTevRegID)reg;
		mCustom         = TRUE;
	}
	void SetNumTexGens(u8 num) { mTexGens.mNum = num; }
	void SetTexGen(u32 coord, u32 func, u32 src, u32 mtx)
	{
		mTexGen.mCoord = (GXTexCoordID)coord;
		mTexGen.mFunc  = (GXTexGenType)func;
		mTexGen.mSrc   = (GXTexGenSrc)src;
		mTexGen.mMtx   = mtx;
	}
	// the stage with its order and blocks, everything else at its default and no next stage
	void Set(GXTevStageID id, GSTevOrder order, GSTevColorOp colorOp, GSTevColorIn colorIn, GSTevAlphaOp alphaOp,
	         GSTevAlphaIn alphaIn);  // 0x802A9DB8
	void SetNext(CGsTevStage* next); // 0x802AA01C
	s32 GetNum();                    // 0x802AA030, how many stages are in use when this is the last one
	void Apply();                    // 0x802AA0DC, sends the whole chain
	void Setup();                    // 0x802AA18C, sends this stage
};
STATIC_ASSERT(sizeof(CGsTevStage) == 0xE0);

// one lighting channel, chained like the tev stages
class CGsChannel {
public:
	GXChannelID mId;   // _00
	GSChanCtrl mCtrl;  // _04
	CColor mAmbColor;  // _20
	CColor mMatColor;  // _28
	CGsChannel* mNext; // _30
	// _34 vtable

	virtual ~CGsChannel() { } // _08

	void SetCtrl(GXChannelID id, GSChanCtrl ctrl);                                // 0x802AA298
	void Set(GXChannelID id, GSChanCtrl ctrl, GXColor ambient, GXColor material); // 0x802AA2D4
	void SetNext(CGsChannel* next);                                               // 0x802AA418
	s32 GetNum();                                                                 // 0x802AA42C, as CGsTevStage's
	void Apply();                                                                 // 0x802AA4CC, sends the whole chain
	void Setup();                                                                 // 0x802AA534, sends this channel
};
STATIC_ASSERT(sizeof(CGsChannel) == 0x38);

// the render mode in use, RTTI gives the base as "_GXRenderModeObj"
class CGsScreen : public GXRenderModeObj {
public:
	// _3C vtable

	virtual ~CGsScreen() { } // _08

	// the SDK render mode with that index in the screen mode table
	GXRenderModeObj* GetRenderMode(s32 mode); // 0x802A977C
	void SetMode(s32 mode, u16 hor, u16 ver); // 0x802A9790, the mode with its overscan trimmed, and the copy settings
	void Configure();                         // 0x802A9860, hands the mode to VI and to g3d
	u32 GetXfbSize();                         // 0x802A9898, bytes of one external frame buffer
	u16 GetXfbWidth();                        // 0x802A98EC
	u16 GetXfbHeight();                       // 0x802A98F4, by TV format
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
	virtual void vf034(s32 index, void* model); // queues a model's draw
	virtual void vf038(s32 index);

	u8 _004[0xE4 - 0x004];
	s32 mPending; // _E4, the queued draws still waiting
	u8 _E8[0x2888 - 0xE8];
	void* _2888; // _2888, set when the stage's scene brings its own lights and fog
};

// nine of these sit in the CGs at _1FE4; only the three flag bytes the stage setup clears are known
struct WiiGsSlot {
	u8 _00;
	u8 _01[7];
	u8 _08;
	u8 _09[0xF];
	u8 _18;
	u8 _19[0xF];
};
STATIC_ASSERT(sizeof(WiiGsSlot) == 0x28);

// a GX light with its place in a list. The Init* names follow the GameCube light types
class CLight : public GXLightObj {
public:
	u32 mFlags;     // _40, 1 ambient, 2 infinite, 4 point, 8 spot
	CVec mPosition; // _44
	CVec mInterest; // _54
	// _64 vtable

	virtual ~CLight() { }                                                                    // _08
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

	void Reset(u8 r, u8 g, u8 b, u8 a); // 0x802A89E0, a cleared light of that color, shared by the four Init kinds
	CVec GetViewPosition(Mtx view);     // 0x802A8D1C
	CVec GetPosition();                 // 0x802A8DA4, an infinite light's is pushed far out along its direction
	CVec GetViewDirection(Mtx view);    // 0x802A8E64
	CColor GetColor();                  // 0x802A8F18
};
STATIC_ASSERT(sizeof(CLight) == 0x6C);

// the scene draw list and the lights it draws under, the CGs keeps it at _1BA4. The list functions
// (fn_802AEDA0 and on) take its address; the destructor null-checks it as an object of its own
struct WiiGsScene {
	WiiGsSceneObj* mHead; // _00, the list the scene pass drains
	BOOL _04;             // _04, Update queues the map's model on the list while it is set
	CLight mLights[10];   // _08
};

class CGs {
public:
	void* mFifoBuffer;               // _00
	GXFifoObj* mFifo;                // _04
	CGsScreen mScreen;               // _08
	CCamera* mCamera;                // _48, the camera the 2D draws take their view matrix from
	CLight* mLight;                  // _4C, the light list the scene draws under, CBase::SetLight stores it
	CGsRenderState mRenderState;     // _50, the CGs's own state, current unless another one was set
	CGsTevStage mTevStages[2];       // _E0
	CGsChannel mChannels[4];         // _2A0
	CGsRenderState* mCurRenderState; // _380
	CGsTevStage* mCurTevStage;       // _384
	CGsChannel* mCurChannel;         // _388
	BOOL mBillboardEnabled;          // _38C
	u8 _390[0x8CC - 0x390];
	CLine mLine;               // _8CC
	CTriangle mTriangle;       // _A38
	CCircline mCircline;       // _BA4
	CCircle mCircle;           // _D1C
	CTile mTile;               // _E98
	CSprite mSprite2;          // _101C
	CCube mCube;               // _11A4
	COctahedron mOctahedron;   // _130C
	CDodeca mDodeca;           // _1474
	CIcosahedron mIcosahedron; // _15DC
	CSphere mSphere;           // _1744
	CCylinder mCylinder;       // _18AC
	CTorus mTorus;             // _1A14
	s32 mRotOrder;             // _1B7C
	TextChar* mTextChars;      // _1B80, 256 entries allocated by Init
	s32 mUseShiftJis;          // _1B84
	f32 mShiftJisWidthScale;   // _1B88
	f32 mSingleByteWidthScale; // _1B8C
	// _1B90 vtable

	virtual ~CGs() { } // _08

	Vec mTextRot;       // _1B94, the text rotation, cleared by the light setup
	u32 mTextFont;      // _1BA0, the font argument handed to the glyph lookups
	WiiGsScene mScene;  // _1BA4
	WiiGsSlot _1FE4[9]; // _1FE4, FinalizeStageObjectSetup clears each one's three flags
	CFont mFont;        // _214C, the game font; CBase::BeforeUpdate loads its resource

	// whether the current state is the CGs's own; the draws only touch that one
	bool IsOwnTevStage() const { return mCurTevStage == &mTevStages[0]; }
	bool IsOwnChannel() const { return mCurChannel == &mChannels[0]; }
	void Init(u32 renderStates, u32 tevStages, u32 channels, u32 texGens); // 0x802AA610
	void SetScreenMode(s32 mode, u16 hor, u16 ver);                        // 0x802AAF6C, hor and ver trim the overscan
	void InitFrameBuffers();                                               // 0x802AAF98
	void StartFrameThread();                                               // 0x802AAFC0
	// each takes the object to fill, or NULL for the CGs's own one, and makes it the current one
	void SetRenderState(CGsRenderState* state, GSBlendMode blend, GSZMode zMode, GSAlphaCompare alphaCompare, GSColorUpdate colorUpdate,
	                    GSAlphaUpdate alphaUpdate, GSDstAlpha dstAlpha, GSZCompLoc zCompLoc, GSDither dither, GSLineWidth lineWidth,
	                    GSPointSize pointSize, GSCullMode cullMode, GSClipMode clipMode); // 0x802AAFC4
	void SetTevStage(CGsTevStage* stage, s32 index, GSTevOrder order, GSTevColorOp colorOp, GSTevColorIn colorIn, GSTevAlphaOp alphaOp,
	                 GSTevAlphaIn alphaIn);                                                                   // 0x802AB1DC
	void SetChannel(CGsChannel* channel, GXChannelID id, GSChanCtrl ctrl, GXColor ambient, GXColor material); // 0x802AB344
	BOOL BeginCamera();                     // 0x802AB790, loads mCamera's matrices; FALSE when there is none
	void fn_802AB81C(MtxP view, BOOL flag); // 0x802AB81C, loads mLight under view and keeps what the scene list returns
	void ResetZMode();                      // 0x802AB864, depth test and write and color update back on
	void EndFrame();                        // 0x802AB8CC, queues the frame's copy and waits for the retrace
	void SwapBuffers();                     // 0x802AB8F0
};
#ifndef offsetof
#define offsetof(type, member) ((u32) & (((type*)0)->member))
#endif
STATIC_ASSERT(offsetof(CGs, mScene) == 0x1BA4);
STATIC_ASSERT(offsetof(CGs, mFont) == 0x214C);

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

	virtual ~CFog() { } // _08

	void Init(GXFogType type, const GXColor& color, f32 startZ, f32 endZ, f32 nearZ, f32 farZ); // 0x802A8FF8
};
STATIC_ASSERT(sizeof(CFog) == 0x20);

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

// The frame buffer queue in front of the screen code (0x802A90C8-0x802A977C), not reconstructed yet;
// the names are ours
void AllocFrameBuffers(u32 size); // 0x802A90C8, the two external frame buffers and the copy queues
void StartFrameThread();          // 0x802A91FC
void QueueFrameCopy();            // 0x802A92BC
void SwapFrameBuffer();           // 0x802A93DC

extern "C" {
// image NULL allocates one under the given name
void CreateFbTexture(WiiFbTexture* texture, u16 width, u16 height, GXTexFmt format, BOOL mipmap, void* image, const char* name);
void FreeFbTexture(WiiFbTexture* texture);
void CopyFbTexture(WiiFbTexture* texture, GXTexFmt format, BOOL clear, BOOL flush);
void DrawFbTexture(WiiFbTexture* texture, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx,
                        f32 ry, f32 rz);
void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1);
void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count);
void DrawGsColoredLineStrip(CGs* gs, Vec* pos, u32* color, u16 count);
void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor);
void DrawGsCylinder(CGs* gs, u32 color, u8 sides, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz);
void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides);
void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2, u32 color2,
                      f32 x3, f32 y3, f32 z3, u32 color3);
void DrawGsQuad(CGs* gs, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz);
void DrawGsTriangleVertices(CGs* gs, GXPrimitive type, Mtx view, PrimVertex* vtx, u16 count);
void DrawGsColoredTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32* color, u16 count);
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
void EndGsCamera(CGs* gs); // 0x802AB7E4
}

#endif
