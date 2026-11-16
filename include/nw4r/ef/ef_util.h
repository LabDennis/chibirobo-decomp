#ifndef NW4R_EF_UTIL_H
#define NW4R_EF_UTIL_H
#include "nw4r/types_nw4r.h"

#include "nw4r/ut/ut_LinkList.h"
#include "nw4r/ut/ut_algorithm.h"
#include "nw4r/ut/ut_list.h"

namespace nw4r {
namespace ef {

u16 UtlistToArray(const ut::List* pList, void** ppArray, int maxlen);

math::VEC3& Rotation2VecY(const math::VEC3& rRot, math::VEC3* pVec);
void GetDirMtxY(math::MTX34* pMtx, const math::VEC3& rVec);

void MtxGetRotationMtx(const math::MTX34& rMtx, math::MTX34* pDst);

inline void MtxSetColVec(math::MTX34* pMtx, int col, const math::VEC3* pVec) {
    pMtx->m[0][col] = pVec->x;
    pMtx->m[1][col] = pVec->y;
    pMtx->m[2][col] = pVec->z;
}

inline math::VEC3* MtxColVec(const math::MTX34* pMtx, int col,
                             math::VEC3* pVec) {
    pVec->x = pMtx->m[0][col];
    pVec->y = pMtx->m[1][col];
    pVec->z = pMtx->m[2][col];

    return pVec;
}

void MtxGetRotation(const math::MTX34& rMtx, math::VEC3* pRot);
void MtxGetTranslate(const math::MTX34& rMtx, math::VEC3* pTrans);
void MtxGetScale(const math::MTX34& rMtx, math::VEC3* pScale);

// Returns FALSE (and copies the input) when the vector has no length
BOOL VecNormalize(math::VEC3* pOut, const math::VEC3* pIn);

// Radian sine/cosine through the nw4r::math table
void SinCosRad(f32* pSinCos, f32 rad); // pSinCos[0] = sin, pSinCos[1] = cos
void SinCosRad(f32* pSin, f32* pCos, f32 rad);

void MtxRotXYZRad(math::MTX34* pMtx, f32 rx, f32 ry, f32 rz);

// Like math::MTX34Scale, but a zero scale component counts as 0.00001
void MtxScale(math::MTX34* pOut, const math::MTX34* pIn,
              const math::VEC3* pScale);
void MtxScale(math::MTX34* pOut, const math::VEC3* pScale,
              const math::MTX34* pIn);
// Scales by the reciprocal (a zero scale component counts as 1/100000)
math::MTX34* MtxInvScale(math::MTX34* pOut, const math::MTX34* pIn,
                         const math::VEC3* pScale);

f32 MtxColVecLen(const math::MTX34* pMtx, int col);

// 0x802465A0: false (and a plain copy) when the vector has no length
bool Normalize(math::VEC3* pOut, const math::VEC3* pIn);
// 0x80246604: pSinCos[0] = sin, pSinCos[1] = cos
void SinCosRad(f32* pSinCos, f32 rad);
// 0x80246674
void SinCosRad(f32* pSin, f32* pCos, f32 rad);

} // namespace ef
} // namespace nw4r

#endif
