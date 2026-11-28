#include "nw4r/math.h"

#include "nw4r/ef/ef_util.h"
#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_list.h"

#include <cmath>

namespace nw4r {
namespace ef {

u16 UtlistToArray(const ut::List* pList, void** ppArray, int maxlen) {
    u16 num = 0;

    NW4R_UT_LIST_FOREACH (void, it, *pList, {
        ppArray[num++] = it;

        if (num >= maxlen) {
            break;
        }
    })

    return num;
}

void GetDirMtxY(math::MTX34* pMtx, const math::VEC3& rVec) {
    f32 sx, cx;
    f32 sz, cz;

    sx = rVec.z;

    if (1.0f - math::FAbs(sx) < NW4R_MATH_FLT_EPSILON) {
        cx = 0.0f;
        sz = 0.0f;
        cz = 1.0f;
    } else {
        f32 sq = 1.0f - sx * sx;
        cx = math::FSqrt(sq);
        cz = rVec.y / cx;
        sz = rVec.x / -cx;
    }

    pMtx->_00 = cz;
    pMtx->_01 = rVec.x;
    pMtx->_02 = sx * sz;
    pMtx->_03 = 0.0f;

    pMtx->_10 = sz;
    pMtx->_11 = rVec.y;
    pMtx->_12 = -cz * sx;
    pMtx->_13 = 0.0f;

    pMtx->_20 = 0.0f;
    pMtx->_21 = rVec.z;
    pMtx->_22 = cx;
    pMtx->_23 = 0.0f;
}

void MtxGetRotationMtx(const math::MTX34& rMtx, math::MTX34* pDst) {
    math::VEC3 x, y, z;

    MtxColVec(&rMtx, 0, &x);
    if (!VecNormalize(&x, &x)) {
        x.x = 1.0f;
    }

    MtxColVec(&rMtx, 1, &y);
    if (!VecNormalize(&y, &y)) {
        y.y = 1.0f;
    }

    math::VEC3Cross(&z, &x, &y);
    math::VEC3Cross(&y, &z, &x);

    MtxSetColVec(pDst, 0, &x);
    MtxSetColVec(pDst, 1, &y);
    MtxSetColVec(pDst, 2, &z);

    pDst->_03 = 0.0f;
    pDst->_13 = 0.0f;
    pDst->_23 = 0.0f;
}

void MtxGetRotation(const math::MTX34& rMtx, math::VEC3* pRot) {
    f32 C;
    f32 sx, sy, sz;
    f32 f;

    if (!((sx = MtxColVecLen(&rMtx, 0)) < NW4R_MATH_FLT_MIN) &&
        !((sy = MtxColVecLen(&rMtx, 1)) < NW4R_MATH_FLT_MIN) &&
        !((sz = MtxColVecLen(&rMtx, 2)) < NW4R_MATH_FLT_MIN)) {

        f = -rMtx._20 / sx;

        if (f > 1.0f) {
            f = 1.0f;
        }

        if (f < -1.0f) {
            f = -1.0f;
        }

        pRot->y = std::asinf(f);
        C = math::CosRad(pRot->y);

        if (C >= NW4R_MATH_FLT_MIN) {
            pRot->x = std::atan2f(rMtx._21 / sy, rMtx._22 / sz);
            pRot->z = std::atan2f(rMtx._10, rMtx._00);
        } else {
            pRot->x = std::atan2f(rMtx._01, rMtx._11);
            pRot->z = 0.0f;
        }
    } else {
        pRot->x = 0.0f;
        pRot->y = 0.0f;
        pRot->z = 0.0f;
    }
}

void MtxGetTranslate(const math::MTX34& rMtx, math::VEC3* pTrans) {
    pTrans->x = rMtx._03;
    pTrans->y = rMtx._13;
    pTrans->z = rMtx._23;
}

void MtxGetScale(const math::MTX34& rMtx, math::VEC3* pScale) {
    f32 sh[3];
    math::VEC3 v0, v1, v2, v;
    f32 mag;

    MtxColVec(&rMtx, 0, &v0);
    mag = math::VEC3LenSq(&v0);

    if (mag > NW4R_MATH_FLT_EPSILON) {
        mag = math::FrSqrt(mag);
        pScale->x = math::FInv(mag);

        math::VEC3Scale(&v0, &v0, mag);
        MtxColVec(&rMtx, 1, &v1);

        sh[0] = math::VEC3Dot(&v0, &v1);
        math::VEC3Scale(&v, &v0, sh[0]);
        math::VEC3Sub(&v1, &v1, &v);

        mag = math::VEC3LenSq(&v1);

        if (mag > NW4R_MATH_FLT_EPSILON) {
            mag = math::FrSqrt(mag);
            pScale->y = math::FInv(mag);

            sh[0] *= mag;
            math::VEC3Scale(&v1, &v1, mag);
            MtxColVec(&rMtx, 2, &v2);

            sh[2] = math::VEC3Dot(&v1, &v2);
            math::VEC3Scale(&v, &v1, sh[2]);
            math::VEC3Sub(&v2, &v2, &v);

            sh[1] = math::VEC3Dot(&v0, &v2);
            math::VEC3Scale(&v, &v0, sh[1]);
            math::VEC3Sub(&v2, &v2, &v);

            mag = math::VEC3LenSq(&v2);

            if (mag > NW4R_MATH_FLT_EPSILON) {
                pScale->z = math::FSqrt(mag);
                math::VEC3Cross(&v, &v1, &v2);

                if (math::VEC3Dot(&v0, &v) < 0.0f) {
                    // Double precision
                    pScale->x *= -1.0;
                    pScale->y *= -1.0;
                    pScale->z *= -1.0;
                }
            } else {
                pScale->z = 0.0f;
            }
        } else {
            pScale->y = 0.0f;
            MtxColVec(&rMtx, 2, &v2);

            sh[1] = math::VEC3Dot(&v0, &v2);
            math::VEC3Scale(&v, &v0, sh[1]);
            math::VEC3Sub(&v2, &v2, &v);

            mag = math::VEC3LenSq(&v2);

            if (mag > NW4R_MATH_FLT_EPSILON) {
                pScale->z = math::FSqrt(mag);
            } else {
                pScale->z = 0.0f;
            }
        }

    } else {
        pScale->x = 0.0f;

        MtxColVec(&rMtx, 1, &v1);
        mag = math::VEC3LenSq(&v1);

        if (mag > NW4R_MATH_FLT_EPSILON) {
            mag = math::FrSqrt(mag);
            pScale->y = math::FInv(mag);

            math::VEC3Scale(&v1, &v1, mag);
            MtxColVec(&rMtx, 2, &v2);

            sh[2] = math::VEC3Dot(&v1, &v2);
            math::VEC3Scale(&v, &v1, sh[2]);
            math::VEC3Sub(&v2, &v2, &v);
            pScale->z = math::VEC3Len(&v2);
        } else {
            pScale->y = 0.0f;

            MtxColVec(&rMtx, 2, &v2);
            pScale->z = math::VEC3Len(&v2);
        }
    }
}

// NONMATCHING: retail uses paired-single assembly
BOOL VecNormalize(math::VEC3* pOut, const math::VEC3* pIn) {
    f32 x = pIn->x;
    f32 y = pIn->y;
    f32 z = pIn->z;
    f32 lenSq = x * x + y * y + z * z;

    if (lenSq != 0.0f) {
        f32 rsqrt = math::FrSqrt(lenSq);
        pOut->x = x * rsqrt;
        pOut->y = y * rsqrt;
        pOut->z = z * rsqrt;
        return TRUE;
    }

    pOut->x = x;
    pOut->y = y;
    pOut->z = z;
    return FALSE;
}

// NONMATCHING: retail uses paired-single assembly
void SinCosRad(f32* pSinCos, f32 rad) {
    math::SinCosFIdx(&pSinCos[0], &pSinCos[1], NW4R_MATH_RAD_TO_FIDX(rad));
}

// NONMATCHING: retail uses paired-single assembly
void SinCosRad(f32* pSin, f32* pCos, f32 rad) {
    math::SinCosFIdx(pSin, pCos, NW4R_MATH_RAD_TO_FIDX(rad));
}

// NONMATCHING: retail uses paired-single assembly
math::VEC3& Rotation2VecY(const math::VEC3& rRot, math::VEC3* pVec) {
    f32 sx, cx;
    f32 sy, cy;
    f32 sz, cz;

    SinCosRad(&sx, &cx, rRot.x);
    SinCosRad(&sy, &cy, rRot.y);
    SinCosRad(&sz, &cz, rRot.z);

    pVec->x = sx * sy * cz - cx * sz;
    pVec->y = sx * sy * sz + cx * cz;
    pVec->z = sx * cy;
    return *pVec;
}

// NONMATCHING: retail uses paired-single assembly
void MtxRotXYZRad(math::MTX34* pMtx, f32 rx, f32 ry, f32 rz) {
    f32 sx, cx;
    f32 sy, cy;
    f32 sz, cz;

    SinCosRad(&sx, &cx, rx);
    SinCosRad(&sy, &cy, ry);
    SinCosRad(&sz, &cz, rz);

    pMtx->_00 = cy * cz;
    pMtx->_01 = sx * sy * cz - cx * sz;
    pMtx->_02 = cx * sy * cz + sx * sz;
    pMtx->_03 = 0.0f;

    pMtx->_10 = cy * sz;
    pMtx->_11 = sx * sy * sz + cx * cz;
    pMtx->_12 = cx * sy * sz - sx * cz;
    pMtx->_13 = 0.0f;

    pMtx->_20 = -sy;
    pMtx->_21 = sx * cy;
    pMtx->_22 = cx * cy;
    pMtx->_23 = 0.0f;
}

// NONMATCHING: retail uses paired-single assembly
void MtxScale(math::MTX34* pOut, const math::MTX34* pIn,
              const math::VEC3* pScale) {
    f32 sx = pScale->x != 0.0f ? pScale->x : 0.00001f;
    f32 sy = pScale->y != 0.0f ? pScale->y : 0.00001f;
    f32 sz = pScale->z != 0.0f ? pScale->z : 0.00001f;

    pOut->_00 = pIn->_00 * sx;
    pOut->_01 = pIn->_01 * sy;
    pOut->_02 = pIn->_02 * sz;
    pOut->_03 = pIn->_03;

    pOut->_10 = pIn->_10 * sx;
    pOut->_11 = pIn->_11 * sy;
    pOut->_12 = pIn->_12 * sz;
    pOut->_13 = pIn->_13;

    pOut->_20 = pIn->_20 * sx;
    pOut->_21 = pIn->_21 * sy;
    pOut->_22 = pIn->_22 * sz;
    pOut->_23 = pIn->_23;
}

// NONMATCHING: retail uses paired-single assembly
void MtxScale(math::MTX34* pOut, const math::VEC3* pScale,
              const math::MTX34* pIn) {
    f32 sx = pScale->x != 0.0f ? pScale->x : 0.00001f;
    f32 sy = pScale->y != 0.0f ? pScale->y : 0.00001f;
    f32 sz = pScale->z != 0.0f ? pScale->z : 0.00001f;

    pOut->_00 = pIn->_00 * sx;
    pOut->_01 = pIn->_01 * sx;
    pOut->_02 = pIn->_02 * sx;
    pOut->_03 = pIn->_03 * sx;

    pOut->_10 = pIn->_10 * sy;
    pOut->_11 = pIn->_11 * sy;
    pOut->_12 = pIn->_12 * sy;
    pOut->_13 = pIn->_13 * sy;

    pOut->_20 = pIn->_20 * sz;
    pOut->_21 = pIn->_21 * sz;
    pOut->_22 = pIn->_22 * sz;
    pOut->_23 = pIn->_23 * sz;
}

math::MTX34* MtxInvScale(math::MTX34* pOut, const math::MTX34* pIn,
                         const math::VEC3* pScale) {
    math::VEC3 inv;

    inv.x = 0.0f != pScale->x ? math::FInv(pScale->x) : 100000.0f;
    inv.y = 0.0f != pScale->y ? math::FInv(pScale->y) : 100000.0f;
    inv.z = 0.0f != pScale->z ? math::FInv(pScale->z) : 100000.0f;

    return math::MTX34Scale(pOut, pIn, &inv);
}

// NONMATCHING: retail uses paired-single assembly
f32 MtxColVecLen(const math::MTX34* pMtx, int col) {
    f32 x = pMtx->m[0][col];
    f32 y = pMtx->m[1][col];
    f32 z = pMtx->m[2][col];
    return math::FSqrt(x * x + y * y + z * z);
}

} // namespace ef
} // namespace nw4r
