#include <dolphin/gx.h>
#include <dolphin/os.h>

#include "__gx.h"

// Color and alpha combiner words for ADD, zero bias, scale 1, and clamped output.
#define TEV_COLOR_PRESET(a, b, c, d) (0xC0080000u | ((a) << 12) | ((b) << 8) | ((c) << 4) | (d))
#define TEV_ALPHA_PRESET(a, b, c, d) (0xC1080000u | ((a) << 13) | ((b) << 10) | ((c) << 7) | ((d) << 4))

static u32 TEVCOpTableST0[] = {
    TEV_COLOR_PRESET(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO),  // GX_MODULATE
    TEV_COLOR_PRESET(GX_CC_RASC, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO),  // GX_DECAL
    TEV_COLOR_PRESET(GX_CC_RASC, GX_CC_ONE,  GX_CC_TEXC, GX_CC_ZERO),  // GX_BLEND
    TEV_COLOR_PRESET(GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC),  // GX_REPLACE
    TEV_COLOR_PRESET(GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC),  // GX_PASSCLR
};

static u32 TEVCOpTableST1[] = {
    TEV_COLOR_PRESET(GX_CC_ZERO,  GX_CC_TEXC, GX_CC_CPREV, GX_CC_ZERO),
    TEV_COLOR_PRESET(GX_CC_CPREV, GX_CC_TEXC, GX_CC_TEXA,  GX_CC_ZERO),
    TEV_COLOR_PRESET(GX_CC_CPREV, GX_CC_ONE,  GX_CC_TEXC,  GX_CC_ZERO),
    TEV_COLOR_PRESET(GX_CC_ZERO,  GX_CC_ZERO, GX_CC_ZERO,  GX_CC_TEXC),
    TEV_COLOR_PRESET(GX_CC_ZERO,  GX_CC_ZERO, GX_CC_ZERO,  GX_CC_CPREV),
};

static u32 TEVAOpTableST0[] = {
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA),
};

static u32 TEVAOpTableST1[] = {
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,  GX_CA_APREV),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,  GX_CA_TEXA),
    TEV_ALPHA_PRESET(GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,  GX_CA_APREV),
};

/**
 * @note Address: 0x8018A324
 * @note Size: 0x8C
 */
void GXSetTevOp(GXTevStageID id, GXTevMode mode) {
    u32* ctmp;
    u32* atmp;
    u32 tevReg;

    CHECK_GXBEGIN(420, "GXSetTevOp");
    ASSERTMSGLINE(421, id < GX_MAX_TEVSTAGE, "GXSetTevColor*: Invalid Tev Stage Index");
    ASSERTMSGLINE(422, mode <= GX_PASSCLR, "GXSetTevOp: Invalid Tev Mode");

    if (id == GX_TEVSTAGE0) {
        ctmp = &TEVCOpTableST0[mode];
        atmp = &TEVAOpTableST0[mode];
    } else {
        ctmp = &TEVCOpTableST1[mode];
        atmp = &TEVAOpTableST1[mode];
    }

    tevReg = __GXData->tevc[id];
    tevReg = (*ctmp & ~0xFF000000) | (tevReg & 0xFF000000);
    GX_WRITE_RAS_REG(tevReg);
    __GXData->tevc[id] = tevReg;

    tevReg = __GXData->teva[id];
    tevReg = (*atmp & ~0xFF00000F) | (tevReg & 0xFF00000F);
    GX_WRITE_RAS_REG(tevReg);
    __GXData->teva[id] = tevReg;

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A3B0
 * @note Size: 0x44
 */
void GXSetTevColorIn(GXTevStageID stage, GXTevColorArg a, GXTevColorArg b, GXTevColorArg c, GXTevColorArg d) {
    u32 tevReg;

    CHECK_GXBEGIN(578, "GXSetTevColorIn");
    ASSERTMSGLINE(579, stage < GX_MAX_TEVSTAGE, "GXSetTevColor*: Invalid Tev Stage Index");
    ASSERTMSGLINE(580, a <= GX_CC_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(581, b <= GX_CC_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(582, c <= GX_CC_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(583, d <= GX_CC_ZERO, "GXSetTev*In: A/B/C/D argument out of range");

    tevReg = __GXData->tevc[stage];
    SET_REG_FIELD(586, tevReg, 4, 12, a);
    SET_REG_FIELD(587, tevReg, 4,  8, b);
    SET_REG_FIELD(588, tevReg, 4,  4, c);
    SET_REG_FIELD(589, tevReg, 4,  0, d);

    GX_WRITE_RAS_REG(tevReg);
    __GXData->tevc[stage] = tevReg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A3F4
 * @note Size: 0x44
 */
void GXSetTevAlphaIn(GXTevStageID stage, GXTevAlphaArg a, GXTevAlphaArg b, GXTevAlphaArg c, GXTevAlphaArg d) {
    u32 tevReg;

    CHECK_GXBEGIN(614, "GXSetTevAlphaIn");
    ASSERTMSGLINE(615, stage < GX_MAX_TEVSTAGE, "GXSetTevAlpha*: Invalid Tev Stage Index");
    ASSERTMSGLINE(616, a <= GX_CA_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(617, b <= GX_CA_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(618, c <= GX_CA_ZERO, "GXSetTev*In: A/B/C/D argument out of range");
    ASSERTMSGLINE(619, d <= GX_CA_ZERO, "GXSetTev*In: A/B/C/D argument out of range");

    tevReg = __GXData->teva[stage];
    SET_REG_FIELD(622, tevReg, 3, 13, a);
    SET_REG_FIELD(623, tevReg, 3, 10, b);
    SET_REG_FIELD(624, tevReg, 3,  7, c);
    SET_REG_FIELD(625, tevReg, 3,  4, d);

    GX_WRITE_RAS_REG(tevReg);
    __GXData->teva[stage] = tevReg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A438
 * @note Size: 0x68
 */
void GXSetTevColorOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID out_reg) {
    u32 tevReg;

    CHECK_GXBEGIN(653, "GXSetTevColorOp");
    ASSERTMSGLINE(654, stage < GX_MAX_TEVSTAGE, "GXSetTevColor*: Invalid Tev Stage Index");

    tevReg = __GXData->tevc[stage];
    SET_REG_FIELD(663, tevReg, 1, 18, op & 1);
    if (op <= 1) {
        SET_REG_FIELD(665, tevReg, 2, 20, scale);
        SET_REG_FIELD(666, tevReg, 2, 16, bias);
    } else {
        SET_REG_FIELD(668, tevReg, 2, 20, (op >> 1) & 3);
        SET_REG_FIELD(672, tevReg, 2, 16, 3);
    }
    SET_REG_FIELD(672, tevReg, 1, 19, clamp & 0xFF);
    SET_REG_FIELD(673, tevReg, 2, 22, out_reg);

    GX_WRITE_RAS_REG(tevReg);
    __GXData->tevc[stage] = tevReg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A4A0
 * @note Size: 0x68
 */
void GXSetTevAlphaOp(GXTevStageID stage, GXTevOp op, GXTevBias bias, GXTevScale scale, GXBool clamp, GXTevRegID out_reg) {
    u32 tevReg;

    CHECK_GXBEGIN(699, "GXSetTevAlphaOp");
    ASSERTMSGLINE(700, stage < GX_MAX_TEVSTAGE, "GXSetTevAlpha*: Invalid Tev Stage Index");

    tevReg = __GXData->teva[stage];
    SET_REG_FIELD(708, tevReg, 1, 18, op & 1);
    if (op <= 1) {
        SET_REG_FIELD(710, tevReg, 2, 20, scale);
        SET_REG_FIELD(711, tevReg, 2, 16, bias);
    } else {
        SET_REG_FIELD(713, tevReg, 2, 20, (op >> 1) & 3);
        SET_REG_FIELD(717, tevReg, 2, 16, 3);
    }
    SET_REG_FIELD(717, tevReg, 1, 19, clamp & 0xFF);
    SET_REG_FIELD(718, tevReg, 2, 22, out_reg);

    GX_WRITE_RAS_REG(tevReg);
    __GXData->teva[stage] = tevReg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A508
 * @note Size: 0x60
 */
void GXSetTevColor(GXTevRegID id, GXColor color) {
    u32 regRA;
    u32 regBG;

    CHECK_GXBEGIN(740, "GXSetTevColor");

    regRA = (0xE0 + id * 2) << 24;
    SET_REG_FIELD(745, regRA, 8, 0, color.r);
    SET_REG_FIELD(746, regRA, 8, 12, color.a);

    regBG = (0xE1 + id * 2) << 24;
    SET_REG_FIELD(749, regBG, 8, 0, color.b);
    SET_REG_FIELD(750, regBG, 8, 12, color.g);

    GX_WRITE_RAS_REG(regRA);
    GX_WRITE_RAS_REG(regBG);
    GX_WRITE_RAS_REG(regBG);
    GX_WRITE_RAS_REG(regBG);

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A568
 * @note Size: 0x64
 */
void GXSetTevColorS10(GXTevRegID id, GXColorS10 color) {
    u32 regRA;
    u32 regBG;

    ASSERTMSGLINE(777, color.r >= -1024 && color.r < 1024, "GXSetTevColorS10: Color not in range -1024 to +1023");
    ASSERTMSGLINE(778, color.g >= -1024 && color.g < 1024, "GXSetTevColorS10: Color not in range -1024 to +1023");
    ASSERTMSGLINE(779, color.b >= -1024 && color.b < 1024, "GXSetTevColorS10: Color not in range -1024 to +1023");
    ASSERTMSGLINE(780, color.a >= -1024 && color.a < 1024, "GXSetTevColorS10: Color not in range -1024 to +1023");

    CHECK_GXBEGIN(782, "GXSetTevColorS10");

    regRA = (0xE0 + id * 2) << 24;
    SET_REG_FIELD(789, regRA, 11, 0, (u32)color.r & 0x7FF);
    SET_REG_FIELD(790, regRA, 11, 12, (u32)color.a & 0x7FF);

    regBG = (0xE1 + id * 2) << 24;
    SET_REG_FIELD(793, regBG, 11, 0, (u32)color.b & 0x7FF);
    SET_REG_FIELD(794, regBG, 11, 12, (u32)color.g & 0x7FF);

    GX_WRITE_RAS_REG(regRA);
    GX_WRITE_RAS_REG(regBG);
    GX_WRITE_RAS_REG(regBG);
    GX_WRITE_RAS_REG(regBG);

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A5CC
 * @note Size: 0x64
 */
void GXSetTevKColor(GXTevKColorID id, GXColor color) {
    u32 regRA;
    u32 regBG;

    CHECK_GXBEGIN(833, "GXSetTevKColor");

    regRA = (0xE0 + id * 2) << 24;
    SET_REG_FIELD(838, regRA, 8, 0, color.r);
    SET_REG_FIELD(839, regRA, 8, 12, color.a);
    SET_REG_FIELD(839, regRA, 4, 20, 8);

    regBG = (0xE1 + id * 2) << 24;
    SET_REG_FIELD(843, regBG, 8, 0, color.b);
    SET_REG_FIELD(844, regBG, 8, 12, color.g);
    SET_REG_FIELD(845, regBG, 4, 20, 8);

    GX_WRITE_RAS_REG(regRA);
    GX_WRITE_RAS_REG(regBG);

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A630
 * @note Size: 0x5C
 */
void GXSetTevKColorSel(GXTevStageID stage, GXTevKColorSel sel) {
    u32* Kreg;

    CHECK_GXBEGIN(872, "GXSetTevKColorSel");
    ASSERTMSGLINE(873, stage < GX_MAX_TEVSTAGE, "GXSetTevKColor*: Invalid Tev Stage Index");

    Kreg = &__GXData->tevKsel[stage >> 1];
    if (stage & 1) {
        SET_REG_FIELD(0x36E, *Kreg, 5, 14, sel);
    } else {
        SET_REG_FIELD(0x370, *Kreg, 5, 4, sel);
    }

    GX_WRITE_RAS_REG(*Kreg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A68C
 * @note Size: 0x5C
 */
void GXSetTevKAlphaSel(GXTevStageID stage, GXTevKAlphaSel sel) {
    u32* Kreg;

    CHECK_GXBEGIN(905, "GXSetTevKAlphaSel");
    ASSERTMSGLINE(906, stage < GX_MAX_TEVSTAGE, "GXSetTevKColor*: Invalid Tev Stage Index");

    Kreg = &__GXData->tevKsel[stage >> 1];
    if (stage & 1) {
        SET_REG_FIELD(911, *Kreg, 5, 19, sel);
    } else {
        SET_REG_FIELD(913, *Kreg, 5, 9, sel);
    }

    GX_WRITE_RAS_REG(*Kreg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A6E8
 * @note Size: 0x48
 */
void GXSetTevSwapMode(GXTevStageID stage, GXTevSwapSel ras_sel, GXTevSwapSel tex_sel) {
    u32* pTevReg;

    CHECK_GXBEGIN(942, "GXSetTevSwapMode");
    ASSERTMSGLINE(943, stage < GX_MAX_TEVSTAGE, "GXSetTevSwapMode: Invalid Tev Stage Index");

    pTevReg = &__GXData->teva[stage];
    SET_REG_FIELD(946, *pTevReg, 2, 0, ras_sel);
    SET_REG_FIELD(947, *pTevReg, 2, 2, tex_sel);

    GX_WRITE_RAS_REG(*pTevReg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A730
 * @note Size: 0x80
 */
void GXSetTevSwapModeTable(GXTevSwapSel table, GXTevColorChan red, GXTevColorChan green, GXTevColorChan blue, GXTevColorChan alpha) {
    u32* Kreg;
    int index;

    CHECK_GXBEGIN(978, "GXSetTevSwapModeTable");
    ASSERTMSGLINE(979, table < GX_MAX_TEVSWAP, "GXSetTevSwapModeTable: Invalid Swap Selection Index");

    index = table * 2;
    Kreg = &__GXData->tevKsel[index];
    SET_REG_FIELD(982, *Kreg, 2, 0, red);
    SET_REG_FIELD(983, *Kreg, 2, 2, green);

    GX_WRITE_RAS_REG(*Kreg);

    Kreg = &__GXData->tevKsel[index + 1];
    SET_REG_FIELD(987, *Kreg, 2, 0, blue);
    SET_REG_FIELD(988, *Kreg, 2, 2, alpha);

    GX_WRITE_RAS_REG(*Kreg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXSetTevClampMode(void) {
    ASSERTMSGLINE(1012, 0, "GXSetTevClampMode: not available on this hardware");
}

/**
 * @note Address: 0x8018A7B0
 * @note Size: 0x44
 */
void GXSetAlphaCompare(GXCompare comp0, u8 ref0, GXAlphaOp op, GXCompare comp1, u8 ref1) {
    u32 reg;

    CHECK_GXBEGIN(1046, "GXSetAlphaCompare");
    reg = 0xF3000000;

    SET_REG_FIELD(1049, reg, 8, 0, ref0);
    SET_REG_FIELD(1050, reg, 8, 8, ref1);
    SET_REG_FIELD(1051, reg, 3, 16, comp0);
    SET_REG_FIELD(1052, reg, 3, 19, comp1);
    SET_REG_FIELD(1053, reg, 2, 22, op);

    GX_WRITE_RAS_REG(reg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A7F4
 * @note Size: 0x8C
 */
void GXSetZTexture(GXZTexOp op, GXTexFmt fmt, u32 bias) {
    u32 zenv0;
    u32 zenv1;
    u32 type;

    CHECK_GXBEGIN(1077, "GXSetZTexture");

    zenv0 = 0;
    SET_REG_FIELD(1080, zenv0, 24, 0, bias);
    SET_REG_FIELD(1081, zenv0, 8, 24, 0xF4);

    zenv1 = 0;
    switch (fmt) {
    case GX_TF_Z8:
        type = 0;
        break;
    case GX_TF_Z16:
        type = 1;
        break;
    case GX_TF_Z24X8:
        type = 2;
        break;
    default:
        ASSERTMSGLINE(1089, 0, "GXSetZTexture: Invalid z-texture format");
        type = 2;
        break;
    }

    SET_REG_FIELD(1092, zenv1, 2, 0, type);
    SET_REG_FIELD(1093, zenv1, 2, 2, op);
    SET_REG_FIELD(1094, zenv1, 8, 24, 0xF5);

    GX_WRITE_RAS_REG(zenv0);
    GX_WRITE_RAS_REG(zenv1);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018A880
 * @note Size: 0x19C
 */
void GXSetTevOrder(GXTevStageID stage, GXTexCoordID coord, GXTexMapID map, GXChannelID color) {
    u32* ptref;
    u32 tmap;
    u32 tcoord;
    // Raster color selector for each GXChannelID through GX_ALPHA_BUMPN.
    static int colorToRaster[] = { 0, 1, 0, 1, 0, 1, 7, 5, 6 };

    CHECK_GXBEGIN(1131, "GXSetTevOrder");
    ASSERTMSGLINE(1132, stage < GX_MAX_TEVSTAGE, "GXSetTevOrder: Invalid Tev Stage Index");
    ASSERTMSGLINE(1134, coord < GX_MAX_TEXCOORD || coord == GX_TEXCOORD_NULL, "GXSetTevOrder: Invalid Texcoord");
    ASSERTMSGLINE(1136, (map & ~GX_TEX_DISABLE) < GX_MAX_TEXMAP || map == GX_TEXMAP_NULL, "GXSetTevOrder: Invalid Tex Map");
    ASSERTMSGLINE(1138, color >= GX_COLOR0A0 && color <= GX_COLOR_NULL, "GXSetTevOrder: Invalid Color Channel ID");

    ptref = &__GXData->tref[stage / 2];
    __GXData->texmapId[stage] = map;

    tmap = map & ~GX_TEX_DISABLE;
    tmap = (tmap >= GX_MAX_TEXMAP) ? GX_TEXMAP0 : tmap;

    if (coord >= GX_MAX_TEXCOORD) {
        tcoord = GX_TEXCOORD0;
        __GXData->tevTcEnab = __GXData->tevTcEnab & ~(1 << stage);
    } else {
        tcoord = coord;
        __GXData->tevTcEnab = __GXData->tevTcEnab | (1 << stage);
    }

    if (stage & 1) {
        SET_REG_FIELD(1158, *ptref, 3, 12, tmap);
        SET_REG_FIELD(1159, *ptref, 3, 15, tcoord);
        SET_REG_FIELD(1161, *ptref, 3, 19, (color == GX_COLOR_NULL) ? 7 : colorToRaster[color]);
        SET_REG_FIELD(1163, *ptref, 1, 18, (map != GX_TEXMAP_NULL && !(map & GX_TEX_DISABLE)));
    } else {
        SET_REG_FIELD(1166, *ptref, 3, 0, tmap);
        SET_REG_FIELD(1167, *ptref, 3, 3, tcoord);
        SET_REG_FIELD(1169, *ptref, 3, 7, (color == GX_COLOR_NULL) ? 7 : colorToRaster[color]);
        SET_REG_FIELD(1171, *ptref, 1, 6, (map != GX_TEXMAP_NULL && !(map & GX_TEX_DISABLE)));
    }

    GX_WRITE_RAS_REG(*ptref);
    __GXData->bpSentNot = 0;
    __GXData->dirtyState |= 1;
}

/**
 * @note Address: 0x8018AA1C
 * @note Size: 0x28
 */
void GXSetNumTevStages(u8 nStages) {
    CHECK_GXBEGIN(1187, "GXSetNumTevStages");

    ASSERTMSGLINE(1189, nStages != 0 && nStages <= 16, "GXSetNumTevStages: Exceed max number of tex stages");
    SET_REG_FIELD(1190, __GXData->genMode, 4, 10, nStages - 1);
    __GXData->dirtyState |= 4;
}
