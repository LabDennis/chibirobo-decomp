#ifndef _GAME_MENUSELECTIONBAR_H
#define _GAME_MENUSELECTIONBAR_H

#include "game/cgame.h"

static inline void InitMenuSelectionBar(CTexObj& tex)
{
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
#if defined(VERSION_JP)
	const int edgeTex = 115;
#else
	const int edgeTex = 117;
#endif
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[edgeTex], 0);
	PSMTXScale(tex.mMtx[1], 0.25f, 1.0f, 1.0f);
	tex.mMtxDirty = TRUE;
	PSMTXTrans(tex.mMtx[0], 0.25f, 0.0f, 0.0f);
	tex.mMtxDirty = TRUE;
}

static inline void DrawMenuSelectionBar(CTexObj& tex, f32 barX, f32 barY, f32 barW, f32 barH, f32 alpha, f32 textureOffset, u32 barColor)
{
#if defined(VERSION_JP)
	const int fillTex = 116;
#else
	const int fillTex = 118;
#endif
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, barX, barY, 0.0f, barW, barH, 0.0f, 0.0f, 0.0f);
	PSMTXTrans(tex.mMtx[0], 0.0f, 0.0f, 0.0f);
	tex.mMtxDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, (barX - barW * 0.5f) - 4.0f, barY, 0.0f, 8.0f, barH, 0.0f,
	                   0.0f, 0.0f);
	PSMTXTrans(tex.mMtx[0], 0.5f, 0.0f, 0.0f);
	tex.mMtxDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, barColor, barColor, barColor, barColor, 4.0f + (barX + barW * 0.5f), barY, 0.0f, 8.0f, barH, 0.0f,
	                   0.0f, 0.0f);
	CTexObj& fill = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[fillTex], 0);
	PSMTXTrans(fill.mMtx[0], textureOffset, 0.0f, 0.0f);
	fill.mMtxDirty = TRUE;
	PSMTXScale(fill.mMtx[1], 0.5f * (barW / GXGetTexObjWidth(&fill)), 0.5f, 1.0f);
	fill.mMtxDirty = TRUE;
	u32 shine      = (u32)(40.0f * alpha) | 0xFFFFFF00;
	DrawGsTexturedQuad(&gGame.mGs, &fill, shine, shine, shine, shine, 2.0f + barX, barY, 0.0f, barW, barH - 4.0f, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

#endif
