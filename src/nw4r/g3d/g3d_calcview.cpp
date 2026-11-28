#include <nw4r/g3d.h>

#include <algorithm>

namespace nw4r {
namespace g3d {

/******************************************************************************
 *
 * Paired-single math implementation
 *
 ******************************************************************************/
namespace {

const f32 norm_epsilon = 1e-18f;

// NONMATCHING: retail uses paired-single assembly
void GetModelLocalAxisY2(
    math::VEC3 *pVec, const math::MTX34 *pModelMtx, const math::MTX34 *pParentModelMtx
) {
    f32 p00 = pParentModelMtx->_00, p01 = pParentModelMtx->_01, p02 = pParentModelMtx->_02;
    f32 p10 = pParentModelMtx->_10, p11 = pParentModelMtx->_11, p12 = pParentModelMtx->_12;
    f32 p20 = pParentModelMtx->_20, p21 = pParentModelMtx->_21, p22 = pParentModelMtx->_22;

    f32 c00 = p11 * p22 - p21 * p12;
    f32 c10 = p21 * p02 - p01 * p22;
    f32 c20 = p01 * p12 - p11 * p02;

    f32 det = p00 * c00 + p10 * c10 + p20 * c20;
    pVec->z = 0.0f;
    if (math::FAbs(det) < norm_epsilon) {
        pVec->x = 0.0f;
        pVec->y = 0.0f;
        return;
    }

    f32 invDet = math::FInv(det);
    f32 c11 = p22 * p00 - p02 * p20;
    f32 c12 = p01 * p20 - p00 * p21;

    f32 y0 = c10 * invDet;
    f32 y1 = c11 * invDet;
    f32 y2 = c12 * invDet;

    pVec->x = pModelMtx->_00 * y0 + pModelMtx->_01 * y1 + pModelMtx->_02 * y2;
    pVec->y = pModelMtx->_10 * y0 + pModelMtx->_11 * y1 + pModelMtx->_12 * y2;
}

// NONMATCHING: retail uses paired-single assembly
void GetModelLocalAxisY3(
    math::VEC3 *pVec, const math::MTX34 *pModelMtx, const math::MTX34 *pParentModelMtx
) {
    f32 p00 = pParentModelMtx->_00, p01 = pParentModelMtx->_01, p02 = pParentModelMtx->_02;
    f32 p10 = pParentModelMtx->_10, p11 = pParentModelMtx->_11, p12 = pParentModelMtx->_12;
    f32 p20 = pParentModelMtx->_20, p21 = pParentModelMtx->_21, p22 = pParentModelMtx->_22;

    f32 c00 = p11 * p22 - p21 * p12;
    f32 c10 = p21 * p02 - p01 * p22;
    f32 c20 = p01 * p12 - p11 * p02;

    f32 det = p00 * c00 + p10 * c10 + p20 * c20;
    pVec->z = 0.0f;
    if (math::FAbs(det) < norm_epsilon) {
        pVec->x = 0.0f;
        pVec->y = 0.0f;
        return;
    }

    f32 invDet = math::FInv(det);
    f32 c11 = p22 * p00 - p02 * p20;
    f32 c12 = p01 * p20 - p00 * p21;

    f32 y0 = c10 * invDet;
    f32 y1 = c11 * invDet;
    f32 y2 = c12 * invDet;

    pVec->x = pModelMtx->_00 * y0 + pModelMtx->_01 * y1 + pModelMtx->_02 * y2;
    pVec->y = pModelMtx->_10 * y0 + pModelMtx->_11 * y1 + pModelMtx->_12 * y2;
    // Retail's paired-single routine multiplies row 2's (M20, M21) by f9 instead of f12.
    pVec->z = pModelMtx->_20 * y2 + pModelMtx->_21 * ((p02 - p22) * invDet) + pModelMtx->_22 * y2;
}

inline void SetMdlViewMtxSR(math::MTX34 *pViewPos, const math::VEC3 &rRY, f32 s) {
    f32 rx = rRY.x * s;
    f32 ry = rRY.y * s;

    pViewPos->_00 = ry;
    pViewPos->_01 = rx;
    pViewPos->_02 = 0.0f;

    pViewPos->_10 = -rx;
    pViewPos->_11 = ry;
    pViewPos->_12 = 0.0f;

    pViewPos->_20 = 0.0f;
    pViewPos->_21 = 0.0f;
    pViewPos->_22 = s;
}

inline void SetMdlViewMtxSR(
    math::MTX34 *pViewPos, const math::VEC3 &rRY, f32 sx, f32 sy, f32 sz
) {
    pViewPos->_00 = rRY.y * sx;
    pViewPos->_01 = rRY.x * sy;
    pViewPos->_02 = 0.0f;

    pViewPos->_10 = -rRY.x * sx;
    pViewPos->_11 = rRY.y * sy;
    pViewPos->_12 = 0.0f;

    pViewPos->_20 = 0.0f;
    pViewPos->_21 = 0.0f;
    pViewPos->_22 = sz;
}

inline void SetMdlViewMtxSR(
    math::MTX34 *pViewPos, const math::VEC3 &rRX, const math::VEC3 &rRY,
    const math::VEC3 &rRZ, f32 sx, f32 sy, f32 sz
) {
    pViewPos->_00 = rRX.x * sx;
    pViewPos->_01 = rRY.x * sy;
    pViewPos->_02 = rRZ.x * sz;

    pViewPos->_10 = rRX.y * sx;
    pViewPos->_11 = rRY.y * sy;
    pViewPos->_12 = rRZ.y * sz;

    pViewPos->_20 = rRX.z * sx;
    pViewPos->_21 = rRY.z * sy;
    pViewPos->_22 = rRZ.z * sz;
}

} // namespace

/******************************************************************************
 *
 * Billboard implementation
 *
 ******************************************************************************/
namespace {

inline f32 GetMtx34Scale(const math::MTX34 &rMtx, int col) {
    return math::FSqrt(
        rMtx.m[0][col] * rMtx.m[0][col] + rMtx.m[1][col] * rMtx.m[1][col] + rMtx.m[2][col] * rMtx.m[2][col]
    );
}

void Calc_BILLBOARD_STD(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vy(pViewPos->_01, pViewPos->_11, 0.0f);

    if (math::FAbs(vy.x) >= norm_epsilon || math::FAbs(vy.y) >= norm_epsilon) {
        math::VEC3Normalize(&vy, &vy);
        if (uniformScale) {
            f32 s = GetMtx34Scale(pModelMtxArray[id], 0);
            SetMdlViewMtxSR(pViewPos, vy, s);
        } else {
            f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
            f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
            f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
            SetMdlViewMtxSR(pViewPos, vy, sx, sy, sz);
        }
    } else {
        math::MTX34Zero(pViewPos);
    }
}

void Calc_BILLBOARD_PERSP_STD(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vx;
    math::VEC3 vy(pViewPos->_01, pViewPos->_11, pViewPos->_21);
    math::VEC3 vz(-pViewPos->_03, -pViewPos->_13, -pViewPos->_23);

    if (math::FAbs(vz.x) >= norm_epsilon || math::FAbs(vz.y) >= norm_epsilon || math::FAbs(vz.z) >= norm_epsilon) {
        math::VEC3Normalize(&vz, &vz);
        math::VEC3Cross(&vx, &vy, &vz);

        if (math::FAbs(vx.x) >= norm_epsilon || math::FAbs(vx.y) >= norm_epsilon || math::FAbs(vx.z) >= norm_epsilon) {
            math::VEC3Normalize(&vx, &vx);
            math::VEC3Cross(&vy, &vz, &vx);

            if (uniformScale) {
                f32 s = GetMtx34Scale(pModelMtxArray[id], 0);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, s, s, s);
            } else {
                f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
                f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
                f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sx, sy, sz);
            }
            return;
        }
    }
    math::MTX34Zero(pViewPos);
}

void Calc_BILLBOARD_ROT(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vy;

    int nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(id);

    if (nodeID >= 0) {
        ResNode node = mdl.GetResNode(nodeID);
        ResNode parent = node.GetParentNode();

        if (parent.IsValid()) {
            u32 parentMtxID = parent.GetMtxID();
            GetModelLocalAxisY2(&vy, &pModelMtxArray[id], &pModelMtxArray[parentMtxID]);
        } else {
            vy.x = pModelMtxArray[id]._01;
            vy.y = pModelMtxArray[id]._11;
            vy.z = 0.0f;
        }
    } else {
        vy.x = pModelMtxArray[id]._01;
        vy.y = pModelMtxArray[id]._11;
        vy.z = 0.0f;
    }

    if (math::FAbs(vy.x) >= norm_epsilon || math::FAbs(vy.y) >= norm_epsilon) {
        math::VEC3Normalize(&vy, &vy);

        if (uniformScale) {
            f32 s = GetMtx34Scale(pModelMtxArray[id], 0);
            SetMdlViewMtxSR(pViewPos, vy, s);
        } else {
            f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
            f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
            f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
            SetMdlViewMtxSR(pViewPos, vy, sx, sy, sz);
        }
    } else {
        math::MTX34Zero(pViewPos);
    }
}

void Calc_BILLBOARD_PERSP_ROT(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vx;
    math::VEC3 vy;
    math::VEC3 vz(-pViewPos->_03, -pViewPos->_13, -pViewPos->_23);

    int nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(id);

    if (nodeID >= 0) {
        ResNode node = mdl.GetResNode(nodeID);
        ResNode parent = node.GetParentNode();

        if (parent.IsValid()) {
            u32 parentMtxID = parent.GetMtxID();
            GetModelLocalAxisY3(&vy, &pModelMtxArray[id], &pModelMtxArray[parentMtxID]);
        } else {
            vy.x = pModelMtxArray[id]._01;
            vy.y = pModelMtxArray[id]._11;
            vy.z = pModelMtxArray[id]._21;
        }
    } else {
        vy.x = pModelMtxArray[id]._01;
        vy.y = pModelMtxArray[id]._11;
        vy.z = pModelMtxArray[id]._21;
    }

    if (math::FAbs(vz.x) >= norm_epsilon || math::FAbs(vz.y) >= norm_epsilon || math::FAbs(vz.z) >= norm_epsilon) {
        math::VEC3Normalize(&vz, &vz);
        math::VEC3Cross(&vx, &vy, &vz);

        if (math::FAbs(vx.x) >= norm_epsilon || math::FAbs(vx.y) >= norm_epsilon || math::FAbs(vx.z) >= norm_epsilon) {
            math::VEC3Normalize(&vx, &vx);
            math::VEC3Cross(&vy, &vz, &vx);

            if (uniformScale) {
                f32 s = GetMtx34Scale(pModelMtxArray[id], 0);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, s, s, s);
            } else {
                f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
                f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
                f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sx, sy, sz);
            }
            return;
        }
    }
    math::MTX34Zero(pViewPos);
}

void Calc_BILLBOARD_Y(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vz;
    math::VEC3 vy(pViewPos->_01, pViewPos->_11, pViewPos->_21);
    math::VEC3 vx(vy.y, -vy.x, 0.0f);
    if (math::FAbs(vy.x) >= norm_epsilon || math::FAbs(vx.x) >= norm_epsilon || math::FAbs(vy.z) >= norm_epsilon) {
        f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
        f32 invSY = math::FInv(sy);

        vy *= invSY;

        if (math::FAbs(vx.x) >= norm_epsilon || math::FAbs(vx.y) >= norm_epsilon) {
            math::VEC3Normalize(&vx, &vx);
            math::VEC3Cross(&vz, &vx, &vy);

            if (uniformScale) {
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sy, sy, sy);
            } else {
                f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
                f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sx, sy, sz);
            }
            return;
        }
    }
    math::MTX34Zero(pViewPos);
}

void Calc_BILLBOARD_PERSP_Y(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
) {

    math::VEC3 vx;
    math::VEC3 vy(pViewPos->_01, pViewPos->_11, pViewPos->_21);
    math::VEC3 vz(-pViewPos->_03, -pViewPos->_13, -pViewPos->_23);

    if (math::FAbs(vy.x) >= norm_epsilon || math::FAbs(vy.y) >= norm_epsilon || math::FAbs(vy.z) >= norm_epsilon) {
        f32 sy = GetMtx34Scale(pModelMtxArray[id], 1);
        f32 invSY = math::FInv(sy);

        vy *= invSY;
        math::VEC3Cross(&vx, &vy, &vz);

        if (math::FAbs(vx.x) >= norm_epsilon || math::FAbs(vx.y) >= norm_epsilon || math::FAbs(vx.z) >= norm_epsilon) {
            math::VEC3Normalize(&vx, &vx);
            math::VEC3Cross(&vz, &vx, &vy);

            if (uniformScale) {
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sy, sy, sy);
            } else {
                f32 sx = GetMtx34Scale(pModelMtxArray[id], 0);
                f32 sz = GetMtx34Scale(pModelMtxArray[id], 2);
                SetMdlViewMtxSR(pViewPos, vx, vy, vz, sx, sy, sz);
            }
            return;
        }
    }
    math::MTX34Zero(pViewPos);
}

typedef void (*BillBoardFunc)(
    math::MTX34 *pViewPos, const math::MTX34 *pModelMtxArray, bool uniformScale, const math::MTX34 *pViewMtx,
    const ResMdl mdl, const u32 id
);

const BillBoardFunc bbFunc[ResNodeData::NUM_BILLBOARD] = {
    NULL, // BILLBOARD_OFF is ignored

    Calc_BILLBOARD_STD,       //
    Calc_BILLBOARD_PERSP_STD, //

    Calc_BILLBOARD_ROT,       //
    Calc_BILLBOARD_PERSP_ROT, //

    Calc_BILLBOARD_Y,      //
    Calc_BILLBOARD_PERSP_Y //
};

} // namespace

/******************************************************************************
 *
 * CalcView
 *
 ******************************************************************************/
void CalcView(
    math::MTX34 *pViewPosArray, math::MTX33 *pViewNrmArray, const math::MTX34 *pModelMtxArray,
    const u32 *pModelMtxAttribArray, u32 numMtx, const math::MTX34 *pViewMtx, const ResMdl mdl,
    math::MTX34 *pViewTexMtxArray
) {
    if (numMtx <= 0) {
        return;
    }

    math::MTX34 *pVArray;
    math::MTX33 *pNArray;
    math::MTX34 *pCurTArray;

    u32 sizeVArray = align32(numMtx * sizeof(math::MTX34));
    u32 sizeNArray = align32(numMtx * sizeof(math::MTX33));
    u32 sizeTArray = align32(numMtx * sizeof(math::MTX34));

    pVArray = pViewPosArray;

    if (numMtx > 1) {
        math::MTX34MultArray(pVArray, pViewMtx, pModelMtxArray, numMtx);
    } else {
        math::MTX34Mult(pVArray, pViewMtx, pModelMtxArray);
    }

    math::MTX34 *pBbMtxArray = detail::workmem::GetBillboardMtxTemporary();

    for (u32 i = 0; i < numMtx; i++) {
        u32 attr = pModelMtxAttribArray[i];

        ResNodeData::Billboard bbType = detail::WorldMtxAttr::GetBillboard(attr);

        if (bbType != ResNodeData::BILLBOARD_OFF) {
            math::MTX34 &rVMtx = pVArray[i];

            bbFunc[bbType](&rVMtx, pModelMtxArray, detail::WorldMtxAttr::IsAllScaleUniform(attr), pViewMtx, mdl, i);

            s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(i);
            ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));

            if (node.IsValid() && node.GetChildNode().IsValid()) {
                math::MTX34 invWorld;

                u32 ret = detail::CalcInvWorldMtx(&invWorld, &pModelMtxArray[i]);
                if (ret == TRUE) {
                    math::MTX34Mult(&pBbMtxArray[i], &pVArray[i], &invWorld);
                } else {
                    math::MTX34Identity(&pBbMtxArray[i]);
                    pBbMtxArray[i]._03 = pViewMtx->_03;
                    pBbMtxArray[i]._13 = pViewMtx->_13;
                    pBbMtxArray[i]._23 = pViewMtx->_23;
                }
            }
        } else {
            s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(i);
            if (nodeID < 0) {
                continue;
            }

            ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));
            if (!node.IsValid()) {
                continue;
            }

            if (!(node.ref().flags & ResNodeData::FLAG_BILLBOARD_PARENT)) {
                continue;
            }

            u32 bbMtxID = mdl.GetResNode(node.ref().bbref_nodeid).GetMtxID();

            math::MTX34Mult(&pVArray[i], &pBbMtxArray[bbMtxID], &pModelMtxArray[i]);
        }
    }

    pNArray = pViewNrmArray;
    pCurTArray = pViewTexMtxArray;

    if (pNArray != NULL) {
        for (u32 i = 0; i < numMtx; i++) {
            math::MTX34 &rVMtx = pVArray[i];
            math::MTX33 &rNMtx = pNArray[i];
            u32 attr = pModelMtxAttribArray[i];

            if (detail::WorldMtxAttr::IsAllScaleUniform(attr)) {
                if (pCurTArray != NULL) {
                    math::MTX34Copy(&pCurTArray[i], &rVMtx);
                    pCurTArray[i]._03 = pCurTArray[i]._13 = pCurTArray[i]._23 = 0.0f;
                }

                math::MTX34ToMTX33(&rNMtx, &rVMtx);
            } else {
                if (pCurTArray != NULL) {
                    detail::CalcViewTexMtx(&pCurTArray[i], &rVMtx);
                    math::MTX34ToMTX33(&rNMtx, &pCurTArray[i]);
                } else {
                    detail::CalcViewNrmMtx(&rNMtx, &rVMtx);
                }
            }
        }
    }

    DC::FlushRangeNoSync(pVArray, sizeVArray);

    if (pNArray != NULL) {
        DC::FlushRangeNoSync(pViewNrmArray, sizeNArray);

        if (pCurTArray != NULL) {
            DC::FlushRangeNoSync(pViewTexMtxArray, sizeTArray);
        }
    }
}

void CalcView_LC(
    math::MTX34 *pViewPosArray, math::MTX33 *pViewNrmArray, const math::MTX34 *pModelMtxArray,
    const u32 *pModelMtxAttribArray, u32 numMtx, const math::MTX34 *pViewMtx, const ResMdl mdl,
    math::MTX34 *pViewTexMtxArray
) {
    if (numMtx <= 0) {
        return;
    }

    detail::MtxCacheMap *pMtxCache = static_cast<detail::MtxCacheMap *>(ut::LC::GetBase());

    math::MTX34 *pCurVArray = reinterpret_cast<math::MTX34 *>(pMtxCache->currViewArray);
    math::MTX33 *pCurNArray = reinterpret_cast<math::MTX33 *>(pMtxCache->currNrmArray);
    math::MTX34 *pCurTArray = reinterpret_cast<math::MTX34 *>(pMtxCache->currTexArray);

    math::MTX34 *pPrevVArray = reinterpret_cast<math::MTX34 *>(pMtxCache->prevViewArray);
    math::MTX33 *pPrevNArray = reinterpret_cast<math::MTX33 *>(pMtxCache->prevNrmArray);
    math::MTX34 *pPrevTArray = reinterpret_cast<math::MTX34 *>(pMtxCache->prevTexArray);

    u32 sizeVArray = align32(numMtx * sizeof(math::MTX34));
    DC::InvalidateRange(pViewPosArray, sizeVArray);

    u32 queueLen = 1;
    u32 sizeNArray, sizeTArray;

    if (pViewNrmArray != NULL) {
        queueLen++;

        sizeNArray = align32(numMtx * sizeof(math::MTX33));
        DC::InvalidateRange(pViewNrmArray, sizeNArray);

        if (pViewTexMtxArray != NULL) {
            queueLen++;

            sizeTArray = align32(numMtx * sizeof(math::MTX34));
            DC::InvalidateRange(pViewTexMtxArray, sizeTArray);
        }
    }

    math::MTX34 *pBbMtxArray = detail::workmem::GetBillboardMtxTemporary();

    ut::LC::QueueWait(0);

    u32 baseMtx;
    for (baseMtx = 0; baseMtx < numMtx; baseMtx += 40) {
        u32 remain = numMtx - baseMtx;

        u32 numBlocks, numBlocksNrm;
        if (remain > 40) {
            remain = 40;
            numBlocks = 60;
            numBlocksNrm = 45;
        } else {
            numBlocks = align32(remain * sizeof(math::MTX34)) / 32;
            numBlocksNrm = align32(remain * sizeof(math::MTX33)) / 32;
        }

        ut::LC::QueueWait(queueLen);

        if (remain > 1) {
            math::MTX34MultArray(pCurVArray, pViewMtx, &pModelMtxArray[baseMtx], remain);
        } else {
            math::MTX34Mult(pCurVArray, pViewMtx, &pModelMtxArray[baseMtx]);
        }

        for (u32 i = 0; i < remain; i++) {
            u32 attr = pModelMtxAttribArray[baseMtx + i];

            ResNodeData::Billboard bbType = detail::WorldMtxAttr::GetBillboard(attr);

            if (bbType != ResNodeData::BILLBOARD_OFF) {
                math::MTX34 &rVMtx = pCurVArray[i];

                bbFunc[bbType](
                    &rVMtx, pModelMtxArray, detail::WorldMtxAttr::IsAllScaleUniform(attr), pViewMtx, mdl, baseMtx + i
                );

                s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(baseMtx + i);

                ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));

                if (node.IsValid() && node.GetChildNode().IsValid()) {
                    math::MTX34 invWorld;

                    u32 ret = detail::CalcInvWorldMtx(&invWorld, &pModelMtxArray[baseMtx + i]);

                    if (ret == TRUE) {
                        math::MTX34Mult(&pBbMtxArray[baseMtx + i], &pCurVArray[i], &invWorld);
                    } else {
                        math::MTX34Identity(&pBbMtxArray[baseMtx + i]);
                        pBbMtxArray[baseMtx + i]._03 = pViewMtx->_03;
                        pBbMtxArray[baseMtx + i]._13 = pViewMtx->_13;
                        pBbMtxArray[baseMtx + i]._23 = pViewMtx->_23;
                    }
                }
            } else {
                s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(baseMtx + i);
                if (nodeID < 0) {
                    continue;
                }

                ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));
                if (!node.IsValid()) {
                    continue;
                }

                if (!(node.ref().flags & ResNodeData::FLAG_BILLBOARD_PARENT)) {
                    continue;
                }

                u32 bbMtxID = mdl.GetResNode(node.ref().bbref_nodeid).GetMtxID();

                math::MTX34Mult(&pCurVArray[i], &pBbMtxArray[bbMtxID], &pModelMtxArray[baseMtx + i]);
            }
        }

        ut::LC::StoreBlocks(&pViewPosArray[baseMtx], pCurVArray, numBlocks);

        if (pViewNrmArray != NULL) {
            for (u32 i = 0; i < remain; i++) {
                math::MTX34 &rVMtx = pCurVArray[i];
                math::MTX33 &rNMtx = pCurNArray[i];
                u32 attr = pModelMtxAttribArray[baseMtx + i];

                if (detail::WorldMtxAttr::IsAllScaleUniform(attr)) {
                    if (pViewTexMtxArray != NULL) {
                        math::MTX34Copy(&pCurTArray[i], &rVMtx);

                        pCurTArray[i]._03 = pCurTArray[i]._13 = pCurTArray[i]._23 = 0.0f;
                    }

                    math::MTX34ToMTX33(&rNMtx, &rVMtx);
                } else {
                    if (pViewTexMtxArray != NULL) {
                        detail::CalcViewTexMtx(&pCurTArray[i], &rVMtx);
                        math::MTX34ToMTX33(&rNMtx, &pCurTArray[i]);
                    } else {
                        detail::CalcViewNrmMtx(&rNMtx, &rVMtx);
                    }
                }
            }
        }

        if (pViewNrmArray != NULL) {
            ut::LC::StoreBlocks(&pViewNrmArray[baseMtx], pCurNArray, numBlocksNrm);

            if (pViewTexMtxArray != NULL) {
                ut::LC::StoreBlocks(&pViewTexMtxArray[baseMtx], pCurTArray, numBlocks);
            }
        }

        std::swap(pCurVArray, pPrevVArray);
        std::swap(pCurNArray, pPrevNArray);
        std::swap(pCurTArray, pPrevTArray);
    }
}

void CalcView_LC_DMA_ModelMtx(
    math::MTX34 *pViewPosArray, math::MTX33 *pViewNrmArray, const math::MTX34 *pModelMtxArray,
    const u32 *pModelMtxAttribArray, u32 numMtx, const math::MTX34 *pViewMtx, const ResMdl mdl,
    math::MTX34 *pViewTexMtxArray
) {
    if (numMtx <= 0) {
        return;
    }

    detail::MtxCacheMap *pMtxCache = static_cast<detail::MtxCacheMap *>(ut::LC::GetBase());

    math::MTX34 *pCurMArray = reinterpret_cast<math::MTX34 *>(pMtxCache->currDmaArray);
    math::MTX34 *pCurVArray = reinterpret_cast<math::MTX34 *>(pMtxCache->currViewArray);
    math::MTX33 *pCurNArray = reinterpret_cast<math::MTX33 *>(pMtxCache->currNrmArray);
    math::MTX34 *pCurTArray = reinterpret_cast<math::MTX34 *>(pMtxCache->currTexArray);

    math::MTX34 *pPrevMArray = reinterpret_cast<math::MTX34 *>(pMtxCache->prevDmaArray);
    math::MTX34 *pPrevVArray = reinterpret_cast<math::MTX34 *>(pMtxCache->prevViewArray);
    math::MTX33 *pPrevNArray = reinterpret_cast<math::MTX33 *>(pMtxCache->prevNrmArray);
    math::MTX34 *pPrevTArray = reinterpret_cast<math::MTX34 *>(pMtxCache->prevTexArray);

    u32 sizeVArray = align32(numMtx * sizeof(math::MTX34));
    DC::InvalidateRange(pViewPosArray, sizeVArray);

    u32 queueLen = 1;
    u32 sizeNArray, sizeTArray;

    if (pViewNrmArray != NULL) {
        queueLen++;

        sizeNArray = align32(numMtx * sizeof(math::MTX33));
        DC::InvalidateRange(pViewNrmArray, sizeNArray);

        if (pViewTexMtxArray != NULL) {
            queueLen++;

            sizeTArray = align32(numMtx * sizeof(math::MTX34));
            DC::InvalidateRange(pViewTexMtxArray, sizeTArray);
        }
    }

    math::MTX34 *pBbMtxArray = detail::workmem::GetBillboardMtxTemporary();

    ut::LC::QueueWait(0);

    u32 numBlocks;
    if (numMtx > 40) {
        numBlocks = 60;
    } else {
        numBlocks = align32(numMtx * sizeof(math::MTX34)) / 32;
    }

    ut::LC::LoadBlocks(pCurMArray, const_cast<math::MTX34 *>(pModelMtxArray), numBlocks);

    if (numMtx > 20) {
        ut::LC::QueueWaitEx(0);
    } else {
        ut::LC::QueueWait(0);
    }

    u32 baseMtx;
    for (baseMtx = 0; baseMtx < numMtx; baseMtx += 40) {
        u32 remain = numMtx - baseMtx;

        u32 numBlocks, numBlocksNrm;
        if (remain > 40) {
            remain = 40;
            numBlocks = 60;
            numBlocksNrm = 45;
        } else {
            numBlocks = align32(remain * sizeof(math::MTX34)) / 32;
            numBlocksNrm = align32(remain * sizeof(math::MTX33)) / 32;
        }

        ut::LC::QueueWait(queueLen);

        if (baseMtx + 40 < numMtx) {
            u32 n = numMtx - baseMtx - 40;

            u32 blocks;
            if (n > 40) {
                blocks = 60;
            } else {
                blocks = align32(n * sizeof(math::MTX34)) / 32;
            }

            ut::LC::LoadBlocks(pPrevMArray, const_cast<math::MTX34 *>(&pModelMtxArray[baseMtx + 40]), blocks);
        }

        if (remain > 1) {
            math::MTX34MultArray(pCurVArray, pViewMtx, pCurMArray, remain);
        } else {
            math::MTX34Mult(pCurVArray, pViewMtx, pCurMArray);
        }

        for (u32 i = 0; i < remain; i++) {
            u32 attr = pModelMtxAttribArray[baseMtx + i];

            ResNodeData::Billboard bbType = detail::WorldMtxAttr::GetBillboard(attr);

            if (bbType != ResNodeData::BILLBOARD_OFF) {
                math::MTX34 &rVMtx = pCurVArray[i];

                bbFunc[bbType](
                    &rVMtx, pModelMtxArray, detail::WorldMtxAttr::IsAllScaleUniform(attr), pViewMtx, mdl, baseMtx + i
                );

                s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(baseMtx + i);

                ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));

                if (node.IsValid() && node.GetChildNode().IsValid()) {
                    math::MTX34 invWorld;

                    u32 ret = detail::CalcInvWorldMtx(&invWorld, &pModelMtxArray[baseMtx + i]);

                    if (ret == TRUE) {
                        math::MTX34Mult(&pBbMtxArray[baseMtx + i], &pCurVArray[i], &invWorld);
                    } else {
                        math::MTX34Identity(&pBbMtxArray[baseMtx + i]);
                        pBbMtxArray[baseMtx + i]._03 = pViewMtx->_03;
                        pBbMtxArray[baseMtx + i]._13 = pViewMtx->_13;
                        pBbMtxArray[baseMtx + i]._23 = pViewMtx->_23;
                    }
                }
            } else {
                s32 nodeID = mdl.GetResMdlInfo().GetNodeIDFromMtxID(baseMtx + i);
                if (nodeID < 0) {
                    continue;
                }

                ResNode node = mdl.GetResNode(static_cast<u32>(nodeID));
                if (!node.IsValid()) {
                    continue;
                }

                if (!(node.ref().flags & ResNodeData::FLAG_BILLBOARD_PARENT)) {
                    continue;
                }

                u32 bbMtxID = mdl.GetResNode(node.ref().bbref_nodeid).GetMtxID();

                math::MTX34Mult(&pCurVArray[i], &pBbMtxArray[bbMtxID], &pModelMtxArray[baseMtx + i]);
            }
        }

        ut::LC::StoreBlocks(&pViewPosArray[baseMtx], pCurVArray, numBlocks);

        if (pViewNrmArray != NULL) {
            for (u32 i = 0; i < remain; i++) {
                math::MTX34 &rVMtx = pCurVArray[i];
                math::MTX33 &rNMtx = pCurNArray[i];
                u32 attr = pModelMtxAttribArray[baseMtx + i];

                if (detail::WorldMtxAttr::IsAllScaleUniform(attr)) {
                    if (pViewTexMtxArray != NULL) {
                        math::MTX34Copy(&pCurTArray[i], &rVMtx);

                        pCurTArray[i]._03 = pCurTArray[i]._13 = pCurTArray[i]._23 = 0.0f;
                    }

                    math::MTX34ToMTX33(&rNMtx, &rVMtx);
                } else {
                    if (pViewTexMtxArray != NULL) {
                        detail::CalcViewTexMtx(&pCurTArray[i], &rVMtx);
                        math::MTX34ToMTX33(&rNMtx, &pCurTArray[i]);
                    } else {
                        detail::CalcViewNrmMtx(&rNMtx, &rVMtx);
                    }
                }
            }
        }

        if (pViewNrmArray != NULL) {
            ut::LC::StoreBlocks(&pViewNrmArray[baseMtx], pCurNArray, numBlocksNrm);

            if (pViewTexMtxArray != NULL) {
                ut::LC::StoreBlocks(&pViewTexMtxArray[baseMtx], pCurTArray, numBlocks);
            }
        }

        std::swap(pCurMArray, pPrevMArray);
        std::swap(pCurVArray, pPrevVArray);
        std::swap(pCurNArray, pPrevNArray);
        std::swap(pCurTArray, pPrevTArray);
    }
}

} // namespace g3d
} // namespace nw4r
