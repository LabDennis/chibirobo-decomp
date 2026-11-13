#include "game/xobjactor.h"
#include "game/xchr.h"
#include "game/globals.h"
#include "game/infoscrn.h"
#include <stdlib.h>

// XObjActor3's panel on stage02, the count over it

extern "C" {
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
}

// a texture's size on screen at the scale
static inline f32 TexSize(f32 size, f32 scale)
{
	return scale * size;
}

/**
 * @note Address: 0x8010904C
 * @note Size: 0x59C
 */
// Draw the stage02 spider count as two digits over the panel.
// NONMATCHING: the colours, the texture matrix pointer and the loop count take other registers
extern "C" void DrawStage02ActorCounterPanel(XObjActor3* actor)
{
	f32 alpha = actor->mCounterPanelAlpha;
	if (alpha <= 0.0f) {
		return;
	}
	if (!IsStage("stage02")) {
		return;
	}
	if (!actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	tex       = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[9], 0);
	f32 scale = 1.0f;
	f32 w     = scale * GXGetTexObjWidth(&tex);
	u16 h     = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, w, TexSize(h, 1.0f), 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	tex    = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[1], 0);
	f32 dw = 1.4f * TexSize(GXGetTexObjWidth(&tex) / 11, 1.0f);
	f32 x;
	f32 dh    = 1.4f * TexSize(GXGetTexObjHeight(&tex), 1.0f);
	x         = 110.0f - (1 - actor->mSpiderCount / 10) * 12;
	f32 count = actor->mSpiderCount;
	PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f);
	tex.mMtxDirty = TRUE;
	s32 i         = 0;
	u32 a         = 255.0f * alpha;
	u32 c0;
	u32 c1 = a | 0x1F33FF00;
	c0     = a | 0xFFFFFF00;
	for (; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
}

extern "C" {
#if !defined(VERSION_JP)
extern f32 gStage02TimerOutlineX[], gStage02TimerOutlineY[];
#endif
}

#if !defined(VERSION_JP)
static inline void DrawTimerLabel(f32 scale)
{
	const f32 size = 20.0f;
	f32 width      = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x5B), TRUE, size);
	DrawGsQuad(&gGame.mGs, 0xFF, 0xFF, 0xFF, 0xFF, 320.0f, 40.0f, 0.0f, (width - 4.0f) * scale, 12.0f, 0.0f, 0.0f, 0.0f);
	f32 left = 320.0f - MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(0x5B), TRUE, size) * 0.5f;
	for (int i = 0; i < 4; ++i)
		DrawGsProportionalTextFormat(&gGame.mGs, 0xFF, GetLocalizedInterfaceText(0x5B), left + gStage02TimerOutlineX[i],
		                             27.0f + gStage02TimerOutlineY[i], 0.0f, size, size);
	DrawGsProportionalTextFormat(&gGame.mGs, -1, GetLocalizedInterfaceText(0x5B), left, 27.0f, 0.0f, size, size);
}
#endif
extern "C" void DrawStage02TimerPanel(XObjActor3* actor)
{
	f32 alpha = actor->mTimerPanelAlpha;
	if (alpha <= 0.0f) {
		return;
	}
	if (!IsStage("stage02")) {
		return;
	}
	if (!actor->mPanelTpl.mBound) {
		return;
	}
	CTexObj tex;
	tex       = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[4], 0);
	f32 scale = 1.0f;
#if defined(VERSION_JP)
	f32 w = TexSize(GXGetTexObjWidth(&tex), scale);
#else
	u16 w = GXGetTexObjWidth(&tex);
#endif
	u16 h = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, scale * w, TexSize(h, 1.0f), 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if !defined(VERSION_JP)
	DrawTimerLabel(scale);
#endif
	tex    = actor->mPanelTpl.mTex.Init(&((TPLPalette*)actor->mPanelTpl.mBuffer)->descriptorArray[1], 0);
	f32 dw = 1.4f * TexSize(GXGetTexObjWidth(&tex) / 11, 1.0f);
	f32 dh = 1.4f * TexSize(GXGetTexObjHeight(&tex), 1.0f);
	f32 x  = 110.0f;
#if !defined(VERSION_JP)
	x += 11.0f;
#endif
	f32 count = TimerFramesToHundredths(actor->mCountdownFrames) / 100.0f;
	PSMTXScale(tex.mMtx[1], 0.09090909f, 1.0f, 1.0f);
	tex.mMtxDirty = TRUE;
	u32 a         = 255.0f * alpha;
	u32 c0;
	u32 c1 = a | 0x1F33FF00;
	c0     = a | 0xFFFFFF00;
	for (s32 i = 0; i < 2; i++) {
		f32 p = 1.0f;
		if (i > 0) {
			for (s32 k = 0; k < i; k++) {
				p *= 10.0f;
			}
		}
		s32 d = count / p;
		if (i == 0 || d != 0) {
			d %= 10;
			PSMTXTrans(tex.mMtx[0], d * 0.09090909f, 0.0f, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - i * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
			                   0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}

	dw = 1.4f * TexSize(GXGetTexObjWidth(&tex) / 11, 1.0f);
	dh = 1.4f * TexSize(GXGetTexObjHeight(&tex), 1.0f);
	PSMTXTrans(tex.mMtx[0], 10.0f / 11.0f, 0.0f, 0.0f);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	s32 digits = count < 10.0f ? 1 : 2;
	DrawGsTexturedQuad(&gGame.mGs, &tex, c0, c0, c1, c1, 7.0f * dw + (x - digits * (dw - 8.0f)) - 32.0f, 76.0f, 0.0f, dw, dh, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}
