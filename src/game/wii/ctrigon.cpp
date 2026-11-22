// The Wii's map floor collision, .text 0x802BA4F4-0x802BD294 (its static initializer is
// __sinit_\ctrigon_cpp). Unlike the GameCube file, whose functions sit in reverse, the Wii
// keeps them in source order, and the two selector switches stay functions of their own.
// The readers are global functions as in the GameCube map (no local scope there either).
// NONMATCHING: the triangle parser on the NW4R shapes, the grid builder and
// _trigonChecker are not written yet (the parser inlines the g3d accessors with
// their asserts). TrigonGetVtxS8 differs from retail in register choice.
#include "rvl_msl/PowerPC_EABI_Support/MSL_C/MSL_Common/math_api.h"
#include "game/ctrigon.h"
#include "nw4r/g3d/g3d_scnmdl.h"
#include "game/mm.h"
#include "game/ut.h"
#include "Dolphin/gx.h"
#include <math.h>

extern "C" void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);

// what the parser met in the display lists, one count per kind; Create clears it and
// names the pool after it
class _trigonCounter {
public:
	u32 mCount[11]; // _00
	// _2C vtable

	virtual ~_trigonCounter() { } // _08
};

static _trigonCounter sCounter;

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
void fn_802BA678(CTrigon* trigon, nw4r::g3d::ScnMdl* model, nw4r::math::MTX34* worldMtx);
}

/**
 * @note Address: 0x802BA4F4
 * @note Size: 0x184
 */
// CTrigon::Create: reserve enough triangles for every polygon, then let the parser fill the pool and grid.
// NONMATCHING: retail's NW4R assertion strings and CTrigon allocation strings are separate literal objects;
// this unit's literals share @stringBase0, changing pool references and saving r27 instead of r26.
extern "C" TRIGON_INFO* fn_802BA4F4(CTrigon* trigon, nw4r::g3d::ScnMdl* model)
{
	nw4r::g3d::ResMdl mdl = model->GetResMdl();
	trigon->mNumMax       = 0;
	for (int i = 0; i < mdl.GetResShpNumEntries(); i++) {
		nw4r::g3d::ResShp shp = mdl.GetResShp(i);
		if (shp.ptr() == NULL) {
			nw4r::db::Panic("g3d_resshp_ac.h", 0x3A, "%s::%s: Object not valid.", "ResShp", "ref");
		}
		trigon->mNumMax += shp.ref().numPolygon;
	}
	if (trigon->mNumMax == 0) {
		return NULL;
	}
	trigon->mTri = (TRIGON_INFO*)mm::Alloc((trigon->mNumMax * sizeof(TRIGON_INFO) + 31) & ~31, "");
	trigon->mNum = 0;
	for (int i = 0; i < 11; i++) {
		sCounter.mCount[i] = 0;
	}
	trigon->_0C = trigon->_10 = 0;
	trigon->mMin.x = trigon->mMin.y = trigon->mMin.z = NAN;
	trigon->mMax.x = trigon->mMax.y = trigon->mMax.z = NAN;
	fn_802BA678(trigon, model, model->GetWldMtxArray());
	mm::SetName(trigon->mTri, FormatToStaticBuffer("(Tri %d(%d+%d)/%d)", trigon->mNum, trigon->_0C, trigon->_10, trigon->mNumMax));
	return trigon->mTri;
}

/**
 * @note Address: 0x802BC21C
 * @note Size: 0x1C8
 */
// the bytes one vertex takes in the display list, and the index size of each attribute. the
// color's direct size comes from the first format entry
int CTrigon::GetVtxSize(GXVtxDescList* desc, GXVtxAttrFmtList* fmt, int* pos, int* nrm, int* tex, int* clr)
{
	int size    = 0;
	int posSize = 0;
	int nrmSize = 0;
	int texSize = 0;
	int clrSize = 0;

	// the position matrix index through the last texture coordinate
	for (int i = 0; i < GX_VA_TEX7 + 1; i++, desc++) {
		if (desc->mType == GX_NONE) {
			continue;
		}
		switch (desc->mAttr) {
		case GX_VA_PNMTXIDX: // matrix indices take a byte
		case GX_VA_TEX0MTXIDX:
		case GX_VA_TEX1MTXIDX:
		case GX_VA_TEX2MTXIDX:
		case GX_VA_TEX3MTXIDX:
		case GX_VA_TEX4MTXIDX:
		case GX_VA_TEX5MTXIDX:
		case GX_VA_TEX6MTXIDX:
		case GX_VA_TEX7MTXIDX:
			size += 1;
			break;
		case GX_VA_POS: // the index's own size, not the attribute's
			switch (desc->mType) {
			case GX_INDEX8:
				posSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				posSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_NRM:
		case GX_VA_NBT:
			switch (desc->mType) {
			case GX_INDEX8:
				nrmSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				nrmSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_TEX0:
		case GX_VA_TEX1:
		case GX_VA_TEX2:
		case GX_VA_TEX3:
		case GX_VA_TEX4:
		case GX_VA_TEX5:
		case GX_VA_TEX6:
		case GX_VA_TEX7:
			switch (desc->mType) {
			case GX_INDEX8:
				texSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				texSize = 2;
				size += 2;
				break;
			}
			break;
		case GX_VA_CLR0:
		case GX_VA_CLR1:
			switch (desc->mType) {
			case GX_INDEX8:
				clrSize = 1;
				size += 1;
				break;
			case GX_INDEX16:
				clrSize = 2;
				size += 2;
				break;
			case GX_DIRECT:
				switch (fmt->mType) {
				case GX_RGB565:
				case GX_RGBA4:
					clrSize = 2;
					size += 2;
					break;
				case GX_RGB8:
				case GX_RGBA6:
					clrSize = 3;
					size += 3;
					break;
				case GX_RGBX8:
				case GX_RGBA8:
					clrSize = 4;
					size += 4;
					break;
				}
				break;
			}
			break;
		}
	}

	if (pos) {
		*pos = posSize;
	}
	if (nrm) {
		*nrm = nrmSize;
	}
	if (tex) {
		*tex = texSize;
	}
	if (clr) {
		*clr = clrSize;
	}
	return size;
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
	int found         = -1;
	u32 num           = cell->mNum;
	if (tri == NULL || num == 0) {
		return -1;
	}

	// from here on the hint counts from the cell's first triangle
	Vec v[3];
	int base = tri - mTri;
	hint -= base;
	int i = 0;
	for (u32 n = 0; n < num; n++) {
		if (hint + i < num) {
			TRIGON_INFO* t = &tri[hint + i];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = hint + base + i;
				break;
			}
		}
		if (hint - i - 1 >= 0) {
			TRIGON_INFO* t = &tri[hint - i - 1];
			t->mGetVtx(t, v);
			if (IsPointInsideTriangleXZ(pos, &v[0], &v[1], &v[2])) {
				found = hint + base - i - 1;
				break;
			}
		}
		i++;
		if (hint + i >= num && hint - i - 1 < 0) {
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
