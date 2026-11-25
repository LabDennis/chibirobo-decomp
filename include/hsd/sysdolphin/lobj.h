#ifndef _lobj_h_
#define _lobj_h_

#include <global.h>

#include <dolphin/types.h>
#include <dolphin/gx/gxtypes.h>

#include <sysdolphin/object.h>
#include <sysdolphin/fobj.h>
#include <sysdolphin/lobj_setup.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HSD_A_L_LITC_R 9
#define HSD_A_L_LITC_G 10
#define HSD_A_L_LITC_B 11
#define HSD_A_L_VIS 12
#define HSD_A_L_A0 13
#define HSD_A_L_A1 14
#define HSD_A_L_A2 15
#define HSD_A_L_K0 16
#define HSD_A_L_K1 17
#define HSD_A_L_K2 18
#define HSD_A_L_CUTOFF 19
#define HSD_A_L_REFDIST 20
#define HSD_A_L_REFBRIGHT 21
#define HSD_A_L_LITC_A 22

#define LOBJ_AMBIENT (0 << 0)
#define LOBJ_INFINITE (1 << 0)
#define LOBJ_POINT (2 << 0)
#define LOBJ_SPOT (3 << 0)
#define LOBJ_DIFFUSE (1 << 2)
#define LOBJ_SPECULAR (1 << 3)
#define LOBJ_ALPHA (1 << 4)
#define LOBJ_HIDDEN (1 << 5)
#define LOBJ_RAW_PARAM (1 << 6)
#define LOBJ_DIFF_DIRTY (1 << 7)
#define LOBJ_SPEC_DIRTY (1 << 8)

#define LOBJ_TYPE_MASK 3

#define LOBJ_LIGHT_ATTN_NONE 0
#define LOBJ_LIGHT_ATTN 1

typedef struct _HSD_LightParams {
    f32 cutoff;
    GXSpotFn spot_func;
    f32 ref_br;
    f32 ref_dist;
    GXDistAttnFn dist_func;
} HSD_LightParams;

typedef struct _HSD_LightPointDesc {
    f32 ref_br;
    f32 ref_dist;
    GXDistAttnFn dist_func;
} HSD_LightPointDesc;

typedef struct _HSD_LightSpotDesc {
    f32 cutoff;
    GXSpotFn spot_func;
    f32 ref_br;
    f32 ref_dist;
    GXDistAttnFn dist_func;
} HSD_LightSpotDesc;

typedef struct _HSD_LightAttn {
    f32 a0;
    f32 a1;
    f32 a2;
    f32 k0;
    f32 k1;
    f32 k2;
} HSD_LightAttn;

typedef struct _HSD_LObj {
    HSD_Obj parent; //0x00
    u16 flags; //0x08
    u16 priority; //0x0A
    struct _HSD_LObj* next; //0x0C
    GXColor color; //0x10
    GXColor hw_color; //0x14
    struct _HSD_WObj* position; //0x18
    struct _HSD_WObj* interest; //0x1C
    union {
        HSD_LightParams params;
        HSD_LightAttn attn;
    } u;
    f32 shininess;
    Vec lvec;
    struct _HSD_AObj* aobj;
    u32 id; //GXLightID
    GXLightObj lightobj; //0x50
    s32 spec_id; //0x90 GXLightID
    GXLightObj spec_lightobj; //0x94
} HSD_LObj;

static inline u32 HSD_LObjGetType(HSD_LObj* lobj)
{
    return lobj->flags & LOBJ_TYPE_MASK;
}

inline u32 HSD_LObjGetPriority(HSD_LObj* lobj)
{
    assert_line(355, lobj);
    return lobj->priority & 0xFF;
}

typedef struct _HSD_LightDesc {
    char* class_name; //0x00
    struct _HSD_LightDesc* next; //0x04
    u16 flags; //0x08
    u16 attnflags; //0x0A
    GXColor color; //0x0C
    struct _HSD_WObjDesc* position; //0x10
    struct _HSD_WObjDesc* interest; //0x14
    union {
        void* p;
        f32* shininess;
        HSD_LightPointDesc* point;
        HSD_LightSpotDesc* spot;
        HSD_LightAttn* attn;
    } u;
} HSD_LightDesc;

typedef struct _HSD_LightAnim {
    struct _HSD_LightAnim* next;
    struct _HSD_AObjDesc* aobjdesc;
    struct _HSD_WObjAnim* position_anim;
    struct _HSD_WObjAnim* interest_anim;
} HSD_LightAnim;

typedef struct _HSD_LObjInfo {
    HSD_ObjInfo parent;
    int (*load)(HSD_LObj* lobj, HSD_LightDesc* ldesc);
    HSD_ObjUpdateFunc update;
} HSD_LObjInfo;

#define HSD_LOBJ(o) ((HSD_LObj*)(o))
#define HSD_LOBJ_INFO(i) ((HSD_LObjInfo*)(i))
#define HSD_LOBJ_METHOD(o) HSD_LOBJ_INFO(HSD_OBJECT_METHOD(o))

u32 HSD_LObjGetFlags(HSD_LObj* lobj);
void HSD_LObjSetFlags(HSD_LObj* lobj, u32 flags);
void HSD_LObjClearFlags(HSD_LObj* lobj, u32 flags);
void HSD_LObjSetColor(HSD_LObj* lobj, GXColor color);
void HSD_LObjGetColor(HSD_LObj* lobj, GXColor* color);
BOOL HSD_LObjGetPosition(HSD_LObj* lobj, Vec* pos);
void HSD_LObjSetPosition(HSD_LObj* lobj, Vec* pos);
struct _HSD_WObj* HSD_LObjGetPositionWObj(HSD_LObj* lobj);
void HSD_LObjSetSpot(HSD_LObj* lobj, f32 cutoff, GXSpotFn spot_func);
void HSD_LObjSetDistAttn(HSD_LObj* lobj, f32 ref_dist, f32 ref_br, GXDistAttnFn dist_func);
BOOL HSD_LObjGetInterest(HSD_LObj* lobj, Vec* interest);
void HSD_LObjSetInterest(HSD_LObj* lobj, Vec* interest);
struct _HSD_WObj* HSD_LObjGetInterestWObj(HSD_LObj* lobj);
void HSD_LObjGetLightVector(HSD_LObj* lobj, Vec* v);
s32 HSD_LObjGetLightMaskDiffuse(void);
s32 HSD_LObjGetLightMaskAttnFunc(void);
s32 HSD_LObjGetLightMaskAlpha(void);
s32 HSD_LObjGetLightMaskSpecular(void);
s32 HSD_LObjGetNbActive(void);
s32 HSD_LightID2Index(u32 id);
s32 HSD_Index2LightID(s32 index);
HSD_LObjInfo* HSD_LObjGetDefaultClass(void);
HSD_LObj* HSD_LObjLoadDesc(HSD_LightDesc* desc);
void HSD_LObjAnim(HSD_LObj* lobj);
void HSD_LObjReqAnim(HSD_LObj* lobj, f32 frame);
void HSD_LObjAddAnim(HSD_LObj* lobj, HSD_LightAnim* anim);
void HSD_LObjAddAnimAll(HSD_LObj* lobj, HSD_LightAnim* anim);
void HSD_LObjSetCurrentAll(HSD_LObj* lobj);
HSD_LObj* HSD_LObjGetCurrentByType(u16 flags);
void HSD_LObjRemoveAll(HSD_LObj* lobj);

#ifdef __cplusplus
}
#endif

#endif
