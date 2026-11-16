#include <nw4hbm/lyt.h>

namespace nw4hbm {
namespace lyt {

NW4R_UT_RTTI_DEF_DERIVED(Bounding, Pane);

Bounding::Bounding(const res::Bounding* pRes, const ResBlockSet& rBlockSet)
    : Pane(pRes) {

#pragma unused(rBlockSet)
}

Bounding::~Bounding() {}

void Bounding::DrawSelf(const DrawInfo& rInfo) {
    if (rInfo.IsDebugDrawMode()) {
        LoadMtx(rInfo);
        detail::DrawLine(GetVtxPos(), mSize, ut::Color(0x00FF00FF));
    }
}

} // namespace lyt
} // namespace nw4hbm
