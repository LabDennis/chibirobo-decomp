#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <dolphin/os.h>

#include "__gx.h"

static inline f32 PairedFloat(f32 value) {
#if !DEBUG
    union {
        f32 value;
        u32 bits;
    } word;

    // Unquantized paired-single stores flush subnormals to signed zero.
    word.value = value;
    if ((word.bits & 0x7f800000u) == 0) {
        word.bits &= 0x80000000u;
    }
    value = word.value;
#endif
    return value;
}

/**
 * @note Address: 0x8018C14C
 * @note Size: 0x174
 */
void GXProject(f32 x, f32 y, f32 z, const Mtx mtx, const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz) {
    Vec peye;
    f32 xc;
    f32 yc;
    f32 zc;
    f32 wc;

    ASSERTMSGLINE(168, pm && vp && sx && sy && sz, "GXGet*: invalid null pointer");

    peye.x = mtx[0][3] + ((mtx[0][2] * z) + ((mtx[0][0] * x) + (mtx[0][1] * y)));
    peye.y = mtx[1][3] + ((mtx[1][2] * z) + ((mtx[1][0] * x) + (mtx[1][1] * y)));
    peye.z = mtx[2][3] + ((mtx[2][2] * z) + ((mtx[2][0] * x) + (mtx[2][1] * y)));
    if (pm[0] == 0.0f) {
        xc = (peye.x * pm[1]) + (peye.z * pm[2]);
        yc = (peye.y * pm[3]) + (peye.z * pm[4]);
        zc = pm[6] + (peye.z * pm[5]);
        wc = 1.0f / -peye.z;
    } else {
        xc = pm[2] + (peye.x * pm[1]);
        yc = pm[4] + (peye.y * pm[3]);
        zc = pm[6] + (peye.z * pm[5]);
        wc = 1.0f;
    }
    *sx = (vp[2] / 2.0f) + (vp[0] + (wc * (xc * vp[2] / 2.0f)));
    *sy = (vp[3] / 2.0f) + (vp[1] + (wc * (-yc * vp[3] / 2.0f)));
    *sz = vp[5] + (wc * (zc * (vp[5] - vp[4])));
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void __GXSetProjection(void) {
    u32 i;

    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(0x00061020);
    for (i = 0; i < ARRAY_SIZEU(__GXData->projMtx); i++) {
        GX_WRITE_XF_REG_F(32 + i, PairedFloat(__GXData->projMtx[i]));
    }
    GX_WRITE_XF_REG_2(38, __GXData->projType);
}

/**
 * @note Address: 0x8018C2C0
 * @note Size: 0xA4
 */
void GXSetProjection(const Mtx44 mtx, GXProjectionType type) {
    CHECK_GXBEGIN(295, "GXSetProjection");

    __GXData->projType = type;
    __GXData->projMtx[0] = mtx[0][0];
    __GXData->projMtx[2] = mtx[1][1];
    __GXData->projMtx[4] = mtx[2][2];
    __GXData->projMtx[5] = mtx[2][3];
    if (type == GX_ORTHOGRAPHIC) {
        __GXData->projMtx[1] = mtx[0][3];
        __GXData->projMtx[3] = mtx[1][3];
    } else {
        __GXData->projMtx[1] = mtx[0][2];
        __GXData->projMtx[3] = mtx[1][2];
    }

    __GXSetProjection();
    __GXData->bpSentNot = 1;
}

/**
 * @note Address: 0x8018C364
 * @note Size: 0x8C
 */
void GXSetProjectionv(const f32* ptr) {
    u32 i;

    CHECK_GXBEGIN(339, "GXSetProjectionv");

    __GXData->projType = ptr[0] == 0.0f ? GX_PERSPECTIVE : GX_ORTHOGRAPHIC;
    for (i = 0; i < ARRAY_SIZEU(__GXData->projMtx); i++) {
        __GXData->projMtx[i] = PairedFloat(ptr[i + 1]);
    }

    __GXSetProjection();
    __GXData->bpSentNot = 1;
}

/**
 * @note Address: 0x8018C3F0
 * @note Size: 0x48
 */
void GXGetProjectionv(f32* ptr) {
    u32 i;

    ASSERTMSGLINE(370, ptr, "GXGet*: invalid null pointer");

    ptr[0] = __GXData->projType != GX_PERSPECTIVE ? 1.0f : 0.0f;
    for (i = 0; i < ARRAY_SIZEU(__GXData->projMtx); i++) {
        ptr[i + 1] = PairedFloat(__GXData->projMtx[i]);
    }
}

static inline void WriteMatrixWord(u32 address, f32 value) {
    GX_WRITE_F32(value);
#if DEBUG
    {
        union {
            f32 value;
            u32 bits;
        } word;
        word.value = value;
        VERIF_MTXLIGHT(address, word.bits);
    }
#endif
}

/* Emit the values of a paired store in XF word order. */
static inline void WritePairedMatrixWords(u32 address, const f32* values, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        WriteMatrixWord(address + i, PairedFloat(values[i]));
    }
}

/**
 * @note Address: 0x8018C438
 * @note Size: 0x50
 */
void GXLoadPosMtxImm(const Mtx mtx, u32 id) {
    u32 address;
    u32 row;

    CHECK_GXBEGIN(507, "GXLoadPosMtxImm");

    address = id * 4;
    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(address | 0xB0000);
    for (row = 0; row < 3; row++) {
        WritePairedMatrixWords(address + row * 4, mtx[row], 4);
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXLoadPosMtxIndx(u16 mtx_indx, u32 id) {
    u32 offset;
    u32 reg;

    CHECK_GXBEGIN(555, "GXLoadPosMtxIndx");
    offset = id * 4;
    reg = 0;
    SET_REG_FIELD(561, reg, 12, 0, offset);
    SET_REG_FIELD(563, reg, 4, 12, 11);
    SET_REG_FIELD(563, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x20);
    GX_WRITE_U32(reg);
#if DEBUG
    __GXShadowIndexState(4, reg);
#endif
}

/**
 * @note Address: 0x8018C488
 * @note Size: 0x50
 */
void GXLoadNrmMtxImm(const Mtx mtx, u32 id) {
    u32 address;
    u32 row;

    CHECK_GXBEGIN(588, "GXLoadNrmMtxImm");

    address = id * 3 + 0x400;
    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(address | 0x80000);
    for (row = 0; row < 3; row++) {
        // Normal matrices skip the fourth column and store the third column as a scalar.
        WritePairedMatrixWords(address + row * 3, mtx[row], 2);
        WriteMatrixWord(address + row * 3 + 2, mtx[row][2]);
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXLoadNrmMtxImm3x3(const f32 mtx[3][3], u32 id) {
    u32 address;
    u32 row;

    CHECK_GXBEGIN(633, "GXLoadNrmMtxImm");

    address = id * 3 + 0x400;
    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(address | 0x80000);
    for (row = 0; row < 3; row++) {
        // The packed matrix pairs the first eight elements across rows.
        WritePairedMatrixWords(address + row * 3, mtx[row], row == 2 ? 2 : 3);
    }
    WriteMatrixWord(address + 8, mtx[2][2]);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXLoadNrmMtxIndx3x3(u16 mtx_indx, u32 id) {
    u32 offset;
    u32 reg;

    CHECK_GXBEGIN(679, "GXLoadNrmMtxIndx3x3");
    offset = id * 3 + 0x400;
    reg = 0;
    SET_REG_FIELD(685, reg, 12, 0, offset);
    SET_REG_FIELD(687, reg, 4, 12, 8);
    SET_REG_FIELD(687, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x28);
    GX_WRITE_U32(reg);
#if DEBUG
    __GXShadowIndexState(5, reg);
#endif
}

/**
 * @note Address: 0x8018C4D8
 * @note Size: 0x34
 */
void GXSetCurrentMtx(u32 id) {
    CHECK_GXBEGIN(708, "GXSetCurrentMtx");
    SET_REG_FIELD(712, __GXData->matIdxA, 6, 0, id);
    __GXSetMatrixIndex(GX_VA_PNMTXIDX);
}

/**
 * @note Address: 0x8018C50C
 * @note Size: 0xB4
 */
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, GXTexMtxType type) {
    u32 address;
    u32 count;
    u32 rows;
    u32 row;

    CHECK_GXBEGIN(741, "GXLoadTexMtxImm");

    if (id >= GX_PTTEXMTX0) {
        address = (id - GX_PTTEXMTX0) * 4 + 0x500;
        ASSERTMSGLINE(751, type == GX_MTX3x4, "GXLoadTexMtx: Invalid matrix type");
    } else {
        address = id * 4;
    }
    count = (type == GX_MTX2x4) ? 8 : 12;
    rows = (type == GX_MTX3x4) ? 3 : 2;

    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(address | ((count - 1) << 16));
    for (row = 0; row < rows; row++) {
        WritePairedMatrixWords(address + row * 4, mtx[row], 4);
    }
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXLoadTexMtxIndx(u16 mtx_indx, u32 id, GXTexMtxType type) {
    u32 offset;
    u32 reg;
    u32 count;

    CHECK_GXBEGIN(813, "GXLoadTexMtxIndx");

    if (id >= GX_PTTEXMTX0) {
        offset = (id - GX_PTTEXMTX0) * 4 + 0x500;
        ASSERTMSGLINE(0x337, type == GX_MTX3x4, "GXLoadTexMtx: Invalid matrix type");
    } else {
        offset = id * 4;
    }
    count = (type == GX_MTX2x4) ? 8 : 12;

    reg = 0;
    SET_REG_FIELD(830, reg, 12, 0, offset);
    SET_REG_FIELD(831, reg, 4, 12, (count - 1));
    SET_REG_FIELD(832, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x30);
    GX_WRITE_U32(reg);
#if DEBUG
    __GXShadowIndexState(6, reg);
#endif
}

/**
 * @note Address: 0x8018C5C0
 * @note Size: 0x90
 */
void __GXSetViewport(void) {
    f32 sx;
    f32 sy;
    f32 sz;
    f32 ox;
    f32 oy;
    f32 oz;
    f32 zmin;
    f32 zmax;
    u32 reg;

    sx = __GXData->vpWd / 2.0f;
    sy = -__GXData->vpHt / 2.0f;
    ox = 342.0f + (__GXData->vpLeft + (__GXData->vpWd / 2.0f));
    oy = 342.0f + (__GXData->vpTop + (__GXData->vpHt / 2.0f));

    zmin = __GXData->vpNearz * __GXData->zScale;
    zmax = __GXData->vpFarz * __GXData->zScale;

    sz = zmax - zmin;
    oz = zmax + __GXData->zOffset;

    reg = 0x5101A;
    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    GX_WRITE_XF_REG_F(26, sx);
    GX_WRITE_XF_REG_F(27, sy);
    GX_WRITE_XF_REG_F(28, sz);
    GX_WRITE_XF_REG_F(29, ox);
    GX_WRITE_XF_REG_F(30, oy);
    GX_WRITE_XF_REG_F(31, oz);
}

/**
 * @note Address: 0x8018C650
 * @note Size: 0x58
 */
void GXSetViewportJitter(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz, u32 field) {
    CHECK_GXBEGIN(903, "GXSetViewport");  // not the correct function name

    if (field == 0) {
        top -= 0.5f;
    }

    __GXData->vpLeft = left;
    __GXData->vpTop = top;
    __GXData->vpWd = wd;
    __GXData->vpHt = ht;
    __GXData->vpNearz = nearz;
    __GXData->vpFarz = farz;
    
    __GXSetViewport();
    __GXData->bpSentNot = 1;
}

/**
 * @note Address: 0x8018C6A8
 * @note Size: 0x48
 */
void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz) {
    GXSetViewportJitter(left, top, wd, ht, nearz, farz, 1);
}

/**
 * @note Address: 0x8018C6F0
 * @note Size: 0x24
 */
void GXGetViewportv(f32* vp) {
    ASSERTMSGLINE(968, vp, "GXGet*: invalid null pointer");

    vp[0] = PairedFloat(__GXData->vpLeft);
    vp[1] = PairedFloat(__GXData->vpTop);
    vp[2] = PairedFloat(__GXData->vpWd);
    vp[3] = PairedFloat(__GXData->vpHt);
    vp[4] = PairedFloat(__GXData->vpNearz);
    vp[5] = PairedFloat(__GXData->vpFarz);
}

#define GX_WRITE_XF_REG_F_(addr, value) \
do { \
    GX_WRITE_U8(0x10); \
    GX_WRITE_U32(0x1000 + (addr)); \
    { \
        f32 xfData = (value); \
        GX_WRITE_F32(value); \
        VERIF_XF_REG_alt(addr, *(u32 *)&xfData); \
    } \
} while (0)

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXSetZScaleOffset(f32 scale, f32 offset) {
    f32 sz;
    f32 oz;
    f32 zmin;
    f32 zmax;

    CHECK_GXBEGIN(996, "GXSetZScaleOffset");

    oz = offset * 1.6777215e7f;
    __GXData->zOffset = oz;

    sz = (scale * 1.6777215e7f) + 1.0f;
    __GXData->zScale = sz;

    zmin = __GXData->vpNearz * sz;
    zmax = __GXData->vpFarz * sz;
    sz = zmax - zmin;
    oz = oz + zmax;

    GX_WRITE_XF_REG_F_(0x1C, sz);
    GX_WRITE_XF_REG_F_(0x1F, oz);

    __GXData->bpSentNot = 1;
}

/**
 * @note Address: 0x8018C714
 * @note Size: 0x78
 */
void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht) {
    u32 tp;
    u32 lf;
    u32 bm;
    u32 rt;

    CHECK_GXBEGIN(1048, "GXSetScissor");
    ASSERTMSGLINE(1049, left < 1706, "GXSetScissor: Left origin > 1708");
    ASSERTMSGLINE(1050, top < 1706, "GXSetScissor: top origin > 1708");
    ASSERTMSGLINE(1051, left + wd < 1706, "GXSetScissor: right edge > 1708");
    ASSERTMSGLINE(1052, top + ht < 1706, "GXSetScissor: bottom edge > 1708");

    tp = top + 342;
    lf = left + 342;
    bm = tp + ht - 1;
    rt = lf + wd - 1;

    SET_REG_FIELD(1059, __GXData->suScis0, 11, 0, tp);
    SET_REG_FIELD(1060, __GXData->suScis0, 11, 12, lf);
    SET_REG_FIELD(1062, __GXData->suScis1, 11, 0, bm);
    SET_REG_FIELD(1063, __GXData->suScis1, 11, 12, rt);

    GX_WRITE_RAS_REG(__GXData->suScis0);
    GX_WRITE_RAS_REG(__GXData->suScis1);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetScissor(u32* left, u32* top, u32* wd, u32* ht) {
    u32 tp;
    u32 lf;
    u32 bm;
    u32 rt;

    ASSERTMSGLINE(1089, left && top && wd && ht, "GXGet*: invalid null pointer");

    tp = __GXData->suScis0 & 0x7FF;
    lf = (__GXData->suScis0 & 0x7FF000) >> 12;
    bm = __GXData->suScis1 & 0x7FF;
    rt = (__GXData->suScis1 & 0x7FF000) >> 12;

    *left = lf - 342;
    *top = tp - 342;
    *wd = rt - lf + 1;
    *ht = bm - tp + 1;
}

/**
 * @note Address: 0x8018C78C
 * @note Size: 0x40
 */
void GXSetScissorBoxOffset(s32 x_off, s32 y_off) {
    u32 reg = 0;
    u32 hx;
    u32 hy;

    CHECK_GXBEGIN(1119, "GXSetScissorBoxOffset");

    ASSERTMSGLINE(1122, (u32)(x_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid X offset");
    ASSERTMSGLINE(1124, (u32)(y_off + 342) < 2048, "GXSetScissorBoxOffset: Invalid Y offset");

    hx = (u32)(x_off + 342) >> 1;
    hy = (u32)(y_off + 342) >> 1;

    SET_REG_FIELD(1129, reg, 10, 0, hx);
    SET_REG_FIELD(1130, reg, 10, 10, hy);
    SET_REG_FIELD(1131, reg, 8, 24, 0x59);
    GX_WRITE_RAS_REG(reg);
    __GXData->bpSentNot = 0;
}

/**
 * @note Address: 0x8018C7CC
 * @note Size: 0x28
 */
void GXSetClipMode(GXClipMode mode) {
    CHECK_GXBEGIN(1151, "GXSetClipMode");
    GX_WRITE_XF_REG(5, mode);
    __GXData->bpSentNot = 1;
}

/**
 * @note Address: 0x8018C7F4
 * @note Size: 0x84
 */
void __GXSetMatrixIndex(GXAttr matIdxAttr) {
    if (matIdxAttr < GX_VA_TEX4MTXIDX) {
        GX_WRITE_CP_REG(GX_CP_REG_MTXIDXA, __GXData->matIdxA);
        GX_WRITE_XF_REG(24, __GXData->matIdxA);
    } else {
        GX_WRITE_CP_REG(GX_CP_REG_MTXIDXB, __GXData->matIdxB);
        GX_WRITE_XF_REG(25, __GXData->matIdxB);
    }
    __GXData->bpSentNot = 1;
}
