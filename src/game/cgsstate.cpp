#include "game/cgs.h"
#include "Version.h"

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
