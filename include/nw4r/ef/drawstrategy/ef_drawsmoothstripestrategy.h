#ifndef NW4R_EF_DRAW_STRATEGY_DRAW_SMOOTH_STRIPE_STRATEGY_H
#define NW4R_EF_DRAW_STRATEGY_DRAW_SMOOTH_STRIPE_STRATEGY_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/drawstrategy/ef_drawstrategyimpl.h"

#include "nw4r/math.h"

namespace nw4r {
namespace ef {

// Forward declarations
class Particle;

class DrawSmoothStripeStrategy : public DrawStrategyImpl {
public:
    // Bits of the `flags` argument the draw functions pass on
    enum DrawFlag {
        FLAG_TEXCOORD = 1 << 0,     // send a texture coordinate per vertex
        FLAG_BILLBOARD = 1 << 3,    // face the screen (ASSIST_ST_BILLBOARD)
        FLAG_ELDERS_FIRST = 1 << 4, // draw order of the emitter
    };

    struct Trigonometric {
        f32 mCos; // at 0x0
        f32 mSin; // at 0x4
    };

    struct Vertex {
        math::VEC3 mVertex0; // at 0x0
        math::VEC3 mVertex1; // at 0xC
        f32 mTexCoord;       // at 0x18

        Vertex();

        Vertex(const math::VEC3& rVertex0, const math::VEC3& rVertex1,
               f32 texCoord)
            : mVertex0(rVertex0), mVertex1(rVertex1), mTexCoord(texCoord) {}
    };

    struct VertexTube {
        math::VEC3 mCenter; // at 0x0
        math::VEC3 mX;      // at 0xC
        math::VEC3 mZ;      // at 0x18
        f32 mTexCoord;      // at 0x24

        VertexTube() {}

        VertexTube(const math::VEC3& rCenter, const math::VEC3& rX,
                   const math::VEC3& rZ, f32 texCoord)
            : mCenter(rCenter), mX(rX), mZ(rZ), mTexCoord(texCoord) {}
    };

    struct AheadContextStripe : public AheadContext {
        math::VEC3 mEmitterAxisX;      // at 0xBC
        math::VEC3 mScreenAxisZ;       // at 0xC8
        Trigonometric* mTrigonometric; // at 0xD4

        AheadContextStripe(const math::MTX34& rViewMtx,
                           ParticleManager* pManager)
            : AheadContext(rViewMtx, pManager) {

            math::VEC3 axisX(mCommon.mEmitterMtx._00, mCommon.mEmitterMtx._10,
                             mCommon.mEmitterMtx._20);

            math::VEC3TransformNormal(&mEmitterAxisX,
                                      &mCommon.mParticleManagerMtxInv, &axisX);
        }
    };

    typedef void (*CalcAheadFuncStripe)(math::VEC3* pAxisY,
                                        AheadContextStripe* pContext,
                                        Particle* pParticle);

public:
    DrawSmoothStripeStrategy();

    virtual void Draw(const DrawInfo& rInfo,
                      ParticleManager* pManager); // at 0xC

    virtual CalcAheadFunc
    GetCalcAheadFunc(ParticleManager* pManager); // at 0x18

private:
    void InitGraphics(const DrawInfo& rInfo, ParticleManager* pManager) {
        const EmitterDrawSetting& rSetting =
            *pManager->mResource->GetEmitterDrawSetting();

        InitGP(pManager, rSetting, rInfo);

        GXEnableTexOffsets(GX_TEXCOORD0, TRUE, TRUE);

        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);

        if (mNumTexmap > 0) {
            GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        }

        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        GXSetCurrentMtx(GX_PNMTX0);
    }

    int GetDrawOrder(const EmitterDrawSetting& rSetting) const {
        return rSetting.mFlags & EmitterDrawSetting::FLAG_DRAW_ORDER;
    }

    int GetStripeTexmapType(const EmitterDrawSetting& rSetting) const {
        return rSetting.typeOption2 & 0xC0;
    }

    void CalcVertex(Vertex* pVertex, AheadContextStripe* pContext, int flags,
                    Particle* pParticle, CalcAheadFuncStripe pCalcAheadFunc,
                    const math::VEC3& rPos, const math::VEC3& rP0,
                    const math::VEC3& rP1, f32 pivot, f32 texCoord);

    void CalcVertexTube(VertexTube* pVertex, AheadContextStripe* pContext,
                        int flags, Particle* pParticle,
                        CalcAheadFuncStripe pCalcAheadFunc, f32 pivotX,
                        f32 pivotY, f32 texCoord);

    void DrawSection_BSpline(int numDiv, const Vertex& rV0, const Vertex& rV1,
                             const Vertex& rV2, int flags);

    void DrawSectionEnd_BSpline(const Vertex& rV0, const Vertex& rV1,
                                const Vertex& rV2, int flags);

    void DrawSectionTube_BSpline(int numDiv, int numVertex,
                                 const VertexTube& rV0, const VertexTube& rV1,
                                 const VertexTube& rV2, int flags,
                                 const Trigonometric* pTrigonometric);

    void DrawSmoothStripe_BSpline_ConnectNone(AheadContextStripe* pContext,
                                              int numDiv, int flags,
                                              const math::VEC3& rP0,
                                              const math::VEC3& rP1);
    void DrawSmoothStripe_BSpline_ConnectRing(AheadContextStripe* pContext,
                                              int numDiv, int flags,
                                              const math::VEC3& rP0,
                                              const math::VEC3& rP1);
    void DrawSmoothStripe_BSpline_ConnectEmitter(AheadContextStripe* pContext,
                                                 int numDiv, int flags,
                                                 const math::VEC3& rP0,
                                                 const math::VEC3& rP1);

    void DrawSmoothStripe_BSpline(AheadContextStripe* pContext, int numDiv,
                                  int flags, const math::VEC3& rP0,
                                  const math::VEC3& rP1) {
        switch (pContext->mCommon.mParticleManager->mResource
                    ->GetEmitterDrawSetting()
                    ->typeOption2 &
                7) {
        case 0:
        default: {
            DrawSmoothStripe_BSpline_ConnectNone(pContext, numDiv, flags, rP0,
                                                 rP1);
            break;
        }

        case 1: {
            DrawSmoothStripe_BSpline_ConnectRing(pContext, numDiv, flags, rP0,
                                                 rP1);
            break;
        }

        case 2: {
            DrawSmoothStripe_BSpline_ConnectEmitter(pContext, numDiv, flags,
                                                    rP0, rP1);
            break;
        }
        }
    }

    void DrawSmoothStripeTube_BSpline_ConnectNone(AheadContextStripe* pContext,
                                                  int numDiv, int flags);
    void DrawSmoothStripeTube_BSpline_ConnectRing(AheadContextStripe* pContext,
                                                  int numDiv, int flags);
    void
    DrawSmoothStripeTube_BSpline_ConnectEmitter(AheadContextStripe* pContext,
                                                int numDiv, int flags);
    void DrawSmoothStripeTube(AheadContextStripe* pContext, int numDiv,
                              int flags, bool xfFlush);

    // Eight degenerate vertices, sent before a tube when the XF state may be stale
    void DrawDummy(int flags) {
        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);

        for (int i = 0; i < 8; i++) {
            GXPosition3f32(0.0f, 0.0f, 0.0f);

            if (flags & FLAG_TEXCOORD) {
                GXTexCoord2f32(0.0f, 0.0f);
            }
        }

        GXEnd();
    }

    math::VEC3 GetInitialPrevAxis(const EmitterDrawSetting& rSetting,
                                  const AheadContextStripe& rContext);

    static math::MTX34 CalcAxisMtx(AheadContextStripe* pContext, int flags,
                                   Particle* pParticle,
                                   const math::VEC3& rAxisY,
                                   const math::VEC3& rPos);

    static math::MTX34 CalcRotMtx(Particle* pParticle, f32 pivot);

    static void CalcAhead_Particle_Stripe(math::VEC3* pAxisY,
                                          AheadContextStripe* pContext,
                                          Particle* pParticle);
    static void CalcAhead_ParticleBoth_Stripe(math::VEC3* pAxisY,
                                              AheadContextStripe* pContext,
                                              Particle* pParticle);
    static void CalcAhead_ParticleBoth_Ring(math::VEC3* pAxisY,
                                            AheadContextStripe* pContext,
                                            Particle* pParticle);
    static void CalcAhead_ParticleBoth_Origin(math::VEC3* pAxisY,
                                              AheadContextStripe* pContext,
                                              Particle* pParticle);
};

} // namespace ef
} // namespace nw4r

#endif
