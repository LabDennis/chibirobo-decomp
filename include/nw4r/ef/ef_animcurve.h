#ifndef NW4R_EF_ANIM_CURVE_H
#define NW4R_EF_ANIM_CURVE_H
#include "nw4r/types_nw4r.h"

#include "nw4r/math.h"

namespace nw4r {
namespace ef {

// Forward declarations
class Particle;
struct AnimCurvePostFieldInfo;
struct AnimCurvePostFieldParam;
struct EmitterResource;

void AnimCurveExecuteU8(u8* pCmdList, u8* pTarget, u32 tick, u16 seed,
                        u32 life);

void AnimCurveExecuteF32(u8* pCmdList, f32* pTarget, u32 tick, u16 seed,
                         u32 life);

void AnimCurveExecuteRotate(u8* pCmdList, f32* pTarget, u32 tick, u16 seed,
                            u32 life);

void AnimCurveExecuteBakedU8(u8* pCmdList, u8* pTarget, u32 tick, u32 life);

void AnimCurveExecuteBakedF32(u8* pCmdList, f32* pTarget, u32 tick, u32 life);

void AnimCurveExecuteTexture(u8* pCmdList, Particle* pParticle, u32 tick,
                             u16 seed, u32 life);

void AnimCurveExecuteChild(u8* pCmdList, Particle* pParticle, u32 tick,
                           u16 seed, u32 life);

// 0x8024172C: collides the particle's move with the post field. TRUE when the
// velocity in *pVel is to be taken over, *pRetire when the particle dies
BOOL AnimCurveExecutePostField(Particle* pParticle,
                               AnimCurvePostFieldParam* pParam,
                               AnimCurvePostFieldInfo* pInfo,
                               EmitterResource* pChild,
                               const math::MTX34* pEmToGlb,
                               const math::MTX34* pGlbToLoc,
                               const math::VEC3* pPrevPos, math::VEC3 addPos,
                               math::VEC3* pVel, bool* pRetire);

} // namespace ef
} // namespace nw4r

#endif
