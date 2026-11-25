#include <global.h>

#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/mtx/vec.h>
#include <kar/math.h>
#include <sdk/msl/c/cstddef>
#include <sysdolphin/cobj.h>
#include <sysdolphin/displayfunc.h>
#include <sysdolphin/dobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/identity_mtx.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/lobj_setup.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/video.h>
#include <math.h>
#include <string.h>

#define FLT_EPSILON 1.1920929e-7F

enum {
	JOBJ_OPA = HSD_TRSP_OPA << JOBJ_TRSP_SHIFT,
	JOBJ_XLU = HSD_TRSP_XLU << JOBJ_TRSP_SHIFT,
	JOBJ_TEXEDGE = HSD_TRSP_TEXEDGE << JOBJ_TRSP_SHIFT,
};

typedef struct _HSD_ZList {
	Mtx pmtx;
	MtxPtr vmtx;
	HSD_JObj* jobj;
	u32 rendermode;
	struct _HSD_ZList* next_texedge;
	struct _HSD_ZList* next_xlu;
	struct _HSD_ZList* next_all;
} HSD_ZList;

static HSD_ObjAllocData zlist_alloc_data;

// .sbss comes out last declaration first, so these run backwards
static s32 zlist_xlu_nb;
static HSD_ZList* zlist_xlu_top;
static s32 zlist_texedge_nb;
static HSD_ZList* zlist_texedge_top;
static HSD_ZList* zlist_top;
static s32 zsort_sorting;
static s32 zsort_listing;
static HSD_JObjParticleCallback sptcl_callback;

static HSD_ZList** zlist_bottom         = &zlist_top;
static HSD_ZList** zlist_texedge_bottom = &zlist_texedge_top;
static HSD_ZList** zlist_xlu_bottom     = &zlist_xlu_top;

/**
 * @note Address: 0x801C93E4
 * @note Size: 0x30
 */
void HSD_ZListInitAllocData(void)
{
	HSD_ObjAllocInit(&zlist_alloc_data, sizeof(HSD_ZList), 4);
}

/**
 * @note Address: N/A
 * @note Size: 0x94
 */
void HSD_StateInitDirect(u32 vtxfmt, u32 rendermode)
{
	HSD_ClearVtxDesc();
	GXSetVtxAttrFmt(vtxfmt, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(vtxfmt, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	HSD_StateInitTev();
	HSD_SetupRenderMode(rendermode | RENDER_ZMODE_ALWAYS | RENDER_NO_ZUPDATE);
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetCurrentMtx(GX_PNMTX0);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 displayfunc_col_mag(MtxPtr mtx, s32 col)
{
	return dolsqrtf(mtx[0][col] * mtx[0][col] + mtx[1][col] * mtx[1][col] + mtx[2][col] * mtx[2][col]);
}

/**
 * @note Address: 0x801C9414
 * @note Size: 0x2DC
 */
void mkVBillBoardMtx(HSD_JObj* jobj, Mtx src, Mtx dst)
{
	Vec pos, ax, ay, az, ay_n;
	f32 sx, sz;
	f32 mag;

	pos.x = src[0][3];
	pos.y = src[1][3];
	pos.z = src[2][3];

	ay.x = src[0][1];
	ay.y = src[1][1];
	ay.z = src[2][1];

	PSVECScale(&ay, &ay_n, 1.0F / (FLT_EPSILON + PSVECMag(&ay)));

	sx = displayfunc_col_mag(src, 0);
	sz = displayfunc_col_mag(src, 2);

	if (jobj->flags & PBILLBOARD) {
		// az holds the flipped eye direction for a moment
		PSVECScale(&pos, &az, -1.0F / (FLT_EPSILON + PSVECMag(&pos)));
		PSVECCrossProduct(&ay_n, &az, &ax);
	} else {
		Vec zOne = { 0.0F, 0.0F, 1.0F };
		PSVECCrossProduct(&ay_n, &zOne, &ax);
	}

	mag = PSVECMag(&ax);
	if (!(mag < FLT_EPSILON)) {
		sx /= mag;
		PSVECCrossProduct(&ax, &ay_n, &az);
		mag = PSVECMag(&az);
		sz /= FLT_EPSILON + mag;

		dst[0][0] = sx * ax.x;
		dst[1][0] = sx * ax.y;
		dst[2][0] = sx * ax.z;

		dst[0][1] = ay.x;
		dst[1][1] = ay.y;
		dst[2][1] = ay.z;

		dst[0][2] = sz * az.x;
		dst[1][2] = sz * az.y;
		dst[2][2] = sz * az.z;

		dst[0][3] = pos.x;
		dst[1][3] = pos.y;
		dst[2][3] = pos.z;
	} else {
		PSMTXCopy(src, dst);
	}
}

/**
 * @note Address: 0x801C96F0
 * @note Size: 0x344
 */
void mkHBillBoardMtx(HSD_JObj* jobj, Mtx src, Mtx dst)
{
	Vec pos, ax, ay, az, ax_n;
	f32 sy, sz;
	f32 mag;

	pos.x = src[0][3];
	pos.y = src[1][3];
	pos.z = src[2][3];

	ax.x = src[0][0];
	ax.y = src[1][0];
	ax.z = src[2][0];

	PSVECScale(&ax, &ax_n, 1.0F / (FLT_EPSILON + PSVECMag(&ax)));

	sy = displayfunc_col_mag(src, 1);
	sz = displayfunc_col_mag(src, 2);

	// ay starts out as the up vector to cross against
	if (jobj->flags & PBILLBOARD) {
		ay.y = FLT_EPSILON + dolsqrtf(__fmadds(pos.x, pos.x, pos.z * pos.z));
		ay.x = -pos.y / ay.y * pos.x;
		ay.z = -pos.y / ay.y * pos.z;
		PSVECNormalize(&ay, &ay);
	} else {
		ay.x = 0.0F;
		ay.y = 1.0F;
		ay.z = 0.0F;
	}

	PSVECCrossProduct(&ax_n, &ay, &az);
	mag = PSVECMag(&az);
	if (!(mag < FLT_EPSILON)) {
		sz /= mag;
		PSVECCrossProduct(&az, &ax_n, &ay);
		mag = PSVECMag(&ay);
		sy /= FLT_EPSILON + mag;

		dst[0][0] = ax.x;
		dst[1][0] = ax.y;
		dst[2][0] = ax.z;

		dst[0][1] = sy * ay.x;
		dst[1][1] = sy * ay.y;
		dst[2][1] = sy * ay.z;

		dst[0][2] = sz * az.x;
		dst[1][2] = sz * az.y;
		dst[2][2] = sz * az.z;

		dst[0][3] = pos.x;
		dst[1][3] = pos.y;
		dst[2][3] = pos.z;
	} else {
		PSMTXCopy(src, dst);
	}
}

/**
 * @note Address: 0x801C9A34
 * @note Size: 0x398
 */
void mkBillBoardMtx(HSD_JObj* jobj, Mtx src, Mtx dst)
{
	Vec ax, ay, az, pos;
	f32 sx, sy, sz;
	f32 mag;

	sx = displayfunc_col_mag(src, 0);
	sz = displayfunc_col_mag(src, 2);

	ay.x = src[0][1];
	ay.y = src[1][1];
	ay.z = src[2][1];
	sy   = PSVECMag(&ay);

	pos.x = src[0][3];
	pos.y = src[1][3];
	pos.z = src[2][3];

	if (jobj->flags & PBILLBOARD) {
		PSVECScale(&pos, &az, -1.0F / (FLT_EPSILON + PSVECMag(&pos)));
	} else {
		az.x = 0.0F;
		az.y = 0.0F;
		az.z = 1.0F;
	}

	PSVECScale(&ay, &ay, 1.0F / (FLT_EPSILON + sy));

	PSVECCrossProduct(&ay, &az, &ax);
	mag = PSVECMag(&ax);
	if (mag >= FLT_EPSILON) {
		sx /= mag;
		PSVECCrossProduct(&az, &ax, &ay);
		sy /= FLT_EPSILON + PSVECMag(&ay);
	} else {
		ay.y = FLT_EPSILON + dolsqrtf(az.x * az.x + az.z * az.z);
		ay.x = -az.y / ay.y * az.x;
		ay.z = -az.y / ay.y * az.z;

		PSVECCrossProduct(&ay, &az, &ax);
		sx /= FLT_EPSILON + PSVECMag(&ax);
	}

	dst[0][0] = sx * ax.x;
	dst[1][0] = sx * ax.y;
	dst[2][0] = sx * ax.z;

	dst[0][1] = sy * ay.x;
	dst[1][1] = sy * ay.y;
	dst[2][1] = sy * ay.z;

	dst[0][2] = sz * az.x;
	dst[1][2] = sz * az.y;
	dst[2][2] = sz * az.z;

	dst[0][3] = pos.x;
	dst[1][3] = pos.y;
	dst[2][3] = pos.z;
}

/**
 * @note Address: 0x801C9DCC
 * @note Size: 0x1E8
 */
void mkRBillBoardMtx(HSD_JObj* jobj, Mtx src, Mtx dst)
{
	Mtx rot, scl;
	f32 sx, sy, sz;

	sx = displayfunc_col_mag(src, 0);
	sy = displayfunc_col_mag(src, 1);
	sz = displayfunc_col_mag(src, 2);

	PSMTXScale(scl, sx, sy, sz);
	PSMTXRotRad(rot, 'z', jobj->rotate.z);

	rot[0][3] = src[0][3];
	rot[1][3] = src[1][3];
	rot[2][3] = src[2][3];

	PSMTXConcat(rot, scl, dst);
}

/**
 * @note Address: 0x801C9FB4
 * @note Size: 0x100
 */
void HSD_JObjMakePositionMtx(HSD_JObj* jobj, Mtx vmtx, Mtx pmtx)
{
	Mtx sp8;

	if (jobj->flags & BILLBOARD_MASK) {
		PSMTXConcat(vmtx, jobj->mtx, sp8);
		switch (jobj->flags & BILLBOARD_MASK) {
		case BILLBOARD:
			mkBillBoardMtx(jobj, sp8, pmtx);
			break;
		case VBILLBOARD:
			mkVBillBoardMtx(jobj, sp8, pmtx);
			break;
		case HBILLBOARD:
			mkHBillBoardMtx(jobj, sp8, pmtx);
			break;
		case RBILLBOARD:
			mkRBillBoardMtx(jobj, sp8, pmtx);
			break;
		default:
			HSD_Panic(__FILE__, 0x170, "unkown type of billboard.\n");
			break;
		}
	} else {
		PSMTXConcat(vmtx, jobj->mtx, pmtx);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_JObj* FindSkeleton(HSD_JObj* jobj)
{
	assert_line(0x184, jobj);
	for (; jobj != NULL; jobj = jobj->parent) {
		if (jobj->flags & (SKELETON | SKELETON_ROOT)) {
			return jobj;
		}
	}
	return NULL;
}

/**
 * @note Address: 0x801CA0B4
 * @note Size: 0x10C
 */
MtxPtr _HSD_mkEnvelopeModelNodeMtx(HSD_JObj* jobj, Mtx dst)
{
	HSD_JObj* x;
	Mtx sp8;

	if (jobj->flags & SKELETON_ROOT) {
		return NULL;
	}
	x = FindSkeleton(jobj);
	assert_line(0x1D4, x);

	if (x == jobj) {
		PSMTXInverse(x->envelopemtx, dst);
	} else if (x->flags & SKELETON_ROOT) {
		HSD_MtxInverseConcat(x->mtx, jobj->mtx, dst);
	} else {
		PSMTXConcat(x->mtx, x->envelopemtx, sp8);
		HSD_MtxInverseConcat(sp8, jobj->mtx, dst);
	}
	return dst;
}

/**
 * @note Address: 0x801CA1C0
 * @note Size: 0xD0
 */
void HSD_JObjDispSub(HSD_JObj* jobj, Mtx vmtx, Mtx pmtx, HSD_TrspMask trsp_mask, u32 rendermode)
{
	HSD_DObj* dobj;
	u32 dobj_trsp;

	HSD_JObjSetCurrent(jobj);

	dobj_trsp = trsp_mask << DOBJ_TRSP_SHIFT;

	if (!(rendermode & RENDER_SHADOW)) {
		if (jobj->flags & SPECULAR) {
			HSD_LObjSetupSpecularInit(pmtx);
		}
	}

	HSD_PObjClearMtxMark(NULL, 0);
	for (dobj = jobj->u.dobj; dobj; dobj = dobj->next) {
		if (dobj->flags & DOBJ_HIDDEN) {
			continue;
		}

		if (dobj->flags & dobj_trsp) {
			HSD_DObjSetCurrent(dobj);
			HSD_DOBJ_METHOD(dobj)->disp(dobj, vmtx, pmtx, rendermode);
		}
	}
	HSD_DObjSetCurrent(NULL);
	HSD_JObjSetCurrent(NULL);
}

/**
 * @note Address: 0x801CA290
 * @note Size: 0x208
 */
void HSD_JObjDispDObj(HSD_JObj* jobj, Mtx vmtx, HSD_TrspMask trsp_mask, u32 rendermode)
{
	u32 mask;
	Mtx pmtx;
	HSD_ZList* entry;

	if (jobj->flags & HIDDEN) {
		return;
	}

	mask = jobj->flags & (trsp_mask << JOBJ_TRSP_SHIFT);
	if (mask == 0) {
		return;
	}

	HSD_JObjSetupMatrix(jobj);
	if (vmtx == NULL) {
		vmtx = HSD_CObjGetViewingMtxPtrDirect(HSD_CObjGetCurrent());
	}

	HSD_JOBJ_METHOD(jobj)->make_pmtx(jobj, vmtx, pmtx);

	if (mask & JOBJ_OPA) {
		HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, HSD_TRSP_OPA, rendermode);
	}

	if (zsort_listing == 0) {
		if (mask & JOBJ_TEXEDGE) {
			HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, HSD_TRSP_TEXEDGE, rendermode);
		}
		if (mask & JOBJ_XLU) {
			HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, HSD_TRSP_XLU, rendermode);
		}
	} else if (mask & (JOBJ_TEXEDGE | JOBJ_XLU)) {
		entry = HSD_ObjAlloc(&zlist_alloc_data);
		memset((u8*)entry + offsetof(HSD_ZList, vmtx), 0, sizeof(*entry) - offsetof(HSD_ZList, vmtx));
		PSMTXCopy(pmtx, entry->pmtx);
		if (vmtx != NULL) {
			entry->vmtx = HSD_MtxAlloc();
			PSMTXCopy(vmtx, entry->vmtx);
		}
		entry->jobj       = jobj;
		entry->rendermode = rendermode;
		*zlist_bottom     = entry;
		zlist_bottom      = &entry->next_all;
		if (mask & JOBJ_TEXEDGE) {
			*zlist_texedge_bottom = entry;
			zlist_texedge_bottom  = &entry->next_texedge;
			zlist_texedge_nb++;
		}
		if (mask & JOBJ_XLU) {
			*zlist_xlu_bottom = entry;
			zlist_xlu_bottom  = &entry->next_xlu;
			zlist_xlu_nb++;
		}
	}
}

static inline HSD_ZList** zlist_link(HSD_ZList* entry, size_t next_offset)
{
	return (HSD_ZList**)((u8*)entry + next_offset);
}

/**
 * @note Address: 0x801CA498
 * @note Size: 0x16C
 */
HSD_ZList* zlist_sort(HSD_ZList* list, s32 count, size_t next_offset)
{
	HSD_ZList* left;
	HSD_ZList* right;
	HSD_ZList** tail;
	s32 left_count;
	s32 right_count;
	s32 i;

	if (count <= 1) {
		if (list != NULL) {
			*zlist_link(list, next_offset) = NULL;
		}
		return list;
	}

	left_count  = count / 2;
	right_count = count - left_count;
	right       = list;
	for (i = 0; i < left_count; i++) {
		right = *zlist_link(right, next_offset);
	}

	left  = zlist_sort(list, left_count, next_offset);
	right = zlist_sort(right, right_count, next_offset);

	list = NULL;
	tail = &list;
	while (left != NULL && right != NULL) {
		if (left->pmtx[2][3] <= right->pmtx[2][3]) {
			*tail = left;
			left  = *zlist_link(left, next_offset);
		} else {
			*tail = right;
			right = *zlist_link(right, next_offset);
		}
		tail = zlist_link(*tail, next_offset);
	}
	if (left != NULL) {
		*tail = left;
	} else if (right != NULL) {
		*tail = right;
	}
	return list;
}

/**
 * @note Address: 0x801CA604
 * @note Size: 0x50
 */
void _HSD_ZListSort(void)
{
	if (zsort_sorting != 0) {
		zlist_texedge_top = zlist_sort(zlist_texedge_top, zlist_texedge_nb, offsetof(HSD_ZList, next_texedge));
		zlist_xlu_top     = zlist_sort(zlist_xlu_top, zlist_xlu_nb, offsetof(HSD_ZList, next_xlu));
	}
}

/**
 * @note Address: 0x801CA654
 * @note Size: 0xC8
 */
void _HSD_ZListDisp(void)
{
	HSD_ZList* list;
	MtxPtr vmtx;
	HSD_CObj* cobj;

	cobj = HSD_CObjGetCurrent();
	vmtx = HSD_CObjGetViewingMtxPtrDirect(cobj);

	list = zlist_texedge_top;
	while (list != NULL) {
		HSD_JOBJ_METHOD(list->jobj)->disp(list->jobj, list->vmtx ? list->vmtx : vmtx, list->pmtx,
		                                 HSD_TRSP_TEXEDGE, list->rendermode);
		list = list->next_texedge;
	}

	list = zlist_xlu_top;
	while (list != NULL) {
		HSD_JOBJ_METHOD(list->jobj)->disp(list->jobj, list->vmtx ? list->vmtx : vmtx, list->pmtx,
		                                 HSD_TRSP_XLU, list->rendermode);
		list = list->next_xlu;
	}

	_HSD_ZListClear();
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void reset_zlist(void)
{
	zlist_top            = NULL;
	zlist_bottom         = &zlist_top;
	zlist_texedge_top    = NULL;
	zlist_texedge_bottom = &zlist_texedge_top;
	zlist_texedge_nb     = 0;
	zlist_xlu_top        = NULL;
	zlist_xlu_bottom     = &zlist_xlu_top;
	zlist_xlu_nb         = 0;
}

/**
 * @note Address: 0x801CA71C
 * @note Size: 0xA0
 */
void _HSD_ZListClear(void)
{
	HSD_ZList* entry;
	HSD_ZList* next;

	for (entry = zlist_top; entry != NULL; entry = next) {
		next = entry->next_all;
		if (entry->vmtx != NULL) {
			HSD_MtxFree(entry->vmtx);
		}
		HSD_ObjFree(&zlist_alloc_data, entry);
	}
	reset_zlist();
}

/**
 * @note Address: 0x801CA7BC
 * @note Size: 0xC
 */
void HSD_SetZSortMode(s32 mode, s32 sort)
{
	zsort_listing = mode;
	zsort_sorting = sort;
}

/**
 * @note Address: 0x801CA7C8
 * @note Size: 0xAC
 */
void HSD_JObjDisp(HSD_JObj* jobj, Mtx vmtx, HSD_TrspMask trsp_mask, u32 rendermode)
{
	if (jobj != NULL) {
		if (union_type_dobj(jobj)) {
			HSD_JObjDispDObj(jobj, vmtx, trsp_mask, rendermode);
		} else if (union_type_ptcl(jobj) && sptcl_callback != NULL) {
			HSD_SList* sp;

			for (sp = jobj->u.ptcl; sp != NULL; sp = sp->next) {
				u32 particle = sp->uint_value;
				if (particle & JOBJ_STATIC_PARTICLE_PENDING) {
					u32 bank   = particle & JOBJ_STATIC_PARTICLE_BANK_MASK;
					u32 offset = (particle >> JOBJ_STATIC_PARTICLE_OFFSET_SHIFT) & JOBJ_STATIC_PARTICLE_OFFSET_MASK;

					(*sptcl_callback)(0, bank, offset, jobj);
				}
				sp->uint_value &= ~JOBJ_STATIC_PARTICLE_PENDING;
			}
		}
	}
}

/**
 * @note Address: 0x801CA874
 * @note Size: 0x8
 */
void HSD_JObjSetSPtclCallback(HSD_JObjParticleCallback callback)
{
	sptcl_callback = callback;
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void _HSD_DispForgetMemory(void)
{
	reset_zlist();
}
