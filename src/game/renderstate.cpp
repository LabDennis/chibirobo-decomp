#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/cgsdefaults.h"
#include "game/csound.h"
#include "game/mm.h"
#include "game/progressive.h"
#include "game/video.h"
#include "game/vistate.h"
extern "C" {
#include <dolphin/gx/gx.h>
extern GXRenderModeObj GXPal528IntDf;
void HSD_SetPixelFormat(s32, s32);
void HSD_SetZSortMode(s32, s32);
void GXSetClipMode(s32);
}
extern "C" {
#include <sysdolphin/initialize.h>
}

extern "C" void GameXFBDrawDoneCallback(int idx);

/**
 * @note Address: 0x8001D51C
 * @note Size: 0x384
 */
extern "C" void InitGameRenderState(CBase* base)
{
	CGs* gs = &base->mGs;
#if defined(VERSION_PAL)
	SetGsScreenModeAndRenderDefaults(gs, 21, 0, 0, 0);
	gs->mScreen._40      = gGame.mGs.mScreen.xfbHeight / 448.0f;
	gs->mScreen.viHeight = 480;
#else
	SetGsScreenModeAndRenderDefaults(gs, 4, 0, 16, 0);
#endif
#if !defined(VERSION_JP)
	SetGsFixedGlyphWidthScale(gs, 1.2f);
#endif
	HSD_SetPixelFormat(1, 0);
	GXSetPixelFmt(1, 0);
	GXSetCopyFilter(0, NULL, 0, NULL);

	InitializeVideoClearState((GXColor) { 0, 0, 0, 0 });
	GXSetClipMode(0);
	HSD_SetZSortMode(1, 1);

	ConfigureGsRenderMode(gs, NULL, (u8) { GX_TRUE }, (u8) { GX_FALSE }, (GsBlendMode) { 1, 4, 5, 15 },
	                      (GsAlphaCompare) { 4, 0, 0, 3, 255 }, (GsDstAlpha) { 0, 0 }, (GsZMode) { 1, 7, 1 }, (u8) { GX_TRUE },
	                      (u8) { GX_TRUE }, (s32) { 2 }, (GsLineWidth) { 12, 5 }, (GsLineWidth) { 24, 5 });
	ConfigureGsPrimaryTevStage(gs);
	ConfigureGsChannel(gs, NULL, (s32) { GX_COLOR0 }, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 2, 2 },
	                   (GXColor) { 0, 0, 0, 255 }, (GXColor) { 255, 255, 255, 255 });
	ConfigureGsChannel(gs, NULL, (s32) { GX_ALPHA0 }, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 0, 2 },
	                   (GXColor) { 0, 0, 0, 255 }, (GXColor) { 255, 255, 255, 255 });
	if (gs->mCurRenderMode)
		ApplyGsRenderMode(gs->mCurRenderMode, TRUE);
	if (gs->mCurTevStage)
		ApplyGsTevStage(gs->mCurTevStage, TRUE);
	if (gs->mCurChannel)
		ApplyGsChannel(gs->mCurChannel, TRUE);
	HSD_VISetUserGXDrawDoneCallback(GameXFBDrawDoneCallback);
}
