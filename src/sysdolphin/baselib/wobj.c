#include <dolphin/mtx/mtx.h>
#include <sysdolphin/wobj.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/robj.h>
#include <sysdolphin/spline.h>

void WObjInfoInit(void);

HSD_WObjInfo hsdWObj = { { { { WObjInfoInit } } } };

static HSD_WObjInfo* WObjCurrentInfo = NULL;

#define assert_line_named(line, cond, condstr) ((cond) ? ((void)0) : __assert("wobj.c", line, condstr))

/**
 * @note Address: 0x801D2F08
 * @note Size: 0x4C
 */
void HSD_WObjReqAnim(HSD_WObj* wobj, f32 frame)
{
    if (wobj != NULL) {
        HSD_AObjReqAnim(wobj->aobj, frame);
        HSD_RObjReqAnimAll(wobj->robj, frame);
    }
}

/**
 * @note Address: 0x801D2F54
 * @note Size: 0x68
 */
void HSD_WObjAddAnim(HSD_WObj* wobj, HSD_WObjAnim* anim)
{
    if (wobj != NULL && anim != NULL) {
        if (wobj->aobj != NULL) {
            HSD_AObjRemove(wobj->aobj);
        }
        wobj->aobj = HSD_AObjLoadDesc(anim->aobjdesc);
        HSD_RObjAddAnimAll(wobj->robj, anim->robjanim);
    }
}

/**
 * @note Address: 0x801D2FBC
 * @note Size: 0x144
 */
void WObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val)
{
    HSD_WObj* wobj;
    Vec p;
    HSD_JObj* jp;

    wobj = obj;
    if (wobj != NULL) {
        switch (type) {
        case HSD_A_J_PATH:
            if (val->fv < 0.0) {
                val->fv = 0.0f;
            }
            if (1.0 < val->fv) {
                val->fv = 1.0f;
            }

            assert_line_named(152, wobj->aobj, "wobj->aobj");
            jp = (HSD_JObj*)wobj->aobj->hsd_obj;
            assert_line_named(154, jp, "jp");
            assert_line_named(155, jp->u.spline, "jp->u.spline");

            splArcLengthPoint(&p, jp->u.spline, val->fv);
            HSD_WObjSetPosition(wobj, &p);
            wobj->flags |= WOBJ_POS_LOCAL;
            break;

        case HSD_A_J_TRAX:
            HSD_WObjSetPositionX(wobj, val->fv);
            break;

        case HSD_A_J_TRAY:
            HSD_WObjSetPositionY(wobj, val->fv);
            break;

        case HSD_A_J_TRAZ:
            HSD_WObjSetPositionZ(wobj, val->fv);
            break;
        }
    }
}

/**
 * @note Address: 0x801D3100
 * @note Size: 0x48
 */
void HSD_WObjInterpretAnim(HSD_WObj* wobj)
{
    if (wobj != NULL) {
        HSD_AObjInterpretAnim(wobj->aobj, wobj, HSD_WOBJ_METHOD(wobj)->update);
        HSD_RObjAnimAll(wobj->robj);
    }
}

/**
 * @note Address: 0x801D3148
 * @note Size: 0x68
 */
int WObjLoad(HSD_WObj* wobj, HSD_WObjDesc* desc)
{
    HSD_WObjSetPosition(wobj, &desc->pos);
    if (wobj->robj != NULL) {
        HSD_RObjRemoveAll(wobj->robj);
    }
    wobj->robj = HSD_RObjLoadDesc(desc->robjdesc);
    HSD_RObjResolveRefsAll(wobj->robj, desc->robjdesc);
    return 0;
}

/**
 * @note Address: 0x801D31B0
 * @note Size: 0x74
 */
void HSD_WObjInit(HSD_WObj* wobj, HSD_WObjDesc* desc)
{
    if (wobj == NULL || desc == NULL) {
        return;
    }

    HSD_WObjSetPosition(wobj, &desc->pos);
    if (wobj->robj != NULL) {
        HSD_RObjRemoveAll(wobj->robj);
    }
    wobj->robj = HSD_RObjLoadDesc(desc->robjdesc);
    HSD_RObjResolveRefsAll(wobj->robj, desc->robjdesc);
}

/**
 * @note Address: 0x801D3224
 * @note Size: 0x98
 */
HSD_WObj* HSD_WObjLoadDesc(HSD_WObjDesc* desc)
{
    if (desc != NULL) {
        HSD_WObj* wobj;
        HSD_ClassInfo* info;
        if (desc->class_name == NULL || !(info = hsdSearchClassInfo(desc->class_name))) {
            wobj = HSD_WObjAlloc();
        } else {
            wobj = hsdNew(info);
            assert_line_named(260, wobj, "wobj");
        }
        HSD_WOBJ_METHOD(wobj)->load(wobj, desc);
        return wobj;
    }
    return NULL;
}

/**
 * @note Address: 0x801D32BC
 * @note Size: 0x48
 */
void HSD_WObjSetPosition(HSD_WObj* wobj, Vec* pos)
{
    if (wobj == NULL || pos == NULL) {
        return;
    }

    wobj->pos = *pos;
    wobj->flags |= WOBJ_POS_DIRTY;
    wobj->flags &= ~WOBJ_POS_LOCAL;
}

static inline void WObjSetupPosition(HSD_WObj* wobj)
{
    if ((wobj->flags & WOBJ_POS_LOCAL) != 0) {
        if (wobj->aobj != NULL && wobj->aobj->hsd_obj != NULL) {
            HSD_JObj* joint = (HSD_JObj*)wobj->aobj->hsd_obj;
            HSD_JObjSetupMatrix(joint);
            PSMTXMultVec(joint->mtx, &wobj->pos, &wobj->pos);
        }
        wobj->flags &= ~WOBJ_POS_LOCAL;
    }
}

/**
 * @note Address: 0x801D3304
 * @note Size: 0xD4
 */
void HSD_WObjSetPositionX(HSD_WObj* wobj, f32 val)
{
    if (wobj != NULL) {
        WObjSetupPosition(wobj);
        wobj->pos.x = val;
        wobj->flags |= WOBJ_POS_DIRTY;
    }
}

/**
 * @note Address: 0x801D33D8
 * @note Size: 0xD4
 */
void HSD_WObjSetPositionY(HSD_WObj* wobj, f32 val)
{
    if (wobj != NULL) {
        WObjSetupPosition(wobj);
        wobj->pos.y = val;
        wobj->flags |= WOBJ_POS_DIRTY;
    }
}

/**
 * @note Address: 0x801D34AC
 * @note Size: 0xD4
 */
void HSD_WObjSetPositionZ(HSD_WObj* wobj, f32 val)
{
    if (wobj != NULL) {
        WObjSetupPosition(wobj);
        wobj->pos.z = val;
        wobj->flags |= WOBJ_POS_DIRTY;
    }
}

/**
 * @note Address: 0x801D3580
 * @note Size: 0xE8
 */
void HSD_WObjGetPosition(HSD_WObj* wobj, Vec* vec)
{
    if (wobj == NULL || vec == NULL) {
        return;
    }
    WObjSetupPosition(wobj);
    *vec = wobj->pos;
}

void HSD_WObjSetDefaultClass(HSD_WObjInfo* info)
{
    if (info != NULL) {
        assert_line_named(__LINE__, hsdIsDescendantOf(HSD_CLASS_INFO(info), HSD_CLASS_INFO(&hsdWObj)),
                          "hsdIsDescendantOf(info, &hsdWObj)");
    }
    WObjCurrentInfo = info;
}

/**
 * @note Address: 0x801D3668
 * @note Size: 0x5C
 */
HSD_WObj* HSD_WObjAlloc(void)
{
    HSD_WObj* wobj =
        hsdNew((HSD_ClassInfo*) (WObjCurrentInfo ? WObjCurrentInfo : &hsdWObj));
    assert_line_named(599, wobj, "wobj");
    return wobj;
}

/**
 * @note Address: 0x801D36C4
 * @note Size: 0x54
 */
void WObjRelease(HSD_Class* o)
{
    HSD_WObj* wobj = (HSD_WObj*) o;
    HSD_RObjRemoveAll(wobj->robj);
    HSD_AObjRemove(wobj->aobj);
    HSD_OBJECT_PARENT_INFO(&hsdWObj)->release(o);
}

/**
 * @note Address: 0x801D3718
 * @note Size: 0x48
 */
void WObjAmnesia(HSD_ClassInfo* info)
{
    if (info == HSD_CLASS_INFO(WObjCurrentInfo)) {
        WObjCurrentInfo = NULL;
    }
    HSD_OBJECT_PARENT_INFO(&hsdWObj)->amnesia(info);
}

/**
 * @note Address: 0x801D3760
 * @note Size: 0x80
 */
void WObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&hsdWObj), HSD_CLASS_INFO(&hsdObj), "sysdolphin_base_library", "had_wobj", sizeof(HSD_WObjInfo), sizeof(HSD_WObj));
    HSD_CLASS_INFO(&hsdWObj)->release = WObjRelease;
    HSD_CLASS_INFO(&hsdWObj)->amnesia = WObjAmnesia;
    HSD_WOBJ_INFO(&hsdWObj)->load     = WObjLoad;
    HSD_WOBJ_INFO(&hsdWObj)->update   = WObjUpdateFunc;
}
