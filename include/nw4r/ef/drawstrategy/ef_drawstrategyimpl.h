#ifndef NW4R_EF_DRAW_STRATEGY_DRAW_STRATEGY_IMPL_H
#define NW4R_EF_DRAW_STRATEGY_DRAW_STRATEGY_IMPL_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/ef_drawstrategy.h"
#include "nw4r/ef/ef_particle.h"
#include "nw4r/ef/ef_particlemanager.h"
#include "nw4r/ef/ef_types.h"
#include "nw4r/math.h"

#include "nw4r/ef/ef_gx.h"

#include <limits>

namespace nw4r {
namespace ef {

// Forward declarations
class Particle;
class ParticleManager;
struct EmitterDrawSetting;
struct TextureData;

/******************************************************************************
 *
 * DrawStrategyImpl
 *
 ******************************************************************************/
class DrawStrategyImpl : public DrawStrategy {
public:
    typedef Particle* (*GetFirstDrawParticleFunc)(ParticleManager* pManager);
    typedef Particle* (*GetNextDrawParticleFunc)(ParticleManager* pManager,
                                                 Particle* pParticle);

    enum ParticleDrawOrder { DRAWORDER_YOUNGERS_FIRST, DRAWORDER_ELDERS_FIRST };

    struct AheadContext {
        struct {
            ParticleManager* mParticleManager;  // at 0x0
            const math::MTX34* mViewMtx;        // at 0x4
            math::MTX34 mEmitterMtx;            // at 0x8
            math::MTX34 mParticleManagerMtx;    // at 0x38
            math::MTX34 mParticleManagerMtxInv; // at 0x68
            math::VEC3 mEmitterAxisY;           // at 0x98
            math::VEC3 mEmitterCenter;          // at 0xA4
        } mCommon;                              // at 0x0

        struct {
            math::_VEC3 mWorldYAxis; // at 0x0
        } mNoDesign;                 // at 0xB0

        AheadContext(const math::MTX34& rViewMtx, ParticleManager* pManager);
    };

    typedef void (*CalcAheadFunc)(math::VEC3* pAxisY, AheadContext* pContext,
                                  Particle* pParticle);

    struct PrevTexture {
        const TextureData* data; // at 0x0
        f32 scaleS;              // at 0x4
        f32 scaleT;              // at 0x8
        f32 transS;              // at 0xC
        f32 transT;              // at 0x10
        GXTexWrapMode wrapS;     // at 0x14
        GXTexWrapMode wrapT;     // at 0x18
        math::VEC2 scale;        // at 0x1C
        f32 rotate;              // at 0x24
        math::VEC2 translate;    // at 0x28

        PrevTexture() {
            data = NULL;
            scaleS = 1.0f;
            scaleT = 1.0f;
            transS = 1.0f;
            transT = 1.0f;
            wrapS = GX_CLAMP;
            wrapT = GX_CLAMP;
            scale.x = 1.0f;
            scale.y = 1.0f;
            rotate = 0.0f;
            translate.x = 0.0f;
            translate.y = 0.0f;
        }
    };

public:
    DrawStrategyImpl();

    virtual GetFirstDrawParticleFunc
    GetGetFirstDrawParticleFunc(int drawOrder); // at 0x10

    virtual GetNextDrawParticleFunc
    GetGetNextDrawParticleFunc(int drawOrder); // at 0x14

protected:
    void InitGP(ParticleManager* pManager, const EmitterDrawSetting& rSetting,
                const DrawInfo& rInfo);
    void InitTexture(const EmitterDrawSetting& rSetting);
    void InitTev(const EmitterDrawSetting& rSetting, const DrawInfo& rInfo);
    void InitColor(ParticleManager* pManager,
                   const EmitterDrawSetting& rSetting, const DrawInfo& rInfo);

    void SetupGP(Particle* pParticle, const EmitterDrawSetting& rSetting,
                 const DrawInfo& rInfo, bool first, bool xfDirty);

    static void CalcAhead_Speed(math::VEC3* pAxisY, AheadContext* pContext,
                                Particle* pParticle) {

        pParticle->GetMoveDir(pAxisY);

        if (!Normalize(pAxisY)) {
            *pAxisY = pContext->mCommon.mEmitterAxisY;
        }
    }

    static void CalcAhead_EmitterCenter(math::VEC3* pAxisY,
                                        AheadContext* pContext,
                                        Particle* pParticle) {

        math::VEC3Sub(pAxisY, &pParticle->mParameter.mPosition,
                      &pContext->mCommon.mEmitterCenter);

        if (!Normalize(pAxisY)) {
            *pAxisY = pContext->mCommon.mEmitterAxisY;
        }
    }

    static void CalcAhead_EmitterDesign(math::VEC3* pAxisY,
                                        AheadContext* pContext,
                                        Particle* /* pParticle */) {

        *pAxisY = pContext->mCommon.mEmitterAxisY;
    }

    static void CalcAhead_Particle(math::VEC3* pAxisY, AheadContext* pContext,
                                   Particle* pParticle) {

        Particle* pElder =
            GetElderDrawParticle(pContext->mCommon.mParticleManager, pParticle);

        if (pElder != NULL) {
            math::VEC3Sub(pAxisY, &pElder->mParameter.mPosition,
                          &pParticle->mParameter.mPosition);
        } else {
            math::VEC3Sub(pAxisY, &pParticle->mParameter.mPosition,
                          &pContext->mCommon.mEmitterCenter);
        }

        if (!Normalize(pAxisY)) {
            *pAxisY = pContext->mCommon.mEmitterAxisY;
        }
    }

    static void CalcAhead_NoDesign(math::VEC3* pAxisY, AheadContext* pContext,
                                   Particle* /* pParticle */) {
        *pAxisY = math::VEC3(pContext->mNoDesign.mWorldYAxis);
    }

    static void CalcAhead_ParticleBoth(math::VEC3* pAxisY,
                                       AheadContext* pContext,
                                       Particle* pParticle) {

        Particle* pElder =
            GetElderDrawParticle(pContext->mCommon.mParticleManager, pParticle);

        Particle* pYounger = GetYoungerDrawParticle(
            pContext->mCommon.mParticleManager, pParticle);

        math::VEC3 elderPos(0.0f, 0.0f, 0.0f);

        if (pElder != NULL) {
            math::VEC3Sub(&elderPos, &pElder->mParameter.mPosition,
                          &pParticle->mParameter.mPosition);

            if (!Normalize(&elderPos)) {
                elderPos = math::VEC3(0.0f, 0.0f, 0.0f);
            }
        }

        math::VEC3 youngerPos(0.0f, 0.0f, 0.0f);

        if (pYounger != NULL) {
            math::VEC3Sub(&youngerPos, &pYounger->mParameter.mPosition,
                          &pParticle->mParameter.mPosition);

            if (!Normalize(&youngerPos)) {
                youngerPos = math::VEC3(0.0f, 0.0f, 0.0f);
            }
        }

        math::VEC3Sub(pAxisY, &elderPos, &youngerPos);

        if (!Normalize(pAxisY)) {
            *pAxisY = pContext->mCommon.mEmitterAxisY;
        }
    }

    // Wii Play: reads ActivityList::mNumActive (+0x50), not the active list's count
    int GetNumDrawParticle(ParticleManager* pManager) {
        return pManager->mActivityList.mNumActive;
    }

    static Particle* GetElderParticle(ParticleManager* pManager,
                                      Particle* pParticle) {

        return static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(pManager->mActivityList.mActiveList,
                                  pParticle)
                ->prevObject);
    }
    static Particle* GetYoungerParticle(ParticleManager* pManager,
                                        Particle* pParticle) {

        return static_cast<Particle*>(
            NW4R_UT_LIST_GET_LINK(pManager->mActivityList.mActiveList,
                                  pParticle)
                ->nextObject);
    }

    static Particle* GetOldestParticle(ParticleManager* pManager) {
        return static_cast<Particle*>(
            pManager->mActivityList.mActiveList.headObject);
    }
    static Particle* GetYoungestParticle(ParticleManager* pManager) {
        return static_cast<Particle*>(
            pManager->mActivityList.mActiveList.tailObject);
    }

    static Particle* GetElderDrawParticle(ParticleManager* pManager,
                                          Particle* pParticle) {

        Particle* pIt = GetElderParticle(pManager, pParticle);

        while (pIt != NULL &&
               pIt->GetLifeStatus() != ReferencedObject::NW4R_EF_LS_ACTIVE) {

            pIt = GetElderParticle(pManager, pIt);
        }

        return pIt;
    }
    static Particle* GetYoungerDrawParticle(ParticleManager* pManager,
                                            Particle* pParticle) {

        Particle* pIt = GetYoungerParticle(pManager, pParticle);

        while (pIt != NULL &&
               pIt->GetLifeStatus() != ReferencedObject::NW4R_EF_LS_ACTIVE) {

            pIt = GetYoungerParticle(pManager, pIt);
        }

        return pIt;
    }

    static Particle* GetOldestDrawParticle(ParticleManager* pManager) {
        Particle* pIt = GetOldestParticle(pManager);

        while (pIt != NULL &&
               pIt->GetLifeStatus() != ReferencedObject::NW4R_EF_LS_ACTIVE) {

            pIt = GetYoungerParticle(pManager, pIt);
        }

        return pIt;
    }
    static Particle* GetYoungestDrawParticle(ParticleManager* pManager) {
        Particle* pIt = GetYoungestParticle(pManager);

        while (pIt != NULL &&
               pIt->GetLifeStatus() != ReferencedObject::NW4R_EF_LS_ACTIVE) {

            pIt = GetElderDrawParticle(pManager, pIt);
        }

        return pIt;
    }

    static bool Normalize(math::VEC3* pVec) {
        f32 sqMag = math::VEC3Dot(pVec, pVec);

        if (sqMag < NW4R_MATH_FLT_EPSILON) {
            return false;
        }

        *pVec *= math::FrSqrt(sqMag);
        return true;
    }

    static const math::VEC3& GetXUnitVec() {
        return mXUnitVec;
    }
    static const math::VEC3& GetYUnitVec() {
        return mYUnitVec;
    }
    static const math::VEC3& GetZUnitVec() {
        return mZUnitVec;
    }

private:
    static void CheckUseColor(bool use[COLOR_LAYER_MAX][COLOR_IDX_MAX],
                              u8 type) {
        switch (type) {
        case ColorInput::TEVCOLOR1_1: {
            use[COLOR_LAYER_0][COLOR_IDX_PRI] = true;
            break;
        }

        case ColorInput::TEVCOLOR1_2: {
            use[COLOR_LAYER_0][COLOR_IDX_SEC] = true;
            break;
        }

        case ColorInput::TEVCOLOR1_MULT: {
            use[COLOR_LAYER_0][COLOR_IDX_PRI] = true;
            use[COLOR_LAYER_0][COLOR_IDX_SEC] = true;
            break;
        }

        case ColorInput::TEVCOLOR2_3: {
            use[COLOR_LAYER_1][COLOR_IDX_PRI] = true;
            break;
        }

        case ColorInput::TEVCOLOR2_2: {
            use[COLOR_LAYER_1][COLOR_IDX_SEC] = true;
            break;
        }

        case ColorInput::TEVCOLOR2_MULT: {
            use[COLOR_LAYER_1][COLOR_IDX_PRI] = true;
            use[COLOR_LAYER_1][COLOR_IDX_SEC] = true;
            break;
        }

        default: {
            break;
        }
        }
    }

    void GetTevColor(GXColor* pColor, u8 type) {
        switch (type) {
        case ColorInput::TEVCOLOR1_1: {
            pColor->r = mColor[COLOR_LAYER_0][COLOR_IDX_PRI].r;
            pColor->g = mColor[COLOR_LAYER_0][COLOR_IDX_PRI].g;
            pColor->b = mColor[COLOR_LAYER_0][COLOR_IDX_PRI].b;
            break;
        }

        case ColorInput::TEVCOLOR1_2: {
            pColor->r = mColor[COLOR_LAYER_0][COLOR_IDX_SEC].r;
            pColor->g = mColor[COLOR_LAYER_0][COLOR_IDX_SEC].g;
            pColor->b = mColor[COLOR_LAYER_0][COLOR_IDX_SEC].b;
            break;
        }

        case ColorInput::TEVCOLOR1_MULT: {
            pColor->g = (mColor[COLOR_LAYER_0][COLOR_IDX_PRI].g *
                             mColor[COLOR_LAYER_0][COLOR_IDX_SEC].g +
                         128) >>
                        8;
            pColor->b = (mColor[COLOR_LAYER_0][COLOR_IDX_PRI].b *
                             mColor[COLOR_LAYER_0][COLOR_IDX_SEC].b +
                         128) >>
                        8;
            pColor->r = (mColor[COLOR_LAYER_0][COLOR_IDX_PRI].r *
                             mColor[COLOR_LAYER_0][COLOR_IDX_SEC].r +
                         128) >>
                        8;
            break;
        }

        case ColorInput::TEVCOLOR2_3: {
            pColor->r = mColor[COLOR_LAYER_1][COLOR_IDX_PRI].r;
            pColor->g = mColor[COLOR_LAYER_1][COLOR_IDX_PRI].g;
            pColor->b = mColor[COLOR_LAYER_1][COLOR_IDX_PRI].b;
            break;
        }

        case ColorInput::TEVCOLOR2_2: {
            pColor->r = mColor[COLOR_LAYER_1][COLOR_IDX_SEC].r;
            pColor->g = mColor[COLOR_LAYER_1][COLOR_IDX_SEC].g;
            pColor->b = mColor[COLOR_LAYER_1][COLOR_IDX_SEC].b;
            break;
        }

        case ColorInput::TEVCOLOR2_MULT: {
            pColor->g = (mColor[COLOR_LAYER_1][COLOR_IDX_PRI].g *
                             mColor[COLOR_LAYER_1][COLOR_IDX_SEC].g +
                         128) >>
                        8;
            pColor->b = (mColor[COLOR_LAYER_1][COLOR_IDX_PRI].b *
                             mColor[COLOR_LAYER_1][COLOR_IDX_SEC].b +
                         128) >>
                        8;
            pColor->r = (mColor[COLOR_LAYER_1][COLOR_IDX_PRI].r *
                             mColor[COLOR_LAYER_1][COLOR_IDX_SEC].r +
                         128) >>
                        8;
            break;
        }

        default: {
            break;
        }
        }
    }

    void GetTevAlpha(GXColor* pColor, u8 type) {
        switch (type) {
        case ColorInput::TEVCOLOR1_1: {
            pColor->a = mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a;
            break;
        }

        case ColorInput::TEVCOLOR1_2: {
            pColor->a = mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a;
            break;
        }

        case ColorInput::TEVCOLOR1_MULT: {
            pColor->a = (mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a *
                             mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a +
                         128) >>
                        8;
            break;
        }

        case ColorInput::TEVCOLOR2_3: {
            pColor->a = mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a;
            break;
        }

        case ColorInput::TEVCOLOR2_2: {
            pColor->a = mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a;
            break;
        }

        case ColorInput::TEVCOLOR2_MULT: {
            pColor->a = (mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a *
                             mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a +
                         128) >>
                        8;
            break;
        }

        default: {
            break;
        }
        }
    }

    bool _SetupACmp(Particle* pParticle, const EmitterDrawSetting& rSetting,
                    bool first);
    bool _SetupTevReg(Particle* pParticle, const EmitterDrawSetting& rSetting,
                      bool first);
    bool _SetupTexture(Particle* pParticle, const EmitterDrawSetting& rSetting,
                       const DrawInfo& rInfo, bool first);

    static Particle*
    GetFirstDrawParticle_EldersFirst(ParticleManager* pManager);

    static Particle*
    GetFirstDrawParticle_YoungersFirst(ParticleManager* pManager);

    static Particle* GetNextDrawParticle_EldersFirst(ParticleManager* pManager,
                                                     Particle* pParticle);

    static Particle*
    GetNextDrawParticle_YoungersFirst(ParticleManager* pManager,
                                      Particle* pParticle);

protected:
    PrevTexture mPrevTexture[TEX_LAYER_MAX];            // at 0x4
    GXColor mPrevTevColor[GX_MAX_TEVREG - 1];           // at 0x94
    GXColor mPrevTevKColor[GX_MAX_KCOLOR];              // at 0xA0
    int mPrevARef0;                                     // at 0xB0
    int mPrevARef1;                                     // at 0xB4
    bool mUseColor[COLOR_LAYER_MAX][COLOR_IDX_MAX];     // at 0xB8
    bool mUseAlpha[COLOR_LAYER_MAX][COLOR_IDX_MAX];     // at 0xBC
    GXColor mColor[COLOR_LAYER_MAX][COLOR_IDX_MAX];     // at 0xC0
    u8 mNumTexmap;                                      // at 0xD0
    int mTexmapMap[TEX_LAYER_MAX];                      // at 0xD4

    static const math::VEC3 mXUnitVec;
    static const math::VEC3 mYUnitVec;
    static const math::VEC3 mZUnitVec;

    static const math::VEC3 mZeroVec;
    static const math::MTX34 mIdentityMtx;
};

/******************************************************************************
 *
 * Utility functions
 *
 ******************************************************************************/
static inline void GXPosition(const math::VEC3& rPos) {
    GXPosition3f32(rPos.x, rPos.y, rPos.z);
}

} // namespace ef
} // namespace nw4r

#endif
