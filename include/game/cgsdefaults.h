#ifndef GAME_CGS_RENDER_DEFAULTS_H
#define GAME_CGS_RENDER_DEFAULTS_H

#include "game/cgs.h"
#include "game/vistate.h"

static inline void ConfigureGsPrimaryTevStage(CGs* gs)
{
	ConfigureGsTevStage(gs, NULL, 0, (GsTevOrder) { 0xFF, GX_TEXMAP_NULL, GX_COLOR0A0 },
	                    (GsTevOp) { GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, 0 }, (GsTevIn) { 0xA, 0xF, 0xF, 0xF },
	                    (GsTevOp) { GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, 0 }, (GsTevIn) { 5, 7, 7, 7 });
}

static inline void InitializeVideoClearState(GXColor color)
{
	HSD_VIData.current.vi.clear_z   = GX_MAX_Z24;
	HSD_VIData.current.vi.clear_clr = color;
	HSD_VIData.current.vi.gamma     = 0;
}

#endif
