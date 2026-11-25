#ifndef _pobj_h_
#define _pobj_h_

#include <dolphin/types.h>
#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtxtypes.h>

#include <sysdolphin/object.h>

#include <sysdolphin/aobj.h>
#include <sysdolphin/fobj.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/tobj.h>
#include <sysdolphin/displayfunc.h>

#define HSD_A_S_W0 2
#define HSD_DEFAULT_MAX_SHAPE_VERTICES 2000
#define HSD_DEFAULT_MAX_SHAPE_NORMALS 2000

#define POBJ_ANIM (1 << 3)
#define POBJ_SKIN (0 << 12)
#define POBJ_SHAPEANIM (1 << 12)
#define POBJ_ENVELOPE (2 << 12)

#define POBJ_HIDE (1 << 11)

#define POBJ_TYPE_MASK (3 << 12)
#define pobj_type(o) ((o)->flags & POBJ_TYPE_MASK)

#define POBJ_CULLFRONT (1 << 14)
#define POBJ_CULLBACK (1 << 15)

#define SHAPESET_AVERAGE 1
#define SHAPESET_ADDITIVE (1 << 1)

typedef struct _HSD_PObj {
    HSD_Class parent; // 0x00
    struct _HSD_PObj* next; // 0x04
    struct _HSD_VtxDescList* verts; // 0x08
    u16 flags; // 0x0C
    u16 n_display; // Display list length in 32-byte blocks
    u8* display;
    union {
        struct _HSD_JObj* jobj;
        struct _HSD_ShapeSet* shape_set;
        struct _HSD_SList* envelope_list;
    } u; // 0x14
    struct _HSD_AObj* aobj; // 0x18
} HSD_PObj;

typedef struct _HSD_PObjDesc {
    char* class_name;
    struct _HSD_PObjDesc* next;
    struct _HSD_VtxDescList* verts;
    u16 flags;
    u16 n_display;
    u8* display;
    union {
        HSD_Joint* joint;
        struct _HSD_ShapeSetDesc* shape_set;
        struct _HSD_EnvelopeDesc** envelope_p;
    } u;
} HSD_PObjDesc;

typedef struct _HSD_VtxDescList {
    GXAttr attr;
    s32 attr_type;
    s32 comp_cnt;
    s32 comp_type;
    u8 frac;
    u16 stride;
    void* vertex;
} HSD_VtxDescList;

typedef struct _HSD_Envelope {
    struct _HSD_Envelope* next;
    struct _HSD_JObj* jobj;
    f32 weight;
} HSD_Envelope;

typedef struct _HSD_EnvelopeDesc {
    HSD_Joint* joint;
    f32 weight;
} HSD_EnvelopeDesc;

typedef struct _HSD_ShapeSet {
    u16 flags;
    u16 nb_shape;
    s32 nb_vertex_index;
    struct _HSD_VtxDescList* vertex_desc;
    u8** vertex_idx_list;
    s32 nb_normal_index;
    struct _HSD_VtxDescList* normal_desc;
    u8** normal_idx_list;
    union {
        f32* bp;
        f32 bl;
    } blend;
} HSD_ShapeSet;

typedef struct _HSD_ShapeSetDesc {
    u16 flags;
    u16 nb_shape;
    s32 nb_vertex_index;
    struct _HSD_VtxDescList* vertex_desc;
    u8** vertex_idx_list;
    s32 nb_normal_index;
    struct _HSD_VtxDescList* normal_desc;
    u8** normal_idx_list;
} HSD_ShapeSetDesc;

typedef struct _HSD_ShapeAnim {
    struct _HSD_ShapeAnim* next;
    HSD_AObjDesc* aobjdesc;
} HSD_ShapeAnim;

typedef struct _HSD_ShapeAnimJoint {
    struct _HSD_ShapeAnimJoint* child;
    struct _HSD_ShapeAnimJoint* next;
    struct _HSD_ShapeAnimDObj* shapeanimdobj;
} HSD_ShapeAnimJoint;

typedef struct _HSD_PObjInfo {
    HSD_ClassInfo parent;
    void (*disp)(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
    void (*setup_mtx)(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
    s32 (*load)(HSD_PObj* pobj, HSD_PObjDesc* desc);
    HSD_ObjUpdateFunc update;
} HSD_PObjInfo;

#define HSD_POBJ(o) ((HSD_PObj*)(o))
#define HSD_POBJ_INFO(i) ((HSD_PObjInfo*)(i))
#define HSD_POBJ_METHOD(o) HSD_POBJ_INFO(HSD_CLASS_METHOD(o))

extern HSD_PObjInfo hsdPObj;

void HSD_PObjClearFlags(HSD_PObj* pobj, u32 flags);
void HSD_PObjSetFlags(HSD_PObj* pobj, u32 flags);
void HSD_PObjRemoveAnimAllByFlags(HSD_PObj* pobj, u32 flags);
void HSD_PObjAddAnimAll(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim);
void HSD_PObjReqAnimAllByFlags(HSD_PObj* pobj, f32 startframe, u32 flags);
void HSD_PObjAnimAll(HSD_PObj* pobj);
HSD_PObj* HSD_PObjLoadDesc(HSD_PObjDesc* pobjdesc);
void HSD_PObjRemoveAll(HSD_PObj* pobj);
void HSD_PObjSetCurrent(HSD_PObjInfo* info);
HSD_PObjInfo* HSD_PObjGetCurrent(void);
HSD_PObj* HSD_PObjAlloc(void);
void HSD_PObjResolveRefsAll(HSD_PObj* pobj, HSD_PObjDesc* pdesc);
void HSD_PObjCountVertices(HSD_PObj* pobj, u32* vertices, u32* polygons);
void HSD_ClearVtxDesc(void);
void HSD_PObjClearMtxMark(void* obj, u32 mark);
void HSD_PObjDisp(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);

#endif
