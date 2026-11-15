#include <nw4r/g3d.h>

namespace nw4r {
namespace g3d {
namespace {

NW4R_G3D_RESFILE_NAME_DEF(LightSet, "LightSet(NW4R)");
NW4R_G3D_RESFILE_NAME_DEF(AmbLights, "AmbLights(NW4R)");
NW4R_G3D_RESFILE_NAME_DEF(Lights, "Lights(NW4R)");
NW4R_G3D_RESFILE_NAME_DEF(Fogs, "Fogs(NW4R)");
NW4R_G3D_RESFILE_NAME_DEF(Cameras, "Cameras(NW4R)");

} // namespace

bool ResAnmScn::HasResAnmAmbLight() const {
    return ResDic(ofs_to_obj<ResDic>(
               ref().toScnTopLevelDic))[ResName(&ResNameData_AmbLights)] !=
           NULL;
}

bool ResAnmScn::HasResAnmLight() const {
    return ResDic(ofs_to_obj<ResDic>(
               ref().toScnTopLevelDic))[ResName(&ResNameData_Lights)] != NULL;
}

ResLightSet ResAnmScn::GetResLightSet(int idx) const {
    void* pResLightSetDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_LightSet)];

    if (pResLightSetDicData != NULL) {
        return ResLightSet(ResDic(pResLightSetDicData)[idx]);
    }

    return ResLightSet(NULL);
}

ResLightSet ResAnmScn::GetResLightSet(u32 idx) const {
    return GetResLightSet(static_cast<int>(idx));
}

u32 ResAnmScn::GetResLightSetNumEntries() const {
    void* pResLightSetDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_LightSet)];

    if (pResLightSetDicData != NULL) {
        return ResDic(pResLightSetDicData).GetNumData();
    }

    return 0;
}

ResAnmAmbLight ResAnmScn::GetResAnmAmbLight(const ResName name) const {
    void* pResAnmAmbLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_AmbLights)];

    if (pResAnmAmbLightDicData != NULL) {
        return ResAnmAmbLight(ResDic(pResAnmAmbLightDicData)[name]);
    }

    return ResAnmAmbLight(NULL);
}

ResAnmAmbLight ResAnmScn::GetResAnmAmbLight(int idx) const {
    void* pResAnmAmbLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_AmbLights)];

    if (pResAnmAmbLightDicData != NULL) {
        return ResAnmAmbLight(ResDic(pResAnmAmbLightDicData)[idx]);
    }

    return ResAnmAmbLight(NULL);
}

ResAnmAmbLight ResAnmScn::GetResAnmAmbLight(u32 idx) const {
    return GetResAnmAmbLight(static_cast<int>(idx));
}

u32 ResAnmScn::GetResAnmAmbLightNumEntries() const {
    void* pResAnmAmbLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_AmbLights)];

    if (pResAnmAmbLightDicData != NULL) {
        return ResDic(pResAnmAmbLightDicData).GetNumData();
    }

    return 0;
}

ResAnmLight ResAnmScn::GetResAnmLight(const ResName name) const {
    void* pResAnmLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Lights)];

    if (pResAnmLightDicData != NULL) {
        return ResAnmLight(ResDic(pResAnmLightDicData)[name]);
    }

    return ResAnmLight(NULL);
}

ResAnmLight ResAnmScn::GetResAnmLight(int idx) const {
    void* pResAnmLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Lights)];

    if (pResAnmLightDicData != NULL) {
        return ResAnmLight(ResDic(pResAnmLightDicData)[idx]);
    }

    return ResAnmLight(NULL);
}

ResAnmLight ResAnmScn::GetResAnmLight(u32 idx) const {
    return GetResAnmLight(static_cast<int>(idx));
}

u32 ResAnmScn::GetResAnmLightNumEntries() const {
    void* pResAnmLightDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Lights)];

    if (pResAnmLightDicData != NULL) {
        return ResDic(pResAnmLightDicData).GetNumData();
    }

    return 0;
}

ResAnmFog ResAnmScn::GetResAnmFog(int idx) const {
    void* pResAnmFogDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Fogs)];

    if (pResAnmFogDicData != NULL) {
        return ResAnmFog(ResDic(pResAnmFogDicData)[idx]);
    }

    return ResAnmFog(NULL);
}

ResAnmFog ResAnmScn::GetResAnmFog(u32 idx) const {
    return GetResAnmFog(static_cast<int>(idx));
}

u32 ResAnmScn::GetResAnmFogNumEntries() const {
    void* pResAnmFogDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Fogs)];

    if (pResAnmFogDicData != NULL) {
        return ResDic(pResAnmFogDicData).GetNumData();
    }

    return 0;
}

ResAnmCamera ResAnmScn::GetResAnmCamera(int idx) const {
    void* pResAnmCameraDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Cameras)];

    if (pResAnmCameraDicData != NULL) {
        return ResAnmCamera(ResDic(pResAnmCameraDicData)[idx]);
    }

    return ResAnmCamera(NULL);
}

ResAnmCamera ResAnmScn::GetResAnmCamera(u32 idx) const {
    return GetResAnmCamera(static_cast<int>(idx));
}

u32 ResAnmScn::GetResAnmCameraNumEntries() const {
    void* pResAnmCameraDicData = ResDic(ofs_to_obj<ResDic>(
        ref().toScnTopLevelDic))[ResName(&ResNameData_Cameras)];

    if (pResAnmCameraDicData != NULL) {
        return ResDic(pResAnmCameraDicData).GetNumData();
    }

    return 0;
}

ResLightSet ResAnmScn::GetResLightSetByRefNumber(u32 refNumber) const {
    const ResAnmScnInfoData& rInfoData = ref().info;

    if (rInfoData.numResLightSetData <= refNumber) {
        return ResLightSet(NULL);
    }

    const ResLightSetData* pArray =
        ofs_to_ptr<ResLightSetData>(ref().toResLightSetDataArray);

    ResLightSetData* pTarget = const_cast<ResLightSetData*>(&pArray[refNumber]);

    return ResLightSet(pTarget->id < rInfoData.numResLightSetData ? pTarget
                                                                  : NULL);
}

ResAnmAmbLight ResAnmScn::GetResAnmAmbLightByRefNumber(u32 refNumber) const {
    const ResAnmScnInfoData& rInfoData = ref().info;

    if (rInfoData.numResAnmAmbLightData <= refNumber) {
        return ResAnmAmbLight(NULL);
    }

    const ResAnmAmbLightData* pArray =
        ofs_to_ptr<ResAnmAmbLightData>(ref().toResAnmAmbLightDataArray);

    ResAnmAmbLightData* pTarget =
        const_cast<ResAnmAmbLightData*>(&pArray[refNumber]);

    return ResAnmAmbLight(
        pTarget->id < rInfoData.numResAnmAmbLightData ? pTarget : NULL);
}

ResAnmLight ResAnmScn::GetResAnmLightByRefNumber(u32 refNumber) const {
    const ResAnmScnInfoData& rInfoData = ref().info;

    if (rInfoData.numResAnmLightData <= refNumber) {
        return ResAnmLight(NULL);
    }

    const ResAnmLightData* pArray =
        ofs_to_ptr<ResAnmLightData>(ref().toResAnmLightDataArray);

    ResAnmLightData* pTarget = const_cast<ResAnmLightData*>(&pArray[refNumber]);

    return ResAnmLight(pTarget->id < rInfoData.numResAnmLightData ? pTarget
                                                                  : NULL);
}

ResAnmFog ResAnmScn::GetResAnmFogByRefNumber(u32 refNumber) const {
    const ResAnmScnInfoData& rInfoData = ref().info;

    if (rInfoData.numResAnmFogData <= refNumber) {
        return ResAnmFog(NULL);
    }

    const ResAnmFogData* pArray =
        ofs_to_ptr<ResAnmFogData>(ref().toResAnmFogDataArray);

    ResAnmFogData* pTarget = const_cast<ResAnmFogData*>(&pArray[refNumber]);

    return ResAnmFog(pTarget->id < rInfoData.numResAnmFogData ? pTarget : NULL);
}

ResAnmCamera ResAnmScn::GetResAnmCameraByRefNumber(u32 refNumber) const {
    const ResAnmScnInfoData& rInfoData = ref().info;

    if (rInfoData.numResAnmCameraData <= refNumber) {
        return ResAnmCamera(NULL);
    }

    const ResAnmCameraData* pArray =
        ofs_to_ptr<ResAnmCameraData>(ref().toResAnmCameraDataArray);

    ResAnmCameraData* pTarget =
        const_cast<ResAnmCameraData*>(&pArray[refNumber]);

    return ResAnmCamera(pTarget->id < rInfoData.numResAnmCameraData ? pTarget
                                                                    : NULL);
}

bool ResAnmScn::Bind(const ResAnmScn scene) {
    u32 lightSetNum = GetResLightSetNumEntries();
    bool success = true;

    for (u32 i = 0; i < lightSetNum; i++) {
        ResLightSet set = GetResLightSet(i);
        success = set.Bind(scene) && success;
    }

    return success;
}

void ResAnmScn::Release() {
    u32 lightSetNum = GetResLightSetNumEntries();

    for (u32 i = 0; i < lightSetNum; i++) {
        ResLightSet set = GetResLightSet(i);
        set.Release();
    }
}

} // namespace g3d
} // namespace nw4r
