#include <global.h>
#include <sysdolphin/particle.h>

u32 lbl_8065BAB8;

#define GX_TEVSTAGE0     0
#define GX_TEVSTAGE1     1
#define GX_TEVSTAGE2     2
#define GX_TEXCOORD0     0
#define GX_TEXCOORD_NULL 0xFF
#define GX_TEV_ADD       0
#define GX_TB_ZERO       0
#define GX_CS_SCALE_1    0
#define GX_TEVPREV       0
#define GX_TEV_SWAP0     0
#define GX_MODULATE      0

#define GX_CC_CPREV 0
#define GX_CC_C0    2
#define GX_CC_C1    4
#define GX_CC_C2    6
#define GX_CC_TEXC  8
#define GX_CC_RASC  10
#define GX_CC_ZERO  15

#define GX_CA_APREV 0
#define GX_CA_A0    1
#define GX_CA_A1    2
#define GX_CA_A2    3
#define GX_CA_TEXA  4
#define GX_CA_RASA  5
#define GX_CA_ZERO  7

extern void GXSetTevOrder(u8 stage, u32 coord, u32 map, u32 color);
extern void GXSetTevColorOp(u8 stage, u32 op, u32 bias, u32 scale, u32 clamp, u32 out_reg);
extern void GXSetTevAlphaOp(u8 stage, u32 op, u32 bias, u32 scale, u32 clamp, u32 out_reg);
extern void GXSetTevSwapMode(u8 stage, u32 ras_swap, u32 tex_swap);
extern void GXSetNumTevStages(u8 n);
extern void GXSetNumTexGens(u8 n);
extern void GXSetTevColorIn(u8 stage, u32 a, u32 b, u32 c, u32 d);
extern void GXSetTevAlphaIn(u8 stage, u32 a, u32 b, u32 c, u32 d);
extern void GXSetTevOp(u8 stage, u32 mode);

/**
 * @note Address: 0x801E4E00
 * @note Size: 0x130
 */
void psSetupTevCommon(void)
{
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
	GXSetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
	GXSetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
	GXSetTevSwapMode(GX_TEVSTAGE2, GX_TEV_SWAP0, GX_TEV_SWAP0);
}

/**
 * @note Address: 0x801E4F30
 * @note Size: 0xC
 */
void psSetupTevInvalidState(void)
{
	lbl_8065BAB8 = -1;
}

/**
 * @note Address: 0x801E4F3C
 * @note Size: 0x6D8
 */
void psSetupTev(HSD_Particle* pp)
{
	s32 kind;

	kind = pp->kind & 0x80100480;
	if (kind != lbl_8065BAB8) {
		lbl_8065BAB8 = kind;
		switch (kind) {
		case 0x80000080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x80000000:
			GXSetNumTevStages(2);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x0:
			GXSetNumTevStages(1);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80000480:
			GXSetNumTevStages(3);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80000400:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x400:
		case 0x480:
			GXSetNumTevStages(1);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			return;
		case 0x100400:
			GXSetNumTevStages(1);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
			return;
		case 0x100480:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80100400:
			GXSetNumTevStages(2);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x80100480:
			GXSetNumTevStages(3);
			GXSetNumTexGens(1);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A1, GX_CA_A0, GX_CA_TEXA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A2, GX_CA_APREV, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_RASC, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_RASA, GX_CA_APREV, GX_CA_ZERO);
			return;
		case 0x100080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x100000:
			GXSetNumTevStages(1);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			return;
		case 0x80100080:
			pp->kind &= ~0x80;
			lbl_8065BAB8 &= ~0x80;
			// fallthrough
		case 0x80100000:
			GXSetNumTevStages(2);
			GXSetNumTexGens(0);
			GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
			GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_A2, GX_CA_RASA, GX_CA_ZERO);
			GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_CPREV, GX_CC_ZERO);
			GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_A0, GX_CA_APREV, GX_CA_ZERO);
			return;
		}
	}
}
