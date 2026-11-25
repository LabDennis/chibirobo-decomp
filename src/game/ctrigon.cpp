#include "Version.h"
#include "game/ctrigon.h"
#include "game/gmath.h"
#include "game/mm.h"
#include "game/hsd_dobj.h"
#include "game/ut.h"
extern "C" {
#include <sdk/dolphin/gx/GXCommandList.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/pobj.h>
#include <dolphin/mtx/vec.h>
}
#include <math.h>
#include <float.h>
#include <stdlib.h>

extern "C" {
void HSD_DObjCountVertices(HSD_DObj* dobj, u32* verts, u32* polys);
void HSD_JObjCountVertices(HSD_JObj* jobj, u32* verts, u32* polys);
}

// returns the POS array entry the index at dl points at
typedef void* (*TrigonGetIdxFunc)(void* base, u8* dl);

static int GetVtxSize(HSD_VtxDescList* desc, int* pos, int* nrm, int* tex, int* clr);
static int ParsePObj(HSD_PObj* pobj, TRIGON_INFO* out, MtxPtr mtx);
extern "C" {
static void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out);
static void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out);
static void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out);
static void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out);
static void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out);
static void* TrigonGetIdxF32U8(void* base, u8* dl);
static void* TrigonGetIdxF32U16(void* base, u8* dl);
static void* TrigonGetIdxS16U8(void* base, u8* dl);
static void* TrigonGetIdxS16U16(void* base, u8* dl);
static void* TrigonGetIdxU16U8(void* base, u8* dl);
static void* TrigonGetIdxU16U16(void* base, u8* dl);
static void* TrigonGetIdxS8U8(void* base, u8* dl);
static void* TrigonGetIdxS8U16(void* base, u8* dl);
static void* TrigonGetIdxU8U8(void* base, u8* dl);
static void* TrigonGetIdxU8U16(void* base, u8* dl);
static void CollectTrigons(HSD_JObj* jobj, void* context, s32 type);
static int CompareTrigonDist(const void* a, const void* b);
}

/**
 * @note Address: 0x801A60C4
 * @note Size: 0x4BC
 */
void HSD_JObjCountVertices(HSD_JObj* jobj, u32* verts, u32* polys)
{
	u32 totalVerts = 0;
	u32 totalPolys = 0;
	if (jobj != NULL) {
		if (jobj->flags & INSTANCE) { // instanced joints share the child's tree
			HSD_JObjCountVertices(jobj->child, &totalVerts, &totalPolys);
		} else {
			if (union_type_dobj(jobj)) {
				HSD_DObjCountVertices(jobj->u.dobj, &totalVerts, &totalPolys);
			}
			for (HSD_JObj* child = jobj->child; child != NULL; child = child->next) {
				u32 nVerts, nPolys;
				HSD_JObjCountVertices(child, &nVerts, &nPolys);
				totalVerts += nVerts;
				totalPolys += nPolys;
			}
		}
	}
	if (verts) {
		*verts = totalVerts;
	}
	if (polys) {
		*polys = totalPolys;
	}
}

/**
 * @note Address: 0x801A6098
 * @note Size: 0x2C
 */
static int CompareTrigonDist(const void* a, const void* b)
{
	const TRIGON_INFO* ta = static_cast<const TRIGON_INFO*>(a);
	const TRIGON_INFO* tb = static_cast<const TRIGON_INFO*>(b);
	if (ta->mDist > tb->mDist) {
		return 1;
	}
	if (ta->mDist < tb->mDist) {
		return -1;
	}
	return 0;
}

/**
 * @note Address: 0x801A5DF4
 * @note Size: 0x2A4
 */
TRIGON_INFO* CTrigon::Create(HSD_JObj* jobj)
{
	mJObj = jobj;
	HSD_JObjCountVertices(jobj, NULL, &mNumMax);
	if (mNumMax == 0) {
		return NULL;
	}

	// first pass into a buffer big enough for every polygon, just to count the floor ones
	mTri = (TRIGON_INFO*)mm::Alloc((mNumMax * sizeof(TRIGON_INFO) + 31) & ~31, "(Tri)");
	mNum = 0;
	HSD_JObjWalkTree(jobj, CollectTrigons, this);
	mm::Free(mTri, 1);
	mTri = NULL;

	// then again into one that fits
	u32 size = mNum * sizeof(TRIGON_INFO);
	mTri     = (TRIGON_INFO*)mm::Alloc((size + 31) & ~31, FormatToStaticBuffer("(tri:%4d * %2d)", mNum, sizeof(TRIGON_INFO)));
	if (mTri != NULL) {
		mNum = 0;
		HSD_JObjWalkTree(jobj, CollectTrigons, this);
	}
	qsort(mTri, mNum, sizeof(TRIGON_INFO), CompareTrigonDist);

	mMinY = mMaxY = NAN;
	for (u32 i = 0; i < mNum; i++) {
		Vec v[3];
		mTri[i].mGetVtx(&mTri[i], v);

		f32 lo = v[0].y;
		if (v[1].y < lo) {
			lo = v[1].y;
		}
		if (v[2].y < lo) {
			lo = v[2].y;
		}
		if (fpclassify(mMinY) == FP_NAN || lo < mMinY) {
			mMinY = lo;
		}

		f32 hi = v[0].y;
		if (v[1].y > hi) {
			hi = v[1].y;
		}
		if (v[2].y > hi) {
			hi = v[2].y;
		}
		// top gets the lowest point here, retail does that
		if (fpclassify(mMaxY) == FP_NAN || hi > mMaxY) {
			mMaxY = lo;
		}
	}
	return mTri;
}

/**
 * @note Address: 0x801A5DB8
 * @note Size: 0x3C
 */
void CTrigon::Release()
{
	mm::Free(mTri, 1);
	mTri = NULL;
}

/**
 * @note Address: 0x801A5B2C
 * @note Size: 0x28C
 */
static void CollectTrigons(HSD_JObj* jobj, void* context, s32 type)
{
	CTrigon* trigon  = static_cast<CTrigon*>(context);
	TRIGON_INFO* tri = trigon->mTri;

	HSD_JObjSetupMatrix(jobj);
	// an identity matrix doesn't need applying to every vertex
	MtxPtr mtx;
	if (jobj->mtx[0][0] == 1.0f && jobj->mtx[0][1] == 0.0f && jobj->mtx[0][2] == 0.0f && jobj->mtx[0][3] == 0.0f && jobj->mtx[1][0] == 0.0f
	    && jobj->mtx[1][1] == 1.0f && jobj->mtx[1][2] == 0.0f && jobj->mtx[1][3] == 0.0f && jobj->mtx[2][0] == 0.0f
	    && jobj->mtx[2][1] == 0.0f && jobj->mtx[2][2] == 1.0f && jobj->mtx[2][3] == 0.0f) {
		mtx = NULL;
	} else {
		mtx = jobj->mtx;
	}

	for (HSD_DObj* dobj = HSD_JObjGetDObj(jobj); dobj != NULL; dobj = HSD_DObjGetNext(dobj)) {
		HSD_MObj* mobj = HSD_DObjGetMObj(dobj);
		if (mobj == NULL || mobj->mat == NULL) {
			continue;
		}
		const GXColor& diffuse = mobj->mat->diffuse;
		const u32 color        = (u32(diffuse.r) << 16) | (u32(diffuse.g) << 8) | diffuse.b;
		for (HSD_PObj* pobj = HSD_DObjGetPObj(dobj); pobj != NULL; pobj = HSD_PObjGetNext(pobj)) {
			u32 n          = ParsePObj(pobj, &tri[trigon->mNum], mtx);
			TRIGON_INFO* t = &tri[trigon->mNum];
			for (u32 i = 0; i < n; i++) {
				Vec v[3];
				Vec center;
				t->mGetVtx(t, v);
				// the sort key is the centroid's distance from the origin
				center.x  = (v[0].x + v[1].x + v[2].x) / 3.0f;
				center.y  = (v[0].y + v[1].y + v[2].y) / 3.0f;
				center.z  = (v[0].z + v[1].z + v[2].z) / 3.0f;
				t->mDist  = PSVECMag(&center);
				t->mColor = color;
				t++;
			}
			trigon->mNum += n;
		}
	}
}

/**
 * @note Address: 0x801A57C0
 * @note Size: 0x324
 */
inline bool _trigonChecker::Check()
{
	Vec v[3];
	Vec e1;
	Vec e2;
	Vec cross;

	mGetVtx(this, v);
	PSVECSubtract(&v[2], &v[1], &e1);
	PSVECSubtract(&v[0], &v[1], &e2);
	C_VECCrossProduct(&e1, &e2, &cross);

	// degenerate triangles never count
	if (PSVECMag(&cross) > FLT_EPSILON) {
		CVec normal;
		CalcTriangleNormal(&v[0], &v[1], &v[2], &normal);

		// slope against the horizontal, keep anything that faces up
		f32 xz    = sqrtf(normal.x * normal.x + normal.z * normal.z);
		f32 y     = sqrtf(normal.y * normal.y);
		f32 angle = atan2(xz, y);
		if (normal.y >= 0.0f) {
			angle = PI / 2 - angle;
		} else {
			angle = angle - PI / 2;
		}
		if (angle <= 0.0f || angle >= PI) {
			mNumNg++;
			return false;
		}
		mNumOk++;
		return true;
	}
	mNumNg++;
	return false;
}

// a,b,c are the triangle's corners in the POS array
#define SET_TRIGON(a, b, c) \
	checker.mVtx[0] = a;    \
	checker.mVtx[1] = b;    \
	checker.mVtx[2] = c;

// the checker gets filled in place and copied out when it faces up
#define ADD_TRIGON()          \
	checker.mMtx    = mtx;    \
	checker.mFrac   = frac;   \
	checker.mGetVtx = getVtx; \
	if (checker.Check()) {    \
		*out = checker;       \
		out++;                \
		num++;                \
	}

static inline TrigonGetIdxFunc GetIdxFunc(int compType, int idxSize)
{
	TrigonGetIdxFunc func;
	switch (compType) {
	case GX_U8:
		func = idxSize == 2 ? TrigonGetIdxU8U16 : TrigonGetIdxU8U8;
		break;
	case GX_S8:
		func = idxSize == 2 ? TrigonGetIdxS8U16 : TrigonGetIdxS8U8;
		break;
	case GX_U16:
		func = idxSize == 2 ? TrigonGetIdxU16U16 : TrigonGetIdxU16U8;
		break;
	case GX_S16:
		func = idxSize == 2 ? TrigonGetIdxS16U16 : TrigonGetIdxS16U8;
		break;
	case GX_F32:
		func = idxSize == 2 ? TrigonGetIdxF32U16 : TrigonGetIdxF32U8;
		break;
	default:
		func = NULL;
		break;
	}
	return func;
}

static inline TrigonGetVtxFunc GetVtxFunc(int compType)
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

// GX display lists store multibyte fields in big-endian byte order.
static u16 ReadDisplayListU16(const u8* data)
{
	return (data[0] << 8) | data[1];
}

/**
 * @note Address: 0x801A50B8
 * @note Size: 0x708
 */
static int ParsePObj(HSD_PObj* pobj, TRIGON_INFO* out, MtxPtr mtx)
{
	HSD_VtxDescList* vertexDesc = pobj->verts;
	int posSize                 = 0;
	int stride                  = GetVtxSize(vertexDesc, &posSize, NULL, NULL, NULL);
	int compType                = vertexDesc->comp_type;
	void* vtx                   = vertexDesc->vertex;
	u8 frac                     = vertexDesc->frac;
	TrigonGetIdxFunc getIdx     = GetIdxFunc(compType, posSize);
	TrigonGetVtxFunc getVtx     = GetVtxFunc(compType);

	u8* dl  = pobj->display;
	u8* end = dl + (pobj->n_display << 5);
	_trigonChecker checker;
	int num = 0;
	int i;

	while (dl < end) {
		u8 cmd = dl[0]; // the display list opcode
		int n  = ReadDisplayListU16(dl + 1);
		dl += 3;

		switch (cmd & GX_OPCODE_MASK) {
		case GX_NOP: // a nop ends the list
			return num;
		case GX_DRAW_POINTS:
			dl += stride * n;
			break;
		case GX_DRAW_LINES:
			dl += stride * n;
			break;
		case GX_DRAW_LINE_STRIP:
			dl += stride * n;
			break;
		case GX_DRAW_TRIANGLES:
			for (i = 0; i < n / 3; i++) {
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride * 3;
			}
			break;
		case GX_DRAW_TRIANGLE_STRIP:
			// every other triangle of a strip winds backwards
			for (i = 2; i < n; i++) {
				if (i & 1) {
					SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride * 2), getIdx(vtx, dl + stride));
				} else {
					SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				}
				ADD_TRIGON();
				dl += stride;
			}
			dl += stride * 2;
			break;
		case GX_DRAW_TRIANGLE_FAN: {
			void* first = getIdx(vtx, dl); // the fan's hub vertex
			for (i = 2; i < n; i++) {
				SET_TRIGON(first, getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride;
			}
			dl += stride * 2;
			break;
		}
		case GX_DRAW_QUADS: // each quad becomes two triangles
			for (i = 0; i < n / 4; i++) {
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride * 2), getIdx(vtx, dl + stride * 3));
				ADD_TRIGON();
				dl += stride * 4;
			}
			break;
		}
	}
	return num;
}
/**
 * @note Address: 0x801A4F28
 * @note Size: 0x190
 */
static int GetVtxSize(HSD_VtxDescList* desc, int* pos, int* nrm, int* tex, int* clr)
{
	int size    = 0;
	int posSize = 0;
	int nrmSize = 0;
	int texSize = 0;
	int clrSize = 0;
	int attr;

	for (; desc != NULL && (attr = desc->attr) != GX_VA_NULL; desc++) {
		switch (attr) {
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
			if (desc->attr_type == GX_INDEX16) {
				posSize = 2;
				size += 2;
			} else {
				posSize = 1;
				size += 1;
			}
			break;
		case GX_VA_NRM:
		case GX_VA_NBT:
			if (desc->attr_type == GX_INDEX16) {
				nrmSize = 2;
				size += 2;
			} else {
				nrmSize = 1;
				size += 1;
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
			if (desc->attr_type == GX_INDEX16) {
				texSize = 2;
				size += 2;
			} else {
				texSize = 1;
				size += 1;
			}
			break;
		case GX_VA_CLR0: // indexed, or a direct color's format size
		case GX_VA_CLR1:
			if (desc->attr_type == GX_INDEX16) {
				clrSize = 2;
				size += 2;
			} else if (desc->attr_type == GX_INDEX8) {
				clrSize = 1;
				size += 1;
			} else {
				switch (desc->comp_type) {
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
 * @note Address: 0x801A4F18
 * @note Size: 0x10
 */
static void* TrigonGetIdxU8U16(void* base, u8* dl)
{
	return static_cast<u8*>(base) + ReadDisplayListU16(dl) * 3;
}

/**
 * @note Address: 0x801A4F08
 * @note Size: 0x10
 */
static void* TrigonGetIdxU8U8(void* base, u8* dl)
{
	return static_cast<u8*>(base) + *dl * 3;
}

/**
 * @note Address: 0x801A4EF8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS8U16(void* base, u8* dl)
{
	return static_cast<s8*>(base) + ReadDisplayListU16(dl) * 3;
}

/**
 * @note Address: 0x801A4EE8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS8U8(void* base, u8* dl)
{
	return static_cast<s8*>(base) + *dl * 3;
}

/**
 * @note Address: 0x801A4ED8
 * @note Size: 0x10
 */
static void* TrigonGetIdxU16U16(void* base, u8* dl)
{
	return static_cast<u16*>(base) + ReadDisplayListU16(dl) * 3;
}

/**
 * @note Address: 0x801A4EC8
 * @note Size: 0x10
 */
static void* TrigonGetIdxU16U8(void* base, u8* dl)
{
	return static_cast<u16*>(base) + *dl * 3;
}

/**
 * @note Address: 0x801A4EB8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS16U16(void* base, u8* dl)
{
	return static_cast<s16*>(base) + ReadDisplayListU16(dl) * 3;
}

/**
 * @note Address: 0x801A4EA8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS16U8(void* base, u8* dl)
{
	return static_cast<s16*>(base) + *dl * 3;
}

/**
 * @note Address: 0x801A4E98
 * @note Size: 0x10
 */
static void* TrigonGetIdxF32U16(void* base, u8* dl)
{
	return static_cast<f32*>(base) + ReadDisplayListU16(dl) * 3;
}

/**
 * @note Address: 0x801A4E88
 * @note Size: 0x10
 */
static void* TrigonGetIdxF32U8(void* base, u8* dl)
{
	return static_cast<f32*>(base) + *dl * 3;
}

/**
 * @note Address: 0x801A4CA0
 * @note Size: 0x1E8
 */
static void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out)
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
 * @note Address: 0x801A4A74
 * @note Size: 0x22C
 */
static void TrigonGetVtxS8(const TRIGON_INFO* tri, Vec* out)
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
 * @note Address: 0x801A488C
 * @note Size: 0x1E8
 */
static void TrigonGetVtxU16(const TRIGON_INFO* tri, Vec* out)
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
 * @note Address: 0x801A4684
 * @note Size: 0x208
 */
static void TrigonGetVtxS16(const TRIGON_INFO* tri, Vec* out)
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
 * @note Address: 0x801A45F8
 * @note Size: 0x8C
 */
static void TrigonGetVtxF32(const TRIGON_INFO* tri, Vec* out)
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
 * @note Address: 0x801A4428
 * @note Size: 0x1D0
 */
#if defined(VERSION_JP)
int CTrigon::Check(Vec* pos, f32* floorY, Vec* tri)
#else
int CTrigon::Search(Vec* pos, f32* floorY)
#endif
{
	Vec* corner2;
	Vec* corner1;
	if (mJObj->flags & HIDDEN) {
		return -1;
	}

	int found         = -1;
	TRIGON_INFO* list = mTri;
	u32 num           = mNum;
	if (list == NULL || num == 0) {
		return -1;
	}

	// the list is sorted by distance from the origin, find where pos would sit
	int step = num;
	int base = 0;
	f32 dist = PSVECMag(pos);
	while (step > 0) {
		int mid = base + step / 2;
		step /= 2;
		if (dist > list[mid].mDist) {
			base += step;
		}
	}

	// then walk outwards both ways from there
	Vec v[3];
	corner2 = &v[2];
	corner1 = &v[1];
	TRIGON_INFO* t;
	int k;
	int j = 0;
	for (u32 i = 0; i < num; i++) {
		k = base + j;
		if ((u32)k < num) {
			t = &list[k];
			t->mGetVtx(t, v);
			// triangles whose material rgb is black don't count
			if (t->mColor != 0 && IsPointInsideTriangleXZ(pos, &v[0], corner1, corner2)) {
				found = k;
				break;
			}
		}
		k = base - j - 1;
		if (k >= 0) {
			t = &list[k];
			t->mGetVtx(t, v);
			if (t->mColor != 0 && IsPointInsideTriangleXZ(pos, &v[0], corner1, corner2)) {
				found = k;
				break;
			}
		}
		j++;
	}

#if defined(VERSION_JP)
	if (found >= 0) {
		if (floorY != NULL) {
			CVec hit;
			IntersectVerticalLineWithTrianglePlane(&v[0], corner1, corner2, pos, &hit);
			*floorY = hit.y;
		}
		if (tri != NULL) {
			mTri[found].mGetVtx(&mTri[found], tri);
		}
	}
#else
	if (found >= 0 && floorY != NULL) {
		CVec hit;
		IntersectVerticalLineWithTrianglePlane(&v[0], corner1, corner2, pos, &hit);
		*floorY = hit.y;
	}
#endif
	return found;
}

/**
 * @note Address: 0x801A43B8
 * @note Size: 0x70
 */
// japan has the search written straight into Check, see Search above
#if !defined(VERSION_JP)
int CTrigon::Check(Vec* pos, f32* floorY, Vec* tri)
{
	int idx = Search(pos, floorY);
	if (idx >= 0 && tri != NULL) {
		mTri[idx].mGetVtx(&mTri[idx], tri);
	}
	return idx;
}
#endif
