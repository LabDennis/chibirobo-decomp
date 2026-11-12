#include "game/cgame.h"
#include "game/cgsdefaults.h"
#include "game/ccamera.h"
#include "game/clight.h"
#include "Version.h"
extern "C" {
#include <dolphin/gx/gx.h>
}

#include <stdio.h>
#include <stdarg.h>
#include "game/ccamera.h"

// the hsd headers have no vertex lists or write gather pipe, these follow the sdk ones
typedef struct _GXVtxDescList {
	GXAttr attr; // _00
	u32 type;    // _04
} GXVtxDescList;
typedef struct _GXVtxAttrFmtList {
	GXAttr attr;  // _00
	u32 cnt;      // _04
	u32 compType; // _08
	u8 frac;      // _0C
} GXVtxAttrFmtList;
typedef union {
	u32 u32;
	f32 f32;
} PPCWGPipe;
volatile PPCWGPipe GXWGFifo : 0xCC008000;

static inline void GXPosition3f32(f32 x, f32 y, f32 z)
{
	GXWGFifo.f32 = x;
	GXWGFifo.f32 = y;
	GXWGFifo.f32 = z;
}

static inline void GXColor1u32(u32 c)
{
	GXWGFifo.u32 = c;
}

extern "C" {
void GXSetVtxAttrFmtv(u32 fmt, GXVtxAttrFmtList* list);
void GXSetVtxDescv(GXVtxDescList* list);
}

extern GXRenderModeObj* gVideoRenderModes[31];
extern u8 gGlyphStrokeTable[47][13];

// sends whatever render, tev and channel state changed since the last draw
static inline void SetupDrawState(CGs* gs)
{
	if (gs->mCurRenderMode != NULL) {
		gs->mCurRenderMode->Setup(FALSE);
	}
	if (gs->mCurTevStage != NULL) {
		gs->mCurTevStage->Setup(FALSE);
	}
	if (gs->mCurChannel != NULL) {
		gs->mCurChannel->Setup(FALSE);
	}
}

// one character out of a text string, \C and \B followed by eight hex digits set the text
// and background colors and give no character
struct TextGlyph {
	char mStr[4]; // _00, one or two bytes and a terminator
	u32 mColor;   // _04
	u32 mBgColor; // _08
};

// fills glyph with the next character, returns where the one after starts or null at the end
static inline const char* GetGlyph(CGs* gs, const char* text, TextGlyph* glyph)
{
	switch (*text) {
	case '\0':
		return NULL;
	case '\n':
		glyph->mStr[0] = '\n';
		break;
	case '\\':
		text++;
		switch (*text) {
		case 'C':
		case 'c':
			glyph->mColor = ParseHexDigits(text + 1, 8);
			text += 8;
#if !defined(VERSION_JP)
			glyph->mStr[0] = '\0';
#endif
			break;
		case 'B':
		case 'b':
			glyph->mBgColor = ParseHexDigits(text + 1, 8);
			text += 8;
#if !defined(VERSION_JP)
			glyph->mStr[0] = '\0';
#endif
			break;
		default:
#if !defined(VERSION_JP)
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
#endif
			break;
		}
#if defined(VERSION_JP)
		glyph->mStr[0] = '\0';
#endif
		break;
	default:
#if defined(VERSION_JP)
		if (IsSjisLead(*text) & 1) {
#else
		if (gs->mUseShiftJis && (IsSjisLead(*text) & 1)) {
#endif
			glyph->mStr[0] = *text;
			glyph->mStr[1] = *++text;
		} else {
			glyph->mStr[0] = *text;
			glyph->mStr[1] = '\0';
		}
		break;
	}
	return text + 1;
}

static inline void DebugVertex(f32 x, f32 y, u32 color)
{
	f32 depth = 0.0f;
	GXPosition3f32(x, y, depth);
	GXColor1u32(color);
}

// a stroke point is on a grid of 0.11 steps inside the character cell
static inline f32 StrokePos(s32 n)
{
	f32 pos = 0.11f * n;
	return 0.2f + pos;
}

#include <stdio.h>
#include <stdarg.h>
#include "game/ccamera.h"

// hsd's gx headers stop short of these
enum {
	GX_TEXCOORD0     = 0,
	GX_TEXCOORD_NULL = 0xFF,
};
enum {
	GX_TG_MTX2x4 = 1,
};
enum {
	GX_TG_TEX0 = 4,
};
enum {
	GX_MODULATE = 0,
	GX_PASSCLR  = 4,
};

// one character's sprite and its background tile, the text draw keeps 256 of them
struct TextChar {
	CTexObj mTex;    // _00
	CSprite mSprite; // _100
	CTile mBg;       // _288
	u32 mHasBg;      // _40C
};

// SetGsTevMode, inlined
static inline void SetTevMode(CGsTevStage* stage, s32 mode)
{
	if (stage->flags != 0) {
		stage->flags = 0;
		stage->Invalidate();
	}
	if (stage->u.tevop.tevmode != mode) {
		stage->u.tevop.tevmode = mode;
		stage->Invalidate();
	}
}

static inline void SetBillboard(CPrimitive* prim, BOOL on)
{
	if (on) {
		prim->mBillboard = 1;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty = TRUE;
}

// where the quad sits in its own unit space and its four corner colors
static inline void SetCenter(CSprite* sprite, f32 x, f32 y, f32 z)
{
	sprite->mCenter.x = x;
	sprite->mCenter.y = y;
	sprite->mCenter.z = z;
}

static inline void SetCenter(CTile* tile, f32 x, f32 y, f32 z)
{
	tile->mCenter.x = x;
	tile->mCenter.y = y;
	tile->mCenter.z = z;
}

static inline void SetColor(CSprite* sprite, u32 color)
{
	sprite->_16C = sprite->_170 = sprite->_174 = sprite->_178 = color;
}

static inline void SetColor(CTile* tile, u32 color)
{
	tile->_168 = tile->_16C = tile->_170 = tile->_174 = color;
}

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}

// places a flat shape on screen, w and h are its full size. with camera type 3 y points
// the other way, so the shape gets flipped back
static inline void SetFlatTransform(CGs* gs, CPrimitive* prim, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	if (gs->mCamera->projection_type == 3) {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, -h / 2.0f, 1.0f);
		prim->SetRot(-rx, ry, -rz);
	} else {
		prim->SetTrans(x, y, z);
		prim->SetScale(w / 2.0f, h / 2.0f, 1.0f);
		prim->SetRot(rx, ry, rz);
	}
	if (gs->mBillboardEnabled) {
		prim->mBillboard  = 2;
		prim->mBillboardZ = rz;
	} else {
		prim->mBillboard = 0;
	}
	prim->mDirty    = TRUE;
	prim->mRotOrder = gs->_17A0;
}

extern "C" {
#include <sysdolphin/lobj.h>
#include <sysdolphin/lobj_setup.h>

HSD_LObj* HSD_LObjGetCurrentByType(u16 flags);
}

// gx enums the hsd headers don't carry
#define GX_AF_SPOT 1
#define GX_AF_NONE 2

static inline void GsChanSetDirty(CGsChannel* ch)
{
	if (!ch->mDirty) {
		ch->mDirty = TRUE;
	}
}

static inline void GsChanSetNext(CGsChannel* ch, HSD_Chan* next)
{
	if (ch->next != next) {
		ch->next = next;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetAmbColor(CGsChannel* ch, GXColor color)
{
	if (*(u32*)&ch->amb_color != *(u32*)&color) {
		ch->amb_color = color;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetMatColor(CGsChannel* ch, GXColor color)
{
	if (*(u32*)&ch->mat_color != *(u32*)&color) {
		ch->mat_color = color;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetEnable(CGsChannel* ch, u8 enable)
{
	if (ch->enable != enable) {
		ch->enable = enable;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetMatSrc(CGsChannel* ch, s32 src)
{
	if (ch->mat_src != src) {
		ch->mat_src = src;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetLightMask(CGsChannel* ch, s32 mask)
{
	if (ch->light_mask != mask) {
		ch->light_mask = mask;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetAttnFn(CGsChannel* ch, s32 fn)
{
	if (ch->attn_fn != fn) {
		ch->attn_fn = fn;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanInit(CGsChannel* ch, s32 chan, GsChanCtrl ctrl, GXColor amb, GXColor mat)
{
	ch->next       = NULL;
	ch->chan       = chan;
	ch->flags      = 0;
	ch->enable     = ctrl.mEnable;
	ch->amb_src    = ctrl.mAmbSrc;
	ch->mat_src    = ctrl.mMatSrc;
	ch->light_mask = ctrl.mLightMask;
	ch->diff_fn    = ctrl.mDiffFn;
	ch->attn_fn    = ctrl.mAttnFn;
	ch->unk2C      = NULL;
	GsChanSetDirty(ch);
	GsChanSetAmbColor(ch, amb);
	GsChanSetMatColor(ch, mat);
}

// every active light's id
static inline s32 GsGetLightMask()
{
	s32 i;
	s32 mask = 0;
	s32 n    = HSD_LObjGetNbActive();
	for (i = 0; i < n; i++) {
		if (HSD_LObjGetActiveByIndex(i) != NULL) {
			mask |= HSD_Index2LightID(i);
		}
	}
	return mask;
}

static inline GXColor GsZeroColor()
{
	GXColor c = { 0, 0, 0, 0 };
	return c;
}
// a times b per channel, out of 255, into a
static inline GXColor GsMulColor(GXColor& a, GXColor b, GXColor c)
{
	c.r = (u32)(a.r * b.r) / 255;
	c.g = (u32)(a.g * b.g) / 255;
	c.b = (u32)(a.b * b.b) / 255;
	c.a = (u32)(a.a * b.a) / 255;
	a   = c;
	return a;
}

// HSD_PEDesc flag bits
#define PE_COLOR_UPDATE 0x1
#define PE_ALPHA_UPDATE 0x2
#define PE_DST_ALPHA    0x4
#define PE_BEFORE_TEX   0x8
#define PE_COMPARE      0x10
#define PE_ZUPDATE      0x20
#define PE_DITHER       0x40

extern "C" {
void GXAdjustForOverscan(const GXRenderModeObj* rmin, GXRenderModeObj* rmout, u16 hor, u16 ver);
void GXSetNumTexGens(u8 count);
void GXSetTexCoordGen2(u32 coord, u32 genType, u32 srcParam, u32 mtx, GXBool doNormalise, u32 postMtx);
void HSD_SetPixelFormat(s32 pix_fmt, s32 z_fmt);
void HSD_VISetConfigure(GXRenderModeObj* rmode);
void HSD_CObjSetReferenceSize(s32 ref_width, s32 ref_height);
void HSD_SetupTevStageAll(HSD_TevDesc* desc);
void HSD_StateInvalidate(s32 mask);
void HSD_SetZSortMode(s32 mode, s32 sort);
}

extern "C" {
#include <sysdolphin/video.h>
void GXSetClipMode(s32 mode);
}

#include "game/vistate.h"

// forward decls so the definitions can go in reverse
// reverse order below, mwcc defers the inlines

/**
 * @note Address: 0x8019B850
 * @note Size: 0xA0
 */
extern "C" void ConfigureGsScreenMode(CGsScreen* screen, s32 mode, u16 hor, u16 ver)
{
	screen->mMode = mode;
	GXAdjustForOverscan(gVideoRenderModes[screen->mMode], screen, hor, ver);
	screen->viWidth   = 660;
	screen->viXOrigin = (720 - screen->viWidth) / 2;
	// viTVMode is a VITVMode in the sdk, signed
	if (((s32)screen->viTVMode & 3) == 1) {
		screen->viYOrigin /= 2;
	}
	HSD_VISetConfigure(screen);
	HSD_CObjSetReferenceSize(screen->fbWidth, screen->efbHeight);
#if defined(VERSION_PAL)
	screen->_40 = 1.0f;
#endif
}
// ConfigureGsScreenMode
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r3
    lis      r3, gVideoRenderModes@ha
    stw      r4, 0x3c(r31)
    addi     r3, r3, gVideoRenderModes@l
    mr       r4, r31
    lwz      r0, 0x3c(r31)
    slwi     r0, r0, 2
    lwzx     r3, r3, r0
    bl       GXAdjustForOverscan
    li       r0, 0x294
    sth      r0, 0xe(r31)
    lhz      r0, 0xe(r31)
    subfic   r3, r0, 0x2d0
    srwi     r0, r3, 31
    add      r0, r0, r3
    srawi    r0, r0, 1
    sth      r0, 0xa(r31)
    lwz      r0, 0x0(r31)
    clrlwi   r0, r0, 30
    cmpwi    r0, 0x1
    bne      .L_8019B8C8
    lhz      r3, 0xc(r31)
    srwi     r0, r3, 31
    add      r0, r0, r3
    srawi    r0, r0, 1
    sth      r0, 0xc(r31)
.L_8019B8C8:
    mr       r3, r31
    bl       HSD_VISetConfigure
    lhz      r3, 0x4(r31)
    lhz      r4, 0x6(r31)
    bl       HSD_CObjSetReferenceSize
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x8019B6C0
 * @note Size: 0x190
 */
extern "C" void InitializeGsRenderModeDescriptor(CGsRenderMode* mode, u8 colorUpdate, u8 alphaUpdate, GsBlendMode blend,
                                                 GsAlphaCompare compare, GsDstAlpha dstAlpha, GsZMode zmode, u8 beforeTex, u8 dither,
                                                 s32 cull, GsLineWidth line, GsLineWidth point)
{
	mode->flags = 0;
	if (colorUpdate) {
		mode->flags |= PE_COLOR_UPDATE;
	}
	if (alphaUpdate) {
		mode->flags |= PE_ALPHA_UPDATE;
	}
	if (dstAlpha.mEnable) {
		mode->flags |= PE_DST_ALPHA;
	}
	if (beforeTex) {
		mode->flags |= PE_BEFORE_TEX;
	}
	if (zmode.mEnable) {
		mode->flags |= PE_COMPARE;
	}
	if (zmode.mUpdate) {
		mode->flags |= PE_ZUPDATE;
	}
	if (dither) {
		mode->flags |= PE_DITHER;
	}
	mode->ref0          = compare.mRef0;
	mode->ref1          = compare.mRef1;
	mode->dst_alpha     = dstAlpha.mValue;
	mode->type          = blend.mType;
	mode->src_factor    = blend.mSrcFactor;
	mode->dst_factor    = blend.mDstFactor;
	mode->logic_op      = blend.mLogicOp;
	mode->z_comp        = zmode.mFunc;
	mode->alpha_comp0   = compare.mComp0;
	mode->alpha_op      = compare.mOp;
	mode->alpha_comp1   = compare.mComp1;
	mode->mCullMode     = cull;
	mode->mLineWidth    = line.mWidth;
	mode->mLineTexOffs  = line.mTexOffs;
	mode->mPointSize    = point.mWidth;
	mode->mPointTexOffs = point.mTexOffs;
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B648
 * @note Size: 0x78
 */
extern "C" void SetGsColorUpdate(CGsRenderMode* mode, u8 enable)
{
	if (enable) {
		mode->flags |= PE_COLOR_UPDATE;
	} else {
		mode->flags &= ~PE_COLOR_UPDATE;
	}
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B5D0
 * @note Size: 0x78
 */
extern "C" void SetGsAlphaUpdate(CGsRenderMode* mode, u8 enable)
{
	if (enable) {
		mode->flags |= PE_ALPHA_UPDATE;
	} else {
		mode->flags &= ~PE_ALPHA_UPDATE;
	}
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B56C
 * @note Size: 0x64
 */
extern "C" void SetGsBlendMode(CGsRenderMode* mode, s32 type, s32 src, s32 dst, s32 op)
{
	mode->type       = type;
	mode->src_factor = src;
	mode->dst_factor = dst;
	mode->logic_op   = op;
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B504
 * @note Size: 0x68
 */
extern "C" void SetGsAlphaCompare(CGsRenderMode* mode, s32 comp0, s32 ref0, s32 op, s32 comp1, s32 ref1)
{
	mode->ref0        = ref0;
	mode->ref1        = ref1;
	mode->alpha_comp0 = comp0;
	mode->alpha_op    = op;
	mode->alpha_comp1 = comp1;
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B464
 * @note Size: 0xA0
 */
extern "C" void SetGsDepthMode(CGsRenderMode* mode, u8 enable, s32 func, u8 update)
{
	if (enable) {
		mode->flags |= PE_COMPARE;
	} else {
		mode->flags &= ~PE_COMPARE;
	}
	if (update) {
		mode->flags |= PE_ZUPDATE;
	} else {
		mode->flags &= ~PE_ZUPDATE;
	}
	mode->z_comp = func;
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B3E8
 * @note Size: 0x7C
 */
extern "C" void SetGsDestinationAlpha(CGsRenderMode* mode, u8 enable, u8 value)
{
	if (enable) {
		mode->flags |= PE_DST_ALPHA;
	} else {
		mode->flags &= ~PE_DST_ALPHA;
	}
	mode->dst_alpha = value;
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B370
 * @note Size: 0x78
 */
extern "C" void SetGsDepthCompareBeforeTexture(CGsRenderMode* mode, u8 beforeTex)
{
	if (beforeTex) {
		mode->flags |= PE_BEFORE_TEX;
	} else {
		mode->flags &= ~PE_BEFORE_TEX;
	}
	mode->Invalidate();
}

/**
 * @note Address: 0x8019B360
 * @note Size: 0x10
 */
extern "C" void SetGsCullMode(CGsRenderMode* mode, s32 cull)
{
	mode->mCullMode  = cull;
	mode->mCullDirty = TRUE;
}

/**
 * @note Address: 0x8019B34C
 * @note Size: 0x14
 */
extern "C" void SetGsLineWidth(CGsRenderMode* mode, u8 width, s32 texOffs)
{
	mode->mLineWidth   = width;
	mode->mLineTexOffs = texOffs;
	mode->mLineDirty   = TRUE;
}

/**
 * @note Address: 0x8019B278
 * @note Size: 0xD4
 */
extern "C" void ApplyGsRenderMode(CGsRenderMode* mode, BOOL force)
{
	if (force || mode->mPEDirty) {
		mode->mPEDirty = FALSE;
		HSD_StateInvalidate(0x40);
		HSD_SetupPEMode(0, mode);
	}
	if (force || mode->mCullDirty) {
		mode->mCullDirty = FALSE;
		HSD_StateSetCullMode(mode->mCullMode);
	}
	if (force || mode->mLineDirty) {
		mode->mLineDirty = FALSE;
		HSD_StateSetLineWidth(mode->mLineWidth, mode->mLineTexOffs);
	}
	if (force || mode->mPointDirty) {
		mode->mPointDirty = FALSE;
		HSD_StateSetPointSize(mode->mPointSize, mode->mPointTexOffs);
	}
}

/**
 * @note Address: 0x8019B128
 * @note Size: 0x150
 */
extern "C" void InitializeGsTevStageDescriptor(CGsTevStage* stage, u32 id, GsTevOrder order, GsTevOp colorOp, GsTevIn colorIn,
                                               GsTevOp alphaOp, GsTevIn alphaIn)
{
	stage->next                    = NULL;
	stage->stage                   = id;
	stage->flags                   = 1;
	stage->coord                   = order.mCoord;
	stage->map                     = order.mMap;
	stage->color                   = order.mColor;
	stage->u.tevconf.clr_op        = colorOp.mOp;
	stage->u.tevconf.clr_a         = colorIn.mA;
	stage->u.tevconf.clr_b         = colorIn.mB;
	stage->u.tevconf.clr_c         = colorIn.mC;
	stage->u.tevconf.clr_d         = colorIn.mD;
	stage->u.tevconf.clr_scale     = colorOp.mScale;
	stage->u.tevconf.clr_bias      = colorOp.mBias;
	stage->u.tevconf.clr_clamp     = colorOp.mClamp;
	stage->u.tevconf.clr_out_reg   = colorOp.mReg;
	stage->u.tevconf.alpha_op      = alphaOp.mOp;
	stage->u.tevconf.alpha_a       = alphaIn.mA;
	stage->u.tevconf.alpha_b       = alphaIn.mB;
	stage->u.tevconf.alpha_c       = alphaIn.mC;
	stage->u.tevconf.alpha_d       = alphaIn.mD;
	stage->u.tevconf.alpha_scale   = alphaOp.mScale;
	stage->u.tevconf.alpha_bias    = alphaOp.mBias;
	stage->u.tevconf.alpha_clamp   = alphaOp.mClamp;
	stage->u.tevconf.alpha_out_reg = alphaOp.mReg;
	stage->u.tevconf.mode          = 0;
	stage->u.tevconf.ras_swap      = 0;
	stage->u.tevconf.tex_swap      = 0;
	stage->u.tevconf.kcsel         = 0xC;
	stage->u.tevconf.kasel         = 0x1C;
	stage->mNumTexGens             = 0;
	stage->mTexCoord               = 0;
	stage->mTexGenFunc             = 1;
	stage->mTexGenSrc              = 4;
	stage->mTexMtx                 = 0x3C;
	stage->Invalidate();
}
// InitializeGsTevStageDescriptor
/*
    stwu     r1, -0x20(r1)
    li       r10, 0x0
    li       r0, 0x1
    li       r12, 0x1c
    stw      r31, 0x1c(r1)
    li       r31, 0xc
    li       r11, 0x4
    stw      r30, 0x18(r1)
    lwz      r30, 0x4(r5)
    stw      r29, 0x14(r1)
    lwz      r29, 0x0(r5)
    stw      r10, 0x0(r3)
    lwz      r5, 0x8(r5)
    stw      r0, 0x4(r3)
    stw      r4, 0x8(r3)
    li       r4, 0x3c
    stw      r29, 0xc(r3)
    lwz      r29, 0x0(r6)
    stw      r30, 0x10(r3)
    lwz      r30, 0x0(r7)
    stw      r5, 0x14(r3)
    lwz      r5, 0x4(r7)
    stw      r29, 0x18(r3)
    lwz      r29, 0x8(r7)
    stw      r30, 0x1c(r3)
    lwz      r30, 0xc(r7)
    stw      r5, 0x20(r3)
    lwz      r5, 0x8(r6)
    stw      r29, 0x24(r3)
    lwz      r7, 0x4(r6)
    stw      r30, 0x28(r3)
    lbz      r30, 0xc(r6)
    stw      r5, 0x2c(r3)
    lwz      r5, 0x10(r6)
    stw      r7, 0x30(r3)
    lwz      r7, 0x0(r8)
    stb      r30, 0x34(r3)
    lwz      r6, 0x0(r9)
    stw      r5, 0x38(r3)
    lwz      r5, 0x4(r9)
    stw      r7, 0x3c(r3)
    lwz      r7, 0x8(r9)
    stw      r6, 0x40(r3)
    lwz      r6, 0xc(r9)
    stw      r5, 0x44(r3)
    lwz      r5, 0x8(r8)
    stw      r7, 0x48(r3)
    lwz      r7, 0x4(r8)
    stw      r6, 0x4c(r3)
    lbz      r6, 0xc(r8)
    stw      r5, 0x50(r3)
    lwz      r5, 0x10(r8)
    stw      r7, 0x54(r3)
    stb      r6, 0x58(r3)
    stw      r5, 0x5c(r3)
    stw      r10, 0x60(r3)
    stw      r10, 0x64(r3)
    stw      r10, 0x68(r3)
    stw      r31, 0x6c(r3)
    stw      r12, 0x70(r3)
    stb      r10, 0x80(r3)
    stw      r10, 0x84(r3)
    stw      r0, 0x88(r3)
    stw      r11, 0x8c(r3)
    stw      r4, 0x90(r3)
    lwz      r4, 0x74(r3)
    cmpwi    r4, 0x0
    bne      .L_8019B23C
    stw      r0, 0x74(r3)
.L_8019B23C:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_8019B250
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_8019B250:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_8019B264
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_8019B264:
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: 0x8019B040
 * @note Size: 0xE8
 */
extern "C" void SetGsTevOrder(CGsTevStage* stage, s32 coord, s32 map, s32 color)
{
	if (stage->coord != coord) {
		stage->coord = coord;
		stage->Invalidate();
	}
	if (stage->map != map) {
		stage->map = map;
		stage->Invalidate();
	}
	if (stage->color != color) {
		stage->color = color;
		stage->Invalidate();
	}
}

/**
 * @note Address: 0x8019AFA0
 * @note Size: 0xA0
 */
extern "C" void SetGsTevMode(CGsTevStage* stage, s32 mode)
{
	if (stage->flags != 0) {
		stage->flags = 0;
		stage->Invalidate();
	}
	if (stage->u.tevop.tevmode != mode) {
		stage->u.tevop.tevmode = mode;
		stage->Invalidate();
	}
}

/**
 * @note Address: 0x8019AED0
 * @note Size: 0xD0
 */
extern "C" void ApplyGsTevStage(CGsTevStage* stage, BOOL force)
{
	if (force || stage->mStageDirty) {
		stage->mStageDirty = FALSE;
		HSD_StateInvalidate(8);
		HSD_StateInvalidate(0x10);
		HSD_StateInvalidate(0x20);
		HSD_SetupTevStageAll(stage);
	}
	if (force || stage->mNumGensDirty) {
		HSD_StateInvalidate(0x20);
		GXSetNumTexGens(stage->mNumTexGens);
	}
	if (force || stage->mTexGenDirty) {
		HSD_StateInvalidate(0x10);
		HSD_StateInvalidate(0x20);
		GXSetTexCoordGen2(stage->mTexCoord, stage->mTexGenFunc, stage->mTexGenSrc, stage->mTexMtx, GX_FALSE, 125);
	}
}

/**
 * @note Address: 0x8019AE94
 * @note Size: 0x3C
 */
extern "C" void ApplyGsChannel(CGsChannel* chan, BOOL force)
{
	if (force || chan->mDirty) {
		chan->mDirty = FALSE;
		HSD_SetupChannelAll(chan);
	}
}

/**
 * @note Address: 0x8019ADDC
 * @note Size: 0xB8
 */
extern "C" void SetGsScreenModeAndRenderDefaults(CGs* gs, s32 mode, u16 hor, u16 ver, s32 arg)
{
	ConfigureGsScreenMode(&gs->mScreen, mode, hor, ver);
#if defined(VERSION_JP)
	InitializeGsRenderDefaultsJP(gs);
#else
	InitializeGsRenderDefaults(gs, arg);
#endif
}
// SetGsScreenModeAndRenderDefaults
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    mr       r31, r7
    stw      r30, 0x8(r1)
    mr       r30, r3
    lis      r3, gVideoRenderModes@ha
    stw      r4, 0x3c(r30)
    addi     r3, r3, gVideoRenderModes@l
    mr       r4, r30
    lwz      r0, 0x3c(r30)
    slwi     r0, r0, 2
    lwzx     r3, r3, r0
    bl       GXAdjustForOverscan
    li       r0, 0x294
    sth      r0, 0xe(r30)
    lhz      r0, 0xe(r30)
    subfic   r3, r0, 0x2d0
    srwi     r0, r3, 31
    add      r0, r0, r3
    srawi    r0, r0, 1
    sth      r0, 0xa(r30)
    lwz      r0, 0x0(r30)
    clrlwi   r0, r0, 30
    cmpwi    r0, 0x1
    bne      .L_8019AE5C
    lhz      r3, 0xc(r30)
    srwi     r0, r3, 31
    add      r0, r0, r3
    srawi    r0, r0, 1
    sth      r0, 0xc(r30)
.L_8019AE5C:
    mr       r3, r30
    bl       HSD_VISetConfigure
    lhz      r3, 0x4(r30)
    lhz      r4, 0x6(r30)
    bl       HSD_CObjSetReferenceSize
    mr       r3, r30
    mr       r4, r31
    bl       InitializeGsRenderDefaults
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: 0x8019A4E0
 * @note Size: 0x8FC
 */
#if defined(VERSION_JP)
extern "C" void InitializeGsRenderDefaultsJP(CGs* gs)
#else
extern "C" void InitializeGsRenderDefaults(CGs* gs, s32 arg)
#endif
{
	HSD_SetPixelFormat(1, 0);
	GXSetPixelFmt(1, 0);

	InitializeVideoClearState((GXColor) { 0, 0, 0x20, 0 });
	GXSetClipMode(0);
	HSD_SetZSortMode(1, 1);
	gs->mCamera = NULL;
	gs->mLight  = NULL;

	ConfigureGsRenderMode(gs, NULL, GX_TRUE, GX_FALSE, (GsBlendMode) { 1, 4, 5, 15 }, (GsAlphaCompare) { 4, 0, 0, 3, 0xFF },
	                      (GsDstAlpha) { 0, 0 }, (GsZMode) { 0, 3, 0 }, GX_TRUE, GX_TRUE, 2, (GsLineWidth) { 12, 5 },
	                      (GsLineWidth) { 24, 5 });

	ConfigureGsPrimaryTevStage(gs);

	// The second stage adds its result to the previous stage's color and alpha.
	ConfigureGsTevStage(gs, &gs->mTevStages[1], 1, (GsTevOrder) { 0xFF, GX_TEXMAP_NULL, GX_COLOR1A1 },
	                    (GsTevOp) { GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, 0 }, (GsTevIn) { 0xF, 0xA, 0xC, 0 },
	                    (GsTevOp) { GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, 0 }, (GsTevIn) { 7, 5, 6, 0 });

	ConfigureGsChannel(gs, NULL, GX_COLOR0, (GsChanCtrl) { 0, 0, 0, 0, 2, 2 }, (GXColor) { 0, 0, 0, 0xFF },
	                   (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	ConfigureGsChannel(gs, NULL, GX_ALPHA0, (GsChanCtrl) { 0, 0, 0, 0, 0, 2 }, (GXColor) { 0, 0, 0, 0xFF },
	                   (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	ConfigureGsChannel(gs, NULL, GX_COLOR1, (GsChanCtrl) { 1, 0, 0, 0, 0, 0 }, (GXColor) { 0, 0, 0, 0xFF },
	                   (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });
	ConfigureGsChannel(gs, NULL, GX_ALPHA1, (GsChanCtrl) { 0, 0, 0, 0, 0, 2 }, (GXColor) { 0, 0, 0, 0xFF },
	                   (GXColor) { 0xFF, 0xFF, 0xFF, 0xFF });

	gs->mBillboardEnabled = 0;
	gs->mSprite.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mSprite.mTex = &gs->mTex;
	gs->mLine.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mTriangle.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mCircle.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mTile.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mSprite2.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mCube.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mOctahedron.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mDodeca.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mIcosahedron.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mSphere.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mCylinder.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mTorus.Init(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->_17A0 = 5;
#if !defined(VERSION_JP)
	gs->mUseShiftJis          = arg;
	gs->mShiftJisWidthScale   = 1.0f;
	gs->mSingleByteWidthScale = 1.0f;
#endif
}
// InitializeGsRenderDefaults
/*
    stwu     r1, -0x2d0(r1)
    mflr     r0
    lis      r5, sGsRenderDefaultsBase@ha
    stw      r0, 0x2d4(r1)
    stmw     r14, 0x288(r1)
    mr       r30, r3
    li       r3, 0x1
    addi     r31, r5, sGsRenderDefaultsBase@l
    stw      r4, 0x18(r1)
    li       r4, 0x0
    bl       HSD_SetPixelFormat
    li       r3, 0x1
    li       r4, 0x0
    bl       GXSetPixelFmt
    lwz      r3, lbl_8065F5A0@sda21(r0)
    lis      r5, 0x100
    lis      r4, HSD_VIData@ha
    li       r0, 0x0
    stw      r3, 0x24(r1)
    addi     r8, r4, HSD_VIData@l
    subi     r9, r5, 0x1
    li       r3, 0x0
    lbz      r7, 0x24(r1)
    lbz      r6, 0x25(r1)
    lbz      r5, 0x26(r1)
    lbz      r4, 0x27(r1)
    stw      r9, 0x4c(r8)
    stb      r7, 0x48(r8)
    stb      r6, 0x49(r8)
    stb      r5, 0x4a(r8)
    stb      r4, 0x4b(r8)
    stw      r0, 0x44(r8)
    bl       GXSetClipMode
    li       r3, 0x1
    li       r4, 0x1
    bl       HSD_SetZSortMode
    li       r0, 0x0
    addi     r29, r30, 0x4c
    stw      r0, 0x44(r30)
    addi     r6, r1, 0xc0
    addi     r7, r1, 0x1a0
    addi     r8, r1, 0x1c
    stw      r0, 0x48(r30)
    addi     r9, r1, 0xd0
    lwz      r26, lbl_8065F5B4@sda21(r0)
    lwz      r3, lbl_8065F5B8@sda21(r0)
    lwz      r25, lbl_8065F5AC@sda21(r0)
    lwz      r24, lbl_8065F5B0@sda21(r0)
    lwz      r23, 0x48(r31)
    lwz      r22, 0x4c(r31)
    lwz      r21, 0x50(r31)
    lhz      r20, lbl_806602A4@sda21(r0)
    lwz      r19, 0x34(r31)
    lwz      r18, 0x38(r31)
    lwz      r17, 0x3c(r31)
    lwz      r16, 0x40(r31)
    lwz      r15, 0x44(r31)
    lwz      r14, 0x24(r31)
    lwz      r12, 0x28(r31)
    lwz      r11, 0x2c(r31)
    lwz      r0, 0x30(r31)
    lbz      r10, lbl_8065F5A5@sda21(r0)
    lbz      r5, lbl_806602A0@sda21(r0)
    lbz      r4, lbl_8065F5A4@sda21(r0)
    lwz      r28, lbl_8065F5A8@sda21(r0)
    lbz      r27, lbl_8065F5A6@sda21(r0)
    stw      r26, 0x58(r1)
    stw      r29, 0x290(r30)
    stw      r26, 0x50(r1)
    addi     r26, r1, 0x48
    stw      r3, 0x54(r1)
    stw      r25, 0x48(r1)
    stw      r24, 0x4c(r1)
    stw      r23, 0xd0(r1)
    stw      r22, 0xd4(r1)
    stw      r21, 0xd8(r1)
    sth      r20, 0x1c(r1)
    stw      r19, 0x1a0(r1)
    stw      r18, 0x1a4(r1)
    stw      r17, 0x1a8(r1)
    stw      r16, 0x1ac(r1)
    stw      r15, 0x1b0(r1)
    stw      r14, 0xc0(r1)
    stw      r12, 0xc4(r1)
    stw      r11, 0xc8(r1)
    stw      r0, 0xcc(r1)
    stw      r27, 0x8(r1)
    stw      r28, 0xc(r1)
    stw      r26, 0x10(r1)
    addi     r26, r1, 0x50
    stw      r26, 0x14(r1)
    stw      r3, 0x5c(r1)
    lwz      r3, 0x290(r30)
    stw      r25, 0x60(r1)
    stw      r24, 0x64(r1)
    stw      r23, 0x134(r1)
    stw      r22, 0x138(r1)
    stw      r21, 0x13c(r1)
    sth      r20, 0x20(r1)
    stw      r19, 0x264(r1)
    stw      r18, 0x268(r1)
    stw      r17, 0x26c(r1)
    stw      r16, 0x270(r1)
    stw      r15, 0x274(r1)
    stw      r14, 0x140(r1)
    stw      r12, 0x144(r1)
    stw      r11, 0x148(r1)
    stw      r0, 0x14c(r1)
    bl       InitializeGsRenderModeDescriptor
    addi     r0, r30, 0x80
    lwz      r15, 0x98(r31)
    stw      r0, 0x278(r1)
    addi     r5, r1, 0x94
    lwz      r0, 0x5c(r31)
    addi     r6, r1, 0x178
    lwz      r3, 0x9c(r31)
    addi     r7, r1, 0xa0
    lwz      r16, 0xa0(r31)
    addi     r8, r1, 0x18c
    lwz      r17, 0xa4(r31)
    addi     r9, r1, 0xb0
    lwz      r18, 0x84(r31)
    lwz      r19, 0x88(r31)
    lwz      r20, 0x8c(r31)
    lwz      r21, 0x90(r31)
    lwz      r22, 0x94(r31)
    lwz      r23, 0x74(r31)
    lwz      r24, 0x78(r31)
    lwz      r25, 0x7c(r31)
    lwz      r26, 0x80(r31)
    lwz      r27, 0x60(r31)
    lwz      r28, 0x64(r31)
    lwz      r29, 0x68(r31)
    lwz      r12, 0x6c(r31)
    lwz      r11, 0x70(r31)
    lwz      r10, 0x54(r31)
    lwz      r14, 0x58(r31)
    stw      r0, 0x27c(r1)
    lwz      r4, lbl_806602A8@sda21(r0)
    lwz      r0, 0x278(r1)
    stw      r15, 0x108(r1)
    stw      r0, 0x294(r30)
    lwz      r0, 0x27c(r1)
    stw      r15, 0xb0(r1)
    stw      r3, 0xb4(r1)
    stw      r16, 0xb8(r1)
    stw      r17, 0xbc(r1)
    stw      r18, 0x18c(r1)
    stw      r19, 0x190(r1)
    stw      r20, 0x194(r1)
    stw      r21, 0x198(r1)
    stw      r22, 0x19c(r1)
    stw      r23, 0xa0(r1)
    stw      r24, 0xa4(r1)
    stw      r25, 0xa8(r1)
    stw      r26, 0xac(r1)
    stw      r27, 0x178(r1)
    stw      r28, 0x17c(r1)
    stw      r29, 0x180(r1)
    stw      r12, 0x184(r1)
    stw      r11, 0x188(r1)
    stw      r10, 0x94(r1)
    stw      r14, 0x98(r1)
    stw      r0, 0x9c(r1)
    stw      r3, 0x10c(r1)
    lwz      r3, 0x294(r30)
    stw      r16, 0x110(r1)
    stw      r17, 0x114(r1)
    stw      r18, 0x23c(r1)
    stw      r19, 0x240(r1)
    stw      r20, 0x244(r1)
    stw      r21, 0x248(r1)
    stw      r22, 0x24c(r1)
    stw      r23, 0x118(r1)
    stw      r24, 0x11c(r1)
    stw      r25, 0x120(r1)
    stw      r26, 0x124(r1)
    stw      r27, 0x250(r1)
    stw      r28, 0x254(r1)
    stw      r29, 0x258(r1)
    stw      r12, 0x25c(r1)
    stw      r11, 0x260(r1)
    stw      r10, 0x128(r1)
    stw      r14, 0x12c(r1)
    stw      r0, 0x130(r1)
    bl       InitializeGsTevStageDescriptor
    lwz      r4, 0xec(r31)
    addic.   r0, r30, 0x118
    lwz      r23, 0xf0(r31)
    lwz      r22, 0xf4(r31)
    lwz      r21, 0xf8(r31)
    lwz      r20, 0xd8(r31)
    lwz      r19, 0xdc(r31)
    lwz      r18, 0xe0(r31)
    lwz      r17, 0xe4(r31)
    lwz      r16, 0xe8(r31)
    lwz      r15, 0xc8(r31)
    lwz      r14, 0xcc(r31)
    lwz      r12, 0xd0(r31)
    lwz      r11, 0xd4(r31)
    lwz      r10, 0xb4(r31)
    lwz      r9, 0xb8(r31)
    lwz      r8, 0xbc(r31)
    lwz      r7, 0xc0(r31)
    lwz      r6, 0xc4(r31)
    lwz      r5, 0xa8(r31)
    lwz      r3, 0xac(r31)
    lwz      r0, 0xb0(r31)
    stw      r4, 0xdc(r1)
    lwz      r4, lbl_8065F5BC@sda21(r0)
    stw      r23, 0xe0(r1)
    stw      r22, 0xe4(r1)
    stw      r21, 0xe8(r1)
    stw      r20, 0x214(r1)
    stw      r19, 0x218(r1)
    stw      r18, 0x21c(r1)
    stw      r17, 0x220(r1)
    stw      r16, 0x224(r1)
    stw      r15, 0xec(r1)
    stw      r14, 0xf0(r1)
    stw      r12, 0xf4(r1)
    stw      r11, 0xf8(r1)
    stw      r10, 0x228(r1)
    stw      r9, 0x22c(r1)
    stw      r8, 0x230(r1)
    stw      r7, 0x234(r1)
    stw      r6, 0x238(r1)
    stw      r5, 0xfc(r1)
    stw      r3, 0x100(r1)
    stw      r0, 0x104(r1)
    beq      .L_8019A888
    addi     r0, r30, 0x118
    stw      r0, 0x294(r30)
    b        .L_8019A890
.L_8019A888:
    addi     r0, r30, 0x80
    stw      r0, 0x294(r30)
.L_8019A890:
    lwz      r28, 0xdc(r1)
    addi     r5, r1, 0x68
    lwz      r27, 0xe0(r1)
    addi     r6, r1, 0x150
    lwz      r26, 0xe4(r1)
    addi     r7, r1, 0x74
    lwz      r25, 0xe8(r1)
    addi     r8, r1, 0x164
    lwz      r24, 0x214(r1)
    addi     r9, r1, 0x84
    lwz      r23, 0x218(r1)
    lwz      r22, 0x21c(r1)
    lwz      r21, 0x220(r1)
    lwz      r20, 0x224(r1)
    lwz      r19, 0xec(r1)
    lwz      r18, 0xf0(r1)
    lwz      r17, 0xf4(r1)
    lwz      r16, 0xf8(r1)
    lwz      r15, 0x228(r1)
    lwz      r14, 0x22c(r1)
    lwz      r12, 0x230(r1)
    lwz      r11, 0x234(r1)
    lwz      r10, 0x238(r1)
    lwz      r3, 0xfc(r1)
    lwz      r0, 0x100(r1)
    lwz      r29, 0x104(r1)
    stw      r28, 0x84(r1)
    stw      r27, 0x88(r1)
    stw      r26, 0x8c(r1)
    stw      r25, 0x90(r1)
    stw      r24, 0x164(r1)
    stw      r23, 0x168(r1)
    stw      r22, 0x16c(r1)
    stw      r21, 0x170(r1)
    stw      r20, 0x174(r1)
    stw      r19, 0x74(r1)
    stw      r18, 0x78(r1)
    stw      r17, 0x7c(r1)
    stw      r16, 0x80(r1)
    stw      r15, 0x150(r1)
    stw      r14, 0x154(r1)
    stw      r12, 0x158(r1)
    stw      r11, 0x15c(r1)
    stw      r10, 0x160(r1)
    stw      r3, 0x68(r1)
    stw      r0, 0x6c(r1)
    stw      r29, 0x70(r1)
    lwz      r3, 0x294(r30)
    bl       InitializeGsTevStageDescriptor
    lwz      r5, lbl_8065F5C4@sda21(r0)
    mr       r3, r30
    lwz      r15, lbl_8065F5C0@sda21(r0)
    addi     r6, r1, 0x1fc
    lwz      r14, 0xfc(r31)
    addi     r7, r1, 0x44
    lwz      r12, 0x100(r31)
    addi     r8, r1, 0x40
    lwz      r11, 0x104(r31)
    li       r4, 0x0
    lwz      r10, 0x108(r31)
    lwz      r9, 0x10c(r31)
    lwz      r0, 0x110(r31)
    stw      r5, 0x40(r1)
    lwz      r5, lbl_806602AC@sda21(r0)
    stw      r15, 0x44(r1)
    stw      r14, 0x1fc(r1)
    stw      r12, 0x200(r1)
    stw      r11, 0x204(r1)
    stw      r10, 0x208(r1)
    stw      r9, 0x20c(r1)
    stw      r0, 0x210(r1)
    bl       ConfigureGsChannel
    lwz      r5, lbl_8065F5D0@sda21(r0)
    mr       r3, r30
    lwz      r15, lbl_8065F5CC@sda21(r0)
    addi     r6, r1, 0x1e4
    lwz      r14, 0x114(r31)
    addi     r7, r1, 0x3c
    lwz      r12, 0x118(r31)
    addi     r8, r1, 0x38
    lwz      r11, 0x11c(r31)
    li       r4, 0x0
    lwz      r10, 0x120(r31)
    lwz      r9, 0x124(r31)
    lwz      r0, 0x128(r31)
    stw      r5, 0x38(r1)
    lwz      r5, lbl_8065F5C8@sda21(r0)
    stw      r15, 0x3c(r1)
    stw      r14, 0x1e4(r1)
    stw      r12, 0x1e8(r1)
    stw      r11, 0x1ec(r1)
    stw      r10, 0x1f0(r1)
    stw      r9, 0x1f4(r1)
    stw      r0, 0x1f8(r1)
    bl       ConfigureGsChannel
    lwz      r5, lbl_8065F5DC@sda21(r0)
    mr       r3, r30
    lwz      r15, lbl_8065F5D8@sda21(r0)
    addi     r6, r1, 0x1cc
    lwz      r14, 0x12c(r31)
    addi     r7, r1, 0x34
    lwz      r12, 0x130(r31)
    addi     r8, r1, 0x30
    lwz      r11, 0x134(r31)
    li       r4, 0x0
    lwz      r10, 0x138(r31)
    lwz      r9, 0x13c(r31)
    lwz      r0, 0x140(r31)
    stw      r5, 0x30(r1)
    lwz      r5, lbl_8065F5D4@sda21(r0)
    stw      r15, 0x34(r1)
    stw      r14, 0x1cc(r1)
    stw      r12, 0x1d0(r1)
    stw      r11, 0x1d4(r1)
    stw      r10, 0x1d8(r1)
    stw      r9, 0x1dc(r1)
    stw      r0, 0x1e0(r1)
    bl       ConfigureGsChannel
    lwz      r5, lbl_8065F5E8@sda21(r0)
    mr       r3, r30
    lwz      r15, lbl_8065F5E4@sda21(r0)
    addi     r6, r1, 0x1b4
    lwz      r14, 0x144(r31)
    addi     r7, r1, 0x2c
    lwz      r12, 0x148(r31)
    addi     r8, r1, 0x28
    lwz      r11, 0x14c(r31)
    li       r4, 0x0
    lwz      r10, 0x150(r31)
    lwz      r9, 0x154(r31)
    lwz      r0, 0x158(r31)
    stw      r5, 0x28(r1)
    lwz      r5, lbl_8065F5E0@sda21(r0)
    stw      r15, 0x2c(r1)
    stw      r14, 0x1b4(r1)
    stw      r12, 0x1b8(r1)
    stw      r11, 0x1bc(r1)
    stw      r10, 0x1c0(r1)
    stw      r9, 0x1c4(r1)
    stw      r0, 0x1c8(r1)
    bl       ConfigureGsChannel
    li       r0, 0x0
    lfs      f1, lbl_8065F5EC@sda21(r0)
    stw      r0, 0x29c(r30)
    addi     r3, r30, 0x4e0
    lfs      f4, lbl_8065F5F0@sda21(r0)
    fmr      f2, f1
    stfs     f1, 0x8(r1)
    fmr      f3, f1
    fmr      f5, f4
    lwz      r12, 0x644(r30)
    fmr      f6, f4
    fmr      f7, f1
    lwz      r12, 0xc(r12)
    fmr      f8, f1
    mtctr    r12
    bctrl
    addi     r0, r30, 0x3e0
    lfs      f1, lbl_8065F5EC@sda21(r0)
    stw      r0, 0x648(r30)
    addi     r3, r30, 0x668
    lfs      f4, lbl_8065F5F0@sda21(r0)
    fmr      f2, f1
    stfs     f1, 0x8(r1)
    fmr      f3, f1
    fmr      f5, f4
    lwz      r12, 0x7cc(r30)
    fmr      f6, f4
    fmr      f7, f1
    lwz      r12, 0xc(r12)
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x7d4
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x938(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x940
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0xaa4(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0xabc
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0xc20(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0xc40
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0xda4(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0xdc8
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0xf2c(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0xf30
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x1094(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x1098
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x11fc(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x1200
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x1364(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x1368
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x14cc(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x14d0
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x1634(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    lfs      f1, lbl_8065F5EC@sda21(r0)
    addi     r3, r30, 0x1638
    lfs      f4, lbl_8065F5F0@sda21(r0)
    stfs     f1, 0x8(r1)
    fmr      f2, f1
    fmr      f3, f1
    lwz      r12, 0x179c(r30)
    fmr      f5, f4
    fmr      f6, f4
    lwz      r12, 0xc(r12)
    fmr      f7, f1
    fmr      f8, f1
    mtctr    r12
    bctrl
    li       r0, 0x5
    lfs      f0, lbl_8065F5F0@sda21(r0)
    stw      r0, 0x17a0(r30)
    lwz      r0, 0x18(r1)
    stw      r0, 0x17a4(r30)
    stfs     f0, 0x17a8(r30)
    stfs     f0, 0x17ac(r30)
    lmw      r14, 0x288(r1)
    lwz      r0, 0x2d4(r1)
    mtlr     r0
    addi     r1, r1, 0x2d0
    blr
*/

/**
 * @note Address: 0x8019A3CC
 * @note Size: 0x114
 */
extern "C" void ConfigureGsRenderMode(CGs* gs, CGsRenderMode* mode, u8 colorUpdate, u8 alphaUpdate, GsBlendMode blend,
                                      GsAlphaCompare compare, GsDstAlpha dstAlpha, GsZMode zmode, u8 beforeTex, u8 dither, s32 cull,
                                      GsLineWidth line, GsLineWidth point)
{
	if (mode != NULL) {
		gs->mCurRenderMode = mode;
	} else {
		gs->mCurRenderMode = &gs->mRenderMode;
	}
	InitializeGsRenderModeDescriptor(gs->mCurRenderMode, colorUpdate, alphaUpdate, blend, compare, dstAlpha, zmode, beforeTex, dither, cull,
	                                 line, point);
}

/**
 * @note Address: 0x8019A2C8
 * @note Size: 0x104
 */
extern "C" void ConfigureGsTevStage(CGs* gs, CGsTevStage* stage, u32 id, GsTevOrder order, GsTevOp colorOp, GsTevIn colorIn,
                                    GsTevOp alphaOp, GsTevIn alphaIn)
{
	if (stage != NULL) {
		gs->mCurTevStage = stage;
	} else {
		gs->mCurTevStage = &gs->mTevStages[0];
	}
	InitializeGsTevStageDescriptor(gs->mCurTevStage, id, order, colorOp, colorIn, alphaOp, alphaIn);
}

/**
 * @note Address: 0x80199B98
 * @note Size: 0x730
 */
extern "C" void ConfigureGsChannel(CGs* gs, CGsChannel* ch, s32 chan, GsChanCtrl ctrl, GXColor amb, GXColor mat)
{
	if (ch != NULL) {
		gs->mCurChannel = ch;
		GsChanInit(gs->mCurChannel, chan, ctrl, amb, mat);
		return;
	}
	switch (chan) {
	case GX_COLOR0:
	case GX_COLOR1:
	case GX_ALPHA0:
	case GX_ALPHA1:
		GsChanInit(&gs->mChannels[chan], chan, ctrl, amb, mat);
		break;
	case GX_COLOR0A0:
		GsChanInit(&gs->mChannels[0], GX_COLOR0, ctrl, amb, mat);
		GsChanInit(&gs->mChannels[2], GX_ALPHA0, ctrl, amb, mat);
		break;
	case GX_COLOR1A1:
		GsChanInit(&gs->mChannels[1], GX_COLOR1, ctrl, amb, mat);
		GsChanInit(&gs->mChannels[3], GX_ALPHA1, ctrl, amb, mat);
		break;
	}
	gs->mCurChannel = &gs->mChannels[0];
	GsChanSetNext(gs->mCurChannel, &gs->mChannels[2]);
}

/**
 * @note Address: 0x80199AB0
 * @note Size: 0xE8
 */
extern "C" void SetGsChannelEnabled(CGs* gs, s32 chan, u8 enable)
{
	if (gs->mCurChannel == &gs->mChannels[0]) {
		switch (chan) {
		case GX_COLOR0:
		case GX_ALPHA0:
			GsChanSetEnable(&gs->mChannels[chan], enable);
			break;
		case GX_COLOR0A0:
			GsChanSetEnable(&gs->mChannels[0], enable);
			GsChanSetEnable(&gs->mChannels[2], enable);
			break;
		}
	} else {
		GsChanSetEnable(gs->mCurChannel, enable);
	}
}

/**
 * @note Address: 0x8019997C
 * @note Size: 0x134
 */
extern "C" MtxPtr BeginGsCamera(CBase* base)
{
	CGs* gs    = &base->mGs;
	MtxPtr mtx = gs->mCamera->SetCurrent();
	if (mtx != NULL) {
		if (gs->mLight != NULL) {
			gs->mLight->SetCurrent(gs->mCamera);
			if (gs->mCurChannel == &gs->mChannels[0]) {
				u32 type = LOBJ_AMBIENT;
				s32 n    = HSD_LObjGetNbActive();
				for (s32 i = 0; i < n; i++) {
					HSD_LObj* lobj = HSD_LObjGetActiveByIndex(i);
					if (lobj != NULL) {
						type = lobj->flags & 3;
					}
				}
				if (type != LOBJ_AMBIENT) {
					GsChanSetAttnFn(gs->mCurChannel, GX_AF_SPOT);
				} else {
					GsChanSetAttnFn(gs->mCurChannel, GX_AF_NONE);
				}
			}
		}
#if !defined(VERSION_JP)
		// todo: cleanup, the only spelling that matched, the original was probably an inline
		Vec zero     = { 0.0f, 0.0f, 0.0f };
		zero.x       = 0.0f;
		zero.y       = 0.0f;
		zero.z       = 0.0f;
		gs->mTextRot = zero;
#endif
	}
	return mtx;
}

/**
 * @note Address: 0x80199944
 * @note Size: 0x38
 */
extern "C" void EndGsCamera(CGs* gs)
{
	gs->mCamera->EndCurrent();
	gs->mCamera = NULL;
}

/**
 * @note Address: 0x8019957C
 * @note Size: 0x3C8
 */
extern "C" s32 SetGsChannelMaterialAndLighting(CGs* gs, s32 matSrc, u32 rgba)
{
	GXColor& color     = *(GXColor*)&rgba;
	CGsChannel* color0 = &gs->mChannels[0];
	if (gs->mCurChannel == color0) {
		GsChanSetMatSrc(color0, matSrc);
		GsChanSetMatSrc(&gs->mChannels[2], matSrc);
		if (color0->enable) {
			if (color0->amb_src == GX_SRC_REG) {
				HSD_LObj* lobj = HSD_LObjGetCurrentByType(LOBJ_AMBIENT);
				if (lobj != NULL) {
					GXColor amb = lobj->color;
					GsMulColor(amb, color, GsZeroColor());
					GsChanSetAmbColor(color0, amb);
					GsChanSetAmbColor(&gs->mChannels[2], amb);
				}
			}
			GsChanSetMatColor(color0, color);
			GsChanSetMatColor(&gs->mChannels[2], color);
			s32 mask = GsGetLightMask();
			GsChanSetLightMask(color0, mask);
			GsChanSetLightMask(&gs->mChannels[2], mask);
		} else {
			GsChanSetMatColor(color0, color);
			GsChanSetLightMask(color0, 0);
			GsChanSetMatColor(&gs->mChannels[2], color);
			GsChanSetLightMask(&gs->mChannels[2], 0);
		}
	}
	return 0;
}
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stmw     r27, 0x3c(r1)
    mr       r30, r3
    addi     r31, r30, 0x1b0
    stw      r5, 0x8(r1)
    lwz      r0, 0x298(r3)
    cmplw    r0, r31
    bne      .L_8019992C
    lwz      r0, 0x1c(r31)
    cmpw     r0, r4
    beq      .L_801995C8
    stw      r4, 0x1c(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_801995C8
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_801995C8:
    lwz      r0, 0x23c(r30)
    cmpw     r0, r4
    beq      .L_801995EC
    stw      r4, 0x23c(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_801995EC
    li       r0, 0x1
    stw      r0, 0x250(r30)
.L_801995EC:
    lbz      r0, 0x14(r31)
    cmplwi   r0, 0x0
    beq      .L_8019984C
    lwz      r0, 0x18(r31)
    cmpwi    r0, 0x0
    bne      .L_80199734
    li       r3, 0x0
    bl       HSD_LObjGetCurrentByType
    cmplwi   r3, 0x0
    beq      .L_80199734
    lwz      r4, 0x10(r3)
    lis      r3, 0x8081
    lwz      r0, 0x8(r1)
    subi     r9, r3, 0x7f7f
    stw      r4, 0x2c(r1)
    lwz      r11, lbl_806602B0@sda21(r0)
    stw      r0, 0x28(r1)
    lbz      r3, 0x2c(r1)
    lbz      r0, 0x28(r1)
    lbz      r6, 0x2e(r1)
    mullw    r0, r3, r0
    lbz      r5, 0x2a(r1)
    lbz      r8, 0x2d(r1)
    lbz      r7, 0x29(r1)
    lbz      r4, 0x2f(r1)
    lbz      r3, 0x2b(r1)
    mullw    r5, r6, r5
    stw      r11, 0x24(r1)
    mullw    r7, r8, r7
    mulhwu   r10, r9, r0
    lwz      r0, 0xc(r31)
    mullw    r3, r4, r3
    extrwi   r8, r10, 8, 17
    stb      r8, 0x2c(r1)
    stb      r8, 0x24(r1)
    mulhwu   r5, r9, r5
    mulhwu   r7, r9, r7
    extrwi   r4, r5, 8, 17
    stb      r4, 0x2e(r1)
    stb      r4, 0x26(r1)
    mulhwu   r3, r9, r3
    extrwi   r6, r7, 8, 17
    stb      r6, 0x2d(r1)
    stb      r6, 0x25(r1)
    extrwi   r4, r3, 8, 17
    stb      r4, 0x2f(r1)
    lwz      r3, 0x2c(r1)
    stb      r4, 0x27(r1)
    cmplw    r0, r3
    stw      r3, 0x20(r1)
    beq      .L_801996EC
    lbz      r3, 0x20(r1)
    lbz      r0, 0x21(r1)
    stb      r3, 0xc(r31)
    lbz      r3, 0x22(r1)
    stb      r0, 0xd(r31)
    lbz      r0, 0x23(r1)
    stb      r3, 0xe(r31)
    stb      r0, 0xf(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_801996EC
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_801996EC:
    lwz      r3, 0x2c(r1)
    lwz      r0, 0x22c(r30)
    stw      r3, 0x1c(r1)
    cmplw    r0, r3
    beq      .L_80199734
    lbz      r3, 0x1c(r1)
    lbz      r0, 0x1d(r1)
    stb      r3, 0x22c(r30)
    lbz      r3, 0x1e(r1)
    stb      r0, 0x22d(r30)
    lbz      r0, 0x1f(r1)
    stb      r3, 0x22e(r30)
    stb      r0, 0x22f(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_80199734
    li       r0, 0x1
    stw      r0, 0x250(r30)
.L_80199734:
    lwz      r3, 0x8(r1)
    lwz      r0, 0x10(r31)
    stw      r3, 0x18(r1)
    cmplw    r0, r3
    beq      .L_8019977C
    lbz      r3, 0x18(r1)
    lbz      r0, 0x19(r1)
    stb      r3, 0x10(r31)
    lbz      r3, 0x1a(r1)
    stb      r0, 0x11(r31)
    lbz      r0, 0x1b(r1)
    stb      r3, 0x12(r31)
    stb      r0, 0x13(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_8019977C
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_8019977C:
    lwz      r3, 0x8(r1)
    lwz      r0, 0x230(r30)
    stw      r3, 0x14(r1)
    cmplw    r0, r3
    beq      .L_801997C4
    lbz      r3, 0x14(r1)
    lbz      r0, 0x15(r1)
    stb      r3, 0x230(r30)
    lbz      r3, 0x16(r1)
    stb      r0, 0x231(r30)
    lbz      r0, 0x17(r1)
    stb      r3, 0x232(r30)
    stb      r0, 0x233(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_801997C4
    li       r0, 0x1
    stw      r0, 0x250(r30)
.L_801997C4:
    li       r28, 0x0
    bl       HSD_LObjGetNbActive
    mr       r29, r3
    li       r27, 0x0
    b        .L_801997F8
.L_801997D8:
    mr       r3, r27
    bl       HSD_LObjGetActiveByIndex
    cmplwi   r3, 0x0
    beq      .L_801997F4
    mr       r3, r27
    bl       HSD_Index2LightID
    or       r28, r28, r3
.L_801997F4:
    addi     r27, r27, 0x1
.L_801997F8:
    cmpw     r27, r29
    blt      .L_801997D8
    lwz      r0, 0x20(r31)
    cmpw     r0, r28
    beq      .L_80199824
    stw      r28, 0x20(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_80199824
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_80199824:
    lwz      r0, 0x240(r30)
    cmpw     r0, r28
    beq      .L_8019992C
    stw      r28, 0x240(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_8019992C
    li       r0, 0x1
    stw      r0, 0x250(r30)
    b        .L_8019992C
.L_8019984C:
    lwz      r3, 0x8(r1)
    lwz      r0, 0x10(r31)
    stw      r3, 0x10(r1)
    cmplw    r0, r3
    beq      .L_80199894
    lbz      r3, 0x10(r1)
    lbz      r0, 0x11(r1)
    stb      r3, 0x10(r31)
    lbz      r3, 0x12(r1)
    stb      r0, 0x11(r31)
    lbz      r0, 0x13(r1)
    stb      r3, 0x12(r31)
    stb      r0, 0x13(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_80199894
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_80199894:
    lwz      r0, 0x20(r31)
    cmpwi    r0, 0x0
    beq      .L_801998BC
    li       r0, 0x0
    stw      r0, 0x20(r31)
    lwz      r0, 0x30(r31)
    cmpwi    r0, 0x0
    bne      .L_801998BC
    li       r0, 0x1
    stw      r0, 0x30(r31)
.L_801998BC:
    lwz      r3, 0x8(r1)
    lwz      r0, 0x230(r30)
    stw      r3, 0xc(r1)
    cmplw    r0, r3
    beq      .L_80199904
    lbz      r3, 0xc(r1)
    lbz      r0, 0xd(r1)
    stb      r3, 0x230(r30)
    lbz      r3, 0xe(r1)
    stb      r0, 0x231(r30)
    lbz      r0, 0xf(r1)
    stb      r3, 0x232(r30)
    stb      r0, 0x233(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_80199904
    li       r0, 0x1
    stw      r0, 0x250(r30)
.L_80199904:
    lwz      r0, 0x240(r30)
    cmpwi    r0, 0x0
    beq      .L_8019992C
    li       r0, 0x0
    stw      r0, 0x240(r30)
    lwz      r0, 0x250(r30)
    cmpwi    r0, 0x0
    bne      .L_8019992C
    li       r0, 0x1
    stw      r0, 0x250(r30)
.L_8019992C:
    lmw      r27, 0x3c(r1)
    li       r3, 0x0
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: 0x801992BC
 * @note Size: 0x2C0
 */
extern "C" void DrawGsLineSegment(CGs* gs, Mtx view, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1)
{
	// no matrix means the camera's
	MtxP mtx;
	if (view == NULL) {
		mtx = gs->mCamera->mViewMtx;
	} else {
		mtx = view;
	}
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	PrimVertex vtx[2] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
	};
	gs->mLine.Draw(mtx, vtx, 2);
}

/**
 * @note Address: 0x801990EC
 * @note Size: 0x1D0
 */
extern "C" void DrawGsLineStrip(CGs* gs, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

/**
 * @note Address: 0x80198F1C
 * @note Size: 0x1D0
 */
extern "C" void DrawGsColoredLineStrip(CGs* gs, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mLine.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	gs->mLine.Draw(gs->mCamera->mViewMtx, pos, color, count);
}

/**
 * @note Address: 0x80198D08
 * @note Size: 0x214
 */
extern "C" void DrawGsTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32 color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

/**
 * @note Address: 0x80198AF4
 * @note Size: 0x214
 */
extern "C" void DrawGsColoredTrianglePrimitive(CGs* gs, GXPrimitive type, Mtx view, Vec* pos, u32* color, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, pos, color, count, type);
	} else {
		gs->mTriangle.Draw(view, pos, color, count, type);
	}
}

/**
 * @note Address: 0x801988EC
 * @note Size: 0x208
 */
extern "C" void DrawGsTriangleVertices(CGs* gs, GXPrimitive type, Mtx view, PrimVertex* vtx, u16 count)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, vtx, count, type);
	} else {
		gs->mTriangle.Draw(view, vtx, count, type);
	}
}

/**
 * @note Address: 0x80198614
 * @note Size: 0x2D8
 */
extern "C" void DrawGsVertexQuad(CGs* gs, f32 x0, f32 y0, f32 z0, u32 color0, f32 x1, f32 y1, f32 z1, u32 color1, f32 x2, f32 y2, f32 z2,
                                 u32 color2, f32 x3, f32 y3, f32 z3, u32 color3)
{
	PrimVertex vtx[4] = {
		{ { x0, y0, z0 }, color0 },
		{ { x1, y1, z1 }, color1 },
		{ { x2, y2, z2 }, color2 },
		{ { x3, y3, z3 }, color3 },
	};
	MtxP view = gs->mCamera->mViewMtx;

	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	gs->mTriangle.Set(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTriangle.DrawLit(view, vtx, 4, GX_TRIANGLEFAN);
	} else {
		gs->mTriangle.Draw(view, vtx, 4, GX_TRIANGLEFAN);
	}
}

/**
 * @note Address: 0x80198258
 * @note Size: 0x3BC
 */
extern "C" void DrawGsCircle(CGs* gs, u32 segments, u32 centerColor, u32 edgeColor, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry,
                             f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mCircle, x, y, z, w, h, rx, ry, rz);
	gs->mCircle.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mCircle.DrawLit(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	} else {
		gs->mCircle.Draw(gs->mCamera->mViewMtx, segments, centerColor, edgeColor);
	}
}

/**
 * @note Address: 0x80197E90
 * @note Size: 0x3C8
 */
extern "C" void DrawGsQuad(CGs* gs, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry,
                           f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);
	SetFlatTransform(gs, &gs->mTile, x, y, z, w, h, rx, ry, rz);
	gs->mTile.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mTile.DrawLit(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	} else {
		gs->mTile.Draw(gs->mCamera->mViewMtx, color0, color1, color2, color3);
	}
}

/**
 * @note Address: 0x80197800
 * @note Size: 0x690
 */
extern "C" void DrawGsTexturedQuad(CGs* gs, CTexObj* tex, u32 color0, u32 color1, u32 color2, u32 color3, f32 x, f32 y, f32 z, f32 w, f32 h,
                                   f32 rx, f32 ry, f32 rz)
{
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);

	// negative size means use the texture's own
	if (w < 0.0f) {
		u16 size = GXGetTexObjWidth(tex);
		w        = size;
	}
	if (h < 0.0f) {
		u16 size = GXGetTexObjHeight(tex);
		h        = size;
	}

	SetFlatTransform(gs, &gs->mSprite2, x, y, z, w, h, rx, ry, rz);
	gs->mSprite2.mTex      = tex;
	gs->mSprite2.mFixedMtx = NULL;
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		gs->mSprite2.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	} else {
		gs->mSprite2.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, color0, color1, color2, color3);
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}
/*
    stwu     r1, -0xc0(r1)
    mflr     r0
    stw      r0, 0xc4(r1)
    stfd     f31, 0xb0(r1)
    psq_st   f31, 0xb8(r1), 0, qr0
    stfd     f30, 0xa0(r1)
    psq_st   f30, 0xa8(r1), 0, qr0
    stfd     f29, 0x90(r1)
    psq_st   f29, 0x98(r1), 0, qr0
    stfd     f28, 0x80(r1)
    psq_st   f28, 0x88(r1), 0, qr0
    stfd     f27, 0x70(r1)
    psq_st   f27, 0x78(r1), 0, qr0
    stfd     f26, 0x60(r1)
    psq_st   f26, 0x68(r1), 0, qr0
    stfd     f25, 0x50(r1)
    psq_st   f25, 0x58(r1), 0, qr0
    stfd     f24, 0x40(r1)
    psq_st   f24, 0x48(r1), 0, qr0
    stmw     r25, 0x24(r1)
    fmr      f24, f1
    mr       r27, r4
    fmr      f25, f2
    mr       r28, r5
    fmr      f26, f3
    mr       r26, r3
    fmr      f27, f4
    mr       r29, r6
    fmr      f28, f5
    mr       r30, r7
    fmr      f29, f6
    mr       r31, r8
    fmr      f30, f7
    li       r4, 0x1
    fmr      f31, f8
    li       r5, 0x0
    bl       SetGsChannelMaterialAndLighting
    lwz      r3, 0x294(r26)
    addi     r0, r26, 0x80
    cmplw    r3, r0
    bne      .L_801979C8
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0x0
    beq      .L_801978F4
    li       r0, 0x0
    stw      r0, 0xc(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_801978CC
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_801978CC:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_801978E0
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_801978E0:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_801978F4
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_801978F4:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0x0
    beq      .L_80197944
    li       r0, 0x0
    stw      r0, 0x10(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_8019791C
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_8019791C:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80197930
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80197930:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80197944
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80197944:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x4
    beq      .L_80197994
    li       r0, 0x4
    stw      r0, 0x14(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_8019796C
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_8019796C:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80197980
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80197980:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80197994
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80197994:
    lwz      r6, 0x294(r26)
    li       r5, 0x1
    li       r4, 0x0
    li       r3, 0x4
    stb      r5, 0x80(r6)
    li       r0, 0x1e
    stw      r5, 0x78(r6)
    lwz      r6, 0x294(r26)
    stw      r4, 0x84(r6)
    stw      r5, 0x88(r6)
    stw      r3, 0x8c(r6)
    stw      r0, 0x90(r6)
    stw      r5, 0x7c(r6)
.L_801979C8:
    lwz      r25, 0x290(r26)
    cmplwi   r25, 0x0
    beq      .L_80197A58
    lwz      r0, 0xc(r25)
    cmpwi    r0, 0x0
    beq      .L_801979FC
    li       r0, 0x0
    li       r3, 0x40
    stw      r0, 0xc(r25)
    bl       HSD_StateInvalidate
    mr       r4, r25
    li       r3, 0x0
    bl       HSD_SetupPEMode
.L_801979FC:
    lwz      r0, 0x10(r25)
    cmpwi    r0, 0x0
    beq      .L_80197A18
    li       r0, 0x0
    stw      r0, 0x10(r25)
    lwz      r3, 0x1c(r25)
    bl       HSD_StateSetCullMode
.L_80197A18:
    lwz      r0, 0x14(r25)
    cmpwi    r0, 0x0
    beq      .L_80197A38
    li       r0, 0x0
    stw      r0, 0x14(r25)
    lbz      r3, 0x20(r25)
    lwz      r4, 0x24(r25)
    bl       HSD_StateSetLineWidth
.L_80197A38:
    lwz      r0, 0x18(r25)
    cmpwi    r0, 0x0
    beq      .L_80197A58
    li       r0, 0x0
    stw      r0, 0x18(r25)
    lbz      r3, 0x28(r25)
    lwz      r4, 0x2c(r25)
    bl       HSD_StateSetPointSize
.L_80197A58:
    lwz      r25, 0x294(r26)
    cmplwi   r25, 0x0
    beq      .L_80197AEC
    lwz      r0, 0x74(r25)
    cmpwi    r0, 0x0
    beq      .L_80197A98
    li       r0, 0x0
    li       r3, 0x8
    stw      r0, 0x74(r25)
    bl       HSD_StateInvalidate
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    mr       r3, r25
    bl       HSD_SetupTevStageAll
.L_80197A98:
    lwz      r0, 0x78(r25)
    cmpwi    r0, 0x0
    beq      .L_80197AB4
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lbz      r3, 0x80(r25)
    bl       GXSetNumTexGens
.L_80197AB4:
    lwz      r0, 0x7c(r25)
    cmpwi    r0, 0x0
    beq      .L_80197AEC
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lwz      r3, 0x84(r25)
    li       r7, 0x0
    lwz      r4, 0x88(r25)
    li       r8, 0x7d
    lwz      r5, 0x8c(r25)
    lwz      r6, 0x90(r25)
    bl       GXSetTexCoordGen2
.L_80197AEC:
    lwz      r3, 0x298(r26)
    cmplwi   r3, 0x0
    beq      .L_80197B10
    lwz      r0, 0x30(r3)
    cmpwi    r0, 0x0
    beq      .L_80197B10
    li       r0, 0x0
    stw      r0, 0x30(r3)
    bl       HSD_SetupChannelAll
.L_80197B10:
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fcmpo    cr0, f27, f0
    bge      .L_80197B40
    mr       r3, r27
    bl       GXGetTexObjWidth
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    lfd      f1, lbl_8065F630@sda21(r0)
    stw      r0, 0x10(r1)
    lfd      f0, 0x10(r1)
    fsubs    f27, f0, f1
.L_80197B40:
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fcmpo    cr0, f28, f0
    bge      .L_80197B70
    mr       r3, r27
    bl       GXGetTexObjHeight
    clrlwi   r3, r3, 16
    lis      r0, 0x4330
    stw      r3, 0x14(r1)
    lfd      f1, lbl_8065F630@sda21(r0)
    stw      r0, 0x10(r1)
    lfd      f0, 0x10(r1)
    fsubs    f28, f0, f1
.L_80197B70:
    lwz      r3, 0x44(r26)
    lbz      r0, 0x50(r3)
    cmplwi   r0, 0x3
    bne      .L_80197BFC
    li       r0, 0x1
    fmr      f1, f24
    fmr      f2, f25
    stw      r0, 0xc40(r26)
    fmr      f3, f26
    addi     r3, r26, 0xc4c
    bl       PSMTXTrans
    fneg     f0, f28
    lfs      f2, lbl_8065F5F4@sda21(r0)
    li       r0, 0x1
    lfs      f3, lbl_8065F5F0@sda21(r0)
    fmuls    f1, f27, f2
    stw      r0, 0xc40(r26)
    fmuls    f2, f0, f2
    addi     r3, r26, 0xc7c
    bl       PSMTXScale
    li       r0, 0x1
    fneg     f1, f29
    stw      r0, 0xc40(r26)
    addi     r3, r26, 0xcac
    li       r4, 0x78
    bl       PSMTXRotRad
    fmr      f1, f30
    addi     r3, r26, 0xcdc
    li       r4, 0x79
    bl       PSMTXRotRad
    fneg     f1, f31
    addi     r3, r26, 0xd0c
    li       r4, 0x7a
    bl       PSMTXRotRad
    b        .L_80197C70
.L_80197BFC:
    li       r0, 0x1
    fmr      f1, f24
    fmr      f2, f25
    stw      r0, 0xc40(r26)
    fmr      f3, f26
    addi     r3, r26, 0xc4c
    bl       PSMTXTrans
    lfs      f0, lbl_8065F5F4@sda21(r0)
    li       r0, 0x1
    stw      r0, 0xc40(r26)
    addi     r3, r26, 0xc7c
    fmuls    f1, f27, f0
    lfs      f3, lbl_8065F5F0@sda21(r0)
    fmuls    f2, f28, f0
    bl       PSMTXScale
    li       r0, 0x1
    fmr      f1, f29
    stw      r0, 0xc40(r26)
    addi     r3, r26, 0xcac
    li       r4, 0x78
    bl       PSMTXRotRad
    fmr      f1, f30
    addi     r3, r26, 0xcdc
    li       r4, 0x79
    bl       PSMTXRotRad
    fmr      f1, f31
    addi     r3, r26, 0xd0c
    li       r4, 0x7a
    bl       PSMTXRotRad
.L_80197C70:
    lwz      r0, 0x29c(r26)
    cmpwi    r0, 0x0
    beq      .L_80197C8C
    li       r0, 0x2
    stb      r0, 0xc44(r26)
    stfs     f31, 0xc48(r26)
    b        .L_80197C94
.L_80197C8C:
    li       r0, 0x0
    stb      r0, 0xc44(r26)
.L_80197C94:
    li       r0, 0x1
    li       r3, 0x0
    stw      r0, 0xc40(r26)
    addi     r0, r26, 0x1b0
    lwz      r4, 0x17a0(r26)
    stw      r4, 0xd9c(r26)
    stw      r27, 0xda8(r26)
    stw      r3, 0xda0(r26)
    lwz      r3, 0x298(r26)
    cmplw    r3, r0
    bne      .L_80197CFC
    lbz      r0, 0x1c4(r26)
    cmplwi   r0, 0x0
    beq      .L_80197CFC
    stw      r31, 0x8(r1)
    mr       r8, r28
    mr       r9, r29
    mr       r10, r30
    lwz      r4, 0x44(r26)
    addi     r3, r26, 0xc40
    li       r5, 0x0
    li       r6, 0x1e
    addi     r4, r4, 0xe8
    li       r7, 0x1
    bl       DrawLit__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
    b        .L_80197D28
.L_80197CFC:
    stw      r31, 0x8(r1)
    mr       r8, r28
    mr       r9, r29
    mr       r10, r30
    lwz      r4, 0x44(r26)
    addi     r3, r26, 0xc40
    li       r5, 0x0
    li       r6, 0x1e
    addi     r4, r4, 0xe8
    li       r7, 0x1
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
.L_80197D28:
    lwz      r3, 0x294(r26)
    addi     r0, r26, 0x80
    cmplw    r3, r0
    bne      .L_80197E3C
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0xff
    beq      .L_80197D88
    li       r0, 0xff
    stw      r0, 0xc(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80197D60
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80197D60:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80197D74
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80197D74:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80197D88
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80197D88:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0xff
    beq      .L_80197DD8
    li       r0, 0xff
    stw      r0, 0x10(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80197DB0
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80197DB0:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80197DC4
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80197DC4:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80197DD8
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80197DD8:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x4
    beq      .L_80197E28
    li       r0, 0x4
    stw      r0, 0x14(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80197E00
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80197E00:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80197E14
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80197E14:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80197E28
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80197E28:
    lwz      r4, 0x294(r26)
    li       r3, 0x0
    li       r0, 0x1
    stb      r3, 0x80(r4)
    stw      r0, 0x78(r4)
.L_80197E3C:
    psq_l    f31, 0xb8(r1), 0, qr0
    lfd      f31, 0xb0(r1)
    psq_l    f30, 0xa8(r1), 0, qr0
    lfd      f30, 0xa0(r1)
    psq_l    f29, 0x98(r1), 0, qr0
    lfd      f29, 0x90(r1)
    psq_l    f28, 0x88(r1), 0, qr0
    lfd      f28, 0x80(r1)
    psq_l    f27, 0x78(r1), 0, qr0
    lfd      f27, 0x70(r1)
    psq_l    f26, 0x68(r1), 0, qr0
    lfd      f26, 0x60(r1)
    psq_l    f25, 0x58(r1), 0, qr0
    lfd      f25, 0x50(r1)
    psq_l    f24, 0x48(r1), 0, qr0
    lfd      f24, 0x40(r1)
    lmw      r25, 0x24(r1)
    lwz      r0, 0xc4(r1)
    mtlr     r0
    addi     r1, r1, 0xc0
    blr
*/

/**
 * @note Address: 0x80197538
 * @note Size: 0x2C8
 */
extern "C" void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, 0, color);
	SetupDrawState(gs);
	gs->mCube.SetTrans(tx, ty, tz);
	gs->mCube.SetScale(sx, sy, sz);
	gs->mCube.SetRot(rx, ry, rz);
	gs->mCube.mRotOrder = gs->_17A0;
	gs->mCube.mFixedMtx = NULL;
	gs->mCube.Draw(gs->mCamera->mViewMtx);
}

/**
 * @note Address: 0x80197250
 * @note Size: 0x2E8
 */
extern "C" void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, 0, color);
	SetupDrawState(gs);
	gs->mSphere.SetTrans(tx, ty, tz);
	gs->mSphere.SetScale(sx, sy, sz);
	gs->mSphere.SetRot(rx, ry, rz);
	gs->mSphere.mRotOrder = gs->_17A0;
	gs->mSphere.mFixedMtx = NULL;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

/**
 * @note Address: 0x801970B0
 * @note Size: 0x1A0
 */
extern "C" void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor)
{
	SetGsChannelMaterialAndLighting(gs, 0, color);
	SetupDrawState(gs);
	gs->mSphere.mFixedMtx = mtx;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

/**
 * @note Address: 0x80196DD8
 * @note Size: 0x2D8
 */
extern "C" void DrawGsCylinder(CGs* gs, u32 color, u8 sides, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, 0, color);
	SetupDrawState(gs);
	gs->mCylinder.SetTrans(tx, ty, tz);
	gs->mCylinder.SetScale(sx, sy, sz);
	gs->mCylinder.SetRot(rx, ry, rz);
	gs->mCylinder.mRotOrder = gs->_17A0;
	gs->mCylinder.mFixedMtx = NULL;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

/**
 * @note Address: 0x80196C28
 * @note Size: 0x1B0
 */
extern "C" void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides)
{
	SetGsChannelMaterialAndLighting(gs, 0, color);
	SetupDrawState(gs);
	gs->mCylinder.mFixedMtx = mtx;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

/**
 * @note Address: 0x80196C0C
 * @note Size: 0x1C
 */
extern "C" void SetGsFixedGlyphWidthScale(CGs* gs, f32 width)
{
#if !defined(VERSION_JP)
	if (gs->mUseShiftJis) {
		gs->mShiftJisWidthScale = width;
	} else {
		gs->mSingleByteWidthScale = width;
	}
#endif
}

// Keep the width calculation inline in both tiled-text callers.
static inline f32 GetTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size, f32 halfSize)
{
	CFont font;
	s32 charWidth;
	f32 width[2] = { 0.0f, 0.0f };
	TextGlyph glyph;
	const char* p = text;

	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			width[0] = 0.0f;
			continue;
		}
		GetFontGlyphTexture(&font, glyph.mStr, &charWidth);
		if (size > 0.0f) {
			if (proportional) {
				width[0] += charWidth * size / 24.0f;
#if defined(VERSION_JP)
			} else if (IsSjisLead(glyph.mStr[0]) & 1) {
				width[0] += size;
			} else {
				width[0] += size * 0.5f;
			}
#else
			} else if (gs->mUseShiftJis) {
				if (IsSjisLead(glyph.mStr[0]) & 1) {
					width[0] += size * gs->mShiftJisWidthScale;
				} else {
					width[0] += halfSize * gs->mShiftJisWidthScale;
				}
			} else {
				width[0] += halfSize * gs->mSingleByteWidthScale;
			}
#endif
		} else {
			width[0] += charWidth;
		}
		if (width[0] > width[1]) {
			width[1] = width[0];
		}
	}
	return width[1];
}
/**
 * @note Address: 0x801968BC
 * @note Size: 0x350
 */
extern "C" f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	return GetTextWidth(gs, text, proportional, size, 0.5f * size);
}

/*
    stwu     r1, -0x1a0(r1)
    mflr     r0
    stw      r0, 0x1a4(r1)
    stfd     f31, 0x190(r1)
    psq_st   f31, 0x198(r1), 0, qr0
    stfd     f30, 0x180(r1)
    psq_st   f30, 0x188(r1), 0, qr0
    stw      r31, 0x17c(r1)
    stw      r30, 0x178(r1)
    stw      r29, 0x174(r1)
    fmr      f30, f1
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lwz      r6, lbl_806602B4@sda21(r0)
    lis      r8, __vt__5CFont@ha
    lwz      r0, lbl_806602B8@sda21(r0)
    lis      r7, __vt__7CTexObj@ha
    addi     r9, r8, __vt__5CFont@l
    addi     r8, r7, __vt__7CTexObj@l
    li       r7, 0x0
    fmuls    f31, f0, f30
    stw      r9, 0x15c(r1)
    mr       r29, r3
    mr       r31, r4
    mr       r30, r5
    stw      r8, 0x158(r1)
    stw      r7, 0x44(r1)
    stw      r6, 0xc(r1)
    stw      r0, 0x10(r1)
    b        .L_80196A84
.L_80196930:
    lbz      r3, 0x14(r1)
    extsb.   r0, r3
    beq      .L_80196A84
    extsb    r0, r3
    cmpwi    r0, 0xa
    bne      .L_80196954
    lfs      f0, lbl_8065F5EC@sda21(r0)
    stfs     f0, 0xc(r1)
    b        .L_80196A84
.L_80196954:
    addi     r3, r1, 0x20
    addi     r4, r1, 0x14
    addi     r5, r1, 0x8
    bl       GetFontGlyphTexture
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fcmpo    cr0, f30, f0
    ble      .L_80196A44
    cmpwi    r30, 0x0
    beq      .L_801969B4
    lwz      r3, 0x8(r1)
    lis      r0, 0x4330
    stw      r0, 0x160(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065F620@sda21(r0)
    stw      r0, 0x164(r1)
    lfs      f0, lbl_8065F628@sda21(r0)
    lfd      f1, 0x160(r1)
    lfs      f3, 0xc(r1)
    fsubs    f1, f1, f2
    fmuls    f1, f1, f30
    fdivs    f0, f1, f0
    fadds    f0, f3, f0
    stfs     f0, 0xc(r1)
    b        .L_80196A70
.L_801969B4:
    lwz      r0, 0x17a4(r29)
    cmpwi    r0, 0x0
    beq      .L_80196A30
    lbz      r3, 0x14(r1)
    li       r4, 0x0
    cmplwi   r3, 0x81
    blt      .L_801969DC
    clrlwi   r0, r3, 24
    cmplwi   r0, 0x9f
    ble      .L_801969F0
.L_801969DC:
    clrlwi   r0, r3, 24
    cmplwi   r0, 0xe0
    blt      .L_801969F4
    cmplwi   r0, 0xfc
    bgt      .L_801969F4
.L_801969F0:
    li       r4, 0x1
.L_801969F4:
    clrlwi   r3, r4, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_80196A1C
    lfs      f1, 0x17a8(r29)
    lfs      f0, 0xc(r1)
    fmadds   f0, f30, f1, f0
    stfs     f0, 0xc(r1)
    b        .L_80196A70
.L_80196A1C:
    lfs      f1, 0x17a8(r29)
    lfs      f0, 0xc(r1)
    fmadds   f0, f31, f1, f0
    stfs     f0, 0xc(r1)
    b        .L_80196A70
.L_80196A30:
    lfs      f1, 0x17ac(r29)
    lfs      f0, 0xc(r1)
    fmadds   f0, f31, f1, f0
    stfs     f0, 0xc(r1)
    b        .L_80196A70
.L_80196A44:
    lwz      r3, 0x8(r1)
    lis      r0, 0x4330
    stw      r0, 0x160(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065F620@sda21(r0)
    stw      r0, 0x164(r1)
    lfs      f2, 0xc(r1)
    lfd      f0, 0x160(r1)
    fsubs    f0, f0, f1
    fadds    f0, f2, f0
    stfs     f0, 0xc(r1)
.L_80196A70:
    lfs      f1, 0xc(r1)
    lfs      f0, 0x10(r1)
    fcmpo    cr0, f1, f0
    ble      .L_80196A84
    stfs     f1, 0x10(r1)
.L_80196A84:
    lbz      r4, 0x0(r31)
    extsb    r0, r4
    cmpwi    r0, 0xa
    beq      .L_80196AB8
    bge      .L_80196AA4
    cmpwi    r0, 0x0
    beq      .L_80196AB0
    b        .L_80196B48
.L_80196AA4:
    cmpwi    r0, 0x5c
    beq      .L_80196AC4
    b        .L_80196B48
.L_80196AB0:
    li       r0, 0x0
    b        .L_80196BB8
.L_80196AB8:
    li       r0, 0xa
    stb      r0, 0x14(r1)
    b        .L_80196BB4
.L_80196AC4:
    lbzu     r3, 0x1(r31)
    extsb    r0, r3
    cmpwi    r0, 0x62
    beq      .L_80196B18
    bge      .L_80196AF0
    cmpwi    r0, 0x43
    beq      .L_80196AF8
    bge      .L_80196B38
    cmpwi    r0, 0x42
    bge      .L_80196B18
    b        .L_80196B38
.L_80196AF0:
    cmpwi    r0, 0x64
    bge      .L_80196B38
.L_80196AF8:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x18(r1)
    addi     r31, r31, 0x8
    stb      r0, 0x14(r1)
    b        .L_80196BB4
.L_80196B18:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x1c(r1)
    addi     r31, r31, 0x8
    stb      r0, 0x14(r1)
    b        .L_80196BB4
.L_80196B38:
    li       r0, 0x0
    stb      r3, 0x14(r1)
    stb      r0, 0x15(r1)
    b        .L_80196BB4
.L_80196B48:
    lwz      r0, 0x17a4(r29)
    cmpwi    r0, 0x0
    beq      .L_80196BA8
    cmplwi   r4, 0x81
    li       r3, 0x0
    blt      .L_80196B6C
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_80196B80
.L_80196B6C:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_80196B84
    cmplwi   r0, 0xfc
    bgt      .L_80196B84
.L_80196B80:
    li       r3, 0x1
.L_80196B84:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_80196BA8
    lbzu     r0, 0x1(r31)
    stb      r4, 0x14(r1)
    stb      r0, 0x15(r1)
    b        .L_80196BB4
.L_80196BA8:
    li       r0, 0x0
    stb      r4, 0x14(r1)
    stb      r0, 0x15(r1)
.L_80196BB4:
    addi     r0, r31, 0x1
.L_80196BB8:
    cmplwi   r0, 0x0
    mr       r31, r0
    bne      .L_80196930
    lis      r4, __vt__5CFont@ha
    lis      r3, __vt__7CTexObj@ha
    addi     r4, r4, __vt__5CFont@l
    lfs      f1, 0x10(r1)
    addi     r0, r3, __vt__7CTexObj@l
    stw      r4, 0x15c(r1)
    stw      r0, 0x158(r1)
    psq_l    f31, 0x198(r1), 0, qr0
    lfd      f31, 0x190(r1)
    psq_l    f30, 0x188(r1), 0, qr0
    lfd      f30, 0x180(r1)
    lwz      r31, 0x17c(r1)
    lwz      r30, 0x178(r1)
    lwz      r0, 0x1a4(r1)
    lwz      r29, 0x174(r1)
    mtlr     r0
    addi     r1, r1, 0x1a0
    blr
*/

/**
 * @note Address: 0x80196708
 * @note Size: 0x1B4
 */
// how tall the text is, one line height per line
extern "C" f32 MeasureGameTextHeight(CGs* gs, const char* text, f32 size)
{
	TextGlyph glyph;
	f32 height = size > 0.0f ? size : 24.0f;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\n') {
			height += size > 0.0f ? size : 24.0f;
		}
	}
	return height;
}
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    stfd     f31, 0x20(r1)
    psq_st   f31, 0x28(r1), 0, qr0
    stfd     f30, 0x10(r1)
    psq_st   f30, 0x18(r1), 0, qr0
    stw      r31, 0xc(r1)
    stw      r30, 0x8(r1)
    fmr      f30, f1
    lfs      f0, lbl_8065F5EC@sda21(r0)
    mr       r30, r3
    fcmpo    cr0, f30, f0
    ble      .L_80196748
    fmr      f31, f30
    b        .L_80196778
.L_80196748:
    lfs      f31, lbl_8065F628@sda21(r0)
    b        .L_80196778
.L_80196750:
    extsb    r0, r3
    cmpwi    r0, 0xa
    bne      .L_80196778
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fcmpo    cr0, f30, f0
    ble      .L_80196770
    fmr      f0, f30
    b        .L_80196774
.L_80196770:
    lfs      f0, lbl_8065F628@sda21(r0)
.L_80196774:
    fadds    f31, f31, f0
.L_80196778:
    lbz      r5, 0x0(r4)
    mr       r31, r4
    extsb    r0, r5
    cmpwi    r0, 0xa
    beq      .L_801967B0
    bge      .L_8019679C
    cmpwi    r0, 0x0
    beq      .L_801967A8
    b        .L_80196824
.L_8019679C:
    cmpwi    r0, 0x5c
    beq      .L_801967B8
    b        .L_80196824
.L_801967A8:
    li       r4, 0x0
    b        .L_80196888
.L_801967B0:
    li       r3, 0xa
    b        .L_80196884
.L_801967B8:
    lbz      r3, 0x1(r4)
    addi     r31, r4, 0x1
    extsb    r0, r3
    cmpwi    r0, 0x62
    beq      .L_80196808
    bge      .L_801967E8
    cmpwi    r0, 0x43
    beq      .L_801967F0
    bge      .L_80196884
    cmpwi    r0, 0x42
    bge      .L_80196808
    b        .L_80196884
.L_801967E8:
    cmpwi    r0, 0x64
    bge      .L_80196884
.L_801967F0:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r3, 0x0
    addi     r31, r31, 0x8
    b        .L_80196884
.L_80196808:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r3, 0x0
    addi     r31, r31, 0x8
    b        .L_80196884
    b        .L_80196884
.L_80196824:
    lwz      r0, 0x17a4(r30)
    cmpwi    r0, 0x0
    beq      .L_80196880
    cmplwi   r5, 0x81
    li       r3, 0x0
    blt      .L_80196848
    clrlwi   r0, r5, 24
    cmplwi   r0, 0x9f
    ble      .L_8019685C
.L_80196848:
    clrlwi   r0, r5, 24
    cmplwi   r0, 0xe0
    blt      .L_80196860
    cmplwi   r0, 0xfc
    bgt      .L_80196860
.L_8019685C:
    li       r3, 0x1
.L_80196860:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_80196880
    mr       r3, r5
    addi     r31, r31, 0x1
    b        .L_80196884
.L_80196880:
    mr       r3, r5
.L_80196884:
    addi     r4, r31, 0x1
.L_80196888:
    cmplwi   r4, 0x0
    bne      .L_80196750
    fmr      f1, f31
    psq_l    f31, 0x28(r1), 0, qr0
    lfd      f31, 0x20(r1)
    psq_l    f30, 0x18(r1), 0, qr0
    lfd      f30, 0x10(r1)
    lwz      r31, 0xc(r1)
    lwz      r0, 0x34(r1)
    lwz      r30, 0x8(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: 0x80196610
 * @note Size: 0xF8
 */
// same, with each character as wide as its glyph
extern "C" void DrawGsProportionalTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, TRUE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x80196518
 * @note Size: 0xF8
 */
// printf into a buffer and draw it with fixed width characters
extern "C" void DrawGsFixedWidthTextFormat(CGs* gs, u32 color, const char* format, f32 x, f32 y, f32 z, f32 size, f32 height, ...)
{
	char buf[256];
	va_list args;
	va_start(args, height);
	vsprintf(buf, format, args);
	DrawGsText(gs, color, FALSE, buf, x, y, z, size, height);
}

/**
 * @note Address: 0x801964E0
 * @note Size: 0x38
 */

/**
 * @note Address: 0x8019643C
 * @note Size: 0xA4
 */

/**
 * @note Address: 0x80196404
 * @note Size: 0x38
 */

/**
 * @note Address: 0x801950C8
 * @note Size: 0x133C
 */
// draws text with one sprite per character and a background tile behind each when the
// background color isn't zero
extern "C" void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height)
{
	f32 offsX;
	f32 curX = x;
	f32 curY = y;
	TextGlyph glyph;
	s32 i;

	// an ortho camera has y going up
	f32 lineHeight = gs->mCamera->projection_type == PROJ_ORTHO ? -height : height;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	static TextChar sGsTextChars[256];
	static TextChar* sCurrentTextChar;

	f32 textWidth = GetTextWidth(gs, text, proportional, size, 0.5f * size);

	f32 centerX = x + textWidth / 2.0f;
	f32 centerY = y - lineHeight;
	f32 scaleX  = size / 2.0f;
	f32 scaleY  = lineHeight / 2.0f;
	s32 count   = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = x;
			curY += lineHeight;
			continue;
		}

		sCurrentTextChar               = &sGsTextChars[count];
		sCurrentTextChar->mSprite.mTex = &sCurrentTextChar->mTex;
		s32 charWidth                  = sCurrentTextChar->mSprite.mTex->SetChar(glyph.mStr);
		BOOL wide                      = IsSjisLead(glyph.mStr[0]);
		f32 advance;
		if (proportional) {
			advance = charWidth * (size / 24.0f);
#if defined(VERSION_JP)
		} else if (wide) {
			advance = size;
		} else {
			advance = size * 0.5f;
		}
#else
		} else if (gs->mUseShiftJis) {
			if (wide) {
				advance = size * gs->mShiftJisWidthScale;
			} else {
				advance = size * (0.5f * gs->mShiftJisWidthScale);
			}
		} else {
			advance = size * (0.5f * gs->mSingleByteWidthScale);
		}
#endif

		offsX = curX - x;
		f32 u = 2.0f * (offsX / size) + 1.0f - textWidth / size;
		f32 v = 2.0f * (-(curY - y) / lineHeight) + 1.0f;
#if defined(VERSION_JP)
		if (!proportional && wide && charWidth < size) {
			u += (size - charWidth) * 0.5f / size;
		}
#else
		if (gs->mUseShiftJis) {
			// with fixed widths a character narrower than its cell gets centered in it
			if (!proportional && wide && charWidth < size) {
				u += (size - charWidth) / 2.0f / size;
			}
		} else if (!proportional) {
			u += (size - charWidth) / 2.0f / size;
		}
#endif

		sCurrentTextChar->mSprite.SetTrans(centerX, centerY, z);
		sCurrentTextChar->mSprite.SetScale(scaleX, scaleY, 1.0f);
#if defined(VERSION_JP)
		sCurrentTextChar->mSprite.SetRot(0.0f, 0.0f, 0.0f);
#else
		sCurrentTextChar->mSprite.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
#endif
		SetCenter(&sCurrentTextChar->mSprite, u, v, 0.0f);
		SetColor(&sCurrentTextChar->mSprite, glyph.mColor);
		SetBillboard(&sCurrentTextChar->mSprite, gs->mBillboardEnabled);

		if (sCurrentTextChar->mHasBg = glyph.mBgColor != 0) {
			sCurrentTextChar->mBg.SetTrans(centerX, centerY, z);
			sCurrentTextChar->mBg.SetScale(advance / 2.0f, scaleY, 1.0f);
#if defined(VERSION_JP)
			sCurrentTextChar->mBg.SetRot(0.0f, 0.0f, 0.0f);
#else
			sCurrentTextChar->mBg.SetRot(gs->mTextRot.x, gs->mTextRot.y, gs->mTextRot.z);
#endif
			SetCenter(&sCurrentTextChar->mBg, 2.0f * (offsX / advance) + 1.0f - textWidth / advance, v, 0.0f);
			SetColor(&sCurrentTextChar->mBg, glyph.mBgColor);
			SetBillboard(&sCurrentTextChar->mBg, gs->mBillboardEnabled);
		}
		curX += advance;
		count++;
	}

	// backgrounds first, flat color
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.DrawLit(gs->mCamera->mViewMtx, sGsTextChars[i].mBg._168, sGsTextChars[i].mBg._16C,
				                            sGsTextChars[i].mBg._170, sGsTextChars[i].mBg._174);
			}
		}
	} else {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.Draw(gs->mCamera->mViewMtx, sGsTextChars[i].mBg._168, sGsTextChars[i].mBg._16C,
				                         sGsTextChars[i].mBg._170, sGsTextChars[i].mBg._174);
			}
		}
	}

	// then the characters, textured
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_MODULATE);
		gs->mCurTevStage->SetNumTexGens(1);
		gs->mCurTevStage->SetTexGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		for (i = 0; i < count; i++) {
			sGsTextChars[i].mSprite.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite._16C,
			                                sGsTextChars[i].mSprite._170, sGsTextChars[i].mSprite._174, sGsTextChars[i].mSprite._178);
		}
	} else {
		for (i = 0; i < count; i++) {
			sGsTextChars[i].mSprite.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite._16C,
			                             sGsTextChars[i].mSprite._170, sGsTextChars[i].mSprite._174, sGsTextChars[i].mSprite._178);
		}
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}
/*
    stwu     r1, -0x2c0(r1)
    mflr     r0
    stw      r0, 0x2c4(r1)
    stfd     f31, 0x2b0(r1)
    psq_st   f31, 0x2b8(r1), 0, qr0
    stfd     f30, 0x2a0(r1)
    psq_st   f30, 0x2a8(r1), 0, qr0
    stfd     f29, 0x290(r1)
    psq_st   f29, 0x298(r1), 0, qr0
    stfd     f28, 0x280(r1)
    psq_st   f28, 0x288(r1), 0, qr0
    stfd     f27, 0x270(r1)
    psq_st   f27, 0x278(r1), 0, qr0
    stfd     f26, 0x260(r1)
    psq_st   f26, 0x268(r1), 0, qr0
    stfd     f25, 0x250(r1)
    psq_st   f25, 0x258(r1), 0, qr0
    stfd     f24, 0x240(r1)
    psq_st   f24, 0x248(r1), 0, qr0
    stfd     f23, 0x230(r1)
    psq_st   f23, 0x238(r1), 0, qr0
    stfd     f22, 0x220(r1)
    psq_st   f22, 0x228(r1), 0, qr0
    stfd     f21, 0x210(r1)
    psq_st   f21, 0x218(r1), 0, qr0
    stfd     f20, 0x200(r1)
    psq_st   f20, 0x208(r1), 0, qr0
    stfd     f19, 0x1f0(r1)
    psq_st   f19, 0x1f8(r1), 0, qr0
    stfd     f18, 0x1e0(r1)
    psq_st   f18, 0x1e8(r1), 0, qr0
    stfd     f17, 0x1d0(r1)
    psq_st   f17, 0x1d8(r1), 0, qr0
    stfd     f16, 0x1c0(r1)
    psq_st   f16, 0x1c8(r1), 0, qr0
    stfd     f15, 0x1b0(r1)
    psq_st   f15, 0x1b8(r1), 0, qr0
    stfd     f14, 0x1a0(r1)
    psq_st   f14, 0x1a8(r1), 0, qr0
    stmw     r25, 0x184(r1)
    mr       r31, r3
    lwz      r3, 0x44(r3)
    fmr      f27, f1
    mr       r25, r5
    fmr      f28, f2
    lbz      r0, 0x50(r3)
    fmr      f29, f3
    mr       r26, r6
    fmr      f30, f4
    cmplwi   r0, 0x3
    fmr      f22, f27
    fmr      f21, f28
    bne      .L_801951A4
    fneg     f24, f5
    b        .L_801951A8
.L_801951A4:
    fmr      f24, f5
.L_801951A8:
    lbz      r0, lbl_8065B850@sda21(r0)
    li       r3, 0x0
    stw      r4, 0x2c(r1)
    extsb.   r0, r0
    stw      r3, 0x30(r1)
    bne      .L_80195204
    lis      r3, sGsTextChars@ha
    lis      r4, fn_801964E0@ha
    lis      r5, fn_8019643C@ha
    li       r6, 0x410
    addi     r3, r3, sGsTextChars@l
    addi     r4, r4, fn_801964E0@l
    addi     r5, r5, fn_8019643C@l
    li       r7, 0x100
    bl       __construct_array
    lis      r3, fn_80196404@ha
    lis      r5, sGsTextCharsDestructorChain@ha
    addi     r4, r3, fn_80196404@l
    li       r3, 0x0
    addi     r5, r5, sGsTextCharsDestructorChain@l
    bl       __register_global_object
    li       r0, 0x1
    stb      r0, lbl_8065B850@sda21(r0)
.L_80195204:
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lis      r5, __vt__5CFont@ha
    lwz      r3, lbl_806602B4@sda21(r0)
    lis      r4, __vt__7CTexObj@ha
    lwz      r0, lbl_806602B8@sda21(r0)
    addi     r6, r5, __vt__5CFont@l
    addi     r5, r4, __vt__7CTexObj@l
    li       r4, 0x0
    fmuls    f25, f0, f30
    stw      r6, 0x170(r1)
    mr       r30, r26
    stw      r5, 0x16c(r1)
    stw      r4, 0x58(r1)
    stw      r3, 0x14(r1)
    stw      r0, 0x18(r1)
    b        .L_80195398
.L_80195244:
    lbz      r3, 0x1c(r1)
    extsb.   r0, r3
    beq      .L_80195398
    extsb    r0, r3
    cmpwi    r0, 0xa
    bne      .L_80195268
    lfs      f0, lbl_8065F5EC@sda21(r0)
    stfs     f0, 0x14(r1)
    b        .L_80195398
.L_80195268:
    addi     r3, r1, 0x34
    addi     r4, r1, 0x1c
    addi     r5, r1, 0x10
    bl       GetFontGlyphTexture
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fcmpo    cr0, f30, f0
    ble      .L_80195358
    cmpwi    r25, 0x0
    beq      .L_801952C8
    lwz      r3, 0x10(r1)
    lis      r0, 0x4330
    stw      r0, 0x178(r1)
    xoris    r0, r3, 0x8000
    lfd      f2, lbl_8065F620@sda21(r0)
    stw      r0, 0x17c(r1)
    lfs      f0, lbl_8065F628@sda21(r0)
    lfd      f1, 0x178(r1)
    lfs      f3, 0x14(r1)
    fsubs    f1, f1, f2
    fmuls    f1, f1, f30
    fdivs    f0, f1, f0
    fadds    f0, f3, f0
    stfs     f0, 0x14(r1)
    b        .L_80195384
.L_801952C8:
    lwz      r0, 0x17a4(r31)
    cmpwi    r0, 0x0
    beq      .L_80195344
    lbz      r3, 0x1c(r1)
    li       r4, 0x0
    cmplwi   r3, 0x81
    blt      .L_801952F0
    clrlwi   r0, r3, 24
    cmplwi   r0, 0x9f
    ble      .L_80195304
.L_801952F0:
    clrlwi   r0, r3, 24
    cmplwi   r0, 0xe0
    blt      .L_80195308
    cmplwi   r0, 0xfc
    bgt      .L_80195308
.L_80195304:
    li       r4, 0x1
.L_80195308:
    clrlwi   r3, r4, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_80195330
    lfs      f1, 0x17a8(r31)
    lfs      f0, 0x14(r1)
    fmadds   f0, f30, f1, f0
    stfs     f0, 0x14(r1)
    b        .L_80195384
.L_80195330:
    lfs      f1, 0x17a8(r31)
    lfs      f0, 0x14(r1)
    fmadds   f0, f25, f1, f0
    stfs     f0, 0x14(r1)
    b        .L_80195384
.L_80195344:
    lfs      f1, 0x17ac(r31)
    lfs      f0, 0x14(r1)
    fmadds   f0, f25, f1, f0
    stfs     f0, 0x14(r1)
    b        .L_80195384
.L_80195358:
    lwz      r3, 0x10(r1)
    lis      r0, 0x4330
    stw      r0, 0x178(r1)
    xoris    r0, r3, 0x8000
    lfd      f1, lbl_8065F620@sda21(r0)
    stw      r0, 0x17c(r1)
    lfs      f2, 0x14(r1)
    lfd      f0, 0x178(r1)
    fsubs    f0, f0, f1
    fadds    f0, f2, f0
    stfs     f0, 0x14(r1)
.L_80195384:
    lfs      f1, 0x14(r1)
    lfs      f0, 0x18(r1)
    fcmpo    cr0, f1, f0
    ble      .L_80195398
    stfs     f1, 0x18(r1)
.L_80195398:
    lbz      r4, 0x0(r30)
    extsb    r0, r4
    cmpwi    r0, 0xa
    beq      .L_801953CC
    bge      .L_801953B8
    cmpwi    r0, 0x0
    beq      .L_801953C4
    b        .L_8019545C
.L_801953B8:
    cmpwi    r0, 0x5c
    beq      .L_801953D8
    b        .L_8019545C
.L_801953C4:
    li       r30, 0x0
    b        .L_801954CC
.L_801953CC:
    li       r0, 0xa
    stb      r0, 0x1c(r1)
    b        .L_801954C8
.L_801953D8:
    lbzu     r3, 0x1(r30)
    extsb    r0, r3
    cmpwi    r0, 0x62
    beq      .L_8019542C
    bge      .L_80195404
    cmpwi    r0, 0x43
    beq      .L_8019540C
    bge      .L_8019544C
    cmpwi    r0, 0x42
    bge      .L_8019542C
    b        .L_8019544C
.L_80195404:
    cmpwi    r0, 0x64
    bge      .L_8019544C
.L_8019540C:
    addi     r3, r30, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x20(r1)
    addi     r30, r30, 0x8
    stb      r0, 0x1c(r1)
    b        .L_801954C8
.L_8019542C:
    addi     r3, r30, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x24(r1)
    addi     r30, r30, 0x8
    stb      r0, 0x1c(r1)
    b        .L_801954C8
.L_8019544C:
    li       r0, 0x0
    stb      r3, 0x1c(r1)
    stb      r0, 0x1d(r1)
    b        .L_801954C8
.L_8019545C:
    lwz      r0, 0x17a4(r31)
    cmpwi    r0, 0x0
    beq      .L_801954BC
    cmplwi   r4, 0x81
    li       r3, 0x0
    blt      .L_80195480
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_80195494
.L_80195480:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_80195498
    cmplwi   r0, 0xfc
    bgt      .L_80195498
.L_80195494:
    li       r3, 0x1
.L_80195498:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_801954BC
    lbzu     r0, 0x1(r30)
    stb      r4, 0x1c(r1)
    stb      r0, 0x1d(r1)
    b        .L_801954C8
.L_801954BC:
    li       r0, 0x0
    stb      r4, 0x1c(r1)
    stb      r0, 0x1d(r1)
.L_801954C8:
    addi     r30, r30, 0x1
.L_801954CC:
    cmplwi   r30, 0x0
    bne      .L_80195244
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lis      r3, __vt__5CFont@ha
    lfs      f26, 0x18(r1)
    lis      r4, __vt__7CTexObj@ha
    addi     r5, r3, __vt__5CFont@l
    lis      r3, sGsTextChars@ha
    addi     r0, r4, __vt__7CTexObj@l
    fmadds   f20, f26, f0, f27
    addi     r28, r3, sGsTextChars@l
    fsubs    f19, f28, f24
    fmuls    f18, f30, f0
    stw      r5, 0x170(r1)
    fmuls    f17, f24, f0
    stw      r0, 0x16c(r1)
    mr       r29, r28
    li       r27, 0x0
    b        .L_801958D0
.L_80195518:
    lbz      r3, 0x28(r1)
    extsb.   r0, r3
    beq      .L_801958D0
    extsb    r0, r3
    cmpwi    r0, 0xa
    bne      .L_8019553C
    fmr      f22, f27
    fadds    f21, f21, f24
    b        .L_801958D0
.L_8019553C:
    stw      r29, lbl_8065B854@sda21(r0)
    addi     r4, r1, 0x28
    stw      r29, 0x268(r29)
    lwz      r3, 0x268(r29)
    bl       SetChar__7CTexObjFPCc
    lbz      r4, 0x28(r1)
    li       r5, 0x0
    cmplwi   r4, 0x81
    blt      .L_8019556C
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_80195580
.L_8019556C:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_80195584
    cmplwi   r0, 0xfc
    bgt      .L_80195584
.L_80195580:
    li       r5, 0x1
.L_80195584:
    clrlwi   r4, r5, 24
    cmpwi    r25, 0x0
    neg      r0, r4
    or       r0, r0, r4
    srwi     r5, r0, 31
    beq      .L_801955C8
    lfs      f0, lbl_8065F628@sda21(r0)
    xoris    r4, r3, 0x8000
    lis      r0, 0x4330
    stw      r4, 0x17c(r1)
    fdivs    f0, f30, f0
    lfd      f2, lbl_8065F620@sda21(r0)
    stw      r0, 0x178(r1)
    lfd      f1, 0x178(r1)
    fsubs    f1, f1, f2
    fmuls    f16, f1, f0
    b        .L_8019560C
.L_801955C8:
    lwz      r0, 0x17a4(r31)
    cmpwi    r0, 0x0
    beq      .L_801955FC
    cmpwi    r5, 0x0
    beq      .L_801955E8
    lfs      f0, 0x17a8(r31)
    fmuls    f16, f30, f0
    b        .L_8019560C
.L_801955E8:
    lfs      f1, lbl_8065F5F4@sda21(r0)
    lfs      f0, 0x17a8(r31)
    fmuls    f0, f1, f0
    fmuls    f16, f30, f0
    b        .L_8019560C
.L_801955FC:
    lfs      f1, lbl_8065F5F4@sda21(r0)
    lfs      f0, 0x17ac(r31)
    fmuls    f0, f1, f0
    fmuls    f16, f30, f0
.L_8019560C:
    fsubs    f23, f22, f27
    lwz      r0, 0x17a4(r31)
    fsubs    f0, f21, f28
    lfs      f4, lbl_8065F62C@sda21(r0)
    fdivs    f1, f26, f30
    lfs      f3, lbl_8065F5F0@sda21(r0)
    cmpwi    r0, 0x0
    fdivs    f2, f23, f30
    fneg     f0, f0
    fmadds   f2, f4, f2, f3
    fdivs    f0, f0, f24
    fsubs    f15, f2, f1
    fmadds   f31, f4, f0, f3
    beq      .L_801956A0
    cmpwi    r25, 0x0
    bne      .L_801956D8
    cmpwi    r5, 0x0
    beq      .L_801956D8
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x17c(r1)
    lfd      f2, lbl_8065F620@sda21(r0)
    stw      r0, 0x178(r1)
    lfd      f0, 0x178(r1)
    fsubs    f0, f0, f2
    fcmpo    cr0, f0, f30
    bge      .L_801956D8
    stw      r3, 0x17c(r1)
    lfs      f0, lbl_8065F5F4@sda21(r0)
    stw      r0, 0x178(r1)
    lfd      f1, 0x178(r1)
    fsubs    f1, f1, f2
    fsubs    f1, f30, f1
    fmuls    f0, f1, f0
    fdivs    f0, f0, f30
    fadds    f15, f15, f0
    b        .L_801956D8
.L_801956A0:
    cmpwi    r25, 0x0
    bne      .L_801956D8
    xoris    r3, r3, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x17c(r1)
    lfd      f2, lbl_8065F620@sda21(r0)
    stw      r0, 0x178(r1)
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lfd      f1, 0x178(r1)
    fsubs    f1, f1, f2
    fsubs    f1, f30, f1
    fmuls    f0, f1, f0
    fdivs    f0, f0, f30
    fadds    f15, f15, f0
.L_801956D8:
    lwz      r3, lbl_8065B854@sda21(r0)
    li       r0, 0x1
    fmr      f1, f20
    fmr      f2, f19
    stw      r0, 0x100(r3)
    fmr      f3, f29
    addi     r3, r3, 0x10c
    bl       PSMTXTrans
    lwz      r3, lbl_8065B854@sda21(r0)
    li       r0, 0x1
    fmr      f1, f18
    lfs      f3, lbl_8065F5F0@sda21(r0)
    fmr      f2, f17
    stw      r0, 0x100(r3)
    addi     r3, r3, 0x13c
    bl       PSMTXScale
    lwz      r30, lbl_8065B854@sda21(r0)
    li       r0, 0x1
    lfs      f1, 0x17b4(r31)
    li       r4, 0x78
    lfs      f25, 0x17bc(r31)
    addi     r3, r30, 0x16c
    lfs      f14, 0x17b8(r31)
    stw      r0, 0x100(r30)
    bl       PSMTXRotRad
    fmr      f1, f14
    addi     r3, r30, 0x19c
    li       r4, 0x79
    bl       PSMTXRotRad
    fmr      f1, f25
    addi     r3, r30, 0x1cc
    li       r4, 0x7a
    bl       PSMTXRotRad
    lwz      r3, lbl_8065B854@sda21(r0)
    lfs      f0, lbl_8065F5EC@sda21(r0)
    stfs     f15, 0x27c(r3)
    stfs     f31, 0x280(r3)
    stfs     f0, 0x284(r3)
    lwz      r3, lbl_8065B854@sda21(r0)
    lwz      r0, 0x2c(r1)
    stw      r0, 0x278(r3)
    stw      r0, 0x274(r3)
    stw      r0, 0x270(r3)
    stw      r0, 0x26c(r3)
    lwz      r0, 0x29c(r31)
    lwz      r3, lbl_8065B854@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_801957A4
    li       r0, 0x1
    stb      r0, 0x104(r3)
    b        .L_801957AC
.L_801957A4:
    li       r0, 0x0
    stb      r0, 0x104(r3)
.L_801957AC:
    li       r5, 0x1
    stw      r5, 0x100(r3)
    lwz      r4, 0x30(r1)
    lwz      r3, lbl_8065B854@sda21(r0)
    neg      r0, r4
    or       r0, r0, r4
    srwi.    r0, r0, 31
    stw      r0, 0x40c(r3)
    beq      .L_801958C4
    lwz      r3, lbl_8065B854@sda21(r0)
    fmr      f1, f20
    fmr      f2, f19
    fmr      f3, f29
    stw      r5, 0x288(r3)
    addi     r3, r3, 0x294
    bl       PSMTXTrans
    lfs      f0, lbl_8065F5F4@sda21(r0)
    fmr      f2, f17
    lwz      r3, lbl_8065B854@sda21(r0)
    li       r0, 0x1
    fmuls    f1, f16, f0
    lfs      f3, lbl_8065F5F0@sda21(r0)
    stw      r0, 0x288(r3)
    addi     r3, r3, 0x2c4
    bl       PSMTXScale
    lwz      r30, lbl_8065B854@sda21(r0)
    li       r0, 0x1
    lfs      f1, 0x17b4(r31)
    li       r4, 0x78
    lfs      f15, 0x17bc(r31)
    addi     r3, r30, 0x2f4
    lfs      f14, 0x17b8(r31)
    stw      r0, 0x288(r30)
    bl       PSMTXRotRad
    fmr      f1, f14
    addi     r3, r30, 0x324
    li       r4, 0x79
    bl       PSMTXRotRad
    fmr      f1, f15
    addi     r3, r30, 0x354
    li       r4, 0x7a
    bl       PSMTXRotRad
    fdivs    f3, f23, f16
    lfs      f4, lbl_8065F62C@sda21(r0)
    lfs      f2, lbl_8065F5F0@sda21(r0)
    lwz      r3, lbl_8065B854@sda21(r0)
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fdivs    f1, f26, f16
    fmadds   f2, f4, f3, f2
    fsubs    f1, f2, f1
    stfs     f1, 0x400(r3)
    stfs     f31, 0x404(r3)
    stfs     f0, 0x408(r3)
    lwz      r3, lbl_8065B854@sda21(r0)
    lwz      r0, 0x30(r1)
    stw      r0, 0x3fc(r3)
    stw      r0, 0x3f8(r3)
    stw      r0, 0x3f4(r3)
    stw      r0, 0x3f0(r3)
    lwz      r0, 0x29c(r31)
    lwz      r3, lbl_8065B854@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_801958B4
    li       r0, 0x1
    stb      r0, 0x28c(r3)
    b        .L_801958BC
.L_801958B4:
    li       r0, 0x0
    stb      r0, 0x28c(r3)
.L_801958BC:
    li       r0, 0x1
    stw      r0, 0x288(r3)
.L_801958C4:
    fadds    f22, f22, f16
    addi     r29, r29, 0x410
    addi     r27, r27, 0x1
.L_801958D0:
    lbz      r4, 0x0(r26)
    mr       r30, r26
    extsb    r0, r4
    cmpwi    r0, 0xa
    beq      .L_80195908
    bge      .L_801958F4
    cmpwi    r0, 0x0
    beq      .L_80195900
    b        .L_8019599C
.L_801958F4:
    cmpwi    r0, 0x5c
    beq      .L_80195914
    b        .L_8019599C
.L_80195900:
    li       r0, 0x0
    b        .L_80195A0C
.L_80195908:
    li       r0, 0xa
    stb      r0, 0x28(r1)
    b        .L_80195A08
.L_80195914:
    lbz      r3, 0x1(r26)
    addi     r30, r26, 0x1
    extsb    r0, r3
    cmpwi    r0, 0x62
    beq      .L_8019596C
    bge      .L_80195944
    cmpwi    r0, 0x43
    beq      .L_8019594C
    bge      .L_8019598C
    cmpwi    r0, 0x42
    bge      .L_8019596C
    b        .L_8019598C
.L_80195944:
    cmpwi    r0, 0x64
    bge      .L_8019598C
.L_8019594C:
    addi     r3, r30, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x2c(r1)
    addi     r30, r30, 0x8
    stb      r0, 0x28(r1)
    b        .L_80195A08
.L_8019596C:
    addi     r3, r30, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x30(r1)
    addi     r30, r30, 0x8
    stb      r0, 0x28(r1)
    b        .L_80195A08
.L_8019598C:
    li       r0, 0x0
    stb      r3, 0x28(r1)
    stb      r0, 0x29(r1)
    b        .L_80195A08
.L_8019599C:
    lwz      r0, 0x17a4(r31)
    cmpwi    r0, 0x0
    beq      .L_801959FC
    cmplwi   r4, 0x81
    li       r3, 0x0
    blt      .L_801959C0
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_801959D4
.L_801959C0:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_801959D8
    cmplwi   r0, 0xfc
    bgt      .L_801959D8
.L_801959D4:
    li       r3, 0x1
.L_801959D8:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_801959FC
    lbzu     r0, 0x1(r30)
    stb      r4, 0x28(r1)
    stb      r0, 0x29(r1)
    b        .L_80195A08
.L_801959FC:
    li       r0, 0x0
    stb      r4, 0x28(r1)
    stb      r0, 0x29(r1)
.L_80195A08:
    addi     r0, r30, 0x1
.L_80195A0C:
    cmplwi   r0, 0x0
    mr       r26, r0
    bne      .L_80195518
    mr       r3, r31
    li       r4, 0x1
    li       r5, 0x0
    bl       SetGsChannelMaterialAndLighting
    lwz      r3, 0x294(r31)
    addi     r0, r31, 0x80
    cmplw    r3, r0
    bne      .L_80195BE0
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0xff
    beq      .L_80195A88
    li       r0, 0xff
    stw      r0, 0xc(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195A60
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195A60:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195A74
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195A74:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195A88
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195A88:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0xff
    beq      .L_80195AD8
    li       r0, 0xff
    stw      r0, 0x10(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195AB0
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195AB0:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195AC4
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195AC4:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195AD8
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195AD8:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x4
    beq      .L_80195B28
    li       r0, 0x4
    stw      r0, 0x14(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B00
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195B00:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B14
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195B14:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B28
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195B28:
    lwz      r3, 0x294(r31)
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_80195B7C
    li       r0, 0x0
    stw      r0, 0x4(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B54
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195B54:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B68
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195B68:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195B7C
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195B7C:
    lwz      r0, 0x18(r3)
    cmpwi    r0, 0x4
    beq      .L_80195BCC
    li       r0, 0x4
    stw      r0, 0x18(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195BA4
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195BA4:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195BB8
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195BB8:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195BCC
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195BCC:
    lwz      r4, 0x294(r31)
    li       r3, 0x0
    li       r0, 0x1
    stb      r3, 0x80(r4)
    stw      r0, 0x78(r4)
.L_80195BE0:
    lwz      r25, 0x290(r31)
    cmplwi   r25, 0x0
    beq      .L_80195C70
    lwz      r0, 0xc(r25)
    cmpwi    r0, 0x0
    beq      .L_80195C14
    li       r0, 0x0
    li       r3, 0x40
    stw      r0, 0xc(r25)
    bl       HSD_StateInvalidate
    mr       r4, r25
    li       r3, 0x0
    bl       HSD_SetupPEMode
.L_80195C14:
    lwz      r0, 0x10(r25)
    cmpwi    r0, 0x0
    beq      .L_80195C30
    li       r0, 0x0
    stw      r0, 0x10(r25)
    lwz      r3, 0x1c(r25)
    bl       HSD_StateSetCullMode
.L_80195C30:
    lwz      r0, 0x14(r25)
    cmpwi    r0, 0x0
    beq      .L_80195C50
    li       r0, 0x0
    stw      r0, 0x14(r25)
    lbz      r3, 0x20(r25)
    lwz      r4, 0x24(r25)
    bl       HSD_StateSetLineWidth
.L_80195C50:
    lwz      r0, 0x18(r25)
    cmpwi    r0, 0x0
    beq      .L_80195C70
    li       r0, 0x0
    stw      r0, 0x18(r25)
    lbz      r3, 0x28(r25)
    lwz      r4, 0x2c(r25)
    bl       HSD_StateSetPointSize
.L_80195C70:
    lwz      r25, 0x294(r31)
    cmplwi   r25, 0x0
    beq      .L_80195D04
    lwz      r0, 0x74(r25)
    cmpwi    r0, 0x0
    beq      .L_80195CB0
    li       r0, 0x0
    li       r3, 0x8
    stw      r0, 0x74(r25)
    bl       HSD_StateInvalidate
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    mr       r3, r25
    bl       HSD_SetupTevStageAll
.L_80195CB0:
    lwz      r0, 0x78(r25)
    cmpwi    r0, 0x0
    beq      .L_80195CCC
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lbz      r3, 0x80(r25)
    bl       GXSetNumTexGens
.L_80195CCC:
    lwz      r0, 0x7c(r25)
    cmpwi    r0, 0x0
    beq      .L_80195D04
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lwz      r3, 0x84(r25)
    li       r7, 0x0
    lwz      r4, 0x88(r25)
    li       r8, 0x7d
    lwz      r5, 0x8c(r25)
    lwz      r6, 0x90(r25)
    bl       GXSetTexCoordGen2
.L_80195D04:
    lwz      r3, 0x298(r31)
    cmplwi   r3, 0x0
    beq      .L_80195D28
    lwz      r0, 0x30(r3)
    cmpwi    r0, 0x0
    beq      .L_80195D28
    li       r0, 0x0
    stw      r0, 0x30(r3)
    bl       HSD_SetupChannelAll
.L_80195D28:
    lwz      r3, 0x298(r31)
    addi     r0, r31, 0x1b0
    cmplw    r3, r0
    bne      .L_80195D90
    lbz      r0, 0x1c4(r31)
    cmplwi   r0, 0x0
    beq      .L_80195D90
    mr       r25, r28
    li       r26, 0x0
    b        .L_80195D84
.L_80195D50:
    lwz      r0, 0x40c(r25)
    cmplwi   r0, 0x0
    beq      .L_80195D7C
    lwz      r4, 0x44(r31)
    addi     r3, r25, 0x288
    lwz      r5, 0x3f0(r25)
    lwz      r6, 0x3f4(r25)
    addi     r4, r4, 0xe8
    lwz      r7, 0x3f8(r25)
    lwz      r8, 0x3fc(r25)
    bl       DrawLit__5CTileFPA4_fUlUlUlUl
.L_80195D7C:
    addi     r25, r25, 0x410
    addi     r26, r26, 0x1
.L_80195D84:
    cmpw     r26, r27
    blt      .L_80195D50
    b        .L_80195DD8
.L_80195D90:
    mr       r25, r28
    li       r26, 0x0
    b        .L_80195DD0
.L_80195D9C:
    lwz      r0, 0x40c(r25)
    cmplwi   r0, 0x0
    beq      .L_80195DC8
    lwz      r4, 0x44(r31)
    addi     r3, r25, 0x288
    lwz      r5, 0x3f0(r25)
    lwz      r6, 0x3f4(r25)
    addi     r4, r4, 0xe8
    lwz      r7, 0x3f8(r25)
    lwz      r8, 0x3fc(r25)
    bl       Draw__5CTileFPA4_fUlUlUlUl
.L_80195DC8:
    addi     r25, r25, 0x410
    addi     r26, r26, 0x1
.L_80195DD0:
    cmpw     r26, r27
    blt      .L_80195D9C
.L_80195DD8:
    lwz      r3, 0x294(r31)
    addi     r0, r31, 0x80
    cmplw    r3, r0
    bne      .L_80195FB0
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0x0
    beq      .L_80195E38
    li       r0, 0x0
    stw      r0, 0xc(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E10
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195E10:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E24
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195E24:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E38
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195E38:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0x0
    beq      .L_80195E88
    li       r0, 0x0
    stw      r0, 0x10(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E60
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195E60:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E74
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195E74:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195E88
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195E88:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x4
    beq      .L_80195ED8
    li       r0, 0x4
    stw      r0, 0x14(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195EB0
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195EB0:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195EC4
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195EC4:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195ED8
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195ED8:
    lwz      r3, 0x294(r31)
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_80195F2C
    li       r0, 0x0
    stw      r0, 0x4(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F04
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195F04:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F18
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195F18:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F2C
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195F2C:
    lwz      r0, 0x18(r3)
    cmpwi    r0, 0x0
    beq      .L_80195F7C
    li       r0, 0x0
    stw      r0, 0x18(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F54
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80195F54:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F68
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80195F68:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80195F7C
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80195F7C:
    lwz      r6, 0x294(r31)
    li       r5, 0x1
    li       r4, 0x0
    li       r3, 0x4
    stb      r5, 0x80(r6)
    li       r0, 0x1e
    stw      r5, 0x78(r6)
    lwz      r6, 0x294(r31)
    stw      r4, 0x84(r6)
    stw      r5, 0x88(r6)
    stw      r3, 0x8c(r6)
    stw      r0, 0x90(r6)
    stw      r5, 0x7c(r6)
.L_80195FB0:
    lwz      r25, 0x290(r31)
    cmplwi   r25, 0x0
    beq      .L_80196040
    lwz      r0, 0xc(r25)
    cmpwi    r0, 0x0
    beq      .L_80195FE4
    li       r0, 0x0
    li       r3, 0x40
    stw      r0, 0xc(r25)
    bl       HSD_StateInvalidate
    mr       r4, r25
    li       r3, 0x0
    bl       HSD_SetupPEMode
.L_80195FE4:
    lwz      r0, 0x10(r25)
    cmpwi    r0, 0x0
    beq      .L_80196000
    li       r0, 0x0
    stw      r0, 0x10(r25)
    lwz      r3, 0x1c(r25)
    bl       HSD_StateSetCullMode
.L_80196000:
    lwz      r0, 0x14(r25)
    cmpwi    r0, 0x0
    beq      .L_80196020
    li       r0, 0x0
    stw      r0, 0x14(r25)
    lbz      r3, 0x20(r25)
    lwz      r4, 0x24(r25)
    bl       HSD_StateSetLineWidth
.L_80196020:
    lwz      r0, 0x18(r25)
    cmpwi    r0, 0x0
    beq      .L_80196040
    li       r0, 0x0
    stw      r0, 0x18(r25)
    lbz      r3, 0x28(r25)
    lwz      r4, 0x2c(r25)
    bl       HSD_StateSetPointSize
.L_80196040:
    lwz      r25, 0x294(r31)
    cmplwi   r25, 0x0
    beq      .L_801960D4
    lwz      r0, 0x74(r25)
    cmpwi    r0, 0x0
    beq      .L_80196080
    li       r0, 0x0
    li       r3, 0x8
    stw      r0, 0x74(r25)
    bl       HSD_StateInvalidate
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    mr       r3, r25
    bl       HSD_SetupTevStageAll
.L_80196080:
    lwz      r0, 0x78(r25)
    cmpwi    r0, 0x0
    beq      .L_8019609C
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lbz      r3, 0x80(r25)
    bl       GXSetNumTexGens
.L_8019609C:
    lwz      r0, 0x7c(r25)
    cmpwi    r0, 0x0
    beq      .L_801960D4
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lwz      r3, 0x84(r25)
    li       r7, 0x0
    lwz      r4, 0x88(r25)
    li       r8, 0x7d
    lwz      r5, 0x8c(r25)
    lwz      r6, 0x90(r25)
    bl       GXSetTexCoordGen2
.L_801960D4:
    lwz      r3, 0x298(r31)
    cmplwi   r3, 0x0
    beq      .L_801960F8
    lwz      r0, 0x30(r3)
    cmpwi    r0, 0x0
    beq      .L_801960F8
    li       r0, 0x0
    stw      r0, 0x30(r3)
    bl       HSD_SetupChannelAll
.L_801960F8:
    lwz      r3, 0x298(r31)
    addi     r0, r31, 0x1b0
    cmplw    r3, r0
    bne      .L_80196160
    lbz      r0, 0x1c4(r31)
    cmplwi   r0, 0x0
    beq      .L_80196160
    li       r25, 0x0
    b        .L_80196154
.L_8019611C:
    lwz      r0, 0x278(r28)
    addi     r3, r28, 0x100
    li       r5, 0x0
    li       r6, 0x1e
    stw      r0, 0x8(r1)
    li       r7, 0x1
    lwz      r4, 0x44(r31)
    lwz      r8, 0x26c(r28)
    lwz      r9, 0x270(r28)
    addi     r4, r4, 0xe8
    lwz      r10, 0x274(r28)
    bl       DrawLit__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
    addi     r28, r28, 0x410
    addi     r25, r25, 0x1
.L_80196154:
    cmpw     r25, r27
    blt      .L_8019611C
    b        .L_801961A8
.L_80196160:
    li       r25, 0x0
    b        .L_801961A0
.L_80196168:
    lwz      r0, 0x278(r28)
    addi     r3, r28, 0x100
    li       r5, 0x0
    li       r6, 0x1e
    stw      r0, 0x8(r1)
    li       r7, 0x1
    lwz      r4, 0x44(r31)
    lwz      r8, 0x26c(r28)
    lwz      r9, 0x270(r28)
    addi     r4, r4, 0xe8
    lwz      r10, 0x274(r28)
    bl       Draw__7CSpriteFPA4_f11_GXTexMapIDUl13_GXTexMtxTypeUlUlUlUl
    addi     r28, r28, 0x410
    addi     r25, r25, 0x1
.L_801961A0:
    cmpw     r25, r27
    blt      .L_80196168
.L_801961A8:
    lwz      r3, 0x294(r31)
    addi     r0, r31, 0x80
    cmplw    r3, r0
    bne      .L_80196360
    lwz      r0, 0xc(r3)
    cmpwi    r0, 0xff
    beq      .L_80196208
    li       r0, 0xff
    stw      r0, 0xc(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_801961E0
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_801961E0:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_801961F4
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_801961F4:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80196208
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80196208:
    lwz      r0, 0x10(r3)
    cmpwi    r0, 0xff
    beq      .L_80196258
    li       r0, 0xff
    stw      r0, 0x10(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80196230
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80196230:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80196244
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80196244:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_80196258
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_80196258:
    lwz      r0, 0x14(r3)
    cmpwi    r0, 0x4
    beq      .L_801962A8
    li       r0, 0x4
    stw      r0, 0x14(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80196280
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80196280:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80196294
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80196294:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_801962A8
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_801962A8:
    lwz      r3, 0x294(r31)
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_801962FC
    li       r0, 0x0
    stw      r0, 0x4(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_801962D4
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_801962D4:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_801962E8
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_801962E8:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_801962FC
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_801962FC:
    lwz      r0, 0x18(r3)
    cmpwi    r0, 0x4
    beq      .L_8019634C
    li       r0, 0x4
    stw      r0, 0x18(r3)
    lwz      r0, 0x74(r3)
    cmpwi    r0, 0x0
    bne      .L_80196324
    li       r0, 0x1
    stw      r0, 0x74(r3)
.L_80196324:
    lwz      r0, 0x78(r3)
    cmpwi    r0, 0x0
    bne      .L_80196338
    li       r0, 0x1
    stw      r0, 0x78(r3)
.L_80196338:
    lwz      r0, 0x7c(r3)
    cmpwi    r0, 0x0
    bne      .L_8019634C
    li       r0, 0x1
    stw      r0, 0x7c(r3)
.L_8019634C:
    lwz      r4, 0x294(r31)
    li       r3, 0x0
    li       r0, 0x1
    stb      r3, 0x80(r4)
    stw      r0, 0x78(r4)
.L_80196360:
    psq_l    f31, 0x2b8(r1), 0, qr0
    lfd      f31, 0x2b0(r1)
    psq_l    f30, 0x2a8(r1), 0, qr0
    lfd      f30, 0x2a0(r1)
    psq_l    f29, 0x298(r1), 0, qr0
    lfd      f29, 0x290(r1)
    psq_l    f28, 0x288(r1), 0, qr0
    lfd      f28, 0x280(r1)
    psq_l    f27, 0x278(r1), 0, qr0
    lfd      f27, 0x270(r1)
    psq_l    f26, 0x268(r1), 0, qr0
    lfd      f26, 0x260(r1)
    psq_l    f25, 0x258(r1), 0, qr0
    lfd      f25, 0x250(r1)
    psq_l    f24, 0x248(r1), 0, qr0
    lfd      f24, 0x240(r1)
    psq_l    f23, 0x238(r1), 0, qr0
    lfd      f23, 0x230(r1)
    psq_l    f22, 0x228(r1), 0, qr0
    lfd      f22, 0x220(r1)
    psq_l    f21, 0x218(r1), 0, qr0
    lfd      f21, 0x210(r1)
    psq_l    f20, 0x208(r1), 0, qr0
    lfd      f20, 0x200(r1)
    psq_l    f19, 0x1f8(r1), 0, qr0
    lfd      f19, 0x1f0(r1)
    psq_l    f18, 0x1e8(r1), 0, qr0
    lfd      f18, 0x1e0(r1)
    psq_l    f17, 0x1d8(r1), 0, qr0
    lfd      f17, 0x1d0(r1)
    psq_l    f16, 0x1c8(r1), 0, qr0
    lfd      f16, 0x1c0(r1)
    psq_l    f15, 0x1b8(r1), 0, qr0
    lfd      f15, 0x1b0(r1)
    psq_l    f14, 0x1a8(r1), 0, qr0
    lfd      f14, 0x1a0(r1)
    lmw      r25, 0x184(r1)
    lwz      r0, 0x2c4(r1)
    mtlr     r0
    addi     r1, r1, 0x2c0
    blr
*/

/**
 * @note Address: 0x80194FD4
 * @note Size: 0xF4
 */
extern "C" void DrawGsStrokeTextFormat(CGs* gs, u32 color, const char* fmt, f32 x, f32 y, f32 z, f32 w, f32 h, ...)
{
	char buf[256];
	va_list args;
	va_start(args, h);
	vsprintf(buf, fmt, args);
	DrawGsStrokeText(gs, color, buf, x, y, z, w, h);
}

static inline f32 MeasureStrokeTextWidth(CGs* gs, const char* text)
{
	f32 len[2]    = { 0.0f, 0.0f };
	const char* p = text;
	TextGlyph glyph;
	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] != '\0') {
			if (glyph.mStr[0] == '\n') {
				len[0] = 0.0f;
			} else {
				len[0] += 1.0f;
				if (len[0] > len[1]) {
					len[1] = len[0];
				}
			}
		}
	}
	return len[1];
}
/**
 * @note Address: 0x801943FC
 * @note Size: 0xBD8
 */
extern "C" void DrawGsStrokeText(CGs* gs, u32 color, const char* text, f32 x, f32 y, f32 z, f32 w, f32 h)
{
	if (gs->mCurRenderMode == &gs->mRenderMode) {
		gs->mRenderMode.mLineWidth    = 12;
		gs->mRenderMode.mLineTexOffs  = 5;
		gs->mRenderMode.mLineDirty    = TRUE;
		gs->mRenderMode.mPointSize    = 24;
		gs->mRenderMode.mPointTexOffs = 5;
		gs->mRenderMode.mPointDirty   = TRUE;
	}
	SetGsChannelMaterialAndLighting(gs, 1, 0);
	SetupDrawState(gs);

	f32 width = MeasureStrokeTextWidth(gs, text);

	Mtx trans;
#if defined(VERSION_JP)
	Mtx scale;
	Mtx mtx;
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
		PSMTXTrans(trans, x + width * w * 0.5f, y, z);
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXTrans(trans, x + width * w * 0.5f, y, z);
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, scale, mtx);
#else
	Mtx rotX;
	Mtx rotY;
	Mtx rotZ;
	Mtx scale;
	Mtx mtx;
	PSMTXTrans(trans, x + width * w * 0.5f, y, z);
	PSMTXRotRad(rotX, 'x', gs->mTextRot.x);
	PSMTXRotRad(rotY, 'y', gs->mTextRot.y);
	PSMTXRotRad(rotZ, 'z', gs->mTextRot.z);
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
		PSMTXScale(scale, w, h, 1.0f);
	} else {
		PSMTXScale(scale, w, -h, 1.0f);
	}
	PSMTXConcat(trans, rotX, mtx);
	PSMTXConcat(mtx, rotY, mtx);
	PSMTXConcat(mtx, rotZ, mtx);
	PSMTXConcat(mtx, scale, mtx);
#endif
	PSMTXConcat(gs->mCamera->mViewMtx, mtx, mtx);

	f32 startX;
	f32 curY;
	f32 unit;
	if (gs->mBillboardEnabled) {
		BuildBillboardMatrixFromUp(mtx, mtx);
		startX = width * 0.5f;
		curY   = 0.0f;
		unit   = -1.0f;
	} else {
		startX = -width * 0.5f;
		curY   = 0.0f;
		unit   = 1.0f;
	}
	f32 curX = startX;
	GXLoadPosMtxImm(mtx, 0);
	GXSetCurrentMtx(0);

	// position xyz f32 and color rgba8, both direct
	GXVtxAttrFmtList fmts[] = {
		{ GX_VA_POS, 1, 4, 0 },
		{ GX_VA_CLR0, 1, 5, 0 },
		{ GX_VA_NULL, 0, 0, 0 },
	};
	GXVtxDescList descs[] = {
		{ GX_VA_POS, 1 },
		{ GX_VA_CLR0, 1 },
		{ GX_VA_NULL, 0 },
	};
	GXClearVtxDesc();
	GXSetVtxAttrFmtv(0, fmts);
	GXSetVtxDescv(descs);

	// second pass draws, each character is one unit wide in the scaled space
	TextGlyph glyph;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	while ((text = GetGlyph(gs, text, &glyph)) != NULL) {
		if (glyph.mStr[0] == 0) {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			curX = startX;
			curY += 1.0f;
			continue;
		}
		if (glyph.mBgColor != 0) {
			GXBegin(GX_QUADS, 0, 4);
			DebugVertex(curX, curY, glyph.mBgColor);
			DebugVertex(curX + unit, curY, glyph.mBgColor);
			DebugVertex(curX + unit, 1.0f + curY, glyph.mBgColor);
			DebugVertex(curX, 1.0f + curY, glyph.mBgColor);
		}
		s32 index = -1;
		switch (glyph.mStr[0]) {
		// the 0.35 and 0.85 here are a bit off in retail, probably sums the compiler folded
		case '.':
			GXBegin(GX_POINTS, 0, 1);
			DebugVertex(curX + 0.25f * unit, 0.84999996f + curY, glyph.mColor);
			break;
		case ':':
			GXBegin(GX_POINTS, 0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.35000002f * unit, 0.75f + curY, glyph.mColor);
			break;
		case '%':
			GXBegin(GX_POINTS, 0, 2);
			DebugVertex(curX + 0.35000002f * unit, 0.35000002f + curY, glyph.mColor);
			DebugVertex(curX + 0.75f * unit, 0.75f + curY, glyph.mColor);
			GXBegin(GX_LINES, 0, 2);
			DebugVertex(curX + 0.95f * unit, 0.15f + curY, glyph.mColor);
			DebugVertex(curX + 0.15f * unit, 0.95f + curY, glyph.mColor);
			break;
		case '-':
			index = 36;
			break;
		case '_':
			index = 37;
			break;
		case '~':
			index = 38;
			break;
		case '>':
			index = 39;
			break;
		case '/':
			index = 40;
			break;
		case '(':
			index = 41;
			break;
		case ')':
			index = 42;
			break;
		case ',':
			index = 43;
			break;
		case '<':
			index = 44;
			break;
		case '+':
			index = 45;
			break;
		case '$':
			index = 46;
			break;
		default:
			if (glyph.mStr[0] >= '0' && glyph.mStr[0] <= '9') {
				index = glyph.mStr[0] - '0';
			} else if (glyph.mStr[0] >= 'A' && glyph.mStr[0] <= 'Z') {
				index = glyph.mStr[0] - 'A' + 10;
			} else if (glyph.mStr[0] >= 'a' && glyph.mStr[0] <= 'z') {
				index = glyph.mStr[0] - 'a' + 10;
			}
			break;
		}
		if (index >= 0) {
			u8* stroke = gGlyphStrokeTable[index];
			// 41 is way past the 13 bytes a glyph has, the 0xFF always comes first
			for (s32 i = 0; i < 41 && *stroke != 0xFF;) {
				u8 from = stroke[0];
				u8 to   = stroke[1];
				i += 2;
				stroke += 2;
				GXBegin(GX_LINES, 0, 2);
				DebugVertex(curX + unit * StrokePos(from >> 4), curY + StrokePos(from & 0xF), glyph.mColor);
				DebugVertex(curX + unit * StrokePos(to >> 4), curY + StrokePos(to & 0xF), glyph.mColor);
			}
		}
		curX += unit;
	}
}
/*
    stwu     r1, -0x2b0(r1)
    mflr     r0
    stw      r0, 0x2b4(r1)
    stfd     f31, 0x2a0(r1)
    psq_st   f31, 0x2a8(r1), 0, qr0
    stfd     f30, 0x290(r1)
    psq_st   f30, 0x298(r1), 0, qr0
    stfd     f29, 0x280(r1)
    psq_st   f29, 0x288(r1), 0, qr0
    stfd     f28, 0x270(r1)
    psq_st   f28, 0x278(r1), 0, qr0
    stfd     f27, 0x260(r1)
    psq_st   f27, 0x268(r1), 0, qr0
    stfd     f26, 0x250(r1)
    psq_st   f26, 0x258(r1), 0, qr0
    stfd     f25, 0x240(r1)
    psq_st   f25, 0x248(r1), 0, qr0
    stfd     f24, 0x230(r1)
    psq_st   f24, 0x238(r1), 0, qr0
    stfd     f23, 0x220(r1)
    psq_st   f23, 0x228(r1), 0, qr0
    stfd     f22, 0x210(r1)
    psq_st   f22, 0x218(r1), 0, qr0
    stfd     f21, 0x200(r1)
    psq_st   f21, 0x208(r1), 0, qr0
    stfd     f20, 0x1f0(r1)
    psq_st   f20, 0x1f8(r1), 0, qr0
    stfd     f19, 0x1e0(r1)
    psq_st   f19, 0x1e8(r1), 0, qr0
    stmw     r21, 0x1b4(r1)
    mr       r27, r3
    fmr      f19, f1
    lwz      r3, 0x290(r3)
    addi     r0, r27, 0x4c
    fmr      f24, f2
    mr       r28, r4
    fmr      f23, f3
    fmr      f20, f4
    cmplw    r3, r0
    fmr      f21, f5
    mr       r29, r5
    bne      .L_801944CC
    li       r0, 0xc
    li       r4, 0x5
    stb      r0, 0x6c(r27)
    li       r3, 0x1
    li       r0, 0x18
    stw      r4, 0x70(r27)
    stw      r3, 0x60(r27)
    stb      r0, 0x74(r27)
    stw      r4, 0x78(r27)
    stw      r3, 0x64(r27)
.L_801944CC:
    mr       r3, r27
    li       r4, 0x1
    li       r5, 0x0
    bl       SetGsChannelMaterialAndLighting
    lwz      r21, 0x290(r27)
    cmplwi   r21, 0x0
    beq      .L_8019456C
    lwz      r0, 0xc(r21)
    cmpwi    r0, 0x0
    beq      .L_80194510
    li       r0, 0x0
    li       r3, 0x40
    stw      r0, 0xc(r21)
    bl       HSD_StateInvalidate
    mr       r4, r21
    li       r3, 0x0
    bl       HSD_SetupPEMode
.L_80194510:
    lwz      r0, 0x10(r21)
    cmpwi    r0, 0x0
    beq      .L_8019452C
    li       r0, 0x0
    stw      r0, 0x10(r21)
    lwz      r3, 0x1c(r21)
    bl       HSD_StateSetCullMode
.L_8019452C:
    lwz      r0, 0x14(r21)
    cmpwi    r0, 0x0
    beq      .L_8019454C
    li       r0, 0x0
    stw      r0, 0x14(r21)
    lbz      r3, 0x20(r21)
    lwz      r4, 0x24(r21)
    bl       HSD_StateSetLineWidth
.L_8019454C:
    lwz      r0, 0x18(r21)
    cmpwi    r0, 0x0
    beq      .L_8019456C
    li       r0, 0x0
    stw      r0, 0x18(r21)
    lbz      r3, 0x28(r21)
    lwz      r4, 0x2c(r21)
    bl       HSD_StateSetPointSize
.L_8019456C:
    lwz      r21, 0x294(r27)
    cmplwi   r21, 0x0
    beq      .L_80194600
    lwz      r0, 0x74(r21)
    cmpwi    r0, 0x0
    beq      .L_801945AC
    li       r0, 0x0
    li       r3, 0x8
    stw      r0, 0x74(r21)
    bl       HSD_StateInvalidate
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    mr       r3, r21
    bl       HSD_SetupTevStageAll
.L_801945AC:
    lwz      r0, 0x78(r21)
    cmpwi    r0, 0x0
    beq      .L_801945C8
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lbz      r3, 0x80(r21)
    bl       GXSetNumTexGens
.L_801945C8:
    lwz      r0, 0x7c(r21)
    cmpwi    r0, 0x0
    beq      .L_80194600
    li       r3, 0x10
    bl       HSD_StateInvalidate
    li       r3, 0x20
    bl       HSD_StateInvalidate
    lwz      r3, 0x84(r21)
    li       r7, 0x0
    lwz      r4, 0x88(r21)
    li       r8, 0x7d
    lwz      r5, 0x8c(r21)
    lwz      r6, 0x90(r21)
    bl       GXSetTexCoordGen2
.L_80194600:
    lwz      r3, 0x298(r27)
    cmplwi   r3, 0x0
    beq      .L_80194624
    lwz      r0, 0x30(r3)
    cmpwi    r0, 0x0
    beq      .L_80194624
    li       r0, 0x0
    stw      r0, 0x30(r3)
    bl       HSD_SetupChannelAll
.L_80194624:
    lwz      r3, lbl_806602BC@sda21(r0)
    mr       r31, r29
    lwz      r0, lbl_806602C0@sda21(r0)
    stw      r3, 0x8(r1)
    stw      r0, 0xc(r1)
    b        .L_8019467C
.L_8019463C:
    extsb.   r0, r30
    beq      .L_8019467C
    extsb    r0, r30
    cmpwi    r0, 0xa
    bne      .L_8019465C
    lfs      f0, lbl_8065F5EC@sda21(r0)
    stfs     f0, 0x8(r1)
    b        .L_8019467C
.L_8019465C:
    lfs      f2, 0x8(r1)
    lfs      f1, lbl_8065F5F0@sda21(r0)
    lfs      f0, 0xc(r1)
    fadds    f1, f2, f1
    fcmpo    cr0, f1, f0
    stfs     f1, 0x8(r1)
    ble      .L_8019467C
    stfs     f1, 0xc(r1)
.L_8019467C:
    lbz      r4, 0x0(r31)
    extsb    r0, r4
    cmpwi    r0, 0xa
    beq      .L_801946B0
    bge      .L_8019469C
    cmpwi    r0, 0x0
    beq      .L_801946A8
    b        .L_80194720
.L_8019469C:
    cmpwi    r0, 0x5c
    beq      .L_801946B8
    b        .L_80194720
.L_801946A8:
    li       r31, 0x0
    b        .L_80194784
.L_801946B0:
    li       r30, 0xa
    b        .L_80194780
.L_801946B8:
    lbzu     r30, 0x1(r31)
    extsb    r0, r30
    cmpwi    r0, 0x62
    beq      .L_80194704
    bge      .L_801946E4
    cmpwi    r0, 0x43
    beq      .L_801946EC
    bge      .L_80194780
    cmpwi    r0, 0x42
    bge      .L_80194704
    b        .L_80194780
.L_801946E4:
    cmpwi    r0, 0x64
    bge      .L_80194780
.L_801946EC:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r30, 0x0
    addi     r31, r31, 0x8
    b        .L_80194780
.L_80194704:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r30, 0x0
    addi     r31, r31, 0x8
    b        .L_80194780
    b        .L_80194780
.L_80194720:
    lwz      r0, 0x17a4(r27)
    cmpwi    r0, 0x0
    beq      .L_8019477C
    cmplwi   r4, 0x81
    li       r3, 0x0
    blt      .L_80194744
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_80194758
.L_80194744:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_8019475C
    cmplwi   r0, 0xfc
    bgt      .L_8019475C
.L_80194758:
    li       r3, 0x1
.L_8019475C:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_8019477C
    mr       r30, r4
    addi     r31, r31, 0x1
    b        .L_80194780
.L_8019477C:
    mr       r30, r4
.L_80194780:
    addi     r31, r31, 0x1
.L_80194784:
    cmplwi   r31, 0x0
    bne      .L_8019463C
    lfs      f22, 0xc(r1)
    fmr      f2, f24
    lfs      f0, lbl_8065F5F4@sda21(r0)
    fmr      f3, f23
    fmuls    f1, f22, f20
    addi     r3, r1, 0x154
    fmadds   f1, f1, f0, f19
    bl       PSMTXTrans
    lfs      f1, 0x17b4(r27)
    addi     r3, r1, 0x124
    li       r4, 0x78
    bl       PSMTXRotRad
    lfs      f1, 0x17b8(r27)
    addi     r3, r1, 0xf4
    li       r4, 0x79
    bl       PSMTXRotRad
    lfs      f1, 0x17bc(r27)
    addi     r3, r1, 0xc4
    li       r4, 0x7a
    bl       PSMTXRotRad
    lwz      r3, 0x44(r27)
    lbz      r0, 0x50(r3)
    cmplwi   r0, 0x3
    bne      .L_80194804
    fmr      f1, f20
    lfs      f3, lbl_8065F5F0@sda21(r0)
    fmr      f2, f21
    addi     r3, r1, 0x94
    bl       PSMTXScale
    b        .L_80194818
.L_80194804:
    fmr      f1, f20
    lfs      f3, lbl_8065F5F0@sda21(r0)
    fneg     f2, f21
    addi     r3, r1, 0x94
    bl       PSMTXScale
.L_80194818:
    addi     r3, r1, 0x154
    addi     r4, r1, 0x124
    addi     r5, r1, 0x64
    bl       PSMTXConcat
    addi     r3, r1, 0x64
    addi     r4, r1, 0xf4
    mr       r5, r3
    bl       PSMTXConcat
    addi     r3, r1, 0x64
    addi     r4, r1, 0xc4
    mr       r5, r3
    bl       PSMTXConcat
    addi     r3, r1, 0x64
    addi     r4, r1, 0x94
    mr       r5, r3
    bl       PSMTXConcat
    lwz      r3, 0x44(r27)
    addi     r4, r1, 0x64
    mr       r5, r4
    addi     r3, r3, 0xe8
    bl       PSMTXConcat
    lwz      r0, 0x29c(r27)
    cmpwi    r0, 0x0
    beq      .L_8019489C
    addi     r3, r1, 0x64
    mr       r4, r3
    bl       BuildBillboardMatrixFromUp
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lfs      f21, lbl_8065F5EC@sda21(r0)
    fmuls    f19, f22, f0
    lfs      f20, lbl_8065F5F8@sda21(r0)
    fmr      f22, f19
    b        .L_801948B4
.L_8019489C:
    fneg     f1, f22
    lfs      f0, lbl_8065F5F4@sda21(r0)
    lfs      f21, lbl_8065F5EC@sda21(r0)
    lfs      f20, lbl_8065F5F0@sda21(r0)
    fmuls    f19, f1, f0
    fmr      f22, f19
.L_801948B4:
    addi     r3, r1, 0x64
    li       r4, 0x0
    bl       GXLoadPosMtxImm
    li       r3, 0x0
    bl       GXSetCurrentMtx
    lis      r4, sGsStrokeVertexFormat@ha
    lwzu     r21, sGsStrokeVertexFormat@l(r4)
    lis      r3, sGsStrokeVertexDesc@ha
    addi     r8, r3, sGsStrokeVertexDesc@l
    lwz      r22, 0x4(r4)
    lwz      r23, 0x8(r4)
    lwz      r24, 0xc(r4)
    lwz      r31, 0x10(r4)
    lwz      r30, 0x14(r4)
    lwz      r26, 0x18(r4)
    lwz      r25, 0x1c(r4)
    lwz      r12, 0x20(r4)
    lwz      r11, 0x24(r4)
    lwz      r10, 0x28(r4)
    lwz      r9, 0x2c(r4)
    lwz      r7, 0x0(r8)
    lwz      r6, 0x4(r8)
    lwz      r5, 0x8(r8)
    lwz      r4, 0xc(r8)
    lwz      r3, 0x10(r8)
    lwz      r0, 0x14(r8)
    stw      r21, 0x34(r1)
    stw      r22, 0x38(r1)
    stw      r23, 0x3c(r1)
    stw      r24, 0x40(r1)
    stw      r31, 0x44(r1)
    stw      r30, 0x48(r1)
    stw      r26, 0x4c(r1)
    stw      r25, 0x50(r1)
    stw      r12, 0x54(r1)
    stw      r11, 0x58(r1)
    stw      r10, 0x5c(r1)
    stw      r9, 0x60(r1)
    stw      r7, 0x1c(r1)
    stw      r6, 0x20(r1)
    stw      r5, 0x24(r1)
    stw      r4, 0x28(r1)
    stw      r3, 0x2c(r1)
    stw      r0, 0x30(r1)
    bl       GXClearVtxDesc
    addi     r4, r1, 0x34
    li       r3, 0x0
    bl       GXSetVtxAttrFmtv
    addi     r3, r1, 0x1c
    bl       GXSetVtxDescv
    lfs      f1, lbl_8065F5FC@sda21(r0)
    li       r0, 0x0
    lfs      f0, lbl_8065F600@sda21(r0)
    lfs      f2, lbl_8065F604@sda21(r0)
    fmuls    f27, f1, f20
    lfs      f1, lbl_8065F608@sda21(r0)
    fmuls    f26, f0, f20
    lfs      f0, lbl_8065F60C@sda21(r0)
    fmuls    f25, f2, f20
    fmuls    f24, f1, f20
    fmuls    f23, f0, f20
    stw      r28, 0x14(r1)
    stw      r0, 0x18(r1)
    b        .L_80194E10
.L_801949B4:
    lbz      r3, 0x10(r1)
    extsb.   r0, r3
    beq      .L_80194E10
    extsb    r0, r3
    cmpwi    r0, 0xa
    bne      .L_801949DC
    lfs      f0, lbl_8065F5F0@sda21(r0)
    fmr      f22, f19
    fadds    f21, f21, f0
    b        .L_80194E10
.L_801949DC:
    lwz      r0, 0x18(r1)
    cmplwi   r0, 0x0
    beq      .L_80194A50
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    lis      r3, 0xcc01
    lfs      f0, lbl_8065F5F0@sda21(r0)
    stfs     f22, -0x8000(r3)
    fadds    f2, f22, f20
    lfs      f1, lbl_8065F5EC@sda21(r0)
    fadds    f0, f0, f21
    stfs     f21, -0x8000(r3)
    lwz      r0, 0x18(r1)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f21, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
    stfs     f22, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
.L_80194A50:
    lbz      r0, 0x10(r1)
    li       r21, -0x1
    extsb    r0, r0
    cmpwi    r0, 0x2f
    beq      .L_80194C60
    bge      .L_80194ABC
    cmpwi    r0, 0x29
    beq      .L_80194C70
    bge      .L_80194A98
    cmpwi    r0, 0x25
    beq      .L_80194B94
    bge      .L_80194A8C
    cmpwi    r0, 0x24
    bge      .L_80194C90
    b        .L_80194C98
.L_80194A8C:
    cmpwi    r0, 0x28
    bge      .L_80194C68
    b        .L_80194C98
.L_80194A98:
    cmpwi    r0, 0x2c
    beq      .L_80194C78
    bge      .L_80194AB0
    cmpwi    r0, 0x2b
    bge      .L_80194C88
    b        .L_80194C98
.L_80194AB0:
    cmpwi    r0, 0x2e
    bge      .L_80194B04
    b        .L_80194C40
.L_80194ABC:
    cmpwi    r0, 0x3e
    beq      .L_80194C58
    bge      .L_80194AEC
    cmpwi    r0, 0x3b
    beq      .L_80194C98
    bge      .L_80194AE0
    cmpwi    r0, 0x3a
    bge      .L_80194B40
    b        .L_80194C98
.L_80194AE0:
    cmpwi    r0, 0x3d
    bge      .L_80194C98
    b        .L_80194C80
.L_80194AEC:
    cmpwi    r0, 0x7e
    beq      .L_80194C50
    bge      .L_80194C98
    cmpwi    r0, 0x5f
    beq      .L_80194C48
    b        .L_80194C98
.L_80194B04:
    li       r3, 0xb8
    li       r4, 0x0
    li       r5, 0x1
    bl       GXBegin
    lfs      f1, lbl_8065F610@sda21(r0)
    fadds    f2, f22, f27
    lis      r3, 0xcc01
    lfs      f0, lbl_8065F5EC@sda21(r0)
    fadds    f1, f1, f21
    lwz      r0, 0x14(r1)
    stfs     f2, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stw      r0, -0x8000(r3)
    b        .L_80194CEC
.L_80194B40:
    li       r3, 0xb8
    li       r4, 0x0
    li       r5, 0x2
    bl       GXBegin
    lfs      f1, lbl_8065F600@sda21(r0)
    fadds    f3, f22, f26
    lis      r3, 0xcc01
    lfs      f0, lbl_8065F604@sda21(r0)
    fadds    f2, f1, f21
    lfs      f1, lbl_8065F5EC@sda21(r0)
    stfs     f3, -0x8000(r3)
    lwz      r0, 0x14(r1)
    fadds    f0, f0, f21
    stfs     f2, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
    stfs     f3, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stw      r0, -0x8000(r3)
    b        .L_80194CEC
.L_80194B94:
    li       r3, 0xb8
    li       r4, 0x0
    li       r5, 0x2
    bl       GXBegin
    lfs      f1, lbl_8065F600@sda21(r0)
    fadds    f4, f22, f26
    lis      r6, 0xcc01
    lfs      f0, lbl_8065F604@sda21(r0)
    fadds    f3, f1, f21
    lfs      f2, lbl_8065F5EC@sda21(r0)
    stfs     f4, -0x8000(r6)
    lwz      r0, 0x14(r1)
    fadds    f1, f22, f25
    stfs     f3, -0x8000(r6)
    fadds    f0, f0, f21
    li       r3, 0xa8
    stfs     f2, -0x8000(r6)
    li       r4, 0x0
    li       r5, 0x2
    stw      r0, -0x8000(r6)
    stfs     f1, -0x8000(r6)
    stfs     f0, -0x8000(r6)
    stfs     f2, -0x8000(r6)
    stw      r0, -0x8000(r6)
    bl       GXBegin
    lfs      f1, lbl_8065F60C@sda21(r0)
    fadds    f4, f22, f24
    lis      r3, 0xcc01
    lfs      f0, lbl_8065F608@sda21(r0)
    fadds    f3, f1, f21
    lfs      f2, lbl_8065F5EC@sda21(r0)
    stfs     f4, -0x8000(r3)
    lwz      r0, 0x14(r1)
    fadds    f1, f22, f23
    stfs     f3, -0x8000(r3)
    fadds    f0, f0, f21
    stfs     f2, -0x8000(r3)
    stw      r0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    stw      r0, -0x8000(r3)
    b        .L_80194CEC
.L_80194C40:
    li       r21, 0x24
    b        .L_80194CEC
.L_80194C48:
    li       r21, 0x25
    b        .L_80194CEC
.L_80194C50:
    li       r21, 0x26
    b        .L_80194CEC
.L_80194C58:
    li       r21, 0x27
    b        .L_80194CEC
.L_80194C60:
    li       r21, 0x28
    b        .L_80194CEC
.L_80194C68:
    li       r21, 0x29
    b        .L_80194CEC
.L_80194C70:
    li       r21, 0x2a
    b        .L_80194CEC
.L_80194C78:
    li       r21, 0x2b
    b        .L_80194CEC
.L_80194C80:
    li       r21, 0x2c
    b        .L_80194CEC
.L_80194C88:
    li       r21, 0x2d
    b        .L_80194CEC
.L_80194C90:
    li       r21, 0x2e
    b        .L_80194CEC
.L_80194C98:
    lbz      r0, 0x10(r1)
    extsb    r3, r0
    cmpwi    r3, 0x30
    blt      .L_80194CB8
    cmpwi    r3, 0x39
    bgt      .L_80194CB8
    subi     r21, r3, 0x30
    b        .L_80194CEC
.L_80194CB8:
    extsb    r3, r0
    cmpwi    r3, 0x41
    blt      .L_80194CD4
    cmpwi    r3, 0x5a
    bgt      .L_80194CD4
    subi     r21, r3, 0x37
    b        .L_80194CEC
.L_80194CD4:
    extsb    r3, r0
    cmpwi    r3, 0x61
    blt      .L_80194CEC
    cmpwi    r3, 0x7a
    bgt      .L_80194CEC
    subi     r21, r3, 0x57
.L_80194CEC:
    cmpwi    r21, 0x0
    blt      .L_80194E0C
    mulli    r4, r21, 0xd
    lis      r3, gGlyphStrokeTable@ha
    lfs      f28, lbl_8065F614@sda21(r0)
    li       r30, 0x0
    addi     r0, r3, gGlyphStrokeTable@l
    lfs      f29, lbl_8065F618@sda21(r0)
    lfd      f30, lbl_8065F620@sda21(r0)
    add      r28, r0, r4
    lfs      f31, lbl_8065F5EC@sda21(r0)
    lis      r25, 0x4330
    lwz      r31, 0x14(r1)
    lis      r26, 0xcc01
    b        .L_80194DF8
.L_80194D28:
    lbz      r21, 0x0(r28)
    li       r3, 0xa8
    lbz      r22, 0x1(r28)
    li       r4, 0x0
    li       r5, 0x2
    addi     r30, r30, 0x2
    addi     r28, r28, 0x2
    bl       GXBegin
    srawi    r0, r21, 4
    clrlwi   r3, r21, 28
    xoris    r4, r0, 0x8000
    extrwi   r0, r22, 4, 24
    stw      r4, 0x18c(r1)
    xoris    r4, r3, 0x8000
    xoris    r3, r0, 0x8000
    clrlwi   r0, r22, 28
    stw      r25, 0x188(r1)
    xoris    r0, r0, 0x8000
    lfd      f0, 0x188(r1)
    stw      r4, 0x194(r1)
    fsubs    f1, f0, f30
    stw      r25, 0x190(r1)
    lfd      f0, 0x190(r1)
    fmuls    f1, f29, f1
    stw      r3, 0x19c(r1)
    fsubs    f0, f0, f30
    stw      r25, 0x198(r1)
    fadds    f1, f28, f1
    fmuls    f2, f29, f0
    lfd      f0, 0x198(r1)
    fmadds   f3, f20, f1, f22
    stw      r0, 0x1a4(r1)
    fsubs    f1, f0, f30
    stw      r25, 0x1a0(r1)
    fadds    f2, f28, f2
    lfd      f0, 0x1a0(r1)
    fmuls    f1, f29, f1
    stfs     f3, -0x8000(r26)
    fsubs    f0, f0, f30
    fadds    f2, f21, f2
    fadds    f1, f28, f1
    fmuls    f0, f29, f0
    stfs     f2, -0x8000(r26)
    fmadds   f1, f20, f1, f22
    stfs     f31, -0x8000(r26)
    fadds    f0, f28, f0
    stw      r31, -0x8000(r26)
    fadds    f0, f21, f0
    stfs     f1, -0x8000(r26)
    stfs     f0, -0x8000(r26)
    stfs     f31, -0x8000(r26)
    stw      r31, -0x8000(r26)
.L_80194DF8:
    cmpwi    r30, 0x29
    bge      .L_80194E0C
    lbz      r0, 0x0(r28)
    cmplwi   r0, 0xff
    bne      .L_80194D28
.L_80194E0C:
    fadds    f22, f22, f20
.L_80194E10:
    lbz      r4, 0x0(r29)
    mr       r31, r29
    extsb    r0, r4
    cmpwi    r0, 0xa
    beq      .L_80194E48
    bge      .L_80194E34
    cmpwi    r0, 0x0
    beq      .L_80194E40
    b        .L_80194EDC
.L_80194E34:
    cmpwi    r0, 0x5c
    beq      .L_80194E54
    b        .L_80194EDC
.L_80194E40:
    li       r0, 0x0
    b        .L_80194F4C
.L_80194E48:
    li       r0, 0xa
    stb      r0, 0x10(r1)
    b        .L_80194F48
.L_80194E54:
    lbz      r3, 0x1(r29)
    addi     r31, r29, 0x1
    extsb    r0, r3
    cmpwi    r0, 0x62
    beq      .L_80194EAC
    bge      .L_80194E84
    cmpwi    r0, 0x43
    beq      .L_80194E8C
    bge      .L_80194ECC
    cmpwi    r0, 0x42
    bge      .L_80194EAC
    b        .L_80194ECC
.L_80194E84:
    cmpwi    r0, 0x64
    bge      .L_80194ECC
.L_80194E8C:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x14(r1)
    addi     r31, r31, 0x8
    stb      r0, 0x10(r1)
    b        .L_80194F48
.L_80194EAC:
    addi     r3, r31, 0x1
    li       r4, 0x8
    bl       ParseHexDigits
    li       r0, 0x0
    stw      r3, 0x18(r1)
    addi     r31, r31, 0x8
    stb      r0, 0x10(r1)
    b        .L_80194F48
.L_80194ECC:
    li       r0, 0x0
    stb      r3, 0x10(r1)
    stb      r0, 0x11(r1)
    b        .L_80194F48
.L_80194EDC:
    lwz      r0, 0x17a4(r27)
    cmpwi    r0, 0x0
    beq      .L_80194F3C
    cmplwi   r4, 0x81
    li       r3, 0x0
    blt      .L_80194F00
    clrlwi   r0, r4, 24
    cmplwi   r0, 0x9f
    ble      .L_80194F14
.L_80194F00:
    clrlwi   r0, r4, 24
    cmplwi   r0, 0xe0
    blt      .L_80194F18
    cmplwi   r0, 0xfc
    bgt      .L_80194F18
.L_80194F14:
    li       r3, 0x1
.L_80194F18:
    clrlwi   r3, r3, 24
    neg      r0, r3
    or       r0, r0, r3
    srwi.    r0, r0, 31
    beq      .L_80194F3C
    lbzu     r0, 0x1(r31)
    stb      r4, 0x10(r1)
    stb      r0, 0x11(r1)
    b        .L_80194F48
.L_80194F3C:
    li       r0, 0x0
    stb      r4, 0x10(r1)
    stb      r0, 0x11(r1)
.L_80194F48:
    addi     r0, r31, 0x1
.L_80194F4C:
    cmplwi   r0, 0x0
    mr       r29, r0
    bne      .L_801949B4
    psq_l    f31, 0x2a8(r1), 0, qr0
    lfd      f31, 0x2a0(r1)
    psq_l    f30, 0x298(r1), 0, qr0
    lfd      f30, 0x290(r1)
    psq_l    f29, 0x288(r1), 0, qr0
    lfd      f29, 0x280(r1)
    psq_l    f28, 0x278(r1), 0, qr0
    lfd      f28, 0x270(r1)
    psq_l    f27, 0x268(r1), 0, qr0
    lfd      f27, 0x260(r1)
    psq_l    f26, 0x258(r1), 0, qr0
    lfd      f26, 0x250(r1)
    psq_l    f25, 0x248(r1), 0, qr0
    lfd      f25, 0x240(r1)
    psq_l    f24, 0x238(r1), 0, qr0
    lfd      f24, 0x230(r1)
    psq_l    f23, 0x228(r1), 0, qr0
    lfd      f23, 0x220(r1)
    psq_l    f22, 0x218(r1), 0, qr0
    lfd      f22, 0x210(r1)
    psq_l    f21, 0x208(r1), 0, qr0
    lfd      f21, 0x200(r1)
    psq_l    f20, 0x1f8(r1), 0, qr0
    lfd      f20, 0x1f0(r1)
    psq_l    f19, 0x1e8(r1), 0, qr0
    lfd      f19, 0x1e0(r1)
    lmw      r21, 0x1b4(r1)
    lwz      r0, 0x2b4(r1)
    mtlr     r0
    addi     r1, r1, 0x2b0
    blr
*/
