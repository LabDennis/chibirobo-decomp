#include "nw4r/ef.h"
#include "nw4r/math.h"

#include "nw4r/ef/ef_gx.h"

#include <algorithm>
#include <limits>

namespace nw4r {
namespace ef {

static void DrawPoint(const math::VEC3& rPos, bool texCoord);

DrawPointStrategy::DrawPointStrategy() {}

void DrawPointStrategy::Draw(const DrawInfo& rInfo, ParticleManager* pManager) {
    InitGraphics(rInfo, pManager);

    bool texCoord;

    const EmitterDrawSetting& rSetting =
        *pManager->mResource->GetEmitterDrawSetting();

    texCoord = mNumTexmap > 0;

    math::MTX34 glbMtx;
    math::MTX34 posMtx;

    pManager->CalcGlobalMtx(&glbMtx);
    rInfo.GetViewMtxZOffset(&posMtx);
    math::MTX34Mult(&posMtx, &posMtx, &glbMtx);
    GXLoadPosMtxImm(posMtx, GX_PNMTX0);

    GetFirstDrawParticleFunc pGetFirstFunc = GetGetFirstDrawParticleFunc(
        rSetting.mFlags & EmitterDrawSetting::FLAG_DRAW_ORDER);

    GetNextDrawParticleFunc pGetNextFunc = GetGetNextDrawParticleFunc(
        rSetting.mFlags & EmitterDrawSetting::FLAG_DRAW_ORDER);

    u8 prevPointSize = 0;
    bool first = true;

    for (Particle* pParticle = pGetFirstFunc(pManager); pParticle != NULL;
         pParticle = pGetNextFunc(pManager, pParticle)) {

        f32 sx = pParticle->Draw_GetSizeX();
        if (sx < NW4R_MATH_FLT_EPSILON) {
            continue;
        }

        u8 pointSize = static_cast<u8>(6.0f * std::min(42.5f, sx));
        if (pointSize == 0) {
            continue;
        }

        if (prevPointSize != pointSize) {
            prevPointSize = pointSize;
            GXSetPointSize(pointSize, GX_TO_ONE);

            SetupGP(pParticle, rSetting, rInfo, first, true);
        } else {
            SetupGP(pParticle, rSetting, rInfo, first, false);
        }

        first = false;
        DrawPoint(pParticle->mParameter.mPosition, texCoord);
    }
}

static void DrawPoint(const math::VEC3& rPos, bool texCoord) {
    GXBegin(GX_POINTS, GX_VTXFMT0, 1);
    {
        GXPosition(rPos);

        if (texCoord) {
            GXTexCoord2f32(0.0f, 0.0f);
        }
    }
    GXEnd();
}

} // namespace ef
} // namespace nw4r
