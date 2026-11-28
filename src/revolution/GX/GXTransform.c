#include "revolution/gx.h"
#include <dolphin/mtx.h>
#include <dolphin/os.h>

#include "revolution/gx/__gx.h"

void GXProject(f32 x, f32 y, f32 z, const Mtx mtx, const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz) {
    Vec peye;
    f32 xc;
    f32 yc;
    f32 zc;
    f32 wc;

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

// The paired-single immediate-load entry points are assembly in the Revolution SDK source.
// ogws: https://github.com/doldecomp/ogws/blob/master/src/revolution/GX/GXTransform.c
// pik1wii: https://github.com/projectPiki/pik1wii/blob/main/src/gx/GXTransform.c

void GXSetProjection(const Mtx44 mtx, GXProjectionType type) {
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

    __GXData->dirtyState |= 0x08000000;
}

#define GX_WRITE_MTX_ELEM(addr, value) \
do { \
    f32 xfData = (value); \
    GX_WRITE_F32(value); \
    VERIF_MTXLIGHT((addr), *(u32 *)&xfData); \
} while (0)

void GXLoadPosMtxIndx(u16 mtx_indx, u32 id) {
    u32 offset;
    u32 reg;

    offset = id * 4;
    reg = 0;
    SET_REG_FIELD(0, reg, 12, 0, offset);
    SET_REG_FIELD(0, reg, 4, 12, 11);
    SET_REG_FIELD(0, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x20);
    GX_WRITE_U32(reg);
}

void GXLoadNrmMtxImm3x3(const f32 mtx[3][3], u32 id) {
    u32 reg;
    u32 addr;

    addr = id * 3 + 0x400;
    reg = addr | 0x80000;

    GX_WRITE_U8(0x10);
    GX_WRITE_U32(reg);
    GX_WRITE_F32(mtx[0][0]);
    GX_WRITE_F32(mtx[0][1]);
    GX_WRITE_F32(mtx[0][2]);
    GX_WRITE_F32(mtx[1][0]);
    GX_WRITE_F32(mtx[1][1]);
    GX_WRITE_F32(mtx[1][2]);
    GX_WRITE_F32(mtx[2][0]);
    GX_WRITE_F32(mtx[2][1]);
    GX_WRITE_F32(mtx[2][2]);
}

void GXLoadNrmMtxIndx3x3(u16 mtx_indx, u32 id) {
    u32 offset;
    u32 reg;

    offset = id * 3 + 0x400;
    reg = 0;
    SET_REG_FIELD(0, reg, 12, 0, offset);
    SET_REG_FIELD(0, reg, 4, 12, 8);
    SET_REG_FIELD(0, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x28);
    GX_WRITE_U32(reg);
}

void GXSetCurrentMtx(u32 id) {
    SET_REG_FIELD(0, __GXData->matIdxA, 6, 0, id);
    __GXData->dirtyState |= 0x04000000;
}

void GXLoadTexMtxIndx(u16 mtx_indx, u32 id, GXTexMtxType type) {
    u32 offset;
    u32 reg;
    u32 count;

    if (id >= GX_PTTEXMTX0) {
        offset = (id - GX_PTTEXMTX0) * 4 + 0x500;
    } else {
        offset = id * 4;
    }
    count = (type == GX_MTX2x4) ? 8 : 12;

    reg = 0;
    SET_REG_FIELD(0, reg, 12, 0, offset);
    SET_REG_FIELD(0, reg, 4, 12, (count - 1));
    SET_REG_FIELD(0, reg, 16, 16, mtx_indx);
    GX_WRITE_U8(0x30);
    GX_WRITE_U32(reg);
}

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

void GXSetViewportJitter(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz, u32 field) {
    if (field == 0) {
        top -= 0.5f;
    }

    __GXData->vpLeft = left;
    __GXData->vpTop = top;
    __GXData->vpWd = wd;
    __GXData->vpHt = ht;
    __GXData->vpNearz = nearz;
    __GXData->vpFarz = farz;
    __GXData->dirtyState |= 0x10000000;
}

void GXSetViewport(f32 left, f32 top, f32 wd, f32 ht, f32 nearz, f32 farz) {
    __GXData->vpLeft = left;
    __GXData->vpTop = top;
    __GXData->vpWd = wd;
    __GXData->vpHt = ht;
    __GXData->vpNearz = nearz;
    __GXData->vpFarz = farz;
    __GXData->dirtyState |= 0x10000000;
}

void GXGetViewportv(f32* vp) {
    vp[0] = __GXData->vpLeft;
    vp[1] = __GXData->vpTop;
    vp[2] = __GXData->vpWd;
    vp[3] = __GXData->vpHt;
    vp[4] = __GXData->vpNearz;
    vp[5] = __GXData->vpFarz;
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

void GXSetZScaleOffset(f32 scale, f32 offset) {
    f32 sz;
    f32 oz;
    f32 zmin;
    f32 zmax;

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

void GXSetScissor(u32 left, u32 top, u32 wd, u32 ht) {
    u32 tp;
    u32 lf;
    u32 bm;
    u32 rt;

    tp = top + 342;
    lf = left + 342;
    bm = tp + ht - 1;
    rt = lf + wd - 1;

    SET_REG_FIELD(0, __GXData->suScis0, 11, 0, tp);
    SET_REG_FIELD(0, __GXData->suScis0, 11, 12, lf);
    SET_REG_FIELD(0, __GXData->suScis1, 11, 0, bm);
    SET_REG_FIELD(0, __GXData->suScis1, 11, 12, rt);

    GX_WRITE_RAS_REG(__GXData->suScis0);
    GX_WRITE_RAS_REG(__GXData->suScis1);
    __GXData->bpSentNot = 0;
}

void GXGetScissor(u32* left, u32* top, u32* wd, u32* ht) {
    u32 tp;
    u32 lf;
    u32 bm;
    u32 rt;

    tp = __GXData->suScis0 & 0x7FF;
    lf = (__GXData->suScis0 & 0x7FF000) >> 12;
    bm = __GXData->suScis1 & 0x7FF;
    rt = (__GXData->suScis1 & 0x7FF000) >> 12;

    *left = lf - 342;
    *top = tp - 342;
    *wd = rt - lf + 1;
    *ht = bm - tp + 1;
}

void GXSetScissorBoxOffset(s32 x_off, s32 y_off) {
    u32 reg = 0;
    u32 hx;
    u32 hy;

    hx = (u32)(x_off + 342) >> 1;
    hy = (u32)(y_off + 342) >> 1;

    SET_REG_FIELD(0, reg, 10, 0, hx);
    SET_REG_FIELD(0, reg, 10, 10, hy);
    SET_REG_FIELD(0, reg, 8, 24, 0x59);
    GX_WRITE_RAS_REG(reg);
    __GXData->bpSentNot = 0;
}

void GXSetClipMode(GXClipMode mode) {
    GX_WRITE_XF_REG(5, mode);
    __GXData->bpSentNot = 1;
}

void __GXSetMatrixIndex(GXAttr matIdxAttr) {
    if (matIdxAttr < GX_VA_TEX4MTXIDX) {
        GX_WRITE_CP_REG(0x30, __GXData->matIdxA);
        GX_WRITE_XF_REG(24, __GXData->matIdxA);
    } else {
        GX_WRITE_CP_REG(0x40, __GXData->matIdxB);
        GX_WRITE_XF_REG(25, __GXData->matIdxB);
    }
    __GXData->bpSentNot = 1;
}
