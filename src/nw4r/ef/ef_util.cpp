#include "nw4r/math.h"

#include "nw4r/ef/ef_util.h"
#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_list.h"

#include <cmath>

namespace nw4r {
namespace ef {

// Data of the hand-written paired-single routines at the end of this file.
// They are asm functions and cannot name a namespace-qualified symbol.
using math::detail::gSinCosTbl;

// One turn in table index units (see SinCosRad), for both halves of a pair
static const f32 scMax[2] = {65536.0f, 65536.0f};
// What MtxScale(pOut, pScale, pIn) takes a zero scale for
static const f32 scEpsilon = 0.00001f;
// 3.0 and 0.5 of the Newton-Raphson step in MtxColVecLen
static f32 sRsqrtConst[2] = {3.0f, 0.5f};

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

BOOL VecNormalize(register math::VEC3* pOut, register const math::VEC3* pIn) {
    register f32 half = 0.5f;
    register f32 three = 3.0f;
    register f32 zero, xy, z, lenSq, sq, rsqrt, work;

    asm {
        psq_l  xy, 0(pIn), 0, 0
        ps_mul sq, xy, xy
        psq_l  z, 8(pIn), 1, 0
        fsubs  zero, half, half
        ps_madd lenSq, z, z, sq
        ps_sum0 lenSq, lenSq, z, sq
    }

    if (lenSq != zero) {
        asm {
            frsqrte rsqrt, lenSq
            fmuls work, rsqrt, rsqrt
            fmuls half, rsqrt, half
            fnmsubs work, work, lenSq, three
            fmuls rsqrt, work, half
            ps_muls0 xy, xy, rsqrt
            ps_muls0 z, z, rsqrt
            psq_st xy, 0(pOut), 0, 0
            psq_st z, 8(pOut), 1, 0
        }

        return TRUE;
    }

    asm {
        psq_st xy, 0(pOut), 0, 0
        psq_st z, 8(pOut), 1, 0
    }

    return FALSE;
}

void SinCosRad(register f32* pSinCos, register f32 rad) {
    register f32 fidx = NW4R_MATH_RAD_TO_FIDX(1.0f);
    register const math::SinCosSample* pSample = math::detail::gSinCosTbl;
    register f32 max = 65536.0f;
    register f32 abs;
    register f32 whole, val, zero, delta, tmp;

    asm {
        fmuls  fidx, rad, fidx
        fabs   abs, fidx
        psq_st abs, 0(pSinCos), 1, 3
        fcmpu  cr0, abs, max
        ble    _inRange
    _wrap:
        fsubs  abs, abs, max
        fcmpu  cr0, abs, max
        bge    _wrap
        psq_st abs, 0(pSinCos), 1, 3
    _inRange:
    }

    pSample += *reinterpret_cast<u16*>(pSinCos) & 0xFF;

    asm {
        fsubs  zero, max, max
        psq_l  whole, 0(pSinCos), 1, 3
        fcmpu  cr0, fidx, zero
        psq_l  val, 0(pSample), 0, 0
        fsubs  whole, abs, whole
        psq_l  delta, 8(pSample), 0, 0
        ps_madds0 abs, delta, whole, val
        bge    _positive
        ps_neg tmp, abs
        ps_merge01 abs, tmp, abs
    _positive:
        psq_st abs, 0(pSinCos), 0, 0
    }
}

void SinCosRad(register f32* pSin, register f32* pCos, register f32 rad) {
    register f32 fidx = NW4R_MATH_RAD_TO_FIDX(1.0f);
    register const math::SinCosSample* pSample = math::detail::gSinCosTbl;
    register f32 max = 65536.0f;
    register f32 abs;
    register f32 whole, val, zero, delta;

    asm {
        fmuls  fidx, rad, fidx
        fabs   abs, fidx
        psq_st abs, 0(pSin), 1, 3
        fcmpu  cr0, abs, max
        ble    _inRange
    _wrap:
        fsubs  abs, abs, max
        fcmpu  cr0, abs, max
        bge    _wrap
        psq_st abs, 0(pSin), 1, 3
    _inRange:
    }

    pSample += *reinterpret_cast<u16*>(pSin) & 0xFF;

    asm {
        fsubs  zero, max, max
        psq_l  whole, 0(pSin), 1, 3
        fcmpu  cr0, fidx, zero
        psq_l  val, 0(pSample), 0, 0
        fsubs  whole, abs, whole
        psq_l  delta, 8(pSample), 0, 0
        ps_madds0 delta, delta, whole, val
        ps_merge10 whole, delta, delta
        psq_st whole, 0(pCos), 1, 0
        bge    _positive
        ps_neg delta, delta
    _positive:
        psq_st delta, 0(pSin), 1, 0
    }
}

// Hand-written paired-single routine: r3 = rRot, r4 = pVec
asm math::VEC3& Rotation2VecY(const math::VEC3& rRot, math::VEC3* pVec) {
    // clang-format off
    nofralloc

        lfs f0, 0(r3)
        lis r5, gSinCosTbl@ha
        lfs f1, 4(r3)
        la r0, scMax
        stwu r1, -16(r1)
        addi r5, r5, gSinCosTbl@l
        lfs f2, 8(r3)
        psq_lx f3, r0, r0, 0, 0
        ps_merge00 f6, f0, f1
        lfs f1, 40.743664f
        ps_merge00 f3, f3, f3
        ps_muls0 f6, f6, f1
        ps_neg f0, f3
        ps_abs f5, f6
        ps_sub f4, f3, f3
        ps_cmpu0 cr0, f5, f3
        ble _inRangeX
    _wrapX:
        ps_sum0 f5, f5, f5, f0
        ps_cmpu0 cr0, f5, f3
        bge _wrapX
    _inRangeX:
        ps_cmpu1 cr0, f5, f3
        ble _inRangeY
        ps_merge10 f5, f5, f5
    _wrapY:
        ps_sum0 f5, f5, f5, f0
        ps_cmpu0 cr0, f5, f3
        bge _wrapY
        ps_merge10 f5, f5, f5
    _inRangeY:
        psq_st f5, 8(r1), 0, 3
        fmuls f7, f2, f1
        psq_l f8, 8(r1), 0, 3
        fabs f2, f7
        lwz r0, 8(r1)
        fcmpu cr0, f2, f3
        ble _inRangeZ
    _wrapZ:
        fsubs f2, f2, f3
        fcmpu cr0, f2, f3
        bge _wrapZ
    _inRangeZ:
        psq_st f2, 8(r1), 1, 3
        rlwinm r3, r0, 20, 20, 27
        add r3, r5, r3
        ps_sub f8, f5, f8
        psq_l f3, 0(r3), 0, 0
        clrlslwi r6, r0, 24, 4
        psq_l f5, 8(r3), 0, 0
        ps_cmpu0 cr0, f6, f4
        add r6, r5, r6
        ps_madds0 f0, f5, f8, f3
        psq_l f3, 0(r6), 0, 0
        psq_l f5, 8(r6), 0, 0
        lhz r0, 8(r1)
        bge _positiveX
        ps_neg f9, f0
        ps_merge01 f0, f9, f0
    _positiveX:
        ps_madds1 f1, f5, f8, f3
        psq_l f8, 8(r1), 1, 3
        clrlslwi r0, r0, 24, 4
        ps_cmpu1 cr0, f6, f4
        add r3, r5, r0
        fsubs f8, f2, f8
        psq_l f3, 0(r3), 0, 0
        psq_l f5, 8(r3), 0, 0
        bge _positiveY
        ps_neg f9, f1
        ps_merge01 f1, f9, f1
    _positiveY:
        fcmpu cr0, f7, f4
        ps_madds0 f2, f5, f8, f3
        bge _positiveZ
        ps_neg f9, f2
        ps_merge01 f2, f9, f2
    _positiveZ:
        ps_muls1 f3, f2, f0
        mr r3, r4
        ps_muls0 f1, f1, f0
        ps_merge10 f2, f2, f2
        ps_neg f0, f3
        ps_merge01 f3, f0, f3
        ps_madds0 f2, f2, f1, f3
        ps_merge10 f1, f1, f1
        psq_st f2, 0(r4), 0, 0
        psq_st f1, 8(r4), 1, 0
        addi r1, r1, 0x10
        blr
    // clang-format on
}

// Hand-written paired-single routine: r3 = pMtx, f1-f3 = rx, ry, rz
asm void MtxRotXYZRad(math::MTX34* pMtx, f32 rx, f32 ry, f32 rz) {
    // clang-format off
    nofralloc

        lis r4, gSinCosTbl@ha
        la r0, scMax
        stwu r1, -16(r1)
        addi r4, r4, gSinCosTbl@l
        psq_lx f0, r0, r0, 0, 0
        ps_merge00 f6, f1, f2
        lfs f2, 40.743664f
        ps_merge00 f0, f0, f0
        ps_muls0 f6, f6, f2
        ps_neg f1, f0
        ps_abs f5, f6
        ps_sub f4, f0, f0
        ps_cmpu0 cr0, f5, f0
        ble _inRangeX
    _wrapX:
        ps_sum0 f5, f5, f5, f1
        ps_cmpu0 cr0, f5, f0
        bge _wrapX
    _inRangeX:
        ps_cmpu1 cr0, f5, f0
        ble _inRangeY
        ps_merge10 f5, f5, f5
    _wrapY:
        ps_sum0 f5, f5, f5, f1
        ps_cmpu0 cr0, f5, f0
        bge _wrapY
        ps_merge10 f5, f5, f5
    _inRangeY:
        psq_st f5, 8(r1), 0, 3
        fmuls f3, f3, f2
        psq_l f7, 8(r1), 0, 3
        fabs f2, f3
        lwz r0, 8(r1)
        fcmpu cr0, f2, f0
        ble _inRangeZ
    _wrapZ:
        fsubs f2, f2, f0
        fcmpu cr0, f2, f0
        bge _wrapZ
    _inRangeZ:
        psq_st f2, 8(r1), 1, 3
        rlwinm r5, r0, 20, 20, 27
        add r5, r4, r5
        ps_sub f7, f5, f7
        psq_l f5, 0(r5), 0, 0
        clrlslwi r6, r0, 24, 4
        psq_l f8, 8(r5), 0, 0
        ps_cmpu0 cr0, f6, f4
        add r6, r4, r6
        ps_madds0 f0, f8, f7, f5
        psq_l f5, 0(r6), 0, 0
        psq_l f8, 8(r6), 0, 0
        lhz r0, 8(r1)
        bge _positiveX
        ps_neg f9, f0
        ps_merge01 f0, f9, f0
    _positiveX:
        ps_madds1 f1, f8, f7, f5
        psq_l f7, 8(r1), 1, 3
        clrlslwi r0, r0, 24, 4
        ps_cmpu1 cr0, f6, f4
        add r5, r4, r0
        fsubs f7, f2, f7
        psq_l f5, 0(r5), 0, 0
        psq_l f8, 8(r5), 0, 0
        bge _positiveY
        ps_neg f9, f1
        ps_merge01 f1, f9, f1
    _positiveY:
        fcmpu cr0, f3, f4
        ps_madds0 f2, f8, f7, f5
        bge _positiveZ
        ps_neg f9, f2
        ps_merge01 f2, f9, f2
    _positiveZ:
        ps_neg f3, f0
        ps_muls1 f5, f2, f1
        ps_sub f7, f0, f0
        ps_merge10 f3, f3, f0
        ps_merge10 f6, f5, f5
        ps_muls0 f4, f0, f2
        psq_st f7, 44(r3), 1, 0
        ps_muls0 f8, f3, f2
        psq_st f6, 0(r3), 1, 0
        ps_muls1 f6, f0, f2
        ps_muls1 f2, f3, f2
        ps_madds0 f6, f6, f1, f8
        ps_neg f2, f2
        psq_st f6, 4(r3), 0, 0
        ps_merge00 f6, f7, f5
        psq_st f6, 12(r3), 0, 0
        ps_madds0 f6, f4, f1, f2
        psq_st f6, 20(r3), 0, 0
        ps_neg f6, f1
        ps_merge00 f6, f7, f6
        psq_st f6, 28(r3), 0, 0
        ps_muls1 f6, f0, f1
        psq_st f6, 36(r3), 0, 0
        addi r1, r1, 0x10
        blr
    // clang-format on
}

// Hand-written paired-single routine: r3 = pOut, r4 = pIn, r5 = pScale
asm void MtxScale(math::MTX34* pOut, const math::MTX34* pIn,
                  const math::VEC3* pScale) {
    // clang-format off
    nofralloc

        psq_l f0, 0(r5), 0, 0
        lfs f8, 0.00001f
        ps_sub f6, f0, f0
        psq_l f1, 8(r5), 1, 0
        psq_l f2, 0(r4), 0, 0
        psq_l f3, 8(r4), 0, 0
        ps_cmpu0 cr0, f0, f6
        bne _scaleX
        ps_merge01 f0, f8, f0
    _scaleX:
        ps_cmpu1 cr0, f0, f6
        psq_l f4, 16(r4), 0, 0
        psq_l f5, 24(r4), 0, 0
        bne _scaleY
        ps_merge00 f0, f0, f8
    _scaleY:
        ps_cmpu0 cr0, f1, f6
        psq_l f6, 32(r4), 0, 0
        psq_l f7, 40(r4), 0, 0
        bne _scaleZ
        ps_merge01 f1, f8, f1
    _scaleZ:
        ps_mul f2, f2, f0
        ps_mul f4, f4, f0
        ps_mul f6, f6, f0
        ps_mul f3, f3, f1
        psq_st f2, 0(r3), 0, 0
        ps_mul f5, f5, f1
        ps_mul f7, f7, f1
        psq_st f4, 16(r3), 0, 0
        psq_st f3, 8(r3), 0, 0
        psq_st f6, 32(r3), 0, 0
        psq_st f5, 24(r3), 0, 0
        psq_st f7, 40(r3), 0, 0
        blr
    // clang-format on
}

// Hand-written paired-single routine: r3 = pOut, r4 = pScale, r5 = pIn
asm void MtxScale(math::MTX34* pOut, const math::VEC3* pScale,
                  const math::MTX34* pIn) {
    // clang-format off
    nofralloc

        lis r6, scEpsilon@ha
        lfs f9, 0(r4)
        lfs f12, scEpsilon@l(r6)
        lfs f10, 4(r4)
        fsubs f13, f12, f12
        lfs f11, 8(r4)
        psq_l f4, 0(r5), 0, 0
        psq_l f5, 8(r5), 0, 0
        fcmpu cr0, f9, f13
        bne _scaleX
        fmr f9, f12
    _scaleX:
        fcmpu cr0, f10, f13
        psq_l f6, 16(r5), 0, 0
        psq_l f7, 24(r5), 0, 0
        bne _scaleY
        fmr f10, f12
    _scaleY:
        fcmpu cr0, f11, f13
        psq_l f8, 32(r5), 0, 0
        psq_l f2, 40(r5), 0, 0
        bne _scaleZ
        fmr f11, f12
    _scaleZ:
        ps_muls0 f4, f4, f9
        ps_muls0 f5, f5, f9
        ps_muls0 f6, f6, f10
        ps_muls0 f7, f7, f10
        psq_st f4, 0(r3), 0, 0
        ps_muls0 f8, f8, f11
        ps_muls0 f2, f2, f11
        psq_st f5, 8(r3), 0, 0
        psq_st f6, 16(r3), 0, 0
        psq_st f7, 24(r3), 0, 0
        psq_st f8, 32(r3), 0, 0
        psq_st f2, 40(r3), 0, 0
        blr
    // clang-format on
}

math::MTX34* MtxInvScale(math::MTX34* pOut, const math::MTX34* pIn,
                         const math::VEC3* pScale) {
    math::VEC3 inv;

    inv.x = 0.0f != pScale->x ? math::FInv(pScale->x) : 100000.0f;
    inv.y = 0.0f != pScale->y ? math::FInv(pScale->y) : 100000.0f;
    inv.z = 0.0f != pScale->z ? math::FInv(pScale->z) : 100000.0f;

    return math::MTX34Scale(pOut, pIn, &inv);
}

// Hand-written paired-single routine: r3 = pMtx, r4 = col
asm f32 MtxColVecLen(const math::MTX34* pMtx, int col) {
    // clang-format off
    nofralloc

        slwi r4, r4, 2
        add r4, r3, r4
        psq_l f0, 0(r4), 1, 0
        psq_l f1, 16(r4), 1, 0
        lfs f2, 32(r4)
        ps_merge00 f0, f0, f1
        ps_mul f0, f0, f0
        ps_madd f1, f2, f2, f0
        fsubs f2, f0, f0
        ps_sum0 f1, f1, f0, f0
        fcmpu cr0, f1, f2
        beqlr
        frsqrte f0, f1
        lis r6, sRsqrtConst@ha
        addi r6, r6, sRsqrtConst@l
        psq_l f3, 0(r6), 0, 0
        fmuls f2, f0, f0
        ps_muls1 f0, f0, f3
        fnmsubs f2, f2, f1, f3
        fmuls f0, f2, f0
        fmuls f1, f1, f0
        blr
    // clang-format on
}

} // namespace ef
} // namespace nw4r
