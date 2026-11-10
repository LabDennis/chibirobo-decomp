#include "Version.h"
#include "game/ctrigon.h"
#include "game/gmath.h"
#include "game/mm.h"
#include "game/hsd_dobj.h"
#include "game/ut.h"
extern "C" {
#include <sysdolphin/mobj.h>
#include <sysdolphin/pobj.h>
#include <dolphin/mtx/vec.h>
}
#include <math.h>
#include <float.h>
#include <stdlib.h>

extern "C" {
void PSMTXMultVecArray(const Mtx m, const Vec* srcBase, Vec* dstBase, u32 count);
void C_VECCrossProduct(Vec* a, Vec* b, Vec* axb);
void HSD_JObjWalkTree(HSD_JObj* jobj, HSD_JObjWalkTreeCallback cb, f32** cb_args);
void HSD_DObjCountVertices(HSD_DObj* dobj, u32* verts, u32* polys);
void HSD_JObjCountVertices(HSD_JObj* jobj, u32* verts, u32* polys);
}

// GXPrimitive, the hsd headers don't have it, pobj.c defines its own too
#define GX_QUADS         0x80
#define GX_TRIANGLES     0x90
#define GX_TRIANGLESTRIP 0x98
#define GX_TRIANGLEFAN   0xA0
#define GX_LINES         0xA8
#define GX_LINESTRIP     0xB0
#define GX_POINTS        0xB8

// returns the POS array entry the index at dl points at
typedef void* (*TrigonGetIdxFunc)(void* base, u8* dl);

// the file runs backwards, these are defined further down
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
static void CollectTrigons(HSD_JObj* jobj, f32** args, s32 type);
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
		if (jobj->flags & INSTANCE) {
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
	const TRIGON_INFO* ta = (const TRIGON_INFO*)a;
	const TRIGON_INFO* tb = (const TRIGON_INFO*)b;
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
	HSD_JObjWalkTree(jobj, CollectTrigons, (f32**)this);
	mm::Free(mTri, 1);
	mTri = NULL;

	// then again into one that fits
	u32 size = mNum * sizeof(TRIGON_INFO);
	mTri     = (TRIGON_INFO*)mm::Alloc((size + 31) & ~31, FormatToStaticBuffer("(tri:%4d * %2d)", mNum, sizeof(TRIGON_INFO)));
	if (mTri != NULL) {
		mNum = 0;
		HSD_JObjWalkTree(jobj, CollectTrigons, (f32**)this);
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
static void CollectTrigons(HSD_JObj* jobj, f32** args, s32 type)
{
	CTrigon* trigon  = (CTrigon*)args;
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
		u32 color = *(u32*)&mobj->mat->diffuse >> 8;
		for (HSD_PObj* pobj = HSD_DObjGetPObj(dobj); pobj != NULL; pobj = HSD_PObjGetNext(pobj)) {
			u32 n          = ParsePObj(pobj, &tri[trigon->mNum], mtx);
			TRIGON_INFO* t = &tri[trigon->mNum];
			for (u32 i = 0; i < n; i++) {
				Vec v[3];
				Vec center;
				t->mGetVtx(t, v);
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
 * @note Address: 0x801A5AE4
 * @note Size: 0x48
 */
// _trigonChecker's deleting dtor, the inline virtual from the class. lands weak after
// ParsePObj since weak copies come out in reverse order of first use

/**
 * @note Address: 0x801A57C0
 * @note Size: 0x324
 */
// too big to inline, so it ends up weak after ParsePObj. one statement less and
// ParsePObj swallows all six calls
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

/**
 * @note Address: 0x801A50B8
 * @note Size: 0x708
 */
static int ParsePObj(HSD_PObj* pobj, TRIGON_INFO* out, MtxPtr mtx)
{
	// NONMATCHING: regalloc, retail picks the accessor in r3 and moves it to r25 after, and loads display a few lines later
	int posSize             = 0;
	int stride              = GetVtxSize(pobj->verts, &posSize, NULL, NULL, NULL);
	int compType            = pobj->verts->comp_type;
	void* vtx               = pobj->verts->vertex;
	u8 frac                 = pobj->verts->frac;
	TrigonGetIdxFunc getIdx = GetIdxFunc(compType, posSize);
	TrigonGetVtxFunc getVtx = GetVtxFunc(compType);

	u8* dl  = pobj->display;
	u8* end = dl + (pobj->n_display << 5);
	_trigonChecker checker;
	int num = 0;
	int i;

	while (dl < end) {
		u8 cmd = dl[0];
		int n  = dl[1] << 8 | dl[2];
		dl += 3;

		switch (cmd & GX_OPCODE_MASK) {
		case GX_NOP:
			return num;
		case GX_POINTS:
			dl += stride * n;
			break;
		case GX_LINES:
			dl += stride * n;
			break;
		case GX_LINESTRIP:
			dl += stride * n;
			break;
		case GX_TRIANGLES:
			for (i = 0; i < n / 3; i++) {
				SET_TRIGON(getIdx(vtx, dl), getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride * 3;
			}
			break;
		case GX_TRIANGLESTRIP:
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
		case GX_TRIANGLEFAN: {
			void* first = getIdx(vtx, dl);
			for (i = 2; i < n; i++) {
				SET_TRIGON(first, getIdx(vtx, dl + stride), getIdx(vtx, dl + stride * 2));
				ADD_TRIGON();
				dl += stride;
			}
			dl += stride * 2;
			break;
		}
		case GX_QUADS:
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
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    li       r6, 0x0
    li       r7, 0x0
    stw      r0, 0x84(r1)
    li       r0, 0x0
    stmw     r16, 0x40(r1)
    mr       r26, r3
    mr       r31, r4
    mr       r28, r5
    lwz      r3, 0x8(r3)
    addi     r4, r1, 0x8
    li       r5, 0x0
    stw      r0, 0x8(r1)
    bl       GetVtxSize__FP16_HSD_VtxDescListPiPiPiPi
    lwz      r5, 0x8(r26)
    mr       r29, r3
    lwz      r4, 0x8(r1)
    lwz      r0, 0xc(r5)
    lwz      r30, 0x14(r5)
    cmpwi    r0, 0x2
    lbz      r27, 0x10(r5)
    beq      .L_801A5170
    bge      .L_801A5128
    cmpwi    r0, 0x0
    beq      .L_801A5138
    bge      .L_801A5154
    b        .L_801A51C4
.L_801A5128:
    cmpwi    r0, 0x4
    beq      .L_801A51A8
    bge      .L_801A51C4
    b        .L_801A518C
.L_801A5138:
    cmpwi    r4, 0x2
    lis      r3, TrigonGetIdxU8U8@ha
    addi     r3, r3, TrigonGetIdxU8U8@l
    bne      .L_801A51C8
    lis      r3, TrigonGetIdxU8U16@ha
    addi     r3, r3, TrigonGetIdxU8U16@l
    b        .L_801A51C8
.L_801A5154:
    cmpwi    r4, 0x2
    lis      r3, TrigonGetIdxS8U8@ha
    addi     r3, r3, TrigonGetIdxS8U8@l
    bne      .L_801A51C8
    lis      r3, TrigonGetIdxS8U16@ha
    addi     r3, r3, TrigonGetIdxS8U16@l
    b        .L_801A51C8
.L_801A5170:
    cmpwi    r4, 0x2
    lis      r3, TrigonGetIdxU16U8@ha
    addi     r3, r3, TrigonGetIdxU16U8@l
    bne      .L_801A51C8
    lis      r3, TrigonGetIdxU16U16@ha
    addi     r3, r3, TrigonGetIdxU16U16@l
    b        .L_801A51C8
.L_801A518C:
    cmpwi    r4, 0x2
    lis      r3, TrigonGetIdxS16U8@ha
    addi     r3, r3, TrigonGetIdxS16U8@l
    bne      .L_801A51C8
    lis      r3, TrigonGetIdxS16U16@ha
    addi     r3, r3, TrigonGetIdxS16U16@l
    b        .L_801A51C8
.L_801A51A8:
    cmpwi    r4, 0x2
    lis      r3, TrigonGetIdxF32U8@ha
    addi     r3, r3, TrigonGetIdxF32U8@l
    bne      .L_801A51C8
    lis      r3, TrigonGetIdxF32U16@ha
    addi     r3, r3, TrigonGetIdxF32U16@l
    b        .L_801A51C8
.L_801A51C4:
    li       r3, 0x0
.L_801A51C8:
    cmpwi    r0, 0x2
    mr       r25, r3
    beq      .L_801A5210
    bge      .L_801A51E8
    cmpwi    r0, 0x0
    beq      .L_801A51F8
    bge      .L_801A5204
    b        .L_801A5234
.L_801A51E8:
    cmpwi    r0, 0x4
    beq      .L_801A5228
    bge      .L_801A5234
    b        .L_801A521C
.L_801A51F8:
    lis      r3, TrigonGetVtxU8@ha
    addi     r24, r3, TrigonGetVtxU8@l
    b        .L_801A5238
.L_801A5204:
    lis      r3, TrigonGetVtxS8@ha
    addi     r24, r3, TrigonGetVtxS8@l
    b        .L_801A5238
.L_801A5210:
    lis      r3, TrigonGetVtxU16@ha
    addi     r24, r3, TrigonGetVtxU16@l
    b        .L_801A5238
.L_801A521C:
    lis      r3, TrigonGetVtxS16@ha
    addi     r24, r3, TrigonGetVtxS16@l
    b        .L_801A5238
.L_801A5228:
    lis      r3, TrigonGetVtxF32@ha
    addi     r24, r3, TrigonGetVtxF32@l
    b        .L_801A5238
.L_801A5234:
    li       r24, 0x0
.L_801A5238:
    lhz      r0, 0xe(r26)
    lis      r4, __vt__14_trigonChecker@ha
    li       r3, 0x0
    lwz      r18, 0x10(r26)
    addi     r4, r4, __vt__14_trigonChecker@l
    slwi     r0, r0, 5
    mulli    r22, r29, 0x3
    stw      r4, 0x2c(r1)
    add      r17, r18, r0
    stw      r3, 0x30(r1)
    slwi     r23, r29, 1
    slwi     r21, r29, 2
    stw      r3, 0x34(r1)
    li       r19, 0x0
    b        .L_801A5794
.L_801A5274:
    lbz      r0, 0x0(r18)
    lbz      r3, 0x1(r18)
    rlwinm   r4, r0, 0, 24, 28
    lbz      r20, 0x2(r18)
    cmpwi    r4, 0xa0
    rlwimi   r20, r3, 8, 16, 23
    addi     r18, r18, 0x3
    beq      .L_801A5530
    bge      .L_801A52C8
    cmpwi    r4, 0x90
    beq      .L_801A5324
    bge      .L_801A52BC
    cmpwi    r4, 0x80
    beq      .L_801A5608
    bge      .L_801A5794
    cmpwi    r4, 0x0
    beq      .L_801A52EC
    b        .L_801A5794
.L_801A52BC:
    cmpwi    r4, 0x98
    beq      .L_801A5408
    b        .L_801A5794
.L_801A52C8:
    cmpwi    r4, 0xb0
    beq      .L_801A5318
    bge      .L_801A52E0
    cmpwi    r4, 0xa8
    beq      .L_801A530C
    b        .L_801A5794
.L_801A52E0:
    cmpwi    r4, 0xb8
    beq      .L_801A5300
    b        .L_801A5794
.L_801A52EC:
    lis      r4, __vt__14_trigonChecker@ha
    mr       r3, r19
    addi     r0, r4, __vt__14_trigonChecker@l
    stw      r0, 0x2c(r1)
    b        .L_801A57AC
.L_801A5300:
    mullw    r0, r29, r20
    add      r18, r18, r0
    b        .L_801A5794
.L_801A530C:
    mullw    r0, r29, r20
    add      r18, r18, r0
    b        .L_801A5794
.L_801A5318:
    mullw    r0, r29, r20
    add      r18, r18, r0
    b        .L_801A5794
.L_801A5324:
    lis      r3, 0x5555
    li       r16, 0x0
    addi     r0, r3, 0x5556
    mulhw    r3, r0, r20
    srwi     r0, r3, 31
    add      r26, r3, r0
    b        .L_801A53FC
.L_801A5340:
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0xc(r1)
    mr       r3, r30
    add      r4, r18, r29
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    add      r4, r18, r23
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
    addi     r3, r1, 0xc
    stw      r28, 0x20(r1)
    sth      r27, 0x24(r1)
    stw      r24, 0x18(r1)
    bl       Check__14_trigonCheckerFv
    clrlwi.  r0, r3, 24
    beq      .L_801A53F4
    lwz      r3, 0xc(r1)
    addi     r19, r19, 0x1
    lwz      r0, 0x10(r1)
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x14(r1)
    stw      r0, 0x8(r31)
    lwz      r0, 0x18(r1)
    stw      r0, 0xc(r31)
    lfs      f0, 0x1c(r1)
    stfs     f0, 0x10(r31)
    lwz      r0, 0x20(r1)
    stw      r0, 0x14(r31)
    lhz      r0, 0x24(r1)
    sth      r0, 0x18(r31)
    lhz      r0, 0x26(r1)
    sth      r0, 0x1a(r31)
    lwz      r0, 0x28(r1)
    stw      r0, 0x1c(r31)
    addi     r31, r31, 0x20
.L_801A53F4:
    add      r18, r18, r22
    addi     r16, r16, 0x1
.L_801A53FC:
    cmpw     r16, r26
    blt      .L_801A5340
    b        .L_801A5794
.L_801A5408:
    li       r16, 0x2
    b        .L_801A5520
.L_801A5410:
    clrlwi.  r0, r16, 31
    beq      .L_801A5464
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0xc(r1)
    mr       r3, r30
    add      r4, r18, r23
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    add      r4, r18, r29
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
    b        .L_801A54AC
.L_801A5464:
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0xc(r1)
    mr       r3, r30
    add      r4, r18, r29
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    add      r4, r18, r23
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
.L_801A54AC:
    stw      r28, 0x20(r1)
    addi     r3, r1, 0xc
    sth      r27, 0x24(r1)
    stw      r24, 0x18(r1)
    bl       Check__14_trigonCheckerFv
    clrlwi.  r0, r3, 24
    beq      .L_801A5518
    lwz      r3, 0xc(r1)
    addi     r19, r19, 0x1
    lwz      r0, 0x10(r1)
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x14(r1)
    stw      r0, 0x8(r31)
    lwz      r0, 0x18(r1)
    stw      r0, 0xc(r31)
    lfs      f0, 0x1c(r1)
    stfs     f0, 0x10(r31)
    lwz      r0, 0x20(r1)
    stw      r0, 0x14(r31)
    lhz      r0, 0x24(r1)
    sth      r0, 0x18(r31)
    lhz      r0, 0x26(r1)
    sth      r0, 0x1a(r31)
    lwz      r0, 0x28(r1)
    stw      r0, 0x1c(r31)
    addi     r31, r31, 0x20
.L_801A5518:
    add      r18, r18, r29
    addi     r16, r16, 0x1
.L_801A5520:
    cmpw     r16, r20
    blt      .L_801A5410
    add      r18, r18, r23
    b        .L_801A5794
.L_801A5530:
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r26, r3
    li       r16, 0x2
    b        .L_801A55F8
.L_801A5550:
    mr       r12, r25
    mr       r3, r30
    stw      r26, 0xc(r1)
    add      r4, r18, r29
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    add      r4, r18, r23
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
    addi     r3, r1, 0xc
    stw      r28, 0x20(r1)
    sth      r27, 0x24(r1)
    stw      r24, 0x18(r1)
    bl       Check__14_trigonCheckerFv
    clrlwi.  r0, r3, 24
    beq      .L_801A55F0
    lwz      r3, 0xc(r1)
    addi     r19, r19, 0x1
    lwz      r0, 0x10(r1)
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x14(r1)
    stw      r0, 0x8(r31)
    lwz      r0, 0x18(r1)
    stw      r0, 0xc(r31)
    lfs      f0, 0x1c(r1)
    stfs     f0, 0x10(r31)
    lwz      r0, 0x20(r1)
    stw      r0, 0x14(r31)
    lhz      r0, 0x24(r1)
    sth      r0, 0x18(r31)
    lhz      r0, 0x26(r1)
    sth      r0, 0x1a(r31)
    lwz      r0, 0x28(r1)
    stw      r0, 0x1c(r31)
    addi     r31, r31, 0x20
.L_801A55F0:
    add      r18, r18, r29
    addi     r16, r16, 0x1
.L_801A55F8:
    cmpw     r16, r20
    blt      .L_801A5550
    add      r18, r18, r23
    b        .L_801A5794
.L_801A5608:
    srawi    r0, r20, 2
    li       r16, 0x0
    addze    r26, r0
    b        .L_801A578C
.L_801A5618:
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0xc(r1)
    mr       r3, r30
    add      r4, r18, r29
    mtctr    r12
    bctrl
    add      r20, r18, r23
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    mr       r4, r20
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
    addi     r3, r1, 0xc
    stw      r28, 0x20(r1)
    sth      r27, 0x24(r1)
    stw      r24, 0x18(r1)
    bl       Check__14_trigonCheckerFv
    clrlwi.  r0, r3, 24
    beq      .L_801A56D0
    lwz      r3, 0xc(r1)
    addi     r19, r19, 0x1
    lwz      r0, 0x10(r1)
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x14(r1)
    stw      r0, 0x8(r31)
    lwz      r0, 0x18(r1)
    stw      r0, 0xc(r31)
    lfs      f0, 0x1c(r1)
    stfs     f0, 0x10(r31)
    lwz      r0, 0x20(r1)
    stw      r0, 0x14(r31)
    lhz      r0, 0x24(r1)
    sth      r0, 0x18(r31)
    lhz      r0, 0x26(r1)
    sth      r0, 0x1a(r31)
    lwz      r0, 0x28(r1)
    stw      r0, 0x1c(r31)
    addi     r31, r31, 0x20
.L_801A56D0:
    mr       r12, r25
    mr       r3, r30
    mr       r4, r18
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0xc(r1)
    mr       r3, r30
    mr       r4, r20
    mtctr    r12
    bctrl
    mr       r12, r25
    stw      r3, 0x10(r1)
    mr       r3, r30
    add      r4, r18, r22
    mtctr    r12
    bctrl
    stw      r3, 0x14(r1)
    addi     r3, r1, 0xc
    stw      r28, 0x20(r1)
    sth      r27, 0x24(r1)
    stw      r24, 0x18(r1)
    bl       Check__14_trigonCheckerFv
    clrlwi.  r0, r3, 24
    beq      .L_801A5784
    lwz      r3, 0xc(r1)
    addi     r19, r19, 0x1
    lwz      r0, 0x10(r1)
    stw      r3, 0x0(r31)
    stw      r0, 0x4(r31)
    lwz      r0, 0x14(r1)
    stw      r0, 0x8(r31)
    lwz      r0, 0x18(r1)
    stw      r0, 0xc(r31)
    lfs      f0, 0x1c(r1)
    stfs     f0, 0x10(r31)
    lwz      r0, 0x20(r1)
    stw      r0, 0x14(r31)
    lhz      r0, 0x24(r1)
    sth      r0, 0x18(r31)
    lhz      r0, 0x26(r1)
    sth      r0, 0x1a(r31)
    lwz      r0, 0x28(r1)
    stw      r0, 0x1c(r31)
    addi     r31, r31, 0x20
.L_801A5784:
    add      r18, r18, r21
    addi     r16, r16, 0x1
.L_801A578C:
    cmpw     r16, r26
    blt      .L_801A5618
.L_801A5794:
    cmplw    r18, r17
    blt      .L_801A5274
    lis      r4, __vt__14_trigonChecker@ha
    mr       r3, r19
    addi     r0, r4, __vt__14_trigonChecker@l
    stw      r0, 0x2c(r1)
.L_801A57AC:
    lmw      r16, 0x40(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

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
		case GX_VA_PNMTXIDX:
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
		case GX_VA_POS:
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
		case GX_VA_CLR0:
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
	return (u8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x801A4F08
 * @note Size: 0x10
 */
static void* TrigonGetIdxU8U8(void* base, u8* dl)
{
	return (u8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x801A4EF8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS8U16(void* base, u8* dl)
{
	return (s8*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x801A4EE8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS8U8(void* base, u8* dl)
{
	return (s8*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x801A4ED8
 * @note Size: 0x10
 */
static void* TrigonGetIdxU16U16(void* base, u8* dl)
{
	return (u16*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x801A4EC8
 * @note Size: 0x10
 */
static void* TrigonGetIdxU16U8(void* base, u8* dl)
{
	return (u16*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x801A4EB8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS16U16(void* base, u8* dl)
{
	return (s16*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x801A4EA8
 * @note Size: 0x10
 */
static void* TrigonGetIdxS16U8(void* base, u8* dl)
{
	return (s16*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x801A4E98
 * @note Size: 0x10
 */
static void* TrigonGetIdxF32U16(void* base, u8* dl)
{
	return (f32*)base + *(u16*)dl * 3;
}

/**
 * @note Address: 0x801A4E88
 * @note Size: 0x10
 */
static void* TrigonGetIdxF32U8(void* base, u8* dl)
{
	return (f32*)base + *(u8*)dl * 3;
}

/**
 * @note Address: 0x801A4CA0
 * @note Size: 0x1E8
 */
static void TrigonGetVtxU8(const TRIGON_INFO* tri, Vec* out)
{
	int i;
	int scale = 1 << tri->mFrac;

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
	// NONMATCHING: the initial joint-pointer load is scheduled too late.
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

/*
    stwu     r1, -0x70(r1)
    mflr     r0
    lwz      r6, 0x14(r3)
    stw      r0, 0x74(r1)
    stmw     r20, 0x40(r1)
    mr       r22, r4
    mr       r23, r5
    lwz      r0, 0x14(r6)
    rlwinm.  r0, r0, 0, 27, 27
    beq      .L_801A4458
    li       r3, -0x1
    b        .L_801A45E4
.L_801A4458:
    lwz      r28, 0x0(r3)
    li       r29, -0x1
    lwz      r27, 0x4(r3)
    cmplwi   r28, 0x0
    beq      .L_801A4474
    cmplwi   r27, 0x0
    bne      .L_801A447C
.L_801A4474:
    li       r3, -0x1
    b        .L_801A45E4
.L_801A447C:
    mr       r20, r27
    mr       r3, r22
    li       r26, 0x0
    bl       PSVECMag
    b        .L_801A44BC
.L_801A4490:
    srwi     r0, r20, 31
    add      r3, r0, r20
    srawi    r0, r3, 1
    add      r0, r26, r0
    srawi    r20, r3, 1
    slwi     r3, r0, 5
    addi     r0, r3, 0x10
    lfsx     f0, r28, r0
    fcmpo    cr0, f1, f0
    ble      .L_801A44BC
    add      r26, r26, r20
.L_801A44BC:
    cmpwi    r20, 0x0
    bgt      .L_801A4490
    addi     r31, r1, 0x30
    addi     r30, r1, 0x24
    li       r25, 0x0
    li       r24, 0x0
    b        .L_801A4590
.L_801A44D8:
    add      r21, r26, r25
    cmplw    r21, r27
    bge      .L_801A4530
    slwi     r0, r21, 5
    addi     r4, r1, 0x18
    add      r20, r28, r0
    lwz      r12, 0xc(r20)
    mr       r3, r20
    mtctr    r12
    bctrl
    lwz      r0, 0x1c(r20)
    cmplwi   r0, 0x0
    beq      .L_801A4530
    mr       r3, r22
    mr       r5, r30
    mr       r6, r31
    addi     r4, r1, 0x18
    bl       IsPointInsideTriangleXZ
    cmpwi    r3, 0x0
    beq      .L_801A4530
    mr       r29, r21
    b        .L_801A4598
.L_801A4530:
    subf     r3, r25, r26
    subic.   r21, r3, 0x1
    blt      .L_801A4588
    slwi     r0, r21, 5
    addi     r4, r1, 0x18
    add      r20, r28, r0
    lwz      r12, 0xc(r20)
    mr       r3, r20
    mtctr    r12
    bctrl
    lwz      r0, 0x1c(r20)
    cmplwi   r0, 0x0
    beq      .L_801A4588
    mr       r3, r22
    mr       r5, r30
    mr       r6, r31
    addi     r4, r1, 0x18
    bl       IsPointInsideTriangleXZ
    cmpwi    r3, 0x0
    beq      .L_801A4588
    mr       r29, r21
    b        .L_801A4598
.L_801A4588:
    addi     r25, r25, 0x1
    addi     r24, r24, 0x1
.L_801A4590:
    cmplw    r24, r27
    blt      .L_801A44D8
.L_801A4598:
    cmpwi    r29, 0x0
    blt      .L_801A45E0
    cmplwi   r23, 0x0
    beq      .L_801A45E0
    lis      r3, __vt__4CVec@ha
    mr       r4, r30
    addi     r0, r3, __vt__4CVec@l
    mr       r5, r31
    stw      r0, 0x14(r1)
    mr       r6, r22
    addi     r3, r1, 0x18
    addi     r7, r1, 0x8
    bl       IntersectVerticalLineWithTrianglePlane
    lfs      f0, 0xc(r1)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stfs     f0, 0x0(r23)
    stw      r0, 0x14(r1)
.L_801A45E0:
    mr       r3, r29
.L_801A45E4:
    lmw      r20, 0x40(r1)
    lwz      r0, 0x74(r1)
    mtlr     r0
    addi     r1, r1, 0x70
    blr
*/

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
