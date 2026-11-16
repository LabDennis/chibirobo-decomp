#include <nw4r/g3d.h>

#include <algorithm>
#include <new>

namespace nw4r {
namespace g3d {

NW4R_G3D_RTTI_DEF(AnmScn);
NW4R_G3D_RTTI_DEF(AnmScnRes);

// Only AnmScn::GetLightSetting is in the references (xenoblade's g3d_anmscn.cpp); the rest of this
// file is written from the retail code, with the names of xenoblade's symbols.

namespace {

void MakeDiffuseLightObj(LightObj* pObj, const LightAnmResult* pResult) {
    u32 type = pResult->flags & LightAnmResult::FLAG_LIGHT_TYPE_MASK;

    pObj->Clear();

    switch (type) {
    case LightAnmResult::LIGHTTYPE_POINT: {
        pObj->InitLightColor(pResult->color);
        pObj->InitLightPos(pResult->pos.x, pResult->pos.y, pResult->pos.z);
        pObj->InitLightSpot(0.0f, GX_SP_OFF);
        pObj->InitLightDistAttn(pResult->refDistance, pResult->refBrightness,
                                pResult->distFunc);
        pObj->InitLightDir(0.0f, 0.0f, 0.0f);
        break;
    }

    case LightAnmResult::LIGHTTYPE_DIRECTIONAL: {
        // an infinitely distant position against the light's direction
        math::VEC3 dir = pResult->aim - pResult->pos;
        math::VEC3 pos = dir * -1.0e10f;

        pObj->InitLightColor(pResult->color);
        pObj->InitLightPos(pos.x, pos.y, pos.z);
        pObj->InitLightAttnA(1.0f, 0.0f, 0.0f);
        pObj->InitLightAttnK(1.0f, 0.0f, 0.0f);
        pObj->InitLightDir(0.0f, 0.0f, 0.0f);
        break;
    }

    case LightAnmResult::LIGHTTYPE_SPOT: {
        math::VEC3 dir = pResult->aim - pResult->pos;
        math::VEC3Normalize(&dir, &dir);

        pObj->InitLightColor(pResult->color);
        pObj->InitLightPos(pResult->pos.x, pResult->pos.y, pResult->pos.z);
        pObj->InitLightSpot(pResult->cutoff, pResult->spotFunc);
        pObj->InitLightDistAttn(pResult->refDistance, pResult->refBrightness,
                                pResult->distFunc);
        pObj->InitLightDir(dir.x, dir.y, dir.z);
        break;
    }

    default: {
        break;
    }
    }

    if (!(pResult->flags & LightAnmResult::FLAG_COLOR_ENABLE)) {
        pObj->DisableColor();
    }

    if (!(pResult->flags & LightAnmResult::FLAG_ALPHA_ENABLE)) {
        pObj->DisableAlpha();
    }

    pObj->Enable();
}

} // namespace

/******************************************************************************
 *
 * AnmScn
 *
 ******************************************************************************/
void AnmScn::GetLightSetting(LightSetting* pSetting) {
    const u32 numLightSet = GetLightSetMaxRefNumber();
    const u32 numAmbLight = GetAmbLightMaxRefNumber();
    const u32 numDiffLight = GetDiffuseLightMaxRefNumber();

    if (numLightSet > 0) {
        const u32 numLightSetObj = pSetting->GetNumLightSet();
        const u32 numLoadableSet = std::min(numLightSet, numLightSetObj);

        for (u32 i = 0; i < numLoadableSet; i++) {
            LightSet set = pSetting->GetLightSet(i);
            GetLightSet(set, i);
        }
    }

    if (numAmbLight > 0) {
        AmbLightObj* pAmbObjArray = pSetting->GetAmbLightObjArray();
        const u32 numAmbObj = pSetting->GetNumLightObj();
        const u32 numLoadableAmb = std::min(numAmbLight, numAmbObj);

        for (u32 i = 0; i < numLoadableAmb; i++) {
            AmbLightObj* pAmbObj = &pAmbObjArray[i];
            *reinterpret_cast<u32*>(&pAmbObj->r) = GetAmbLightColor(i);
        }
    }

    if (numDiffLight > 0) {
        LightObj* pLightObjArray = pSetting->GetLightObjArray();
        const u32 numLightObj = pSetting->GetNumLightObj();
        const u32 numSpecLight = GetNumSpecularLight();

        const u32 numLight = numDiffLight + numSpecLight;
        const u32 numLoadableDiffLight = std::min(numDiffLight, numLightObj);
        const u32 numLoadableLight = std::min(numLight, numLightObj);

        for (u32 i = 0; i < numLoadableDiffLight; i++) {
            LightObj* pObj = &pLightObjArray[i];
            pObj->Disable();
        }

        for (u32 i = 0; i < numLoadableDiffLight; i++) {
            LightObj* pDiffObj = &pLightObjArray[i];
            LightObj* pSpecObj = NULL;

            if (pDiffObj->IsEnable()) {
                continue;
            }

            if (HasSpecularLight(i)) {
                const u32 specId = GetSpecularLightID(i);
                if (specId < numLoadableLight) {
                    pSpecObj = &pLightObjArray[specId];
                }
            }

            GetLight(pDiffObj, pSpecObj, i);
        }
    }
}

AnmScn::AnmScn(MEMAllocator* pAllocator) : G3dObj(pAllocator, NULL) {}

AnmScnRes* AnmScn::Attach(int /* idx */, AnmScnRes* /* pRes */) {
    return NULL;
}

AnmScnRes* AnmScn::Detach(int /* idx */) {
    return NULL;
}

AnmScn::~AnmScn() {}

/******************************************************************************
 *
 * AnmScnRes
 *
 ******************************************************************************/
u32 AnmScnRes::GetNumAmbLight() const {
    return mRes.GetResAnmAmbLightNumEntries();
}

u32 AnmScnRes::GetNumDiffuseLight() const {
    return mRes.GetResAnmLightNumEntries();
}

u32 AnmScnRes::GetNumSpecularLight() const {
    return mRes.ref().info.numSpecularLight;
}

u32 AnmScnRes::GetNumFog() const {
    return mRes.GetResAnmFogNumEntries();
}

u32 AnmScnRes::GetNumCamera() const {
    return mRes.GetResAnmCameraNumEntries();
}

u32 AnmScnRes::GetLightSetMaxRefNumber() const {
    return mRes.ref().info.numResLightSetData;
}

u32 AnmScnRes::GetAmbLightMaxRefNumber() const {
    return mRes.ref().info.numResAnmAmbLightData;
}

u32 AnmScnRes::GetDiffuseLightMaxRefNumber() const {
    return mRes.ref().info.numResAnmLightData;
}

u32 AnmScnRes::GetFogMaxRefNumber() const {
    return mRes.ref().info.numResAnmFogData;
}

u32 AnmScnRes::GetCameraMaxRefNumber() const {
    return mRes.ref().info.numResAnmCameraData;
}

AnmScnRes* AnmScnRes::Construct(MEMAllocator* pAllocator, u32* pSize,
                                ResAnmScn scn, bool cache) {
    if (!scn.IsValid()) {
        return NULL;
    }

    u32 numAmbLight = cache ? scn.GetResAnmAmbLightNumEntries() : 0;
    u32 numLight = cache ? scn.GetResAnmLightNumEntries() : 0;
    u32 numFog = cache ? scn.GetResAnmFogNumEntries() : 0;
    u32 numCamera = cache ? scn.GetResAnmCameraNumEntries() : 0;

    u32 ambLightSize = numAmbLight * sizeof(AmbLightAnmResult);
    u32 lightSize = numLight * sizeof(LightAnmResult);
    u32 fogSize = numFog * sizeof(FogAnmResult);
    u32 cameraSize = numCamera * sizeof(CameraAnmResult);

    u32 lightOfs = align4(sizeof(AnmScnRes) + ambLightSize);
    u32 fogOfs = align4(lightOfs + lightSize);
    u32 cameraOfs = align4(fogOfs + fogSize);
    u32 size = align4(cameraOfs + cameraSize);

    if (pSize != NULL) {
        *pSize = size;
    }

    if (pAllocator == NULL) {
        return NULL;
    }

    u8* pBuffer = reinterpret_cast<u8*>(Alloc(pAllocator, size));
    if (pBuffer == NULL) {
        return NULL;
    }

    return new (pBuffer) AnmScnRes(
        pAllocator, scn,
        ambLightSize != 0 ? reinterpret_cast<AmbLightAnmResult*>(pBuffer + sizeof(AnmScnRes)) : NULL,
        lightSize != 0 ? reinterpret_cast<LightAnmResult*>(pBuffer + lightOfs) : NULL,
        fogSize != 0 ? reinterpret_cast<FogAnmResult*>(pBuffer + fogOfs) : NULL,
        cameraSize != 0 ? reinterpret_cast<CameraAnmResult*>(pBuffer + cameraOfs) : NULL);
}

AnmScnRes::AnmScnRes(MEMAllocator* pAllocator, ResAnmScn scn,
                     AmbLightAnmResult* pAmbLightCache,
                     LightAnmResult* pLightCache, FogAnmResult* pFogCache,
                     CameraAnmResult* pCameraCache)
    : AnmScn(pAllocator),
      FrameCtrl(0.0f, scn.GetNumFrame(), GetAnmPlayPolicy(scn.GetAnmPolicy())),
      mRes(scn),
      mFlags(0),
      mpAmbLightCache(pAmbLightCache),
      mpLightCache(pLightCache),
      mpFogCache(pFogCache),
      mpCameraCache(pCameraCache) {

    if (pAmbLightCache != NULL || pLightCache != NULL || pFogCache != NULL ||
        pCameraCache != NULL) {
        mFlags |= FLAG_CACHE;
    }

    if (mFlags & FLAG_CACHE) {
        UpdateCache();
    }
}

AnmScnRes::~AnmScnRes() {}

void AnmScnRes::SetFrame(f32 frame) {
    SetFrm(frame);

    if (mFlags & FLAG_CACHE) {
        UpdateCache();
    }
}

f32 AnmScnRes::GetFrame() const {
    return GetFrm();
}

void AnmScnRes::SetUpdateRate(f32 rate) {
    SetRate(rate);

    if (rate == 0.0f && (mFlags & FLAG_CACHE)) {
        UpdateCache();
    }
}

f32 AnmScnRes::GetUpdateRate() const {
    return GetRate();
}

void AnmScnRes::UpdateFrame() {
    if (GetRate() != 0.0f) {
        UpdateFrm();

        if (mFlags & FLAG_CACHE) {
            UpdateCache();
        }
    }
}

void AnmScnRes::G3dProc(u32 task, u32 /* param */, void* pInfo) {
    switch (task) {
    case G3DPROC_UPDATEFRAME: {
        UpdateFrame();
        break;
    }

    case G3DPROC_DETACH_PARENT: {
        SetParent(NULL);
        break;
    }

    case G3DPROC_ATTACH_PARENT: {
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    }

    default: {
        break;
    }
    }
}

bool AnmScnRes::GetLightSet(LightSet set, u32 refNumber) {
    ResLightSet resSet = mRes.GetResLightSetByRefNumber(refNumber);

    if (!resSet.IsValid()) {
        return false;
    }

    u32 specIdx;
    u32 i;
    u32 numLight;

    numLight = resSet.GetNumLight();
    specIdx = G3DState::NUM_LIGHT_IN_LIGHT_SET - 1;
    i = 0;

    if (numLight > 0) {
        for (; i < numLight; i++) {
            if (resSet.ref().lightId[i] != ResLightSetData::INVALID_ID) {
                ResAnmLight light = mRes.GetResAnmLight(static_cast<u32>(resSet.ref().lightId[i]));

                set.SelectLightObj(i, light.GetRefNumber());

                if (light.HasSpecularLight()) {
                    set.SelectLightObj(specIdx--, light.GetSpecularLightIdx());
                }
            } else {
                set.SelectLightObj(i, -1);
            }
        }
    }

    // the slots between the diffuse lights and the specular ones stay empty
    for (; numLight <= specIdx; numLight++) {
        set.SelectLightObj(numLight, -1);
    }

    if (resSet.HasAmbLight()) {
        ResAnmAmbLight ambLight = mRes.GetResAnmAmbLight(resSet.GetAmbLightID());
        set.SelectAmbLightObj(ambLight.GetRefNumber());
    } else {
        set.SelectAmbLightObj(-1);
    }

    return true;
}

ut::Color AnmScnRes::GetAmbLightColor(u32 refNumber) {
    AmbLightAnmResult result;
    const AmbLightAnmResult* pResult = GetAmbLightResult(&result, refNumber);

    u32 color = pResult->color;

    if (!(pResult->flags & AmbLightAnmResult::FLAG_COLOR_ENABLE)) {
        color |= 0xFFFFFF00;
    }

    if (!(pResult->flags & AmbLightAnmResult::FLAG_ALPHA_ENABLE)) {
        color |= 0x000000FF;
    }

    return color;
}

void AnmScnRes::GetLight(LightObj* pDiff, LightObj* pSpec, u32 refNumber) {
    LightAnmResult result;
    const LightAnmResult* pResult = GetLightResult(&result, refNumber);

    if (pResult->flags & LightAnmResult::FLAG_LIGHT_ENABLE) {
        if (pDiff != NULL) {
            MakeDiffuseLightObj(pDiff, pResult);
        }

        if (pSpec != NULL) {
            if (pResult->flags & LightAnmResult::FLAG_SPECULAR_ENABLE) {
                math::VEC3 dir;
                math::VEC3Sub(&dir, &pResult->aim, &pResult->pos);
                math::VEC3Normalize(&dir, &dir);

                pSpec->Clear();
                pSpec->InitLightColor(pResult->specColor);
                pSpec->InitSpecularDir(dir.x, dir.y, dir.z);
                pSpec->InitLightShininess(pResult->shininess);

                if (!(pResult->flags & LightAnmResult::FLAG_COLOR_ENABLE)) {
                    pSpec->DisableColor();
                }

                if (!(pResult->flags & LightAnmResult::FLAG_ALPHA_ENABLE)) {
                    pSpec->DisableAlpha();
                }

                pSpec->Enable();
            } else {
                pSpec->Disable();
            }
        }
    } else {
        if (pDiff != NULL) {
            pDiff->Disable();
        }

        if (pSpec != NULL) {
            pSpec->Disable();
        }
    }
}

void AnmScnRes::GetFog(Fog fog, u32 refNumber) {
    FogAnmResult result;
    const FogAnmResult* pResult = GetFogResult(&result, refNumber);

    fog.SetFogType(pResult->type);
    fog.SetZ(pResult->startz, pResult->endz);
    fog.SetFogColor(pResult->color);
}

bool AnmScnRes::GetCamera(Camera camera, u32 refNumber) {
    CameraAnmResult result;
    const CameraAnmResult* pResult = GetCameraResult(&result, refNumber);

    if (!(pResult->flags & CameraAnmResult::FLAG_ANM_EXISTS)) {
        return false;
    }

    switch (pResult->projType) {
    case GX_PERSPECTIVE: {
        camera.SetPerspective(pResult->perspFovy, pResult->aspect,
                              pResult->near, pResult->far);
        break;
    }

    case GX_ORTHOGRAPHIC: {
        f32 top = pResult->orthoHeight / 2.0f;
        f32 right = top * pResult->aspect;

        camera.SetOrtho(top, -top, -right, right, pResult->near, pResult->far);
        break;
    }

    default: {
        break;
    }
    }

    Camera::PostureInfo info;

    switch (pResult->flags & CameraAnmResult::FLAG_CAMERA_TYPE_MASK) {
    case CameraAnmResult::CAMERATYPE_ROTATE: {
        info.tp = Camera::POSTURE_ROTATE;
        info.cameraRotate = pResult->rotate.rot;
        break;
    }

    case CameraAnmResult::CAMERATYPE_AIM: {
        info.tp = Camera::POSTURE_AIM;
        info.cameraTarget = pResult->aim.aim;
        info.cameraTwist = pResult->aim.twist;
        break;
    }

    default: {
        break;
    }
    }

    camera.SetPosition(pResult->pos);
    camera.SetPosture(info);

    return true;
}

AmbLightAnmResult* AnmScnRes::GetAmbLightResult(AmbLightAnmResult* pResult,
                                                u32 refNumber) {
    ResAnmAmbLight ambLight = mRes.GetResAnmAmbLightByRefNumber(refNumber);

    if (!ambLight.IsValid()) {
        pResult->flags = 0;
        pResult->color = 0xFFFFFFFF;
        return pResult;
    }

    if (mpAmbLightCache != NULL) {
        return &mpAmbLightCache[ambLight.GetID()];
    }

    ambLight.GetAnmResult(pResult, GetFrm());
    return pResult;
}

LightAnmResult* AnmScnRes::GetLightResult(LightAnmResult* pResult,
                                          u32 refNumber) {
    ResAnmLight light = mRes.GetResAnmLightByRefNumber(refNumber);

    if (!light.IsValid()) {
        pResult->flags = 0;
        return pResult;
    }

    if (mpLightCache != NULL) {
        return &mpLightCache[light.GetID()];
    }

    light.GetAnmResult(pResult, GetFrm());
    return pResult;
}

FogAnmResult* AnmScnRes::GetFogResult(FogAnmResult* pResult, u32 refNumber) {
    ResAnmFog fog = mRes.GetResAnmFogByRefNumber(refNumber);

    if (!fog.IsValid()) {
        pResult->type = GX_FOG_NONE;
        return pResult;
    }

    if (mpFogCache != NULL) {
        return &mpFogCache[fog.GetID()];
    }

    fog.GetAnmResult(pResult, GetFrm());
    return pResult;
}

CameraAnmResult* AnmScnRes::GetCameraResult(CameraAnmResult* pResult,
                                            u32 refNumber) {
    ResAnmCamera camera = mRes.GetResAnmCameraByRefNumber(refNumber);

    if (!camera.IsValid()) {
        pResult->flags = 0;
        return pResult;
    }

    if (mpCameraCache != NULL) {
        return &mpCameraCache[camera.GetID()];
    }

    camera.GetAnmResult(pResult, GetFrm());
    return pResult;
}

u32 AnmScnRes::GetSpecularLightID(u32 refNumber) const {
    ResAnmLight light = mRes.GetResAnmLightByRefNumber(refNumber);

    if (light.IsValid()) {
        return light.GetSpecularLightIdx();
    }

    return -1;
}

bool AnmScnRes::HasSpecularLight(u32 refNumber) const {
    ResAnmLight light = mRes.GetResAnmLightByRefNumber(refNumber);

    return light.IsValid() && light.HasSpecularLight();
}

void AnmScnRes::UpdateCache() {
    u32 numAmbLight = mRes.GetResAnmAmbLightNumEntries();
    u32 numLight = mRes.GetResAnmLightNumEntries();
    u32 numFog = mRes.GetResAnmFogNumEntries();
    u32 numCamera = mRes.GetResAnmCameraNumEntries();

    f32 frame = GetFrm();

    for (u32 i = 0; i < numAmbLight; i++) {
        mRes.GetResAnmAmbLight(i).GetAnmResult(&mpAmbLightCache[i], frame);
    }

    for (u32 i = 0; i < numLight; i++) {
        mRes.GetResAnmLight(i).GetAnmResult(&mpLightCache[i], frame);
    }

    for (u32 i = 0; i < numFog; i++) {
        mRes.GetResAnmFog(i).GetAnmResult(&mpFogCache[i], frame);
    }

    for (u32 i = 0; i < numCamera; i++) {
        mRes.GetResAnmCamera(i).GetAnmResult(&mpCameraCache[i], frame);
    }
}

} // namespace g3d
} // namespace nw4r
