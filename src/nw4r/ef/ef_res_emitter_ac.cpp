#include "nw4r/ef.h"

namespace nw4r {
namespace ef {

ResEmitterDesc ResEmitter::GetEmitterDesc() {
    return ResEmitterDesc(ptr()->GetEmitterDesc());
}

ResParticleParameterDesc ResEmitter::GetParticleParameterDesc() {
    return ResParticleParameterDesc(ptr()->GetParticleParameterDesc());
}

ResAnimCurve ResEmitter::GetPtclTrack(u16 idx) {
    return ResAnimCurve(ptr()->GetPtclTrack(idx));
}

u16 ResEmitterDesc::GetEmitLife() {
    return ptr()->emitLife;
}

void ResParticleParameterDesc::SetColor(int layer, int idx,
                                        const GXColor& rColor) {
    ptr()->mColor[layer][idx] = rColor;
}

} // namespace ef
} // namespace nw4r
