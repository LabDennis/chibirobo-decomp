#ifndef NW4R_EF_DRAW_BILLBOARD_STRATEGY_H
#define NW4R_EF_DRAW_BILLBOARD_STRATEGY_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/drawstrategy/ef_drawstrategyimpl.h"

namespace nw4r {
namespace ef {

class DrawBillboardStrategy : public DrawStrategyImpl {
public:
    DrawBillboardStrategy();

    virtual void Draw(const DrawInfo& rInfo,
                      ParticleManager* pManager); // at 0xC

    virtual CalcAheadFunc
    GetCalcAheadFunc(ParticleManager* pManager); // at 0x18

    void InitGraphics(const DrawInfo& rInfo, ParticleManager* pManager);

private:
    void DrawNormalBillboard(const DrawInfo& rInfo, ParticleManager* pManager);

    // The parameters are register variables: the path without a roll reads
    // them from its paired-single assembly
    inline void DispParticle_Normal(register Particle* pParticle,
                                    register const math::MTX34& rMtx,
                                    register f32 vx, register f32 vy, f32 vz,
                                    register f32 rc, register f32 rs,
                                    register f32 sx, register f32 sy,
                                    const math::VEC2& rPivot, bool texCoord);

    void DrawYBillboard(const DrawInfo& rInfo, ParticleManager* pManager);

    inline void DispParticle_YBillboard(Particle* pParticle,
                                        const math::MTX34& rMtx, f32 vx,
                                        f32 vy, f32 vz, f32 rc, f32 rs, f32 sx,
                                        f32 sy, const math::VEC2& rPivot,
                                        bool texCoord);

    void DrawDirectionalBillboard(const DrawInfo& rInfo,
                                  ParticleManager* pManager);

    inline void DispParticle_Directional(Particle* pParticle,
                                         const math::MTX34& rMtx, f32 vx,
                                         f32 vy, f32 ratio, f32 rc, f32 rs,
                                         f32 sx, f32 sy,
                                         const math::VEC2& rPivot,
                                         bool texCoord);

    inline void DispPolygon(const math::VEC3& rP, const math::VEC3& rD1,
                            const math::VEC3& rD2, bool texCoord);
};

} // namespace ef
} // namespace nw4r

#endif
