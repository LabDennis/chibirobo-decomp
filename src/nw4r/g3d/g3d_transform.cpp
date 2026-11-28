#include "nw4r/g3d.h"

#include "nw4r/math.h"

namespace nw4r {
namespace g3d {
namespace detail {

const f32 dat_epsilon = 1e-36f;

// NONMATCHING: retail uses paired-single assembly
bool CalcViewNrmMtx(math::MTX33 *pOut, const math::MTX34 *pW) {
    f32 w00 = pW->_00, w01 = pW->_01, w02 = pW->_02;
    f32 w10 = pW->_10, w11 = pW->_11, w12 = pW->_12;
    f32 w20 = pW->_20, w21 = pW->_21, w22 = pW->_22;

    f32 c00 = w11 * w22 - w21 * w12;
    f32 c01 = w12 * w20 - w22 * w10;
    f32 c02 = w10 * w21 - w11 * w20;

    f32 c10 = w21 * w02 - w01 * w22;
    f32 c11 = w22 * w00 - w02 * w20;
    f32 c12 = w01 * w20 - w00 * w21;

    f32 c20 = w01 * w12 - w11 * w02;
    f32 c21 = w02 * w10 - w12 * w00;

    f32 det = w00 * c00 + w10 * c10 + w20 * c20;
    if (math::FAbs(det) < dat_epsilon) {
        pOut->_00 = w00;
        pOut->_01 = w01;
        pOut->_02 = w02;
        pOut->_10 = w10;
        pOut->_11 = w11;
        pOut->_12 = w12;
        pOut->_20 = w20;
        pOut->_21 = w21;
        pOut->_22 = w22;
        return false;
    }

    f32 c22 = w00 * w11 - w01 * w10;
    f32 invDet = math::FInv(det);

    pOut->_00 = c00 * invDet;
    pOut->_01 = c01 * invDet;
    pOut->_02 = c02 * invDet;

    pOut->_10 = c10 * invDet;
    pOut->_11 = c11 * invDet;
    pOut->_12 = c12 * invDet;

    pOut->_20 = c20 * invDet;
    pOut->_21 = c21 * invDet;
    pOut->_22 = c22 * invDet;
    return true;
}

// NONMATCHING: retail uses paired-single assembly
bool CalcViewTexMtx(math::MTX34 *pOut, const math::MTX34 *pW) {
    f32 w00 = pW->_00, w01 = pW->_01, w02 = pW->_02;
    f32 w10 = pW->_10, w11 = pW->_11, w12 = pW->_12;
    f32 w20 = pW->_20, w21 = pW->_21, w22 = pW->_22;

    f32 c00 = w11 * w22 - w21 * w12;
    f32 c01 = w12 * w20 - w22 * w10;
    f32 c02 = w10 * w21 - w11 * w20;

    f32 c10 = w21 * w02 - w01 * w22;
    f32 c11 = w22 * w00 - w02 * w20;
    f32 c12 = w01 * w20 - w00 * w21;

    f32 c20 = w01 * w12 - w11 * w02;
    f32 c21 = w02 * w10 - w12 * w00;

    f32 det = w00 * c00 + w10 * c10 + w20 * c20;
    if (math::FAbs(det) < dat_epsilon) {
        if (pOut != pW) {
            pOut->_00 = w00;
            pOut->_01 = w01;
            pOut->_02 = w02;
            pOut->_03 = 0.0f;

            pOut->_10 = w10;
            pOut->_11 = w11;
            pOut->_12 = w12;
            pOut->_13 = 0.0f;

            pOut->_20 = w20;
            pOut->_21 = w21;
            pOut->_22 = w22;
            pOut->_23 = 0.0f;
        }
        return false;
    }

    f32 c22 = w00 * w11 - w01 * w10;
    f32 invDet = math::FInv(det);

    pOut->_00 = c00 * invDet;
    pOut->_01 = c01 * invDet;
    pOut->_02 = c02 * invDet;
    pOut->_03 = 0.0f;

    pOut->_10 = c10 * invDet;
    pOut->_11 = c11 * invDet;
    pOut->_12 = c12 * invDet;
    pOut->_13 = 0.0f;

    pOut->_20 = c20 * invDet;
    pOut->_21 = c21 * invDet;
    pOut->_22 = c22 * invDet;
    pOut->_23 = 0.0f;
    return true;
}

// NONMATCHING: retail uses paired-single assembly
bool CalcInvWorldMtx(math::MTX34 *pOut, const math::MTX34 *pW) {
    f32 w00 = pW->_00, w01 = pW->_01, w02 = pW->_02, w03 = pW->_03;
    f32 w10 = pW->_10, w11 = pW->_11, w12 = pW->_12, w13 = pW->_13;
    f32 w20 = pW->_20, w21 = pW->_21, w22 = pW->_22, w23 = pW->_23;

    f32 c00 = w11 * w22 - w21 * w12;
    f32 c01 = w12 * w20 - w22 * w10;
    f32 c02 = w10 * w21 - w11 * w20;

    f32 c10 = w21 * w02 - w01 * w22;
    f32 c11 = w22 * w00 - w02 * w20;
    f32 c12 = w01 * w20 - w00 * w21;

    f32 c20 = w01 * w12 - w11 * w02;
    f32 c21 = w02 * w10 - w12 * w00;

    f32 det = w00 * c00 + w10 * c10 + w20 * c20;
    if (math::FAbs(det) < dat_epsilon) {
        return false;
    }

    f32 c22 = w00 * w11 - w01 * w10;
    f32 invDet = math::FInv(det);

    f32 m00 = c00 * invDet;
    f32 m01 = c10 * invDet;
    f32 m02 = c20 * invDet;

    f32 m10 = c01 * invDet;
    f32 m11 = c11 * invDet;
    f32 m12 = c21 * invDet;

    f32 m20 = c02 * invDet;
    f32 m21 = c12 * invDet;
    f32 m22 = c22 * invDet;

    pOut->_00 = m00;
    pOut->_01 = m01;
    pOut->_02 = m02;
    pOut->_03 = -(m00 * w03 + m01 * w13 + m02 * w23);

    pOut->_10 = m10;
    pOut->_11 = m11;
    pOut->_12 = m12;
    pOut->_13 = -(m10 * w03 + m11 * w13 + m12 * w23);

    pOut->_20 = m20;
    pOut->_21 = m21;
    pOut->_22 = m22;
    pOut->_23 = -(m20 * w03 + m21 * w13 + m22 * w23);
    return true;
}

} // namespace detail
} // namespace g3d
} // namespace nw4r
