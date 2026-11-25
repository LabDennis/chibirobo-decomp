#include <sysdolphin/dobj.h>
#include <sysdolphin/class_new.h>

extern HSD_DObjInfo hsdDObj;

void DObjRelease(HSD_Class* obj);
void DObjAmnesia(HSD_ClassInfo* info);
void HSD_DObjDisp(HSD_DObj* dobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
int DObjLoad(HSD_DObj* dobj, HSD_DObjDesc* desc);

const char gDObjBlendModeReport[] = "mobj has unexpected blending flags (0x%x).";

/**
 * @note Address: 0x801B80B4
 * @note Size: 0x80
 */
void DObjInfoInit(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&hsdDObj), &hsdClass,
        "sysdolphin_base_library", "hsd_dobj",
        sizeof(HSD_DObjInfo), sizeof(HSD_DObj));

    HSD_CLASS_INFO(&hsdDObj)->release = DObjRelease;
    HSD_CLASS_INFO(&hsdDObj)->amnesia = DObjAmnesia;
    hsdDObj.disp = HSD_DObjDisp;
    hsdDObj.load = DObjLoad;
}
