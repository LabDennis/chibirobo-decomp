#ifndef NW4R_EF_DRAW_STRATEGY_DRAW_LINE_STRATEGY_H
#define NW4R_EF_DRAW_STRATEGY_DRAW_LINE_STRATEGY_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ef/drawstrategy/ef_drawstrategyimpl.h"

namespace nw4r {
namespace ef {

class DrawLineStrategy : public DrawStrategyImpl {
public:
    DrawLineStrategy();

    virtual void Draw(const DrawInfo& rInfo,
                      ParticleManager* pManager); // at 0xC

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
};

} // namespace ef
} // namespace nw4r

#endif
