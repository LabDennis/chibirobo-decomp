#include "game/cgs.h"
#include "Version.h"
#include "game/cgame.h"
#include "game/cgsdefaults.h"
#include "game/ccamera.h"
#include "game/clight.h"
#include "game/cgsdrawstate.h"
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include <sysdolphin/video.h>

void GXAdjustForOverscan(const GXRenderModeObj* rmin, GXRenderModeObj* rmout, u16 hor, u16 ver);
void HSD_CObjSetReferenceSize(s32 ref_width, s32 ref_height);
void HSD_SetupTevStageAll(HSD_TevDesc* desc);
void HSD_StateInvalidate(s32 mask);
}

extern GXRenderModeObj* gVideoRenderModes[31];

// HSD_PEDesc flag bits
#define PE_COLOR_UPDATE 0x1
#define PE_ALPHA_UPDATE 0x2
#define PE_DST_ALPHA    0x4
#define PE_BEFORE_TEX   0x8
#define PE_COMPARE      0x10
#define PE_ZUPDATE      0x20
#define PE_DITHER       0x40

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
	screen->mHeightScale = 1.0f;
#endif
}
// ConfigureGsScreenMode
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
// NONMATCHING: retail orders the field stores and the 0x3c immediate differently.
extern "C" void InitializeGsTevStageDescriptor(CGsTevStage* stage, u32 id, GsTevOrder order, GsTevOp colorOp, GsTevIn colorIn,
                                               GsTevOp alphaOp, GsTevIn alphaIn)
{
	stage->next                    = NULL;
	stage->stage                   = id;
	stage->flags                   = 1; // not 0: HSD_SetupTevStage uses u.tevconf, SetGsTevMode's 0 picks a GXSetTevOp mode
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
	stage->u.tevconf.kcsel         = 0xC;  // GX_TEV_KCSEL_K0
	stage->u.tevconf.kasel         = 0x1C; // GX_TEV_KASEL_K0_A
	stage->mNumTexGens             = 0;
	stage->mTexCoord               = GX_TEXCOORD0;
	stage->mTexGenFunc             = GX_TG_MTX2x4;
	stage->mTexGenSrc              = GX_TG_TEX0;
	stage->mTexMtx                 = GX_IDENTITY;
	stage->Invalidate();
}
// InitializeGsTevStageDescriptor
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
		GXSetTexCoordGen2(static_cast<GXTexCoordID>(stage->mTexCoord), static_cast<GXTexGenType>(stage->mTexGenFunc),
		                  static_cast<GXTexGenSrc>(stage->mTexGenSrc), stage->mTexMtx, GX_FALSE, GX_PTIDENTITY);
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

static inline bool ColorsEqual(const GXColor& first, const GXColor& second)
{
	return first.r == second.r && first.g == second.g && first.b == second.b && first.a == second.a;
}

static inline void GsChanSetAmbColor(CGsChannel* ch, GXColor color)
{
	if (!ColorsEqual(ch->amb_color, color)) {
		ch->amb_color = color;
		GsChanSetDirty(ch);
	}
}

static inline void GsChanSetMatColor(CGsChannel* ch, GXColor color)
{
	if (!ColorsEqual(ch->mat_color, color)) {
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

static inline GXColor MultiplyColors(GXColor first, GXColor second)
{
	GXColor result;
	result.r = (first.r * second.r) / 255u;
	result.g = (first.g * second.g) / 255u;
	result.b = (first.b * second.b) / 255u;
	result.a = (first.a * second.a) / 255u;
	return result;
}

extern "C" {
void HSD_SetPixelFormat(s32 pix_fmt, s32 z_fmt);
void HSD_SetZSortMode(s32 mode, s32 sort);
}

extern "C" {
#include <sysdolphin/video.h>
void GXSetClipMode(s32 mode);
}

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
	HSD_SetPixelFormat(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
	GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);

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
	gs->mRotOrder = 5;
#if !defined(VERSION_JP)
	gs->mUseShiftJis          = arg;
	gs->mShiftJisWidthScale   = 1.0f;
	gs->mSingleByteWidthScale = 1.0f;
#endif
}
// InitializeGsRenderDefaults
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
	// mChannels is indexed by channel id; HSD_SetupChannelAll runs COLOR0, then ALPHA0 through next
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
				// the last active light's type picks the attenuation, any light but ambient turns it on
				u32 type = LOBJ_AMBIENT;
				s32 n    = HSD_LObjGetNbActive();
				for (s32 i = 0; i < n; i++) {
					HSD_LObj* lobj = HSD_LObjGetActiveByIndex(i);
					if (lobj != NULL) {
						type = lobj->flags & LOBJ_TYPE_MASK;
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
		gs->mTextRot.x = 0.0f;
		gs->mTextRot.y = 0.0f;
		gs->mTextRot.z = 0.0f;
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
	GXColor color      = { static_cast<u8>(rgba >> 24), static_cast<u8>(rgba >> 16), static_cast<u8>(rgba >> 8), static_cast<u8>(rgba) };
	CGsChannel* color0 = &gs->mChannels[0];
	if (gs->mCurChannel == color0) {
		GsChanSetMatSrc(color0, matSrc);
		GsChanSetMatSrc(&gs->mChannels[2], matSrc);
		if (color0->enable) {
			if (color0->amb_src == GX_SRC_REG) {
				HSD_LObj* lobj = HSD_LObjGetCurrentByType(LOBJ_AMBIENT);
				if (lobj != NULL) {
					GXColor amb = MultiplyColors(lobj->color, color);
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

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}

// places a flat shape on screen, w and h are its full size. an ortho camera has y growing
// down the screen, so the shape gets flipped back
static inline void SetFlatTransform(CGs* gs, CPrimitive* prim, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rx, f32 ry, f32 rz)
{
	if (gs->mCamera->projection_type == PROJ_ORTHO) {
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
	prim->mRotOrder = gs->mRotOrder;
}

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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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

	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
/**
 * @note Address: 0x80197538
 * @note Size: 0x2C8
 */
extern "C" void DrawGsCube(CGs* gs, u32 color, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCube.SetTrans(tx, ty, tz);
	gs->mCube.SetScale(sx, sy, sz);
	gs->mCube.SetRot(rx, ry, rz);
	gs->mCube.mRotOrder = gs->mRotOrder;
	gs->mCube.mFixedMtx = NULL;
	gs->mCube.Draw(gs->mCamera->mViewMtx);
}

/**
 * @note Address: 0x80197250
 * @note Size: 0x2E8
 */
extern "C" void DrawGsSphere(CGs* gs, u32 color, u8 major, u8 minor, f32 tx, f32 ty, f32 tz, f32 rx, f32 ry, f32 rz, f32 sx, f32 sy, f32 sz)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mSphere.SetTrans(tx, ty, tz);
	gs->mSphere.SetScale(sx, sy, sz);
	gs->mSphere.SetRot(rx, ry, rz);
	gs->mSphere.mRotOrder = gs->mRotOrder;
	gs->mSphere.mFixedMtx = NULL;
	gs->mSphere.Draw(gs->mCamera->mViewMtx, major, minor);
}

/**
 * @note Address: 0x801970B0
 * @note Size: 0x1A0
 */
extern "C" void DrawGsSphereWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 major, u8 minor)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.SetTrans(tx, ty, tz);
	gs->mCylinder.SetScale(sx, sy, sz);
	gs->mCylinder.SetRot(rx, ry, rz);
	gs->mCylinder.mRotOrder = gs->mRotOrder;
	gs->mCylinder.mFixedMtx = NULL;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

/**
 * @note Address: 0x80196C28
 * @note Size: 0x1B0
 */
extern "C" void DrawGsCylinderWithMatrix(CGs* gs, Mtx mtx, u32 color, u8 sides)
{
	SetGsChannelMaterialAndLighting(gs, GX_SRC_REG, color);
	SetupDrawState(gs);
	gs->mCylinder.mFixedMtx = mtx;
	gs->mCylinder.Draw(gs->mCamera->mViewMtx, sides);
}

extern "C" {
#include <dolphin/gx/gx.h>
#include <dolphin/gx/gxfifo.h>
}

// Vertex-list declarations for the HSD GX interface.
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

extern "C" {
void GXSetVtxAttrFmtv(u32 fmt, GXVtxAttrFmtList* list);
void GXSetVtxDescv(GXVtxDescList* list);
}

extern "C" {
extern GXRenderModeObj GXNtsc240Ds;
extern GXRenderModeObj GXNtsc240DsAa;
extern GXRenderModeObj GXNtsc240Int;
extern GXRenderModeObj GXNtsc240IntAa;
extern GXRenderModeObj GXNtsc480IntDf;
extern GXRenderModeObj GXNtsc480Int;
extern GXRenderModeObj GXNtsc480IntAa;
extern GXRenderModeObj GXNtsc480Prog;
extern GXRenderModeObj GXNtsc480ProgSoft;
extern GXRenderModeObj GXNtsc480ProgAa;
extern GXRenderModeObj GXMpal240Ds;
extern GXRenderModeObj GXMpal240DsAa;
extern GXRenderModeObj GXMpal240Int;
extern GXRenderModeObj GXMpal240IntAa;
extern GXRenderModeObj GXMpal480IntDf;
extern GXRenderModeObj GXMpal480Int;
extern GXRenderModeObj GXMpal480IntAa;
extern GXRenderModeObj GXPal264Ds;
extern GXRenderModeObj GXPal264DsAa;
extern GXRenderModeObj GXPal264Int;
extern GXRenderModeObj GXPal264IntAa;
extern GXRenderModeObj GXPal528IntDf;
extern GXRenderModeObj GXPal528Int;
extern GXRenderModeObj GXPal524IntAa;
extern GXRenderModeObj GXEurgb60Hz240Ds;
extern GXRenderModeObj GXEurgb60Hz240DsAa;
extern GXRenderModeObj GXEurgb60Hz240Int;
extern GXRenderModeObj GXEurgb60Hz240IntAa;
extern GXRenderModeObj GXEurgb60Hz480IntDf;
extern GXRenderModeObj GXEurgb60Hz480Int;
extern GXRenderModeObj GXEurgb60Hz480IntAa;
}

// the render modes the screen can switch to
GXRenderModeObj* gVideoRenderModes[31] = {
	&GXNtsc240Ds,         &GXNtsc240DsAa,      &GXNtsc240Int,      &GXNtsc240IntAa,      &GXNtsc480IntDf,      &GXNtsc480Int,
	&GXNtsc480IntAa,      &GXNtsc480Prog,      &GXNtsc480ProgSoft, &GXNtsc480ProgAa,     &GXMpal240Ds,         &GXMpal240DsAa,
	&GXMpal240Int,        &GXMpal240IntAa,     &GXMpal480IntDf,    &GXMpal480Int,        &GXMpal480IntAa,      &GXPal264Ds,
	&GXPal264DsAa,        &GXPal264Int,        &GXPal264IntAa,     &GXPal528IntDf,       &GXPal528Int,         &GXPal524IntAa,
	&GXEurgb60Hz240Ds,    &GXEurgb60Hz240DsAa, &GXEurgb60Hz240Int, &GXEurgb60Hz240IntAa, &GXEurgb60Hz480IntDf, &GXEurgb60Hz480Int,
	&GXEurgb60Hz480IntAa,
};

// per glyph the stroke segments it draws, each byte pair a from,to point with x in the high
// nibble and y (0 at the glyph's top) in the low one, 0xFF ends the row. rows 0-9 are the
// digits, 10-35 the letters and 36-46 the punctuation DrawGsStrokeText looks up
u8 gGlyphStrokeTable[47][13] = {
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x63, 0x63, 0x03, 0x03, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x13, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x03, 0x03, 0x63, 0x40, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x03, 0x03, 0x63, 0x63, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x03, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x62, 0x62, 0x43, 0x43, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF, 0x00, 0x00 },
	{ 0x63, 0x03, 0x03, 0x00, 0x00, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x30, 0x30, 0x66, 0x14, 0x54, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x40, 0x40, 0x61, 0x61, 0x66, 0x66, 0x06, 0x06, 0x00, 0x03, 0x63, 0xFF },
	{ 0x60, 0x20, 0x20, 0x03, 0x03, 0x26, 0x26, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x30, 0x30, 0x63, 0x63, 0x36, 0x36, 0x06, 0x10, 0x16, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x06, 0x66, 0x03, 0x53, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x06, 0x02, 0x42, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x30, 0x30, 0x02, 0x02, 0x06, 0x06, 0x66, 0x66, 0x63, 0x63, 0x33, 0xFF },
	{ 0x00, 0x06, 0x60, 0x66, 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x10, 0x50, 0x16, 0x56, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x56, 0x56, 0x16, 0x16, 0x14, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x33, 0x33, 0x60, 0x60, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x63, 0x63, 0x36, 0x36, 0x03, 0x03, 0x30, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x50, 0x50, 0x61, 0x61, 0x62, 0x62, 0x53, 0x53, 0x03, 0xFF },
	{ 0x06, 0x02, 0x02, 0x60, 0x60, 0x64, 0x64, 0x06, 0x24, 0x46, 0x46, 0x66, 0xFF },
	{ 0x06, 0x00, 0x00, 0x60, 0x60, 0x62, 0x62, 0x02, 0x32, 0x66, 0xFF, 0x00, 0x00 },
	{ 0x60, 0x00, 0x00, 0x66, 0x66, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x06, 0x06, 0x66, 0x66, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x02, 0x02, 0x36, 0x36, 0x62, 0x62, 0x60, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x16, 0x16, 0x56, 0x56, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x66, 0x60, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x32, 0x32, 0x60, 0x32, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x60, 0x60, 0x06, 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x03, 0x22, 0x22, 0x44, 0x44, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x63, 0x63, 0x06, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x50, 0x16, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x40, 0x22, 0x22, 0x24, 0x24, 0x46, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x20, 0x42, 0x42, 0x44, 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x44, 0x26, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x60, 0x03, 0x03, 0x66, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x30, 0x36, 0x03, 0x63, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x61, 0x01, 0x01, 0x65, 0x65, 0x05, 0x30, 0x36, 0xFF, 0x00, 0x00, 0x00, 0x00 },
};

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
		if (IsSjisLead(*text)) {
#else
		if (gs->mUseShiftJis && (IsSjisLead(*text))) {
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
	GXPosition3f32(x, y, 0.0f);
	*reinterpret_cast<volatile u32*>(GX_FIFO_ADDR) = color;
}

// a stroke point is on a grid of 0.11 steps inside the character cell
static inline f32 StrokePos(s32 n)
{
	f32 pos = 0.11f * n;
	return 0.2f + pos;
}

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
	sprite->mColor0 = sprite->mColor1 = sprite->mColor2 = sprite->mColor3 = color;
}

static inline void SetColor(CTile* tile, u32 color)
{
	tile->mColor0 = tile->mColor1 = tile->mColor2 = tile->mColor3 = color;
}

/**
 * @note Address: 0x80196C0C
 * @note Size: 0x1C
 */
#if !defined(VERSION_JP)
extern "C" void SetGsFixedGlyphWidthScale(CGs* gs, f32 width)
{
	if (gs->mUseShiftJis) {
		gs->mShiftJisWidthScale = width;
	} else {
		gs->mSingleByteWidthScale = width;
	}
}
#endif

static inline f32 GetTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	CFont font;
	s32 charWidth;
#if !defined(VERSION_JP)
	const f32 halfSize = 0.5f * size;
#endif
	f32 lineWidth = 0.0f;
	f32 maxWidth  = 0.0f;
	TextGlyph glyph;
	const char* p = text;

	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] == '\0') {
			continue;
		}
		if (glyph.mStr[0] == '\n') {
			lineWidth = 0.0f;
			continue;
		}
		GetFontGlyphTexture(&font, glyph.mStr, &charWidth);
		if (size > 0.0f) {
			if (proportional) {
				lineWidth += charWidth * size / 24.0f;
#if defined(VERSION_JP)
			} else if (IsSjisLead(glyph.mStr[0])) {
				lineWidth += size;
			} else {
				lineWidth += size * 0.5f;
			}
#else
			} else if (gs->mUseShiftJis) {
				if (IsSjisLead(glyph.mStr[0])) {
					lineWidth += size * gs->mShiftJisWidthScale;
				} else {
					lineWidth += halfSize * gs->mShiftJisWidthScale;
				}
			} else {
				lineWidth += halfSize * gs->mSingleByteWidthScale;
			}
#endif
		} else {
			lineWidth += charWidth;
		}
		if (lineWidth > maxWidth) {
			maxWidth = lineWidth;
		}
	}
	return maxWidth;
}
/**
 * @note Address: 0x801968BC
 * @note Size: 0x350
 */
// NONMATCHING: retail keeps lineWidth/maxWidth in different float registers and stack slots; the remaining code differs only in scheduling.
extern "C" f32 MeasureGameTextWidth(CGs* gs, const char* text, BOOL proportional, f32 size)
{
	return GetTextWidth(gs, text, proportional, size);
}

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
 * @note Address: 0x801950C8
 * @note Size: 0x133C
 */
// draws text with one sprite per character and a background tile behind each when the
// background color isn't zero
// NONMATCHING: japan's build hoists size * 0.5f out of the loop where retail
// fuses it into the curX accumulate; float regalloc differs throughout.
extern "C" void DrawGsText(CGs* gs, u32 color, BOOL proportional, const char* text, f32 x, f32 y, f32 z, f32 size, f32 height)
{
	f32 offsX;
	f32 curX = x;
	f32 curY = y;
	TextGlyph glyph;
	s32 i;

	// an ortho camera has y growing down the screen, so the line step flips
	f32 lineHeight = gs->mCamera->projection_type == PROJ_ORTHO ? -height : height;
	glyph.mColor   = color;
	glyph.mBgColor = 0;
	static TextChar sGsTextChars[256];
	static TextChar* sCurrentTextChar;

	f32 textWidth = GetTextWidth(gs, text, proportional, size);

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

		sCurrentTextChar->mHasBg = glyph.mBgColor != 0;
		if (sCurrentTextChar->mHasBg) {
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
	SetupDrawState(gs);
	if (gs->mCurChannel == &gs->mChannels[0] && gs->mChannels[0].enable) {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.DrawLit(gs->mCamera->mViewMtx, sGsTextChars[i].mBg.mColor0, sGsTextChars[i].mBg.mColor1,
				                            sGsTextChars[i].mBg.mColor2, sGsTextChars[i].mBg.mColor3);
			}
		}
	} else {
		for (i = 0; i < count; i++) {
			if (sGsTextChars[i].mHasBg) {
				sGsTextChars[i].mBg.Draw(gs->mCamera->mViewMtx, sGsTextChars[i].mBg.mColor0, sGsTextChars[i].mBg.mColor1,
				                         sGsTextChars[i].mBg.mColor2, sGsTextChars[i].mBg.mColor3);
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
			sGsTextChars[i].mSprite.DrawLit(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite.mColor0,
			                                sGsTextChars[i].mSprite.mColor1, sGsTextChars[i].mSprite.mColor2,
			                                sGsTextChars[i].mSprite.mColor3);
		}
	} else {
		for (i = 0; i < count; i++) {
			sGsTextChars[i].mSprite.Draw(gs->mCamera->mViewMtx, GX_TEXMAP0, GX_TEXMTX0, GX_MTX2x4, sGsTextChars[i].mSprite.mColor0,
			                             sGsTextChars[i].mSprite.mColor1, sGsTextChars[i].mSprite.mColor2, sGsTextChars[i].mSprite.mColor3);
		}
	}

	if (gs->mCurTevStage == &gs->mTevStages[0]) {
		gs->mCurTevStage->SetOrder(GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		SetTevMode(gs->mCurTevStage, GX_PASSCLR);
		gs->mCurTevStage->SetNumTexGens(0);
	}
}
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

static f32 MeasureStrokeTextWidth(CGs* gs, const char* text)
{
	f32 lineWidth = 0.0f;
	f32 maxWidth  = 0.0f;
	const char* p = text;
	TextGlyph glyph;
	while ((p = GetGlyph(gs, p, &glyph)) != NULL) {
		if (glyph.mStr[0] != '\0') {
			if (glyph.mStr[0] == '\n') {
				lineWidth = 0.0f;
			} else {
				lineWidth += 1.0f;
				if (lineWidth > maxWidth) {
					maxWidth = lineWidth;
				}
			}
		}
	}
	return maxWidth;
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
	SetGsChannelMaterialAndLighting(gs, GX_SRC_VTX, 0);
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
		// Preserve the exact subpixel positions used by the punctuation glyphs.
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
