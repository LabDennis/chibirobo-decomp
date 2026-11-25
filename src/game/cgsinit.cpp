#include "game/cgame.h"
#include "game/cgsdefaults.h"
#include "game/ccamera.h"
#include "game/clight.h"

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
	ch->amb_color = amb;
	ch->mat_color = mat;
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
