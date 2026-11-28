#include <nw4r/math.h>

namespace nw4r {
namespace math {

VEC3* VEC3Maximize(VEC3* pOut, const VEC3* pA, const VEC3* pB) {
    pOut->x = (pA->x >= pB->x) ? pA->x : pB->x;
    pOut->y = (pA->y >= pB->y) ? pA->y : pB->y;
    pOut->z = (pA->z >= pB->z) ? pA->z : pB->z;
    return pOut;
}

VEC3* VEC3Minimize(VEC3* pOut, const VEC3* pA, const VEC3* pB) {
    pOut->x = (pB->x >= pA->x) ? pA->x : pB->x;
    pOut->y = (pB->y >= pA->y) ? pA->y : pB->y;
    pOut->z = (pB->z >= pA->z) ? pA->z : pB->z;
    return pOut;
}

// NONMATCHING: retail uses paired-single assembly
MTX33* MTX33Identity(MTX33* pMtx) {
    pMtx->_00 = 1.0f;
    pMtx->_01 = 0.0f;
    pMtx->_02 = 0.0f;

    pMtx->_10 = 0.0f;
    pMtx->_11 = 1.0f;
    pMtx->_12 = 0.0f;

    pMtx->_20 = 0.0f;
    pMtx->_21 = 0.0f;
    pMtx->_22 = 1.0f;

    return pMtx;
}

// NONMATCHING: retail uses paired-single assembly
MTX33* MTX34ToMTX33(MTX33* pOut, const MTX34* pIn) {
    pOut->_00 = pIn->_00;
    pOut->_01 = pIn->_01;
    pOut->_02 = pIn->_02;

    pOut->_10 = pIn->_10;
    pOut->_11 = pIn->_11;
    pOut->_12 = pIn->_12;

    pOut->_20 = pIn->_20;
    pOut->_21 = pIn->_21;
    pOut->_22 = pIn->_22;

    return pOut;
}

// NONMATCHING: retail uses paired-single assembly
u32 MTX34InvTranspose(MTX33* pOut, const MTX34* pIn) {
    f32 a = pIn->_00;
    f32 b = pIn->_01;
    f32 c = pIn->_02;
    f32 d = pIn->_10;
    f32 e = pIn->_11;
    f32 f = pIn->_12;
    f32 g = pIn->_20;
    f32 h = pIn->_21;
    f32 i = pIn->_22;

    f32 c00 = e * i - h * f;
    f32 c01 = f * g - i * d;
    f32 c02 = d * h - e * g;

    f32 c10 = h * c - b * i;
    f32 c11 = i * a - c * g;
    f32 c12 = b * g - a * h;

    f32 c20 = b * f - e * c;
    f32 c21 = c * d - f * a;
    f32 c22 = a * e - b * d;

    f32 det = a * c00 + d * c10 + g * c20;
    if (det == 0.0f) {
        return FALSE;
    }

    f32 invDet = 1.0f / det;

    pOut->_00 = c00 * invDet;
    pOut->_01 = c01 * invDet;
    pOut->_02 = c02 * invDet;

    pOut->_10 = c10 * invDet;
    pOut->_11 = c11 * invDet;
    pOut->_12 = c12 * invDet;

    pOut->_20 = c20 * invDet;
    pOut->_21 = c21 * invDet;
    pOut->_22 = c22 * invDet;

    return TRUE;
}

// NONMATCHING: retail uses paired-single assembly
MTX34* MTX34Zero(MTX34* pMtx) {
    pMtx->_00 = 0.0f;
    pMtx->_01 = 0.0f;
    pMtx->_02 = 0.0f;
    pMtx->_03 = 0.0f;

    pMtx->_10 = 0.0f;
    pMtx->_11 = 0.0f;
    pMtx->_12 = 0.0f;
    pMtx->_13 = 0.0f;

    pMtx->_20 = 0.0f;
    pMtx->_21 = 0.0f;
    pMtx->_22 = 0.0f;
    pMtx->_23 = 0.0f;

    return pMtx;
}

// NONMATCHING: retail uses paired-single assembly
MTX34* MTX34Scale(MTX34* pOut, const MTX34* pIn, const VEC3* pScale) {
    f32 sx = pScale->x;
    f32 sy = pScale->y;
    f32 sz = pScale->z;

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

    return pOut;
}

// NONMATCHING: retail uses paired-single assembly
MTX34* MTX34Trans(MTX34* pOut, const MTX34* pIn, const VEC3* pTrans) {
    f32 tx = pTrans->x;
    f32 ty = pTrans->y;
    f32 tz = pTrans->z;

    f32 t0 = pIn->_00 * tx + pIn->_01 * ty + pIn->_02 * tz + pIn->_03;
    f32 t1 = pIn->_10 * tx + pIn->_11 * ty + pIn->_12 * tz + pIn->_13;
    f32 t2 = pIn->_20 * tx + pIn->_21 * ty + pIn->_22 * tz + pIn->_23;

    if (pOut != pIn) {
        pOut->_00 = pIn->_00;
        pOut->_01 = pIn->_01;
        pOut->_02 = pIn->_02;
        pOut->_10 = pIn->_10;
        pOut->_11 = pIn->_11;
        pOut->_12 = pIn->_12;
        pOut->_20 = pIn->_20;
        pOut->_21 = pIn->_21;
        pOut->_22 = pIn->_22;
    }

    pOut->_03 = t0;
    pOut->_13 = t1;
    pOut->_23 = t2;

    return pOut;
}

MTX34* MTX34RotAxisFIdx(MTX34* pMtx, const VEC3* pAxis, f32 fidx) {
    PSMTXRotAxisRad(*pMtx, *pAxis, NW4R_MATH_FIDX_TO_RAD(fidx));
    return pMtx;
}

// NONMATCHING: retail uses paired-single assembly
MTX34* MTX34RotXYZFIdx(MTX34* pMtx, f32 x, f32 y, f32 z) {
    f32 sx, cx;
    f32 sy, cy;
    f32 sz, cz;

    SinCosFIdx(&sx, &cx, x);
    SinCosFIdx(&sy, &cy, y);
    SinCosFIdx(&sz, &cz, z);

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

    return pMtx;
}

// NONMATCHING: retail uses paired-single assembly
MTX34* MTX33ToMTX34(MTX34* pOut, const MTX33* pIn) {
    f32 m00 = pIn->_00;
    f32 m01 = pIn->_01;
    f32 m02 = pIn->_02;
    f32 m10 = pIn->_10;
    f32 m11 = pIn->_11;
    f32 m12 = pIn->_12;
    f32 m20 = pIn->_20;
    f32 m21 = pIn->_21;
    f32 m22 = pIn->_22;

    pOut->_00 = m00;
    pOut->_01 = m01;
    pOut->_02 = m02;

    pOut->_10 = m10;
    pOut->_11 = m11;
    pOut->_12 = m12;

    pOut->_20 = m20;
    pOut->_21 = m21;
    pOut->_22 = m22;

    return pOut;
}

VEC3* VEC3TransformNormal(VEC3* pOut, const MTX34* pMtx, const VEC3* pVec) {
    VEC3 tmp;
    tmp.x = pMtx->_00 * pVec->x + pMtx->_01 * pVec->y + pMtx->_02 * pVec->z;
    tmp.y = pMtx->_10 * pVec->x + pMtx->_11 * pVec->y + pMtx->_12 * pVec->z;
    tmp.z = pMtx->_20 * pVec->x + pMtx->_21 * pVec->y + pMtx->_22 * pVec->z;

    pOut->x = tmp.x;
    pOut->y = tmp.y;
    pOut->z = tmp.z;

    return pOut;
}

// NONMATCHING: retail uses paired-single assembly
MTX44* MTX44Identity(MTX44* pMtx) {
    pMtx->_00 = 1.0f;
    pMtx->_01 = 0.0f;
    pMtx->_02 = 0.0f;
    pMtx->_03 = 0.0f;

    pMtx->_10 = 0.0f;
    pMtx->_11 = 1.0f;
    pMtx->_12 = 0.0f;
    pMtx->_13 = 0.0f;

    pMtx->_20 = 0.0f;
    pMtx->_21 = 0.0f;
    pMtx->_22 = 1.0f;
    pMtx->_23 = 0.0f;

    pMtx->_30 = 0.0f;
    pMtx->_31 = 0.0f;
    pMtx->_32 = 0.0f;
    pMtx->_33 = 1.0f;

    return pMtx;
}

// NONMATCHING: retail uses paired-single assembly
MTX44* MTX44Copy(MTX44* pDst, const MTX44* pSrc) {
    *pDst = *pSrc;
    return pDst;
}

} // namespace math
} // namespace nw4r
