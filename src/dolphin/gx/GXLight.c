#include <sdk_math.h>
#include <dolphin/gx.h>
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/os.h>

#include "PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h"
#include "__gx.h"

// GXLightObj private data
typedef struct {
    u32 reserved[3];
    GXColor color;
    f32 a[3];
    f32 k[3];
    f32 lpos[3];
    f32 ldir[3];
} __GXLightObjInt_struct;

STATIC_ASSERT(sizeof(__GXLightObjInt_struct) == sizeof(GXLightObj));

/**
 * @note Address: 0x80188ABC
 * @note Size: 0x1C
 */
void GXInitLightAttn(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(129, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(130, "GXInitLightAttn");
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXInitLightAttnA(GXLightObj* lt_obj, f32 a0, f32 a1, f32 a2) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(143, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(144, "GXInitLightAttnA");
    obj->a[0] = a0;
    obj->a[1] = a1;
    obj->a[2] = a2;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetLightAttnA(const GXLightObj* lt_obj, f32* a0, f32* a1, f32* a2) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(153, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(154, "GXGetLightAttnA");
    *a0 = obj->a[0];
    *a1 = obj->a[1];
    *a2 = obj->a[2];
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXInitLightAttnK(GXLightObj* lt_obj, f32 k0, f32 k1, f32 k2) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(163, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(164, "GXInitLightAttnK");
    obj->k[0] = k0;
    obj->k[1] = k1;
    obj->k[2] = k2;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetLightAttnK(const GXLightObj* lt_obj, f32* k0, f32* k1, f32* k2) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(173, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(174, "GXGetLightAttnK");
    *k0 = obj->k[0];
    *k1 = obj->k[1];
    *k2 = obj->k[2];
}

/**
 * @note Address: 0x80188AD8
 * @note Size: 0x190
 */
void GXInitLightSpot(GXLightObj* lt_obj, f32 cutoff, GXSpotFn spot_func) {
    f32 a0, a1, a2;
    f32 r;
    f32 d;
    f32 cr;
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(198, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(200, "GXInitLightSpot");

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

/**
 * @note Address: 0x80188C68
 * @note Size: 0xD0
 */
void GXInitLightDistAttn(GXLightObj* lt_obj, f32 ref_dist, f32 ref_br, GXDistAttnFn dist_func) {
    f32 k0, k1, k2;
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(273, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(275, "GXInitLightDistAttn");

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

/**
 * @note Address: 0x80188D38
 * @note Size: 0x10
 */
void GXInitLightPos(GXLightObj* lt_obj, f32 x, f32 y, f32 z) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(328, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(330, "GXInitLightPos");

    obj->lpos[0] = x;
    obj->lpos[1] = y;
    obj->lpos[2] = z;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetLightPos(const GXLightObj* lt_obj, f32* x, f32* y, f32* z) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(339, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(341, "GXGetLightPos");

    *x = obj->lpos[0];
    *y = obj->lpos[1];
    *z = obj->lpos[2];
}

/**
 * @note Address: 0x80188D48
 * @note Size: 0x1C
 */
void GXInitLightDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(360, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;

    obj->ldir[0] = -nx;
    obj->ldir[1] = -ny;
    obj->ldir[2] = -nz;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetLightDir(const GXLightObj* lt_obj, f32* nx, f32* ny, f32* nz) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(372, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;

    *nx = -obj->ldir[0];
    *ny = -obj->ldir[1];
    *nz = -obj->ldir[2];
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXInitSpecularDir(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz) {
    f32 mag;
    f32 vx;
    f32 vy;
    f32 vz;
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(398, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(399, "GXInitSpecularDir");

    vx = -nx;
    vy = -ny;
    vz = -nz + 1.0f;

    mag = (vx * vx) + (vy * vy) + (vz * vz);
    if (mag != 0.0f) {
        mag = 1.0f / SDKFastSqrt(mag);
    }
    
    obj->ldir[0] = vx * mag;
    obj->ldir[1] = vy * mag;
    obj->ldir[2] = vz * mag;
    obj->lpos[0] = nx * -1000000000000000000.0f;
    obj->lpos[1] = ny * -1000000000000000000.0f;
    obj->lpos[2] = nz * -1000000000000000000.0f;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXInitSpecularDirHA(GXLightObj* lt_obj, f32 nx, f32 ny, f32 nz, f32 hx, f32 hy, f32 hz) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(436, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(437, "GXInitSpecularHA");

    obj->ldir[0] = hx;
    obj->ldir[1] = hy;
    obj->ldir[2] = hz;
    obj->lpos[0] = nx * -1000000000000000000.0f;
    obj->lpos[1] = ny * -1000000000000000000.0f;
    obj->lpos[2] = nz * -1000000000000000000.0f;
}

/**
 * @note Address: 0x80188D64
 * @note Size: 0xC
 */
void GXInitLightColor(GXLightObj* lt_obj, GXColor color) {
    __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(462, lt_obj != NULL, "Light Object Pointer is null");
    obj = (__GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(463, "GXInitLightColor");

    obj->color = color;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXGetLightColor(const GXLightObj* lt_obj, GXColor* color) {
    const __GXLightObjInt_struct* obj;

    ASSERTMSGLINE(476, lt_obj != NULL, "Light Object Pointer is null");
    obj = (const __GXLightObjInt_struct*)lt_obj;
    CHECK_GXBEGIN(477, "GXGetLightColor");

    *color = obj->color;
}

static void WriteLightWord(u32 addr, u32 value) {
    GX_WRITE_U32(value);
#if DEBUG
    VERIF_MTXLIGHT(addr, value);
#endif
}

enum {
    XF_LIGHT_BASE = 0x600,
    XF_LIGHT_WORDS = 16,
    XF_LIGHT_COLOR_WORD = 3,
};

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 ConvLightID2Num(GXLightID id) {
    switch (id) {
    case GX_LIGHT0: return 0;
    case GX_LIGHT1: return 1;
    case GX_LIGHT2: return 2;
    case GX_LIGHT3: return 3;
    case GX_LIGHT4: return 4;
    case GX_LIGHT5: return 5;
    case GX_LIGHT6: return 6;
    case GX_LIGHT7: return 7;
    default:        return 8;
    }
}

/**
 * @note Address: 0x80188D70
 * @note Size: 0x7C
 */
void GXLoadLightObjImm(const GXLightObj* lt_obj, GXLightID light) {
    u32 addr;
    u32 idx;
    u32 wordIndex;

    ASSERTMSGLINE(568, lt_obj != NULL, "Light Object Pointer is null");
    CHECK_GXBEGIN(569, "GXLoadLightObjImm");

#if DEBUG
    idx = ConvLightID2Num(light);
#else
    idx = 31 - __cntlzw(light);
#endif

    ASSERTMSGLINE(575, idx < 8, "GXLoadLightObjImm: Invalid Light Id");
    idx &= 7;

    addr = XF_LIGHT_BASE + idx * XF_LIGHT_WORDS;
    GX_WRITE_U8(GX_LOAD_XF_REG);
    GX_WRITE_U32(addr | ((XF_LIGHT_WORDS - 1) << 16));

    for (wordIndex = 0; wordIndex < XF_LIGHT_COLOR_WORD; ++wordIndex) {
        WriteLightWord(addr + wordIndex, 0);
    }
    WriteLightWord(addr + XF_LIGHT_COLOR_WORD, lt_obj->dummy[XF_LIGHT_COLOR_WORD]);
    for (wordIndex = XF_LIGHT_COLOR_WORD + 1; wordIndex < XF_LIGHT_WORDS; ++wordIndex) {
        u32 word = lt_obj->dummy[wordIndex];
#if !DEBUG
        // Unquantized paired-single stores flush subnormals to signed zero.
        if ((word & 0x7f800000) == 0) {
            word &= 0x80000000;
        }
#endif
        WriteLightWord(addr + wordIndex, word);
    }

    __GXData->bpSentNot = 1;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void GXLoadLightObjIndx(u32 lt_obj_indx, GXLightID light) {
    u32 reg;
    u32 addr;
    u32 idx;

    CHECK_GXBEGIN(624, "GXLoadLightObjIndx");

#if DEBUG
    idx = ConvLightID2Num(light);
#else
    idx = 31 - __cntlzw(light);
#endif

    ASSERTMSGLINE(627, idx < 8, "GXLoadLightObjIndx: Invalid Light Id");
    idx &= 7;

    addr = XF_LIGHT_BASE + idx * XF_LIGHT_WORDS;
    reg = 0;
    SET_REG_FIELD(632, reg, 12, 0, addr);
    SET_REG_FIELD(634, reg, 4, 12, XF_LIGHT_WORDS - 1);
    SET_REG_FIELD(634, reg, 16, 16, lt_obj_indx);
    GX_WRITE_U8(GX_LOAD_INDX_D);
    GX_WRITE_U32(reg);
#if DEBUG
    __GXShadowIndexState(7, reg);
#endif
    __GXData->bpSentNot = 1;
}

static BOOL SetChannelColor(GXChannelID chan, GXColor color, u32* colors, u32 firstReg) {
    u32 index;
    u32 reg;

    switch (chan) {
    case GX_COLOR0:
    case GX_COLOR1:
        index = chan - GX_COLOR0;
        reg = (colors[index] & 0xFFu) | (__GXPackRGBA8(color) & 0xFFFFFF00u);
        break;
    case GX_ALPHA0:
    case GX_ALPHA1:
        index = chan - GX_ALPHA0;
        reg = (colors[index] & 0xFFFFFF00u) | color.a;
        break;
    case GX_COLOR0A0:
    case GX_COLOR1A1:
        index = chan - GX_COLOR0A0;
        reg = __GXPackRGBA8(color);
        break;
    default:
        return FALSE;
    }

    GX_WRITE_XF_REG(firstReg + index, reg);
    __GXData->bpSentNot = 1;
    colors[index] = reg;
    return TRUE;
}

/**
 * @note Address: 0x80188DEC
 * @note Size: 0xE8
 */
void GXSetChanAmbColor(GXChannelID chan, GXColor amb_color) {
    CHECK_GXBEGIN(661, "GXSetChanAmbColor");
    if (!SetChannelColor(chan, amb_color, __GXData->ambColor, 10)) {
        ASSERTMSGLINE(731, 0, "GXSetChanAmbColor: Invalid Channel Id");
    }
}

/**
 * @note Address: 0x80188ED4
 * @note Size: 0xE8
 */
void GXSetChanMatColor(GXChannelID chan, GXColor mat_color) {
    CHECK_GXBEGIN(762, "GXSetChanMatColor");
    if (!SetChannelColor(chan, mat_color, __GXData->matColor, 12)) {
        ASSERTMSGLINE(832, 0, "GXSetChanMatColor: Invalid Channel Id");
    }
}

/**
 * @note Address: 0x80188FBC
 * @note Size: 0x3C
 */
void GXSetNumChans(u8 nChans) {
    CHECK_GXBEGIN(857, "GXSetNumChans");
    ASSERTMSGLINE(858, nChans <= 2, "GXSetNumChans: nChans > 2");

    SET_REG_FIELD(860, __GXData->genMode, 3, 4, nChans);
    GX_WRITE_XF_REG(9, nChans);
    __GXData->dirtyState |= 4;
}

/**
 * @note Address: 0x80188FF8
 * @note Size: 0xB0
 */
void GXSetChanCtrl(GXChannelID chan, GXBool enable, GXColorSrc amb_src, GXColorSrc mat_src, u32 light_mask, GXDiffuseFn diff_fn, GXAttnFn attn_fn) {
    u32 reg;
    u32 idx;

    CHECK_GXBEGIN(892, "GXSetChanCtrl");

    ASSERTMSGLINE(895, chan >= GX_COLOR0 && chan <= GX_COLOR1A1, "GXSetChanCtrl: Invalid Channel Id");

#if DEBUG
    if (chan == GX_COLOR0A0)
        idx = 0;
    else if (chan == GX_COLOR1A1)
        idx = 1;
    else
        idx = chan;
#else
    idx = chan & 0x3;
#endif

    reg = 0;
    SET_REG_FIELD(907, reg, 1, 1, enable);
    SET_REG_FIELD(908, reg, 1, 0, mat_src);
    SET_REG_FIELD(909, reg, 1, 6, amb_src);
    
    SET_REG_FIELD(911, reg, 2, 7, (attn_fn == GX_AF_SPEC) ? 0 : diff_fn);
    SET_REG_FIELD(912, reg, 1, 9, (attn_fn != GX_AF_NONE));
    SET_REG_FIELD(913, reg, 1, 10, (attn_fn != GX_AF_SPEC));

    SET_REG_FIELD(925, reg, 4, 2, light_mask & 0xF);
    SET_REG_FIELD(926, reg, 4, 11, (light_mask >> 4) & 0xF);

    GX_WRITE_XF_REG(idx + 14, reg);
    
    if (chan == GX_COLOR0A0) {
        GX_WRITE_XF_REG(16, reg);
    } else if (chan == GX_COLOR1A1) {
        GX_WRITE_XF_REG(17, reg);
    }

    __GXData->bpSentNot = 1;
}
