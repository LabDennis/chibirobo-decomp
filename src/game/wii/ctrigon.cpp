// The Wii's map floor collision, .text 0x802BA4F4-0x802BD294 (its static initializer is
// __sinit_\ctrigon_cpp). Unlike the GameCube file, whose functions sit in reverse, the Wii
// keeps them in source order, and the two selector switches stay functions of their own.
// The readers are global functions as in the GameCube map (no local scope there either).
// NONMATCHING: only the vertex readers and their selectors are reconstructed; the triangle
// parser on the NW4R shapes, the vertex size scan, Create, the two destructors and the
// counter global are not written yet. TrigonGetVtxS8 and SearchNear differ from retail
// in register choice (SearchNear: retail adds i + at again for the result instead of
// reusing the index it tested).
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "game/ctrigon.h"
#include "game/mm.h"
#include "game/ut.h"
#include "Dolphin/gx.h"
#include <math.h>

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

// returns the position array entry the index at dl points at; the 16 and 32 bit readers
// step by the whole vertex (6 or 12 bytes), the byte readers by three
typedef void* (*TrigonGetIdxFunc)(void* base, u8* dl);

extern "C" {
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out);
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out);
void* TrigonGetIdxF32U8(void* base, u8* dl);
void* TrigonGetIdxF32U16(void* base, u8* dl);
void* TrigonGetIdxS16U8(void* base, u8* dl);
void* TrigonGetIdxS16U16(void* base, u8* dl);
void* TrigonGetIdxU16U8(void* base, u8* dl);
void* TrigonGetIdxU16U16(void* base, u8* dl);
void* TrigonGetIdxS8U8(void* base, u8* dl);
void* TrigonGetIdxS8U16(void* base, u8* dl);
void* TrigonGetIdxU8U8(void* base, u8* dl);
void* TrigonGetIdxU8U16(void* base, u8* dl);
}

/**
 * @note Address: 0x802BC3E4
 * @note Size: 0xC0
 */
TrigonGetIdxFunc GetIdxFunc(int compType, int idxSize)
{
	switch (compType) {
	case GX_U8:
		return idxSize == 2 ? TrigonGetIdxU8U16 : TrigonGetIdxU8U8;
	case GX_S8:
		return idxSize == 2 ? TrigonGetIdxS8U16 : TrigonGetIdxS8U8;
	case GX_U16:
		return idxSize == 2 ? TrigonGetIdxU16U16 : TrigonGetIdxU16U8;
	case GX_S16:
		return idxSize == 2 ? TrigonGetIdxS16U16 : TrigonGetIdxS16U8;
	case GX_F32:
		return idxSize == 2 ? TrigonGetIdxF32U16 : TrigonGetIdxF32U8;
	}
	return NULL;
}

/**
 * @note Address: 0x802BC4A4
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4B8
 * @note Size: 0x14
 */
void* TrigonGetIdxU8U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4CC
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U16(void* base, u8* dl)
{
	return (s8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x802BC4E0
 * @note Size: 0x14
 */
void* TrigonGetIdxS8U8(void* base, u8* dl)
{
	return (s8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x802BC4F4
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC504
 * @note Size: 0x10
 */
void* TrigonGetIdxU16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC514
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 6;
}

/**
 * @note Address: 0x802BC524
 * @note Size: 0x10
 */
void* TrigonGetIdxS16U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 6;
}

/**
 * @note Address: 0x802BC534
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U16(void* base, u8* dl)
{
	return (u8*)base + *(u16*)dl * 12;
}

/**
 * @note Address: 0x802BC544
 * @note Size: 0x10
 */
void* TrigonGetIdxF32U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 12;
}

/**
 * @note Address: 0x802BC554
 * @note Size: 0x70
 */
TrigonGetVtxFunc GetVtxFunc(int compType)
{
	switch (compType) {
	case GX_U8:
		return TrigonGetVtxU8;
	case GX_S8:
		return TrigonGetVtxS8;
	case GX_U16:
		return TrigonGetVtxU16;
	case GX_S16:
		return TrigonGetVtxS16;
	case GX_F32:
		return TrigonGetVtxF32;
	}
	return NULL;
}

/**
 * @note Address: 0x802BC5C4
 * @note Size: 0x1AC
 */
void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	// frac is the fixed point's fraction bits
	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC770
 * @note Size: 0x1EC
 */
void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s8*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s8*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s8*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BC95C
 * @note Size: 0x1AC
 */
void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((u16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((u16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((u16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCB08
 * @note Size: 0x1C8
 */
void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

	for (i = 0; i < 3; i++) {
		out[i].x = (f32)((s16*)tri->mVtx[i])[0] / (f32)scale;
		out[i].y = (f32)((s16*)tri->mVtx[i])[1] / (f32)scale;
		out[i].z = (f32)((s16*)tri->mVtx[i])[2] / (f32)scale;
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCCD0
 * @note Size: 0x74
 */
void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out)
{
	int i;

	for (i = 0; i < 3; i++) {
		out[i] = *(Vec*)tri->mVtx[i];
	}
	if (tri->mMtx) {
		PSMTXMultVecArray(tri->mMtx, out, out, 3);
	}
}

/**
 * @note Address: 0x802BCD44
 * @note Size: 0x88
 */
void CTrigon::Release()
{
	if (mCells != NULL) {
		mm::Free(mCells);
		mCells     = NULL;
		mCellOrder = NULL;
		mNumCells  = 0;
	}
	if (_2C != NULL) {
		mm::Free(_2C);
		_2C = NULL;
	}
	if (mTri != NULL) {
		mm::Free(mTri);
		mTri = NULL;
	}
	mNumMax = 0;
	mNum    = 0;
}

/**
 * @note Address: 0x802BCDCC
 * @note Size: 0xFC
 */
// the triangle under pos, trying the one from last time first, then its cell, then all of them
int CTrigon::Check(Vec* pos, int hint, f32* floorY)
{
	if (hint >= 0 && hint < mNum) {
		Vec v[3];
		mTri[hint].mGetVtx(&mTri[hint], v);
		if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
			if (floorY != NULL) {
				CVec hit;
				IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
				*floorY = hit.y;
			}
			return hint;
		}
		return SearchNear(pos, hint, floorY);
	}
	return Search(pos, floorY);
}

/**
 * @note Address: 0x802BCEC8
 * @note Size: 0x1B0
 */
// walks the hint's cell outward from the hint, one triangle above and one below per step
int CTrigon::SearchNear(Vec* pos, int hint, f32* floorY)
{
	TRIGON_CELL* cell = &mCells[mTri[hint].mCell];
	TRIGON_INFO* tri  = cell->mTri;
	u32 num           = cell->mNum;
	int found         = -1;
	if (tri == NULL || num == 0) {
		return -1;
	}

	Vec v[3];
	int base = tri - mTri;
	int at   = hint - base;
	int i    = 0;
	for (u32 n = 0; n < num; n++) {
		if (at + i < num) {
			TRIGON_INFO* t = &tri[at + i];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = base + (i + at);
				break;
			}
		}
		if (at - i - 1 >= 0) {
			TRIGON_INFO* t = &tri[at - i - 1];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = at + base - i - 1;
				break;
			}
		}
		i++;
		if (at + i >= num && at - i - 1 < 0) {
			break;
		}
	}

	if (found >= 0) {
		if (floorY != NULL) {
			CVec hit;
			IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
			*floorY = hit.y;
		}
	} else {
		found = Search(pos, floorY);
	}
	return found;
}

/**
 * @note Address: 0x802BD078
 * @note Size: 0x1B4
 */
// every cell in mCellOrder: the highest floor at most 5 above pos wins
int CTrigon::Search(Vec* pos, f32* floorY)
{
	f32 top   = 5.0f + pos->y;
	f32 best  = NAN;
	int found = -1;
	for (int i = 0; i < mNumCells; i++) {
		int cell         = mCellOrder[i];
		TRIGON_INFO* tri = mCells[cell].mTri;
		u32 num          = mCells[cell].mNum;
		if (tri == NULL || num == 0) {
			return found;
		}
		for (u32 j = 0; j < num; tri++, j++) {
			Vec v[3];
			tri->mGetVtx(tri, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				CVec hit;
				IntersectVerticalLineWithTrianglePlane(pos, &v[0], &v[1], &v[2], &hit);
				if (__fpclassifyf(best) == FP_NAN || (hit.y < top && top - hit.y < best)) {
					f32 dist = top - hit.y;
					if (dist >= 0.0f) {
						best = dist;
					}
					found = j + (mCells[cell].mTri - mTri);
					if (floorY != NULL) {
						*floorY = hit.y;
					}
				}
			}
		}
	}
	return found;
}
