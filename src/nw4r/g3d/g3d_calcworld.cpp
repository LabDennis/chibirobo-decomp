#include <nw4r/g3d.h>

namespace nw4r {
namespace g3d {

void WorldMtxManip::SetScale(f32 x, f32 y, f32 z) {
    mpS->x = x;
    mpS->y = y;
    mpS->z = z;

    if (x == y && x == z) {
        if (1.f == x) {
            *mpWMAttr |= detail::WorldMtxAttr::ATTR_S_ONE;
        } else {
            *mpWMAttr |= detail::WorldMtxAttr::ATTR_S_UNIFORM;
            *mpWMAttr &= ~(detail::WorldMtxAttr::ATTR_S_ONE | detail::WorldMtxAttr::ATTR_ALL_S_ONE);
        }
    } else {
        *mpWMAttr &=
            ~(detail::WorldMtxAttr::ATTR_S_UNIFORM | detail::WorldMtxAttr::ATTR_ALL_S_UNIFORM |
              detail::WorldMtxAttr::ATTR_S_ONE | detail::WorldMtxAttr::ATTR_ALL_S_ONE);
    }
}

void CalcWorld(
    math::MTX34 *pModelMtxArray, u32 *pModelMtxAttribArray, const u8 *pByteCode, const math::MTX34 *pBaseMtx,
    ResMdl mdl, AnmObjChr *pAnmChr, FuncObjCalcWorld *pFuncObj, u32 rootAttrib
) {
#define pCalcCmd reinterpret_cast<const ResByteCodeData::CalcParams *>(pByteCode)
#define pMtxDupCmd reinterpret_cast<const ResByteCodeData::MtxDupParams *>(pByteCode)

    u8 c;

    ChrAnmResult result;
    const ChrAnmResult *pResult = NULL;

    if (pByteCode == NULL) {
        pByteCode = mdl.GetResByteCode("NodeTree");

        if (pByteCode == NULL) {
            return;
        }
    }

    bool xsi = mdl.GetResMdlInfo().GetScalingRule() == SCALINGRULE_SOFTIMAGE;

    bool ignoreTrans = detail::WorldMtxAttr::IsIgnoreTrans(rootAttrib);

    math::VEC3 *pScaleArray = detail::workmem::GetScaleTemporary();
    pScaleArray[0].x = pScaleArray[0].y = pScaleArray[0].z = 1.0f;

    math::MTX34Copy(pModelMtxArray, pBaseMtx);

    u32 *pMtxIDArray = detail::workmem::GetMtxIDTemporary();
    u32 numMtxID = 0;

    pModelMtxAttribArray[0] = rootAttrib;

    while ((c = *pByteCode) != ResByteCodeData::END) {
        if (c == ResByteCodeData::CALC) {
            u32 nodeID = (pCalcCmd->nodeIdHi << 8) + pCalcCmd->nodeIdLo;

            u32 parentMtxID = (pCalcCmd->parentMtxIdHi << 8) + pCalcCmd->parentMtxIdLo;

            ResNode node = mdl.GetResNode(nodeID);

            u32 mtxID = node.GetMtxID();
            pMtxIDArray[numMtxID++] = mtxID;

            math::MTX34 *pModelMtx = &pModelMtxArray[mtxID];
            math::VEC3 *pScale = &pScaleArray[mtxID];

            math::MTX34 *pParentModelMtx = &pModelMtxArray[parentMtxID];
            math::VEC3 *pParentScale = &pScaleArray[parentMtxID];

            u32 parentAttr = pModelMtxAttribArray[parentMtxID];

            if (pAnmChr != NULL) {
                pResult = pAnmChr->GetResult(&result, node.GetID());
            }

            if (pAnmChr == NULL || pResult->flags == 0) {
                node.CalcChrAnmResult(&result);
                pResult = &result;
            }

            if (nodeID != 0 && ignoreTrans) {
                if (pResult != &result) {
                    result = *pResult;
                    pResult = &result;
                }

                result.flags |= ChrAnmResult::FLAG_PATCH_TRANS;
                node.PatchChrAnmResult(&result);
            }

            if (pFuncObj != NULL) {
                if (pResult != &result) {
                    result = *pResult;
                    pResult = &result;
                }

                pFuncObj->CheckCallbackA(nodeID, &result, mdl);
            }

            if (xsi) {
                pModelMtxAttribArray[mtxID] = detail::dcc::CalcWorldMtx_Xsi(
                    pModelMtx, pScale, pParentModelMtx, pParentScale, parentAttr, pResult
                );
            } else if (pResult->flags & ChrAnmResult::FLAG_SSC_APPLY) {
                pModelMtxAttribArray[mtxID] = detail::dcc::CalcWorldMtx_Maya_SSC_Apply(
                    pModelMtx, pScale, pParentModelMtx, pParentScale, parentAttr, pResult
                );
            } else {
                pModelMtxAttribArray[mtxID] = detail::dcc::CalcWorldMtx_Basic(
                    pModelMtx, pScale, pParentModelMtx, pParentScale, parentAttr, pResult
                );
            }

            pModelMtxAttribArray[mtxID] =
                detail::WorldMtxAttr::SetBillboard(pModelMtxAttribArray[mtxID], node.GetBillboardMode());

            if (pFuncObj != NULL) {
                pFuncObj->CheckCallbackB(nodeID, pModelMtx, pScale, &pModelMtxAttribArray[mtxID], mdl);
            }

            pByteCode += sizeof(ResByteCodeData::CalcParams);

        } else /* Assume MTXDUP */ {
            u32 toMtxID = (pMtxDupCmd->toMtxIdHi << 8) + pMtxDupCmd->toMtxIdLo;

            u32 fromMtxID = (pMtxDupCmd->fromMtxIdHi << 8) + pMtxDupCmd->fromMtxIdLo;

            pMtxIDArray[numMtxID++] = toMtxID;
            pModelMtxAttribArray[toMtxID] = pModelMtxAttribArray[fromMtxID];

            math::MTX34Copy(&pModelMtxArray[toMtxID], &pModelMtxArray[fromMtxID]);

            pScaleArray[toMtxID] = pScaleArray[fromMtxID];

            pByteCode += sizeof(ResByteCodeData::MtxDupParams);
        }
    }

    for (u32 i = 0; i < numMtxID; i++) {
        u32 mtxID = pMtxIDArray[i];

        if (pScaleArray[mtxID].x == 1.0f && pScaleArray[mtxID].y == 1.0f && pScaleArray[mtxID].z == 1.0f) {
            continue;
        }

        math::MTX34Scale(&pModelMtxArray[mtxID], &pModelMtxArray[mtxID], &pScaleArray[mtxID]);
    }

    if (pFuncObj != NULL) {
        pFuncObj->CheckCallbackC(pModelMtxArray, mdl);
    }

#undef pCalcCmd
#undef pMtxDupCmd
}

void CalcWorld(
    math::MTX34 *pModelMtxArray, u32 *pModelMtxAttribArray, const u8 *pByteCode, const math::MTX34 *pBaseMtx,
    ResMdl mdl, AnmObjChr *pAnmChr, FuncObjCalcWorld *pFuncObj
) {
    CalcWorld(
        pModelMtxArray, pModelMtxAttribArray, pByteCode, pBaseMtx, mdl, pAnmChr, pFuncObj,
        detail::WorldMtxAttr::GetRootMtxAttr()
    );
}

// NONMATCHING: retail uses paired-single assembly in the weighted matrix blend loop
void CalcSkinning(math::MTX34 *pModelMtxArray, u32 *pModelMtxAttribArray, const ResMdl mdl, const u8 *pByteCode) {
#define pWeightCmd reinterpret_cast<const ResByteCodeData::WeightParams *>(pByteCode)
#define pWeightEntry reinterpret_cast<const ResByteCodeData::WeightEntry *>(pByteCode)
#define pEvpMtxCmd reinterpret_cast<const ResByteCodeData::EvpMtxParams *>(pByteCode)

    u8 c;

    if (pByteCode == NULL) {
        pByteCode = mdl.GetResByteCode("NodeMix");

        if (pByteCode == NULL) {
            return;
        }
    }

    math::MTX34 *pSkinMtxArray = detail::workmem::GetSkinningMtxTemporary();

    while ((c = *pByteCode) != ResByteCodeData::END) {
        if (c == ResByteCodeData::WEIGHT) {
            u32 targetMtxID = (pWeightCmd->tgtIdHi << 8) + pWeightCmd->tgtIdLo;
            u32 numBlendMtx = pWeightCmd->numBlendMtx;

            pModelMtxAttribArray[targetMtxID] = detail::WorldMtxAttr::GetRootMtxAttr();

            pByteCode += sizeof(ResByteCodeData::WeightParams);

            math::MTX34 acc;
            acc._00 = 0.0f;
            acc._01 = 0.0f;
            acc._02 = 0.0f;
            acc._03 = 0.0f;
            acc._10 = 0.0f;
            acc._11 = 0.0f;
            acc._12 = 0.0f;
            acc._13 = 0.0f;
            acc._20 = 0.0f;
            acc._21 = 0.0f;
            acc._22 = 0.0f;
            acc._23 = 0.0f;

            for (u32 i = 0; i < numBlendMtx; i++) {
                u32 mtxID = (pWeightEntry->mtxIdHi << 8) + pWeightEntry->mtxIdLo;

                u32 iraito = pWeightEntry->fWeight0 << 24 | pWeightEntry->fWeight1 << 16 | pWeightEntry->fWeight2 << 8 |
                             pWeightEntry->fWeight3;

                f32 R = *reinterpret_cast<f32 *>(&iraito);
                const math::MTX34 *pT = &pSkinMtxArray[mtxID];

                acc._00 += pT->_00 * R;
                acc._01 += pT->_01 * R;
                acc._02 += pT->_02 * R;
                acc._03 += pT->_03 * R;
                acc._10 += pT->_10 * R;
                acc._11 += pT->_11 * R;
                acc._12 += pT->_12 * R;
                acc._13 += pT->_13 * R;
                acc._20 += pT->_20 * R;
                acc._21 += pT->_21 * R;
                acc._22 += pT->_22 * R;
                acc._23 += pT->_23 * R;

                pModelMtxAttribArray[targetMtxID] &= pModelMtxAttribArray[mtxID];

                pByteCode += sizeof(ResByteCodeData::WeightEntry);
            }

            pModelMtxArray[targetMtxID] = acc;

        } else /* Assume EVPMTX */ {
            u32 mtxID = (pEvpMtxCmd->mtxIdHi << 8) + pEvpMtxCmd->mtxIdLo;
            u32 nodeID = (pEvpMtxCmd->nodeIdHi << 8) + pEvpMtxCmd->nodeIdLo;

            math::MTX34Mult(
                &pSkinMtxArray[mtxID], &pModelMtxArray[mtxID],
                static_cast<const math::MTX34 *>(&mdl.GetResNode(nodeID).ref().invModelMtx)
            );

            pByteCode += sizeof(ResByteCodeData::EvpMtxParams);
        }
    }
    ResMdlInfo mdlInfo = mdl.GetResMdlInfo();
    switch (mdlInfo.GetEnvelopeMatrixMode()) {
        case ResMdlInfo::EVPMATRIXMODE_NORMAL: {
            return;
        } break;
        case ResMdlInfo::EVPMATRIXMODE_APPROX: {
            u32 numWeighted = mdlInfo.GetNumPosNrmMtx() - mdl.GetResNodeNumEntries();
            u32 end = mdlInfo.GetNumViewMtx();
            u32 begin = end - numWeighted;

            for (u32 i = begin; i < end; i++) {
                pModelMtxAttribArray[i] |= detail::WorldMtxAttr::ATTR_S_UNIFORM;
                pModelMtxAttribArray[i] |= detail::WorldMtxAttr::ATTR_ALL_S_UNIFORM;
            }
        } break;
        case ResMdlInfo::EVPMATRIXMODE_EXACT: {
            u32 numWeighted = mdlInfo.GetNumPosNrmMtx() - mdl.GetResNodeNumEntries();
            u32 end = mdlInfo.GetNumViewMtx();
            u32 begin = end - numWeighted;

            for (u32 i = begin; i < end; i++) {
                pModelMtxAttribArray[i] &= ~detail::WorldMtxAttr::ATTR_ALL_S_UNIFORM;
            }
        } break;
    }

#undef pWeightCmd
#undef pWeightEntry
#undef pEvpMtxCmd
}

} // namespace g3d
} // namespace nw4r
