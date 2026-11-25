#ifndef _dobj_h_
#define _dobj_h_

#include <dolphin/types.h>

#include <sysdolphin/object.h>
#include <sysdolphin/class.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/pobj.h>

typedef struct _HSD_DObj {
    HSD_Class parent;
    struct _HSD_DObj* next; //0x04
    struct _HSD_MObj* mobj; //0x08
    struct _HSD_PObj* pobj; //0x0C
    struct _HSD_AObj* aobj; //0x10
    u32 flags; //0x14
} HSD_DObj;

typedef struct _HSD_DObjDesc {
    char* class_name;
    struct _HSD_DObjDesc* next;
    struct _HSD_MObjDesc* mobjdesc;
    struct _HSD_PObjDesc* pobjdesc;
} HSD_DObjDesc;

typedef struct _HSD_DObjInfo {
    HSD_ClassInfo parent;
    void (*disp)(HSD_DObj* dobj, Mtx vmtx, Mtx pmtx, u32 rendermode); //0x3C
    int (*load)(HSD_DObj* dobj, HSD_DObjDesc* desc); //0x40
    HSD_ObjUpdateFunc update; // 0x44, the class slot the later revision adds
} HSD_DObjInfo;

typedef struct _HSD_ShapeAnimDObj {
    struct _HSD_ShapeAnimDObj* next;
    struct _HSD_ShapeAnim* shapeanim;
} HSD_ShapeAnimDObj;

enum {
    DOBJ_HIDDEN = 1 << 0,
    DOBJ_TRSP_SHIFT = 1,
    DOBJ_OPA = 1 << DOBJ_TRSP_SHIFT,
    DOBJ_XLU = 2 << DOBJ_TRSP_SHIFT,
    DOBJ_TEXEDGE = 4 << DOBJ_TRSP_SHIFT,
    DOBJ_TRSP_MASK = DOBJ_OPA | DOBJ_XLU | DOBJ_TEXEDGE,
};

#define HSD_DOBJ(o) ((HSD_DObj*)(o))
#define HSD_DOBJ_INFO(i) ((HSD_DObjInfo*)(i))
#define HSD_DOBJ_METHOD(o) HSD_DOBJ_INFO(HSD_CLASS_METHOD(o))

void HSD_DObjSetCurrent(HSD_DObj* dobj);

u32 HSD_DObjGetFlags(HSD_DObj* dobj);

void HSD_DObjSetFlags(HSD_DObj* dobj, u32 flags);
void HSD_DObjClearFlags(HSD_DObj* dobj, u32 flags);
void HSD_DObjModifyFlags(HSD_DObj* dobj, u32 flags, u32 mask);

HSD_DObj* HSD_DObjAlloc(void);
HSD_DObj* HSD_DObjLoadDesc(HSD_DObjDesc* desc);
void HSD_DObjAnimAll(HSD_DObj* dobj);
void HSD_DObjAddAnimAll(HSD_DObj* dobj, HSD_MatAnim* matanim, HSD_ShapeAnimDObj* shapeanimdobj);
void HSD_DObjRemoveAll(HSD_DObj* dobj);
void HSD_DObjRemoveAnimAllByFlags(HSD_DObj* dobj, u32 flags);
void HSD_DObjReqAnimAllByFlags(HSD_DObj* dobj, f32 startframe, u32 flags);
void HSD_DObjResolveRefsAll(HSD_DObj* dobj, HSD_DObjDesc* desc);

#endif
