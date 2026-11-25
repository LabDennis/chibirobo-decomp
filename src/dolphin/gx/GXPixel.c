#include <sdk_math.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>

#include "__gx.h"

enum {
    GX_BP_FOG_RANGE_ADJ = 0xE8,
    GX_BP_FOG_RANGE_TABLE = 0xE9,
    GX_BP_FOG_PARAM_A = 0xEE,
    GX_BP_FOG_PARAM_B_MAGNITUDE = 0xEF,
    GX_BP_FOG_PARAM_B_SHIFT = 0xF0,
    GX_BP_FOG_PARAM_C = 0xF1,
    GX_BP_FOG_COLOR = 0xF2,
};

/**
 * @note Address: 0x8018AA44
 * @note Size: 0x214
 */
void GXSetFog(GXFogType type, f32 startz, f32 endz, f32 nearz, f32 farz, GXColor color) {
    u32 fogColor;
    u32 fogParamA;
    u32 fogParamBMagnitude;
    u32 fogParamBShift;
    u32 fogParamC;
    f32 unscaledA;
    f32 bMantissa;
    f32 unscaledC;
    union {
        f32 value;
        u32 bits;
    } a, c;
    s32 bExponent;
    u32 bMagnitude;
    u32 bShift;
    u32 aBits;
    u32 cBits;
    u32 fogFunction;
    u32 projection;

    fogColor = 0;
    fogParamA = 0;
    fogParamBMagnitude = 0;
    fogParamBShift = 0;
    fogParamC = 0;

    CHECK_GXBEGIN(138, "GXSetFog");

    ASSERTMSGLINE(140, farz >= 0.0f, "GXSetFog: The farz should be positive value");
    ASSERTMSGLINE(141, farz >= nearz, "GXSetFog: The farz should be larger than nearz");

    fogFunction = type & 7;
    projection = (type >> 3) & 1;

    if (projection) {
        if (farz == nearz || endz == startz) {
            a.value = 0.0f;
            c.value = 0.0f;
        } else {
            unscaledA = 1.0f / (endz - startz);
            a.value = unscaledA * (farz - nearz);
            c.value = unscaledA * (startz - nearz);
        }
    } else {
        if (farz == nearz || endz == startz) {
            unscaledA = 0.0f;
            bMantissa = 0.5f;
            unscaledC = 0.0f;
        } else {
            unscaledA = (farz * nearz) / ((farz - nearz) * (endz - startz));
            bMantissa = farz / (farz - nearz);
            unscaledC = startz / (endz - startz);
        }

        // Normalize B for the separate magnitude and shift registers.
        bExponent = 0;
        while (bMantissa > 1.0) {
            bMantissa /= 2.0f;
            bExponent++;
        }
        while (bMantissa > 0.0f && bMantissa < 0.5) {
            bMantissa *= 2.0f;
            bExponent--;
        }

        a.value = unscaledA / (f32) (1 << (bExponent + 1));
        bMagnitude = 8388638.0f * bMantissa;
        bShift = bExponent + 1;
        c.value = unscaledC;

        SET_REG_FIELD(198, fogParamBMagnitude, 24, 0, bMagnitude);
        SET_REG_FIELD(198, fogParamBMagnitude, 8, 24, GX_BP_FOG_PARAM_B_MAGNITUDE);

        SET_REG_FIELD(201, fogParamBShift, 5, 0, bShift);
        SET_REG_FIELD(201, fogParamBShift, 8, 24, GX_BP_FOG_PARAM_B_SHIFT);
    }

    // The BP registers use the sign, exponent, and high 11 mantissa bits of A and C.
    aBits = a.bits;
    cBits = c.bits;

    SET_REG_FIELD(209, fogParamA, 11, 0, (aBits >> 12) & 0x7FF);
    SET_REG_FIELD(210, fogParamA, 8, 11, (aBits >> 23) & 0xFF);
    SET_REG_FIELD(211, fogParamA, 1, 19, (aBits >> 31));
    SET_REG_FIELD(211, fogParamA, 8, 24, GX_BP_FOG_PARAM_A);

    SET_REG_FIELD(214, fogParamC, 11, 0, (cBits >> 12) & 0x7FF);
    SET_REG_FIELD(215, fogParamC, 8, 11, (cBits >> 23) & 0xFF);
    SET_REG_FIELD(216, fogParamC, 1, 19, (cBits >> 31));

    SET_REG_FIELD(217, fogParamC, 1, 20, projection);
    SET_REG_FIELD(218, fogParamC, 3, 21, fogFunction);
    SET_REG_FIELD(218, fogParamC, 8, 24, GX_BP_FOG_PARAM_C);

    SET_REG_FIELD(222, fogColor, 24, 0, __GXPackRGBA8(color) >> 8);
    SET_REG_FIELD(222, fogColor, 8, 24, GX_BP_FOG_COLOR);

    GX_WRITE_RAS_REG(fogParamA);
    GX_WRITE_RAS_REG(fogParamBMagnitude);
    GX_WRITE_RAS_REG(fogParamBShift);
    GX_WRITE_RAS_REG(fogParamC);
    GX_WRITE_RAS_REG(fogColor);

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXSetFogColor(GXColor color) {
    u32 fogColor = (u32)GX_BP_FOG_COLOR << 24;

    SET_REG_FIELD(250, fogColor, 24, 0, __GXPackRGBA8(color) >> 8);
    GX_WRITE_RAS_REG(fogColor);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018AC58
 * @note Size: 0x1B0
 */
void GXInitFogAdjTable(GXFogAdjTable *table, u16 width, const f32 projmtx[4][4]) {
    f32 xi;
    f32 iw;
    f32 rangeVal;
    f32 nearZ;
    f32 sideX;
    u32 i;

    CHECK_GXBEGIN(275, "GXInitFogAdjTable");
    ASSERTMSGLINE(276, table != NULL, "GXInitFogAdjTable: table pointer is null");
    ASSERTMSGLINE(277, width <= 640, "GXInitFogAdjTable: invalid width value");

    if (0.0 == projmtx[3][3]) {
        nearZ = projmtx[2][3] / (projmtx[2][2] - 1.0f);
        sideX = nearZ / projmtx[0][0];
    } else {
        sideX = 1.0f / projmtx[0][0];
        nearZ = 1.73205f * sideX;
    }

    iw = 2.0f / width;
    for (i = 0; i < ARRAY_SIZEU(table->r); i++) {
        xi = (i + 1) << 5;
        xi *= iw;
        xi *= sideX;
        rangeVal = SDKFastSqrt(1.0f + ((xi * xi) / (nearZ * nearZ)));
        table->r[i] = (u32)(256.0f * rangeVal) & 0xFFF;
    }
}

/**
 * @note Address: 0x8018AE08
 * @note Size: 0x124
 */
void GXSetFogRangeAdj(GXBool enable, u16 center, const GXFogAdjTable *table) {
    u32 i;
    u32 rangeCoefficients;
    u32 rangeControl;

    CHECK_GXBEGIN(331, "GXSetFogRangeAdj");

    if (enable) {
        ASSERTMSGLINE(334, table != NULL, "GXSetFogRangeAdj: table pointer is null");
        for (i = 0; i < ARRAY_SIZEU(table->r); i += 2) {
            rangeCoefficients = 0;
            SET_REG_FIELD(338, rangeCoefficients, 12, 0, table->r[i]);
            SET_REG_FIELD(339, rangeCoefficients, 12, 12, table->r[i + 1]);
            SET_REG_FIELD(340, rangeCoefficients, 8, 24, (i >> 1) + GX_BP_FOG_RANGE_TABLE);
            GX_WRITE_RAS_REG(rangeCoefficients);
        }
    }
    rangeControl = 0;
    SET_REG_FIELD(346, rangeControl, 10, 0, center + 342);
    SET_REG_FIELD(347, rangeControl, 1, 10, enable);
    SET_REG_FIELD(348, rangeControl, 8, 24, GX_BP_FOG_RANGE_ADJ);
    GX_WRITE_RAS_REG(rangeControl);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018AF2C
 * @note Size: 0x54
 */
void GXSetBlendMode(GXBlendMode type, GXBlendFactor src_factor, GXBlendFactor dst_factor, GXLogicOp op) {
    u32 reg;
    u32 blend_en;

    CHECK_GXBEGIN(375, "GXSetBlendMode");

    reg = __GXData->cmode0;

#if DEBUG
    blend_en = type == GX_BM_BLEND || type == GX_BM_SUBTRACT;
#endif

    SET_REG_FIELD(389, reg, 1, 11, (type == GX_BM_SUBTRACT));
#if DEBUG
    SET_REG_FIELD(392, reg, 1, 0, blend_en);
#else
    SET_REG_FIELD(392, reg, 1, 0, type);
#endif
    SET_REG_FIELD(393, reg, 1, 1, (type == GX_BM_LOGIC));
    SET_REG_FIELD(394, reg, 4, 12, op);
    SET_REG_FIELD(395, reg, 3, 8, src_factor);
    SET_REG_FIELD(396, reg, 3, 5, dst_factor);
    GX_WRITE_RAS_REG(reg);

    __GXData->cmode0 = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018AF80
 * @note Size: 0x2C
 */
void GXSetColorUpdate(GXBool update_enable) {
    u32 reg;
    CHECK_GXBEGIN(419, "GXSetColorUpdate");

    reg = __GXData->cmode0;

    SET_REG_FIELD(421, reg, 1, 3, update_enable);
    GX_WRITE_RAS_REG(reg);

    __GXData->cmode0 = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018AFAC
 * @note Size: 0x2C
 */
void GXSetAlphaUpdate(GXBool update_enable) {
    u32 reg;
    CHECK_GXBEGIN(432, "GXSetAlphaUpdate");

    reg = __GXData->cmode0;

    SET_REG_FIELD(434, reg, 1, 4, update_enable);
    GX_WRITE_RAS_REG(reg);

    __GXData->cmode0 = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018AFD8
 * @note Size: 0x34
 */
void GXSetZMode(GXBool compare_enable, GXCompare func, GXBool update_enable) {
    u32 reg;
    CHECK_GXBEGIN(459, "GXSetZMode");

    reg = __GXData->zmode;

    SET_REG_FIELD(462, reg, 1, 0, compare_enable);
    SET_REG_FIELD(463, reg, 3, 1, func);
    SET_REG_FIELD(464, reg, 1, 4, update_enable);
    GX_WRITE_RAS_REG(reg);

    __GXData->zmode = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B00C
 * @note Size: 0x34
 */
void GXSetZCompLoc(GXBool before_tex) {
    CHECK_GXBEGIN(474, "GXSetZCompLoc");
    SET_REG_FIELD(475, __GXData->peCtrl, 1, 6, before_tex);
    GX_WRITE_RAS_REG(__GXData->peCtrl);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B040
 * @note Size: 0xD4
 */
void GXSetPixelFmt(GXPixelFmt pix_fmt, GXZFmt16 z_fmt) {
    u32 oldPeCtrl;
    u8 aa;
    static u32 p2f[8] = { 0, 1, 2, 3, 4, 4, 4, 5 };

    CHECK_GXBEGIN(511, "GXSetPixelFmt");
    oldPeCtrl = __GXData->peCtrl;
    ASSERTMSGLINE(515, pix_fmt >= GX_PF_RGB8_Z24 && pix_fmt <= GX_PF_YUV420, "Invalid Pixel format");
    SET_REG_FIELD(517, __GXData->peCtrl, 3, 0, p2f[pix_fmt]);
    SET_REG_FIELD(518, __GXData->peCtrl, 3, 3, z_fmt);

    if (oldPeCtrl != __GXData->peCtrl) {
        GX_WRITE_RAS_REG(__GXData->peCtrl);
        if (pix_fmt == GX_PF_RGB565_Z16)
            aa = 1;
        else
            aa = 0;
        SET_REG_FIELD(527, __GXData->genMode, 1, 9, aa);
        __GXData->dirtyState |= 4;
    }

    if (p2f[pix_fmt] == 4) {
        SET_REG_FIELD(534, __GXData->cmode1, 2, 9, (pix_fmt - 4) & 0x3);
        SET_REG_FIELD(534, __GXData->cmode1, 8, 24, 0x42);
        GX_WRITE_RAS_REG(__GXData->cmode1);
    }

    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B114
 * @note Size: 0x2C
 */
void GXSetDither(GXBool dither) {
    u32 reg;
    CHECK_GXBEGIN(556, "GXSetDither");

    reg = __GXData->cmode0;

    SET_REG_FIELD(559, reg, 1, 2, dither);
    GX_WRITE_RAS_REG(reg);

    __GXData->cmode0 = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B140
 * @note Size: 0x3C
 */
void GXSetDstAlpha(GXBool enable, u8 alpha) {
    u32 reg;
    CHECK_GXBEGIN(581, "GXSetDstAlpha");

    reg = __GXData->cmode1;

    SET_REG_FIELD(584, reg, 8, 0, alpha);
    SET_REG_FIELD(585, reg, 1, 8, enable);
    GX_WRITE_RAS_REG(reg);

    __GXData->cmode1 = reg;
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B17C
 * @note Size: 0x38
 */
void GXSetFieldMask(GXBool odd_mask, GXBool even_mask) {
    u32 reg;

    CHECK_GXBEGIN(608, "GXSetFieldMask");
    reg = 0;
    SET_REG_FIELD(610, reg, 1, 0, even_mask);
    SET_REG_FIELD(611, reg, 1, 1, odd_mask);
    SET_REG_FIELD(611, reg, 8, 24, 0x44);
    GX_WRITE_RAS_REG(reg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018B1B4
 * @note Size: 0x78
 */
void GXSetFieldMode(GXBool field_mode, GXBool half_aspect_ratio) {
    u32 reg;

    CHECK_GXBEGIN(637, "GXSetFieldMode");
    SET_REG_FIELD(641, __GXData->lpSize, 1, 22, half_aspect_ratio);
    GX_WRITE_RAS_REG(__GXData->lpSize);
    __GXFlushTextureState();
    reg = field_mode | 0x68000000;
    GX_WRITE_RAS_REG(reg);
    __GXFlushTextureState();
}
