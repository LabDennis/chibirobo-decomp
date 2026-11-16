#include "nw4r/ef.h"

#include "nw4r/ef/ef_gx.h"

#include <cmath>

namespace nw4r {
namespace ef {

const math::VEC3 DrawStrategyImpl::mXUnitVec(1.0f, 0.0f, 0.0f);
const math::VEC3 DrawStrategyImpl::mYUnitVec(0.0f, 1.0f, 0.0f);
const math::VEC3 DrawStrategyImpl::mZUnitVec(0.0f, 0.0f, 1.0f);
const math::VEC3 DrawStrategyImpl::mZeroVec(0.0f, 0.0f, 0.0f);

// clang-format off
const math::MTX34 DrawStrategyImpl::mIdentityMtx(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f);
// clang-format on

/******************************************************************************
 *
 * DrawStrategyImpl
 *
 ******************************************************************************/
DrawStrategyImpl::DrawStrategyImpl() {}

inline void DrawStrategyImpl::InitTexture(const EmitterDrawSetting& rSetting) {
    mNumTexmap = 0;

    mTexmapMap[TEX_LAYER_1] = -1;
    mTexmapMap[TEX_LAYER_2] = -1;
    mTexmapMap[TEX_LAYER_IND] = -1;

    if (rSetting.mFlags & EmitterDrawSetting::FLAG_TEX1_ENABLE) {
        mTexmapMap[TEX_LAYER_1] = 0;
        mNumTexmap++;
    }

    if (rSetting.mFlags & EmitterDrawSetting::FLAG_TEX2_ENABLE) {
        mTexmapMap[TEX_LAYER_2] = mNumTexmap;
        mNumTexmap++;
    }

    if (rSetting.mFlags & EmitterDrawSetting::FLAG_TEXIND_ENABLE) {
        mTexmapMap[TEX_LAYER_IND] = mNumTexmap;
        mNumTexmap++;
    }

    GXSetNumTexGens(mNumTexmap);
}

void DrawStrategyImpl::InitGP(ParticleManager* pManager,
                              const EmitterDrawSetting& rSetting,
                              const DrawInfo& rInfo) {

    InitTexture(rSetting);
    InitTev(rSetting, rInfo);
    InitColor(pManager, rSetting, rInfo);
}

void DrawStrategyImpl::InitTev(const EmitterDrawSetting& rSetting,
                               const DrawInfo& rInfo) {

    int i;

    GXSetClipMode(rSetting.mFlags & EmitterDrawSetting::FLAG_CLIP_DISABLE
                      ? GX_CLIP_DISABLE
                      : GX_CLIP_ENABLE);

    GXSetNumTevStages(rSetting.mNumTevs);
    GXSetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE,
                          GX_CH_ALPHA);

    for (i = 0; i < rSetting.mNumTevs; i++) {
        GXSetTevDirect(static_cast<GXTevStageID>(i));

        GXSetTevColorIn(static_cast<GXTevStageID>(i),
                        static_cast<GXTevColorArg>(rSetting.mTevColor[i].mA),
                        static_cast<GXTevColorArg>(rSetting.mTevColor[i].mB),
                        static_cast<GXTevColorArg>(rSetting.mTevColor[i].mC),
                        static_cast<GXTevColorArg>(rSetting.mTevColor[i].mD));

        GXSetTevAlphaIn(static_cast<GXTevStageID>(i),
                        static_cast<GXTevAlphaArg>(rSetting.mTevAlpha[i].mA),
                        static_cast<GXTevAlphaArg>(rSetting.mTevAlpha[i].mB),
                        static_cast<GXTevAlphaArg>(rSetting.mTevAlpha[i].mC),
                        static_cast<GXTevAlphaArg>(rSetting.mTevAlpha[i].mD));

        GXSetTevColorOp(
            static_cast<GXTevStageID>(i),
            static_cast<GXTevOp>(rSetting.mTevColorOp[i].mOp),
            static_cast<GXTevBias>(rSetting.mTevColorOp[i].mBias),
            static_cast<GXTevScale>(rSetting.mTevColorOp[i].mScale),
            static_cast<GXBool>(rSetting.mTevColorOp[i].mClamp),
            static_cast<GXTevRegID>(rSetting.mTevColorOp[i].mOutReg));

        GXSetTevAlphaOp(
            static_cast<GXTevStageID>(i),
            static_cast<GXTevOp>(rSetting.mTevAlphaOp[i].mOp),
            static_cast<GXTevBias>(rSetting.mTevAlphaOp[i].mBias),
            static_cast<GXTevScale>(rSetting.mTevAlphaOp[i].mScale),
            static_cast<GXBool>(rSetting.mTevAlphaOp[i].mClamp),
            static_cast<GXTevRegID>(rSetting.mTevAlphaOp[i].mOutReg));

        GXSetTevKColorSel(
            static_cast<GXTevStageID>(i),
            static_cast<GXTevKColorSel>(rSetting.mTevKColorSel[i]));

        GXSetTevKAlphaSel(
            static_cast<GXTevStageID>(i),
            static_cast<GXTevKAlphaSel>(rSetting.mTevKAlphaSel[i]));

        GXSetTevSwapMode(static_cast<GXTevStageID>(i), GX_TEV_SWAP0,
                         GX_TEV_SWAP0);

        if (rSetting.mTevTexture[i] == TEX_LAYER_1) {
            GXSetTevOrder(static_cast<GXTevStageID>(i), GX_TEXCOORD0,
                          GX_TEXMAP0, GX_COLOR0A0);
        } else if (rSetting.mTevTexture[i] == TEX_LAYER_2) {
            GXSetTevOrder(static_cast<GXTevStageID>(i),
                          static_cast<GXTexCoordID>(mTexmapMap[TEX_LAYER_2]),
                          static_cast<GXTexMapID>(mTexmapMap[TEX_LAYER_2]),
                          GX_COLOR0A0);
        } else {
            GXSetTevOrder(static_cast<GXTevStageID>(i), GX_TEXCOORD_NULL,
                          GX_TEXMAP_NULL, GX_COLOR0A0);
        }
    }

    if (rSetting.mFlags & EmitterDrawSetting::FLAG_TEXIND_ENABLE) {
        GXSetNumIndStages(1);

        GXSetIndTexOrder(GX_IND_TEX_STAGE_0,
                         static_cast<GXTexCoordID>(mTexmapMap[TEX_LAYER_IND]),
                         static_cast<GXTexMapID>(mTexmapMap[TEX_LAYER_IND]));

        GXSetIndTexCoordScale(GX_IND_TEX_STAGE_0, GX_ITS_1, GX_ITS_1);
        GXSetIndTexMtx(GX_ITM_0, rSetting.mIndTexOffsetMtx,
                       rSetting.mIndTexScaleExp);

        u8 mask = 1;
        for (i = 0; i < rSetting.mNumTevs; i++, mask <<= 1) {
            if (rSetting.mIndirectTargetStage & mask) {
                GXSetTevIndirect(static_cast<GXTevStageID>(i),
                                 GX_IND_TEX_STAGE_0, GX_ITF_8, GX_ITB_STU,
                                 GX_ITM_0, GX_ITW_OFF, GX_ITW_OFF, FALSE, FALSE,
                                 GX_ITBA_OFF);
            }
        }
    } else {
        GXSetNumIndStages(0);
    }

    GXSetZCompLoc(rSetting.mFlags & EmitterDrawSetting::FLAG_ZCOMP_BEFORE_TEX
                      ? TRUE
                      : FALSE);

    GXSetCullMode(GX_CULL_NONE);
    GXSetCoPlanar(FALSE);

    GXSetBlendMode(static_cast<GXBlendMode>(rSetting.mBlendMode.mType),
                   static_cast<GXBlendFactor>(rSetting.mBlendMode.mSrcFactor),
                   static_cast<GXBlendFactor>(rSetting.mBlendMode.mDstFactor),
                   static_cast<GXLogicOp>(rSetting.mBlendMode.mOp));

    GXSetZMode(
        (rSetting.mFlags & EmitterDrawSetting::FLAG_ZCOMP_ENABLE_TEST) != 0,
        static_cast<GXCompare>(rSetting.mZCompareFunc),
        (rSetting.mFlags & EmitterDrawSetting::FLAG_ZCOMP_ENABLE_UPDATE) != 0);

    if (rSetting.mFlags & EmitterDrawSetting::FLAG_FOG_ENABLE) {
        GXFogType type;
        f32 start, end, near, far;
        GXColor color;

        rInfo.GetFog(&type, &start, &end, &near, &far, &color);
        GXSetFog(type, start, end, near, far, color);
    } else {
        GXColor color = {0, 0, 0, 0};
        GXSetFog(GX_FOG_NONE, 0.0f, 100.0f, 0.0f, 100.0f, color);
    }
}

void DrawStrategyImpl::InitColor(ParticleManager* pManager,
                                 const EmitterDrawSetting& rSetting,
                                 const DrawInfo& rInfo) {

    int i;

    mPrevARef0 = -1;
    mPrevARef1 = -1;

    GXSetNumChans(1);

    if (!rInfo.IsLightEnable()) {
        GXSetChanCtrl(GX_COLOR0A0, FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL,
                      GX_DF_NONE, GX_AF_NONE);
    } else {
        if (rSetting.mColorInput.mRasColor == ColorInput::RASCOLOR_LIGHTING) {
            GXSetChanCtrl(GX_COLOR0, TRUE, GX_SRC_REG, GX_SRC_REG,
                          rInfo.GetLightMask(), GX_DF_NONE,
                          rInfo.IsSpotLight() ? GX_AF_SPOT : GX_AF_NONE);
        } else {
            GXSetChanCtrl(GX_COLOR0, FALSE, GX_SRC_REG, GX_SRC_REG,
                          GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }

        if (rSetting.mAlphaInput.mRasColor == ColorInput::RASCOLOR_LIGHTING) {
            GXSetChanCtrl(GX_ALPHA0, TRUE, GX_SRC_REG, GX_SRC_REG,
                          rInfo.GetLightMaskA(), GX_DF_NONE,
                          rInfo.IsSpotLight() ? GX_AF_SPOT : GX_AF_NONE);
        } else {
            GXSetChanCtrl(GX_ALPHA0, FALSE, GX_SRC_REG, GX_SRC_REG,
                          GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
    }

    GXSetChanCtrl(GX_COLOR1A1, FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL,
                  GX_DF_NONE, GX_AF_NONE);

    GXSetChanMatColor(GX_COLOR0A0, rInfo.GetChanMatColor());
    GXSetChanAmbColor(GX_COLOR0A0, rInfo.GetChanAmbColor());

    mUseColor[COLOR_LAYER_0][COLOR_IDX_PRI] = false;
    mUseColor[COLOR_LAYER_0][COLOR_IDX_SEC] = false;
    mUseColor[COLOR_LAYER_1][COLOR_IDX_PRI] = false;
    mUseColor[COLOR_LAYER_1][COLOR_IDX_SEC] = false;

    mUseAlpha[COLOR_LAYER_0][COLOR_IDX_PRI] = false;
    mUseAlpha[COLOR_LAYER_0][COLOR_IDX_SEC] = false;
    mUseAlpha[COLOR_LAYER_1][COLOR_IDX_PRI] = false;
    mUseAlpha[COLOR_LAYER_1][COLOR_IDX_SEC] = false;

    for (i = 0; i < GX_MAX_TEVREG - 1; i++) {
        CheckUseColor(mUseColor, rSetting.mColorInput.mTevColor[i]);
        CheckUseColor(mUseAlpha, rSetting.mAlphaInput.mTevColor[i]);
    }

    for (i = 0; i < GX_MAX_KCOLOR; i++) {
        CheckUseColor(mUseColor, rSetting.mColorInput.mTevKColor[i]);
        CheckUseColor(mUseAlpha, rSetting.mAlphaInput.mTevKColor[i]);
    }
}

inline bool DrawStrategyImpl::_SetupACmp(Particle* pParticle,
                                         const EmitterDrawSetting& rSetting,
                                         bool first) {

    // @bug Copy-paste error, double comparison
    if (first || mPrevARef0 != pParticle->mParameter.mACmpRef0 ||
        mPrevARef0 != pParticle->mParameter.mACmpRef0) {

        // @bug Copy-paste error, assigns ref0 to prev ref1
        mPrevARef0 = pParticle->mParameter.mACmpRef0;
        mPrevARef1 = pParticle->mParameter.mACmpRef0;

        GXSetAlphaCompare(static_cast<GXCompare>(rSetting.mACmpComp0),
                          pParticle->mParameter.mACmpRef0,
                          static_cast<GXAlphaOp>(rSetting.mACmpOp),
                          static_cast<GXCompare>(rSetting.mACmpComp1),
                          pParticle->mParameter.mACmpRef1);

        return true;
    }

    return false;
}

void DrawStrategyImpl::SetupGP(Particle* pParticle,
                               const EmitterDrawSetting& rSetting,
                               const DrawInfo& rInfo, bool first,
                               bool xfDirty) {

    struct VtxDescData {
        GXAttrType type;     // at 0x0
        GXCompCnt compCnt;   // at 0x4
        GXCompType compType; // at 0x8
    };

    bool changed;
    bool xfBug;

    changed = _SetupACmp(pParticle, rSetting, first);
    xfBug = xfDirty || changed;

    changed = _SetupTevReg(pParticle, rSetting, first);
    xfBug = xfBug || changed;

    changed = _SetupTexture(pParticle, rSetting, rInfo, first);
    xfBug = xfBug && !changed;

    if ((rSetting.mFlags & EmitterDrawSetting::FLAG_CLIP_DISABLE) ||
        !pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
             ->mXFFlushSafe) {
        return;
    }

    if (!xfBug) {
        return;
    }

    VtxDescData vtxDescPos;
    VtxDescData vtxDescNrm;
    VtxDescData vtxDescTex0;
    VtxDescData vtxDescClr[2];
    u8 shift;

    GXGetVtxDesc(GX_VA_POS, &vtxDescPos.type);
    GXGetVtxDesc(GX_VA_NRM, &vtxDescNrm.type);
    GXGetVtxDesc(GX_VA_CLR0, &vtxDescClr[0].type);
    GXGetVtxDesc(GX_VA_CLR1, &vtxDescClr[1].type);
    GXGetVtxDesc(GX_VA_TEX0, &vtxDescTex0.type);

    GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, &vtxDescPos.compCnt,
                    &vtxDescPos.compType, &shift);
    GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, &vtxDescNrm.compCnt,
                    &vtxDescNrm.compType, &shift);
    GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, &vtxDescClr[0].compCnt,
                    &vtxDescClr[0].compType, &shift);
    GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR1, &vtxDescClr[1].compCnt,
                    &vtxDescClr[1].compType, &shift);
    GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, &vtxDescTex0.compCnt,
                    &vtxDescTex0.compType, &shift);

    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
    {
        for (int i = 0; i < 8; i++) {
            if (vtxDescPos.type != GX_NONE) {
                switch (vtxDescPos.type) {
                case GX_DIRECT: {

                    switch (vtxDescPos.compCnt) {
                    case GX_POS_XYZ: {

                        switch (vtxDescPos.compType) {
                        case GX_U8: {
                            GXPosition3u8(0, 0, 0);
                            break;
                        }

                        case GX_S8: {
                            GXPosition3s8(0, 0, 0);
                            break;
                        }

                        case GX_U16: {
                            GXPosition3u16(0, 0, 0);
                            break;
                        }

                        case GX_S16: {
                            GXPosition3s16(0, 0, 0);
                            break;
                        }

                        case GX_F32: {
                            GXPosition3f32(0.0f, 0.0f, 0.0f);
                            break;
                        }

                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    case GX_POS_XY: {

                        switch (vtxDescPos.compType) {
                        case GX_U8: {
                            GXPosition2u8(0, 0);
                            break;
                        }

                        case GX_S8: {
                            GXPosition2s8(0, 0);
                            break;
                        }

                        case GX_U16: {
                            GXPosition2u16(0, 0);
                            break;
                        }

                        case GX_S16: {
                            GXPosition2s16(0, 0);
                            break;
                        }

                        case GX_F32: {
                            GXPosition2f32(0.0f, 0.0f);
                            break;
                        }

                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    default: {
                        break;
                    }
                    }

                    break;
                }

                case GX_INDEX8: {
                    GXPosition1x8(0);
                    break;
                }

                case GX_INDEX16: {
                    GXPosition1x16(0);
                    break;
                }

                default: {
                    break;
                }
                }
            }

            if (vtxDescNrm.type != GX_NONE) {
                switch (vtxDescNrm.type) {
                case GX_DIRECT: {

                    switch (vtxDescNrm.compCnt) {
                    case GX_NRM_XYZ: {

                        switch (vtxDescNrm.compType) {
                        case GX_S8: {
                            GXNormal3u8(0, 0, 0);
                            break;
                        }

                        case GX_S16: {
                            GXNormal3u16(0, 0, 0);
                            break;
                        }

                        case GX_F32: {
                            GXNormal3f32(0.0f, 0.0f, 0.0f);
                            break;
                        }

                        // @bug Missing GX_U8/GX_U16
                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    default: {
                        break;
                    }
                    }

                    break;
                }

                case GX_INDEX8: {
                    GXNormal1x8(0);
                    break;
                }

                case GX_INDEX16: {
                    GXNormal1x16(0);
                    break;
                }

                default: {
                    break;
                }
                }
            }

            for (int j = 0; j < 2; j++) {
                if (vtxDescClr[j].type != GX_NONE) {
                    switch (vtxDescClr[j].type) {
                    case GX_DIRECT: {

                        switch (vtxDescClr[j].compCnt) {
                        case GX_CLR_RGB: {
                            GXColor3u8(0, 0, 0);
                            break;
                        }

                        case GX_CLR_RGBA: {
                            GXColor4u8(0, 0, 0, 0);
                            break;
                        }

                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    case GX_INDEX8: {
                        GXColor1x8(0);
                        break;
                    }

                    case GX_INDEX16: {
                        GXColor1x16(0);
                        break;
                    }

                    default: {
                        break;
                    }
                    }
                }
            }

            if (vtxDescTex0.type != GX_NONE) {
                switch (vtxDescTex0.type) {
                case GX_DIRECT: {

                    switch (vtxDescTex0.compCnt) {
                    case GX_TEX_ST: {

                        switch (vtxDescTex0.compType) {
                        case GX_U8: {
                            GXTexCoord2u8(0, 0);
                            break;
                        }

                        case GX_S8: {
                            GXTexCoord2s8(0, 0);
                            break;
                        }

                        case GX_U16: {
                            GXTexCoord2u16(0, 0);
                            break;
                        }

                        case GX_S16: {
                            GXTexCoord2s16(0, 0);
                            break;
                        }

                        case GX_F32: {
                            GXTexCoord2f32(0.0f, 0.0f);
                            break;
                        }

                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    case GX_TEX_S: {

                        switch (vtxDescTex0.compType) {
                        case GX_U8: {
                            GXTexCoord1u8(0);
                            break;
                        }

                        case GX_S8: {
                            GXTexCoord1s8(0);
                            break;
                        }

                        case GX_U16: {
                            GXTexCoord1u16(0);
                            break;
                        }

                        case GX_S16: {
                            GXTexCoord1s16(0);
                            break;
                        }

                        case GX_F32: {
                            GXTexCoord1f32(0.0f);
                            break;
                        }

                        default: {
                            break;
                        }
                        }

                        break;
                    }

                    default: {
                        break;
                    }
                    }

                    break;
                }

                case GX_INDEX8: {
                    GXTexCoord1x8(0);
                    break;
                }

                case GX_INDEX16: {
                    GXTexCoord1x16(0);
                    break;
                }

                default: {
                    break;
                }
                }
            }
        }
    }
    GXEnd();
}

bool DrawStrategyImpl::_SetupTevReg(Particle* pParticle,
                                    const EmitterDrawSetting& rSetting,
                                    bool first) {

    bool changed = false;
    int i;

    if (mUseColor[COLOR_LAYER_0][COLOR_IDX_PRI]) {
        mColor[COLOR_LAYER_0][COLOR_IDX_PRI].r = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_PRI].r;
        mColor[COLOR_LAYER_0][COLOR_IDX_PRI].g = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_PRI].g;
        mColor[COLOR_LAYER_0][COLOR_IDX_PRI].b = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_PRI].b;
    }

    if (mUseColor[COLOR_LAYER_0][COLOR_IDX_SEC]) {
        mColor[COLOR_LAYER_0][COLOR_IDX_SEC].r = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_SEC].r;
        mColor[COLOR_LAYER_0][COLOR_IDX_SEC].g = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_SEC].g;
        mColor[COLOR_LAYER_0][COLOR_IDX_SEC].b = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_SEC].b;
    }

    if (mUseColor[COLOR_LAYER_1][COLOR_IDX_PRI]) {
        mColor[COLOR_LAYER_1][COLOR_IDX_PRI].r = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_PRI].r;
        mColor[COLOR_LAYER_1][COLOR_IDX_PRI].g = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_PRI].g;
        mColor[COLOR_LAYER_1][COLOR_IDX_PRI].b = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_PRI].b;
    }

    if (mUseColor[COLOR_LAYER_1][COLOR_IDX_SEC]) {
        mColor[COLOR_LAYER_1][COLOR_IDX_SEC].r = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_SEC].r;
        mColor[COLOR_LAYER_1][COLOR_IDX_SEC].g = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_SEC].g;
        mColor[COLOR_LAYER_1][COLOR_IDX_SEC].b = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_SEC].b;
    }

    u8 flick = 255;

    if ((mUseAlpha[COLOR_LAYER_0][COLOR_IDX_PRI] ||
         mUseAlpha[COLOR_LAYER_0][COLOR_IDX_SEC] ||
         mUseAlpha[COLOR_LAYER_1][COLOR_IDX_PRI] ||
         mUseAlpha[COLOR_LAYER_1][COLOR_IDX_SEC]) &&
        rSetting.mAlphaFlickType != EmitterDrawSetting::ALPHAFLICK_NONE) {

        flick = pParticle->Draw_GetAlphaFlick();
    }

    if (mUseAlpha[COLOR_LAYER_0][COLOR_IDX_PRI]) {
        mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a;

        if (flick != 255) {
            mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a = (mColor[COLOR_LAYER_0][COLOR_IDX_PRI].a * flick + 128) >> 8;
        }
    }

    if (mUseAlpha[COLOR_LAYER_0][COLOR_IDX_SEC]) {
        mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a = pParticle->mParameter.mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a;

        if (flick != 255) {
            mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a = (mColor[COLOR_LAYER_0][COLOR_IDX_SEC].a * flick + 128) >> 8;
        }
    }

    if (mUseAlpha[COLOR_LAYER_1][COLOR_IDX_PRI]) {
        mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a;

        if (flick != 255) {
            mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a = (mColor[COLOR_LAYER_1][COLOR_IDX_PRI].a * flick + 128) >> 8;
        }
    }

    if (mUseAlpha[COLOR_LAYER_1][COLOR_IDX_SEC]) {
        mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a = pParticle->mParameter.mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a;

        if (flick != 255) {
            mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a = (mColor[COLOR_LAYER_1][COLOR_IDX_SEC].a * flick + 128) >> 8;
        }
    }

    if (pParticle->mParticleManager->mModifier.mLight.mType !=
        ParticleManager::ParticleModifier::SIMPLELIGHT_OFF) {

        if (mUseColor[COLOR_LAYER_0][COLOR_IDX_PRI] ||
            mUseColor[COLOR_LAYER_0][COLOR_IDX_SEC] ||
            mUseAlpha[COLOR_LAYER_0][COLOR_IDX_PRI] ||
            mUseAlpha[COLOR_LAYER_0][COLOR_IDX_SEC]) {

            pParticle->mParticleManager->Draw_ModifyColor(
                pParticle, &mColor[COLOR_LAYER_0][COLOR_IDX_PRI],
                &mColor[COLOR_LAYER_0][COLOR_IDX_SEC]);
        }

        if (mUseColor[COLOR_LAYER_1][COLOR_IDX_PRI] ||
            mUseColor[COLOR_LAYER_1][COLOR_IDX_SEC] ||
            mUseAlpha[COLOR_LAYER_1][COLOR_IDX_PRI] ||
            mUseAlpha[COLOR_LAYER_1][COLOR_IDX_SEC]) {

            pParticle->mParticleManager->Draw_ModifyColor(
                pParticle, &mColor[COLOR_LAYER_1][COLOR_IDX_PRI],
                &mColor[COLOR_LAYER_1][COLOR_IDX_SEC]);
        }
    }

    for (i = 0; i < GX_MAX_TEVREG - 1; i++) {
        GXColor color = {0, 0, 0, 0};

        if (rSetting.mColorInput.mTevColor[i] == ColorInput::TEVCOLOR_NULL &&
            rSetting.mAlphaInput.mTevColor[i] == ColorInput::TEVCOLOR_NULL) {
            continue;
        }

        GetTevColor(&color, rSetting.mColorInput.mTevColor[i]);
        GetTevAlpha(&color, rSetting.mAlphaInput.mTevColor[i]);

        if (first || color.r != mPrevTevColor[i].r || color.g != mPrevTevColor[i].g ||
            color.b != mPrevTevColor[i].b || color.a != mPrevTevColor[i].a) {

            GXSetTevColor(static_cast<GXTevRegID>(GX_TEVREG0 + i), color);
            changed = true;

            mPrevTevColor[i].r = color.r;
            mPrevTevColor[i].g = color.g;
            mPrevTevColor[i].b = color.b;
            mPrevTevColor[i].a = color.a;
        }
    }

    for (i = 0; i < GX_MAX_KCOLOR; i++) {
        GXColor color = {0, 0, 0, 0};

        if (rSetting.mColorInput.mTevKColor[i] == ColorInput::TEVCOLOR_NULL &&
            rSetting.mAlphaInput.mTevKColor[i] == ColorInput::TEVCOLOR_NULL) {
            continue;
        }

        GetTevColor(&color, rSetting.mColorInput.mTevKColor[i]);
        GetTevAlpha(&color, rSetting.mAlphaInput.mTevKColor[i]);

        if (first || color.r != mPrevTevKColor[i].r || color.g != mPrevTevKColor[i].g ||
            color.b != mPrevTevKColor[i].b || color.a != mPrevTevKColor[i].a) {

            GXSetTevKColor(static_cast<GXTevKColorID>(GX_KCOLOR0 + i), color);
            changed = true;

            mPrevTevKColor[i].r = color.r;
            mPrevTevKColor[i].g = color.g;
            mPrevTevKColor[i].b = color.b;
            mPrevTevKColor[i].a = color.a;
        }
    }

    return changed;
}

bool DrawStrategyImpl::_SetupTexture(Particle* pParticle,
                                     const EmitterDrawSetting& rSetting,
                                     const DrawInfo& rInfo, bool first) {

    bool changed = false;

    for (int i = 0; i < TEX_LAYER_MAX; i++) {
        if (mTexmapMap[i] < 0) {
            continue;
        }

        if (pParticle->mParameter.mTexture[i] == NULL) {
            continue;
        }

        const TextureData* pTexData = pParticle->mParameter.mTexture[i];

        GXTexWrapMode wrapS = pParticle->GetTextureWrapS(i);
        GXTexWrapMode wrapT = pParticle->GetTextureWrapT(i);

        if (first || pTexData != mPrevTexture[i].data ||
            wrapS != mPrevTexture[i].wrapS || wrapT != mPrevTexture[i].wrapT) {

            changed = true;

            mPrevTexture[i].data = pTexData;
            mPrevTexture[i].wrapS = wrapS;
            mPrevTexture[i].wrapT = wrapT;

            int format = pTexData->format;

            GXTexObj texObj;
            GXTlutObj tlutObj;

            switch (format) {
            case GX_TF_C4:
            case GX_TF_C8:
            case GX_TF_C14X2: {
                GXInitTlutObj(&tlutObj, pTexData->tlut,
                              static_cast<GXTlutFmt>(pTexData->tlutFormat),
                              pTexData->tlutEntries);

                GXLoadTlut(&tlutObj, static_cast<GXTlut>(mTexmapMap[i]));

                GXInitTexObjCI(&texObj, pTexData->texture, pTexData->width,
                               pTexData->height, static_cast<GXCITexFmt>(format),
                               wrapS, wrapT, pTexData->mipmap > 1,
                               mTexmapMap[i]);
                break;
            }

            case GX_TF_I4:
            case GX_TF_I8:
            case GX_TF_IA4:
            case GX_TF_IA8:
            case GX_TF_RGB565:
            case GX_TF_RGB5A3:
            case GX_TF_RGBA8:
            case GX_TF_CMPR: {
                GXInitTexObj(&texObj, pTexData->texture, pTexData->width,
                             pTexData->height, static_cast<GXTexFmt>(format),
                             wrapS, wrapT, pTexData->mipmap > 1);
                break;
            }

            default: {
                break;
            }
            }

            GXInitTexObjLOD(&texObj,
                            static_cast<GXTexFilter>(pTexData->min_filt),
                            static_cast<GXTexFilter>(pTexData->mag_filt), 0.0f,
                            pTexData->mipmap + -1.0f, pTexData->lod_bias, FALSE,
                            FALSE, GX_ANISO_1);

            GXLoadTexObj(&texObj, static_cast<GXTexMapID>(mTexmapMap[i]));
        }

        f32 scaleS = pParticle->Draw_GetTextureScaleS(i);
        f32 scaleT = pParticle->Draw_GetTextureScaleT(i);

        f32 transS = pParticle->Draw_GetTextureTransS(i);
        f32 transT = pParticle->Draw_GetTextureTransT(i);

        if (first || scaleS != mPrevTexture[i].scaleS ||
            scaleT != mPrevTexture[i].scaleT ||
            transS != mPrevTexture[i].transS ||
            transT != mPrevTexture[i].transT ||
            pParticle->mParameter.mTextureScale[i].x !=
                mPrevTexture[i].scale.x ||
            pParticle->mParameter.mTextureScale[i].y !=
                mPrevTexture[i].scale.y ||
            pParticle->mParameter.mTextureRotate[i] != mPrevTexture[i].rotate ||
            pParticle->mParameter.mTextureTranslate[i].x !=
                mPrevTexture[i].translate.x ||
            pParticle->mParameter.mTextureTranslate[i].y !=
                mPrevTexture[i].translate.y) {

            changed = true;

            mPrevTexture[i].scaleS = scaleS;
            mPrevTexture[i].scaleT = scaleT;
            mPrevTexture[i].transS = transS;
            mPrevTexture[i].transT = transT;
            mPrevTexture[i].scale.x = pParticle->mParameter.mTextureScale[i].x;
            mPrevTexture[i].scale.y = pParticle->mParameter.mTextureScale[i].y;
            mPrevTexture[i].rotate = pParticle->mParameter.mTextureRotate[i];
            mPrevTexture[i].translate.x =
                pParticle->mParameter.mTextureTranslate[i].x;
            mPrevTexture[i].translate.y =
                pParticle->mParameter.mTextureTranslate[i].y;

            math::MTX34 mtx;
            math::MTX34Identity(&mtx);

            mtx._03 = pParticle->mParameter.mTextureTranslate[i].x;
            mtx._13 = pParticle->mParameter.mTextureTranslate[i].y;

            if (pParticle->mParameter.mTextureRotate[i] != 0.0f) {
                f32 tx = mtx._03 - 0.5f;
                f32 ty = mtx._13 - 0.5f;

                f32 sr, cr;
                SinCosRad(&sr, &cr, pParticle->mParameter.mTextureRotate[i]);

                mtx._00 = mtx._11 = cr;
                mtx._10 = sr;
                mtx._01 = -sr;

                mtx._03 = 0.5f + cr * tx - sr * ty;
                mtx._13 = 0.5f + sr * tx + cr * ty;
            }

            if (scaleS != 1.0f || scaleT != 1.0f ||
                pParticle->mParameter.mTextureScale[i].x != 1.0f ||
                pParticle->mParameter.mTextureScale[i].y != 1.0f ||
                transS != 0.0f || transT != 0.0f) {

                f32 sx = scaleS * pParticle->mParameter.mTextureScale[i].x;
                f32 sy = scaleT * pParticle->mParameter.mTextureScale[i].y;

                mtx._00 *= sx;
                mtx._01 *= sx;
                mtx._03 =
                    transS +
                    scaleS * (0.5f + (mtx._03 - 0.5f) *
                                         pParticle->mParameter.mTextureScale[i].x);

                mtx._10 *= sy;
                mtx._11 *= sy;
                mtx._13 =
                    transT +
                    scaleT * (0.5f + (mtx._13 - 0.5f) *
                                         pParticle->mParameter.mTextureScale[i].y);
            }

            if (!((rSetting.mFlags >> (i + 7 /* FLAG_TEX1_PROJ */)) & 1)) {
                GXLoadTexMtxImm(mtx, GX_TEXMTX0 + mTexmapMap[i] * 3,
                                GX_MTX2x4);

                if (first) {
                    GXSetTexCoordGen(static_cast<GXTexCoordID>(mTexmapMap[i]),
                                     GX_TG_MTX2x4, GX_TG_TEX0,
                                     GX_TEXMTX0 + mTexmapMap[i] * 3);
                }
            } else {
                math::MTX34Mult(&mtx, rInfo.GetProjMtx(), &mtx);

                GXLoadTexMtxImm(mtx, GX_PTTEXMTX0 + mTexmapMap[i] * 3,
                                GX_MTX3x4);

                if (first) {
                    GXSetTexCoordGen2(static_cast<GXTexCoordID>(mTexmapMap[i]),
                                      GX_TG_MTX3x4, GX_TG_POS, GX_PNMTX0, FALSE,
                                      GX_PTTEXMTX0 + mTexmapMap[i] * 3);
                }
            }
        }
    }

    return changed;
}

DrawStrategyImpl::GetFirstDrawParticleFunc
DrawStrategyImpl::GetGetFirstDrawParticleFunc(int drawOrder) {

    switch (drawOrder) {
    case DRAWORDER_YOUNGERS_FIRST: {
        return GetFirstDrawParticle_YoungersFirst;
    }

    default: /* DRAWORDER_ELDERS_FIRST */ {
        return GetFirstDrawParticle_EldersFirst;
    }
    }
}

DrawStrategyImpl::GetNextDrawParticleFunc
DrawStrategyImpl::GetGetNextDrawParticleFunc(int drawOrder) {

    switch (drawOrder) {
    case DRAWORDER_YOUNGERS_FIRST: {
        return GetNextDrawParticle_YoungersFirst;
    }

    default: /* DRAWORDER_ELDERS_FIRST */ {
        return GetNextDrawParticle_EldersFirst;
    }
    }
}

Particle*
DrawStrategyImpl::GetFirstDrawParticle_EldersFirst(ParticleManager* pManager) {

    return GetOldestDrawParticle(pManager);
}

Particle* DrawStrategyImpl::GetFirstDrawParticle_YoungersFirst(
    ParticleManager* pManager) {

    return GetYoungestDrawParticle(pManager);
}

Particle*
DrawStrategyImpl::GetNextDrawParticle_EldersFirst(ParticleManager* pManager,
                                                  Particle* pParticle) {

    return GetYoungerDrawParticle(pManager, pParticle);
}

Particle*
DrawStrategyImpl::GetNextDrawParticle_YoungersFirst(ParticleManager* pManager,
                                                    Particle* pParticle) {

    return GetElderDrawParticle(pManager, pParticle);
}

/******************************************************************************
 *
 * AheadContext
 *
 ******************************************************************************/
DrawStrategyImpl::AheadContext::AheadContext(const math::MTX34& rViewMtx,
                                             ParticleManager* pManager) {

    mCommon.mViewMtx = &rViewMtx;
    mCommon.mParticleManager = pManager;

    pManager->mManagerEM->CalcGlobalMtx(&mCommon.mEmitterMtx);
    pManager->CalcGlobalMtx(&mCommon.mParticleManagerMtx);

    math::MTX34Inv(&mCommon.mParticleManagerMtxInv,
                   &mCommon.mParticleManagerMtx);

    math::VEC3 axisY(mCommon.mEmitterMtx._01, mCommon.mEmitterMtx._11,
                     mCommon.mEmitterMtx._21);
    math::VEC3TransformNormal(&axisY, &mCommon.mParticleManagerMtxInv, &axisY);

    if (!Normalize(&axisY)) {
        axisY = math::VEC3(0.0f, 1.0f, 0.0f);
    }

    mCommon.mEmitterAxisY = axisY;

    math::VEC3 center(mCommon.mEmitterMtx._03, mCommon.mEmitterMtx._13,
                      mCommon.mEmitterMtx._23);
    math::VEC3TransformCoord(&center, &mCommon.mParticleManagerMtxInv, &center);

    mCommon.mEmitterCenter = center;

    EmitterDrawSetting& rDesc = *pManager->mResource->GetEmitterDrawSetting();

    switch (rDesc.typeDir) {
    case EmitterDrawSetting::DIR_NO_DESIGN:
    case EmitterDrawSetting::DIR_NO_DESIGN_7: {
        math::MTX34& rMtx = mCommon.mParticleManagerMtxInv;
        math::VEC3 worldAxisY(rMtx._01, rMtx._11, rMtx._21);

        if (!Normalize(&worldAxisY)) {
            worldAxisY = math::VEC3(0.0f, 1.0f, 0.0f);
        }

        mNoDesign.mWorldYAxis = worldAxisY;
        break;
    }

    default: {
        break;
    }
    }
}

} // namespace ef
} // namespace nw4r
