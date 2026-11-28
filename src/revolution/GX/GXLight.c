#include <cmath>
#include "revolution/gx.h"
#include <dolphin/os.h>

#include "revolution/gx/__gx.h"

// The paired-single FIFO writes in GXLoadLightObjImm are assembly in the Revolution SDK source.
// ogws: https://github.com/doldecomp/ogws/blob/master/src/revolution/GX/GXLight.c
// pik1wii: https://github.com/projectPiki/pik1wii/blob/main/src/gx/GXLight.c

// GXLightObj private data
typedef struct {
    u32 reserved[3];
    u32 Color;
    f32 a[3];
    f32 k[3];
    f32 lpos[3];
    f32 ldir[3];
} __GXLightObjInt_struct;

void GXInitLightAttn(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

void GXInitLightAttnA(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
}

void GXGetLightAttnA(const GXLightObj* lt_obj, f32* a0, f32* a1, f32* a2) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;
    *a0 = obj->a[0];
    *a1 = obj->a[1];
    *a2 = obj->a[2];
}

void GXInitLightAttnK(GXLightObj* lt_obj, f32 k0, f32 k1, f32 k2) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

void GXGetLightAttnK(const GXLightObj* lt_obj, f32* k0, f32* k1, f32* k2) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;
    *k0 = obj->k[0];
    *k1 = obj->k[1];
    *k2 = obj->k[2];
}

void GXInitLightSpot(GXLightObj* lt_obj, f32 cutoff, GXSpotFn spot_func) {
    f32 a0, a1, a2;
    f32 r;
    f32 d;
    f32 cr;
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    if (cutoff <= 0.0f || cutoff > 90.0f)
        spot_func = GX_SP_OFF;

    r = (3.1415927f * cutoff) / 180.0f;
    cr = cosf(r);

    switch (spot_func) {
    case GX_SP_FLAT:
        a0 = -1000.0f * cr;
        a1 = 1000.0f;
        a2 = 0.0f;
        break;
    case GX_SP_COS:
        a1 = 1.0f / (1.0f - cr);
        a0 = -cr * a1;
        a2 = 0.0f;
        break;
    case GX_SP_COS2:
        a2 = 1.0f / (1.0f - cr);
        a0 = 0.0f;
        a1 = -cr * a2;
        break;
    case GX_SP_SHARP:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a0 = (cr * (cr - 2.0f)) * d;
        a1 = 2.0f * d;
        a2 = -d;
        break;
    case GX_SP_RING1:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a2 = -4.0f * d;
        a0 = a2 * cr;
        a1 = (4.0f * (1.0f + cr)) * d;
        break;
    case GX_SP_RING2:
        d = 1.0f / ((1.0f - cr) * (1.0f - cr));
        a0 = 1.0f - ((2.0f * cr * cr) * d);
        a1 = (4.0f * cr) * d;
        a2 = -2.0f * d;
        break;
    case GX_SP_OFF:
    default:
        a0 = 1.0f;
        a1 = 0.0f;
        a2 = 0.0f;
        break;
    }
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
}

void GXInitLightDistAttn(GXLightObj* lt_obj, f32 ref_dist, f32 ref_br, GXDistAttnFn dist_func) {
    f32 k0, k1, k2;
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    if (ref_dist < 0.0f)
        dist_func = GX_DA_OFF;
    if (ref_br <= 0.0f || ref_br >= 1.0f)
        dist_func = GX_DA_OFF;

    switch (dist_func) {
    case GX_DA_GENTLE:
        k0 = 1.0f;
        k1 = (1.0f - ref_br) / (ref_br * ref_dist);
        k2 = 0.0f;
        break;
    case GX_DA_MEDIUM:
        k0 = 1.0f;
        k1 = (0.5f * (1.0f - ref_br)) / (ref_br * ref_dist);
        k2 = (0.5f * (1.0f - ref_br)) / (ref_br * ref_dist * ref_dist);
        break;
    case GX_DA_STEEP:
        k0 = 1.0f;
        k1 = 0.0f;
        k2 = (1.0f - ref_br) / (ref_br * ref_dist * ref_dist);
        break;
    case GX_DA_OFF:
    default:
        k0 = 1.0f;
        k1 = 0.0f;
        k2 = 0.0f;
        break;
    }

    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

void GXInitLightPos(GXLightObj* lt_obj, f32 x, f32 y, f32 z) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    obj->lpos[0] = x;
    obj->lpos[1] = y;
    obj->lpos[2] = z;
}

void GXGetLightPos(const GXLightObj* lt_obj, f32* x, f32* y, f32* z) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    *x = obj->lpos[0];
    *y = obj->lpos[1];
    *z = obj->lpos[2];
}

void GXInitLightDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    obj->ldir[0] = -nx;
    obj->ldir[1] = -ny;
    obj->ldir[2] = -nz;
}

void GXGetLightDir(const GXLightObj* lt_obj, f32* nx, f32* ny, f32* nz) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    *nx = -obj->ldir[0];
    *ny = -obj->ldir[1];
    *nz = -obj->ldir[2];
}

void GXInitSpecularDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz) {
    f32 mag;
    f32 vx;
    f32 vy;
    f32 vz;
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    vx = -nx;
    vy = -ny;
    vz = -nz + 1.0f;

    mag = (vx * vx) + (vy * vy) + (vz * vz);
    if (mag != 0.0f) {
        mag = 1.0f / (f32)sqrt(mag);
    }
    
    obj->ldir[0] = vx * mag;
    obj->ldir[1] = vy * mag;
    obj->ldir[2] = vz * mag;
    obj->lpos[0] = nx * -1000000000000000000.0f;
    obj->lpos[1] = ny * -1000000000000000000.0f;
    obj->lpos[2] = nz * -1000000000000000000.0f;
}

void GXInitSpecularDirHA(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz, f32 hx, f32 hy, f32 hz) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    obj->ldir[0] = hx;
    obj->ldir[1] = hy;
    obj->ldir[2] = hz;
    obj->lpos[0] = nx * -1000000000000000000.0f;
    obj->lpos[1] = ny * -1000000000000000000.0f;
    obj->lpos[2] = nz * -1000000000000000000.0f;
}

void GXInitLightColor(GXLightObj* lt_obj, GXColor color) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    *(u32*)&obj->Color = *(u32*)&color;
}

void GXGetLightColor(const GXLightObj* lt_obj, GXColor* color) {
    __GXLightObjInt_struct* obj;

    obj = (__GXLightObjInt_struct*)lt_obj;

    *(u32*)color = *(u32*)&obj->Color;
}

void GXLoadLightObjIndx(u32 lt_obj_indx, GXLightID light) {
    u32 reg;
    u32 addr;
    u32 idx;

    idx = 31 - __cntlzw(light);

    idx &= 7;

    addr = idx * 0x10 + 0x600;
    reg = 0;
    SET_REG_FIELD(0, reg, 12, 0, addr);
    SET_REG_FIELD(0, reg, 4, 12, 0xF);
    SET_REG_FIELD(0, reg, 16, 16, lt_obj_indx);
    GX_WRITE_U8(0x38);
    GX_WRITE_U32(reg);
    __GXData->bpSentNot = 1;
}

#define GXCOLOR_AS_U32(color) (*((u32*)&(color)))

void GXSetChanAmbColor(GXChannelID chan, GXColor amb_color) {
    u32 reg;
    u32 rgb;
    u32 colIdx;

    switch (chan) {
    case GX_COLOR0:
        reg = __GXData->ambColor[GX_COLOR0];
        rgb = GXCOLOR_AS_U32(amb_color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        colIdx = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->ambColor[GX_COLOR1];
        rgb = GXCOLOR_AS_U32(amb_color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->ambColor[GX_COLOR0];
        SET_REG_FIELD(0, reg, 8, 0, amb_color.a);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->ambColor[GX_COLOR1];
        SET_REG_FIELD(0, reg, 8, 0, amb_color.a);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        reg = GXCOLOR_AS_U32(amb_color);
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        reg = GXCOLOR_AS_U32(amb_color);
        colIdx = 1;
        break;
    default:
        return;
    }

    __GXData->dirtyState |= 0x100 << colIdx;
    __GXData->ambColor[colIdx] = reg;
}

void GXSetChanMatColor(GXChannelID chan, GXColor mat_color) {
    u32 reg;
    u32 rgb;
    u32 colIdx;

    switch (chan) {
    case GX_COLOR0:
        reg = __GXData->matColor[GX_COLOR0];
        rgb = GXCOLOR_AS_U32(mat_color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        colIdx = 0;
        break;
    case GX_COLOR1:
        reg = __GXData->matColor[GX_COLOR1];
        rgb = GXCOLOR_AS_U32(mat_color) >> 8;
        SET_REG_FIELD(0, reg, 24, 8, rgb);
        colIdx = 1;
        break;
    case GX_ALPHA0:
        reg = __GXData->matColor[GX_COLOR0];
        SET_REG_FIELD(0, reg, 8, 0, mat_color.a);
        colIdx = 0;
        break;
    case GX_ALPHA1:
        reg = __GXData->matColor[GX_COLOR1];
        SET_REG_FIELD(0, reg, 8, 0, mat_color.a);
        colIdx = 1;
        break;
    case GX_COLOR0A0:
        reg = GXCOLOR_AS_U32(mat_color);
        colIdx = 0;
        break;
    case GX_COLOR1A1:
        reg = GXCOLOR_AS_U32(mat_color);
        colIdx = 1;
        break;
    default:
        return;
    }

    __GXData->dirtyState |= 0x400 << colIdx;
    __GXData->matColor[colIdx] = reg;
}

void GXSetNumChans(u8 nChans) {
    SET_REG_FIELD(0, __GXData->genMode, 3, 4, nChans);
    __GXData->dirtyState |= 0x01000004;
}

void GXSetChanCtrl(GXChannelID chan, GXBool enable, GXColorSrc amb_src, GXColorSrc mat_src, u32 light_mask, GXDiffuseFn diff_fn, GXAttnFn attn_fn) {
    u32 reg;
    u32 idx;

    idx = chan & 0x3;

    reg = 0;
    SET_REG_FIELD(0, reg, 1, 1, enable);
    SET_REG_FIELD(0, reg, 1, 0, mat_src);
    SET_REG_FIELD(0, reg, 1, 6, amb_src);
    
    if (attn_fn == GX_AF_SPEC) {
        diff_fn = GX_DF_NONE;
    }

    SET_REG_FIELD(0, reg, 2, 7, diff_fn);
    SET_REG_FIELD(0, reg, 1, 9, (attn_fn != 2));
    SET_REG_FIELD(0, reg, 1, 10, (attn_fn != 0));

    SET_REG_FIELD(0, reg, 4, 2, light_mask & 0xF);
    SET_REG_FIELD(0, reg, 4, 11, (light_mask >> 4) & 0xF);

    __GXData->chanCtrl[idx] = reg;
    __GXData->dirtyState |= 0x1000 << idx;

    if (chan == GX_COLOR0A0) {
        __GXData->chanCtrl[GX_ALPHA0] = reg;
        __GXData->dirtyState |= 0x5000;
    } else if (chan == GX_COLOR1A1) {
        __GXData->chanCtrl[GX_ALPHA1] = reg;
        __GXData->dirtyState |= 0xA000;
    }
}
