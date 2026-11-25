#include "Dolphin/gx.h"

/**
 * @note Address: 0x80189BE0
 * @note Size: 0x17C
 */
void __GXSetSUTexRegs(void)
{
	u32 stage;
	u32 indirectStages;
	u32 tevStages;
	u32 texMap;
	u32 texCoord;

	if (gx->tcsManEnab != ((1u << GX_MAX_TEXCOORD) - 1)) {
		tevStages = GX_GET_REG(gx->genMode, 18, 21) + 1;
		indirectStages = GX_GET_REG(gx->genMode, GX_BP_GENMODE_NUMINDSTAGES_ST, GX_BP_GENMODE_NUMINDSTAGES_END);
		for (stage = 0; stage < indirectStages; stage++) {
			switch (stage) {
			case 0:
				texMap = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_MAP0_ST, GX_BP_RAS1_IREF_MAP0_END);
				texCoord = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_TXC0_ST, GX_BP_RAS1_IREF_TXC0_END);
				break;
			case 1:
				texMap = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_MAP1_ST, GX_BP_RAS1_IREF_MAP1_END);
				texCoord = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_TXC1_ST, GX_BP_RAS1_IREF_TXC1_END);
				break;
			case 2:
				texMap = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_MAP2_ST, GX_BP_RAS1_IREF_MAP2_END);
				texCoord = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_TXC2_ST, GX_BP_RAS1_IREF_TXC2_END);
				break;
			case 3:
				texMap = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_MAP3_ST, GX_BP_RAS1_IREF_MAP3_END);
				texCoord = GX_GET_REG(gx->iref, GX_BP_RAS1_IREF_TXC3_ST, GX_BP_RAS1_IREF_TXC3_END);
				break;
			}

			if (!(gx->tcsManEnab & (1 << texCoord))) {
				__SetSURegs(texMap, texCoord);
			}
		}

		for (stage = 0; stage < tevStages; stage++) {
			// Each TEV order register holds two stages.
			u32 tevOrderIndex = stage / 2;
			const u32* tevOrder = &gx->tref[tevOrderIndex];

			texMap = gx->texmapId[stage] & ~GX_TEX_DISABLE;

			if (stage & 1) {
				texCoord = GX_GET_REG(*tevOrder, 14, 16);
			} else {
				texCoord = GX_GET_REG(*tevOrder, 26, 28);
			}

			if (texMap != GX_TEXMAP_NULL && !(gx->tcsManEnab & (1 << texCoord)) && (gx->tevTcEnab & (1 << stage))) {
				__SetSURegs(texMap, texCoord);
			}
		}
	}
}

/**
 * @note Address: 0x80189D5C
 * @note Size: 0x354
 */
void __GXSetTmemConfig(u32 config)
{
	switch (config) {
	case 2:
		GX_BP_LOAD_REG(0x8c0d8000);
		GX_BP_LOAD_REG(0x900dc000);

		GX_BP_LOAD_REG(0x8d0d8800);
		GX_BP_LOAD_REG(0x910dc800);

		GX_BP_LOAD_REG(0x8e0d9000);
		GX_BP_LOAD_REG(0x920dd000);

		GX_BP_LOAD_REG(0x8f0d9800);
		GX_BP_LOAD_REG(0x930dd800);

		GX_BP_LOAD_REG(0xac0da000);
		GX_BP_LOAD_REG(0xb00dc400);

		GX_BP_LOAD_REG(0xad0da800);
		GX_BP_LOAD_REG(0xb10dcc00);

		GX_BP_LOAD_REG(0xae0db000);
		GX_BP_LOAD_REG(0xb20dd400);

		GX_BP_LOAD_REG(0xaf0db800);
		GX_BP_LOAD_REG(0xb30ddc00);
		break;
	case 1:
		GX_BP_LOAD_REG(0x8c0d8000);
		GX_BP_LOAD_REG(0x900dc000);

		GX_BP_LOAD_REG(0x8d0d8800);
		GX_BP_LOAD_REG(0x910dc800);

		GX_BP_LOAD_REG(0x8e0d9000);
		GX_BP_LOAD_REG(0x920dd000);

		GX_BP_LOAD_REG(0x8f0d9800);
		GX_BP_LOAD_REG(0x930dd800);

		GX_BP_LOAD_REG(0xac0da000);
		GX_BP_LOAD_REG(0xb00de000);

		GX_BP_LOAD_REG(0xad0da800);
		GX_BP_LOAD_REG(0xb10de800);

		GX_BP_LOAD_REG(0xae0db000);
		GX_BP_LOAD_REG(0xb20df000);

		GX_BP_LOAD_REG(0xaf0db800);
		GX_BP_LOAD_REG(0xb30df800);

		break;
	case 0:
	default:
		GX_BP_LOAD_REG(0x8c0d8000);
		GX_BP_LOAD_REG(0x900dc000);

		GX_BP_LOAD_REG(0x8d0d8400);
		GX_BP_LOAD_REG(0x910dc400);

		GX_BP_LOAD_REG(0x8e0d8800);
		GX_BP_LOAD_REG(0x920dc800);

		GX_BP_LOAD_REG(0x8f0d8c00);
		GX_BP_LOAD_REG(0x930dcc00);

		GX_BP_LOAD_REG(0xac0d9000);
		GX_BP_LOAD_REG(0xb00dd000);

		GX_BP_LOAD_REG(0xad0d9400);
		GX_BP_LOAD_REG(0xb10dd400);

		GX_BP_LOAD_REG(0xae0d9800);
		GX_BP_LOAD_REG(0xb20dd800);

		GX_BP_LOAD_REG(0xaf0d9c00);
		GX_BP_LOAD_REG(0xb30ddc00);

		break;
	}
}
