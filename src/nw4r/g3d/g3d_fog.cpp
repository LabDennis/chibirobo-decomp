#include <nw4r/g3d.h>

#include "revolution/gx.h"
#include "revolution/gx/GXHardwareFields.h"

namespace nw4r {
namespace g3d {

Fog::Fog(FogData* pData) : ResCommon(pData) {}

void Fog::Init() {
    if (!IsValid()) {
        return;
    }

    FogData& r = ref();

    r.type = GX_FOG_NONE;

    r.startz = 0.0f;
    r.endz = 0.0f;
    r.nearz = 0.0f;
    r.farz = 0.0f;

    r.color.r = r.color.g = r.color.b = r.color.a = 0;

    r.adjEnable = FALSE;
    r.PADDING_0x19 = 0;

    r.adjCenter = 0;
    for (int i = 0; i < GX_FOG_ADJ_TABLE_SZ; i++) {
        r.adjTable.fogVals[i] = 0;
    }
}

// NONMATCHING: retail copies FogData with six inline-asm lfd/stfd pairs; kept as struct copy here.
Fog Fog::CopyTo(void* pDst) const {
    if (pDst != NULL && IsValid()) {
        FogData* pFogDst = static_cast<FogData*>(pDst);
        *pFogDst = *ptr();
        return Fog(pFogDst);
    }

    return Fog(NULL);
}

void Fog::GetFog(GXFogType* type, f32* startz, f32* endz,
                f32* nearz, f32* farz, GXColor* color) {
    if (IsValid()) {
        FogData& r = ref();

        if (type != NULL) {
            *type = r.type;
        }

        if (startz != NULL) {
            *startz = r.startz;
        }

        if (endz != NULL) {
            *endz = r.endz;
        }

        if (nearz != NULL) {
            *nearz = r.nearz;
        }

        if (farz != NULL) {
            *farz = r.farz;
        }

        if (color != NULL) {
            *color = r.color;
        }
    }
}

void Fog::SetFogRangeAdjParam(u16 width, u16 center,
                              const math::MTX44& rProjMtx) {
    if (!IsValid()) {
        return;
    }

    FogData& r = ref();

    r.adjCenter = center;
    GXInitFogAdjTable(&r.adjTable, width, rProjMtx);
}

void Fog::SetGP() const {
    if (!IsValid()) {
        return;
    }

    const FogData& r = ref();

    if (r.type != GX_FOG_NONE) {
        GXSetFogRangeAdj(r.adjEnable, r.adjCenter, const_cast<GXFogAdjTable*>(&r.adjTable));
    }

    GXSetFog(r.type, r.startz, r.endz, r.nearz, r.farz, r.color);
}

} // namespace g3d
} // namespace nw4r
