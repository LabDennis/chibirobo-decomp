#include <global.h>

#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/mtx/vec.h>
#include <kar/math.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/displayfunc.h>
#include <sysdolphin/dobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/identity_mtx.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/lobj_setup.h>
#include <sysdolphin/mtx_inverse.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/video.h>

#define HSD_JOBJ_METHOD(o)           ((HSD_JObjInfo*)((o)->object.parent.class_info))
#define HSD_ZLIST_NEXT(list, offset) (*(HSD_ZList**)((u8*)(list) + (offset)))
#define HSD_ZLIST_DEPTH(list)        (*(f32*)((u8*)(list) + 0x2C))
#define FLT_EPSILON                  1.1920929e-7F

typedef struct _HSD_ZList {
	Mtx pmtx;
	MtxPtr vmtx;
	HSD_JObj* jobj;
	u32 rendermode;
	struct _HSD_ZList* next_opa;
	struct _HSD_ZList* next_xlu;
	struct _HSD_ZList* next_all;
} HSD_ZList;

extern MtxPtr HSD_MtxAlloc(void);
extern void HSD_MtxFree(MtxPtr mtx);
extern void memset(void* ptr, s32 value, u32 size);
extern void PSVECScale(f32 scale, Vec* src, Vec* dst);
extern void PSMTXRotRad(Mtx m, char axis, f32 rad);

// GX parts HSD_EraseRect needs that the HSD GX header leaves out
#define GX_TEXCOORD0     0
#define GX_TEXCOORD_NULL 0xFF
#define GX_TEVSTAGE0     0
#define GX_TG_MTX2x4     1
#define GX_TG_TEX0       4
#define GX_NONE          0
#define GX_PASSCLR       4
#define GX_ZT_DISABLE    0
#define GX_ZT_REPLACE    2
#define GX_ALWAYS        7
#define GX_AOP_OR        1
#define GX_BM_LOGIC      2
#define GX_BL_ZERO       0
#define GX_BL_ONE        1
#define GX_LO_COPY       3
#define GX_LIGHT_NULL    0
#define GX_DF_NONE       0
#define GX_AF_NONE       2
#define GX_CLR_RGBA      1
#define GX_TEX_ST        1
#define GX_QUADS         0x80
#define GX_WGPIPE        ((volatile u8*)0xCC008000)

typedef struct _GXTexObj {
	u8 pad[0x20];
} GXTexObj;

extern void GXInitTexObj(GXTexObj* obj, void* image, u16 width, u16 height, GXTexFmt format, u32 wrap_s, u32 wrap_t, u8 mipmap);
extern void GXLoadTexObj(GXTexObj* obj, u32 map);
extern void GXSetNumTexGens(u8 n);
extern void GXSetTexCoordGen2(u32 dst, u32 func, u32 src, u32 mtx, u8 normalize, u32 postmtx);
extern void GXSetZTexture(u32 op, u32 format, u32 bias);
extern void GXSetChanCtrl(u32 chan, u8 enable, u32 amb_src, u32 mat_src, u32 light_mask, u32 diff_fn, u32 attn_fn);

static inline void GXPosition3f32(f32 x, f32 y, f32 z)
{
	*(volatile f32*)GX_WGPIPE = x;
	*(volatile f32*)GX_WGPIPE = y;
	*(volatile f32*)GX_WGPIPE = z;
}

static inline void GXColor4u8(u8 r, u8 g, u8 b, u8 a)
{
	*(volatile u8*)GX_WGPIPE = r;
	*(volatile u8*)GX_WGPIPE = g;
	*(volatile u8*)GX_WGPIPE = b;
	*(volatile u8*)GX_WGPIPE = a;
}

static inline void GXTexCoord2u8(u8 s, u8 t)
{
	*(volatile u8*)GX_WGPIPE = s;
	*(volatile u8*)GX_WGPIPE = t;
}

static inline void GXEnd(void)
{
}

HSD_ObjAllocData zlist_alloc_data;

// .sbss comes out last declaration first, so these run backwards
static s32 zlist_xlu_nb;
static HSD_ZList* zlist_xlu_top;
static s32 zlist_texedge_nb;
static HSD_ZList* zlist_texedge_top;
static HSD_ZList* zlist_top;
static s32 zsort_sorting;
static s32 zsort_listing;
static void (*sptcl_callback)(s32, s32, s32, HSD_JObj*);

static HSD_ZList** zlist_bottom         = &zlist_top;
static HSD_ZList** zlist_texedge_bottom = &zlist_texedge_top;
static HSD_ZList** zlist_xlu_bottom     = &zlist_xlu_top;

/**
 * @note Address: N/A
 * @note Size: ?
 */
inline MtxPtr HSD_CObjGetViewingMtxPtrDirect(HSD_CObj* cobj)
{
	return cobj->view_mtx;
}

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
	GXSetVtxAttrFmt(vtxfmt, 9, 1, 4, 0);
	GXSetVtxAttrFmt(vtxfmt, 0xB, 1, 5, 0);
	HSD_StateInitTev();
	HSD_SetupRenderMode(rendermode | 0x28000000);
	GXSetVtxDesc(9, 1);
	GXSetVtxDesc(0xB, 1);
	GXSetCurrentMtx(0);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 displayfunc_sqrtf(f32 x)
{
	volatile f32 y;

	if (x > 0.0F) {
		f64 guess = __frsqrte((f64)x);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		guess     = 0.5 * guess * __fnmsub(x, guess * guess, 3.0);
		y         = (f32)(x * guess);
		return y;
	}
	return x;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 displayfunc_col_mag(MtxPtr mtx, s32 col)
{
	return displayfunc_sqrtf(mtx[0][col] * mtx[0][col] + mtx[1][col] * mtx[1][col] + mtx[2][col] * mtx[2][col]);
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

	PSVECScale(1.0F / (FLT_EPSILON + PSVECMag(&ay)), &ay, &ay_n);

	sx = displayfunc_col_mag(src, 0);
	sz = displayfunc_col_mag(src, 2);

	if (jobj->flags & PBILLBOARD) {
		// az holds the flipped eye direction for a moment
		PSVECScale(-1.0F / (FLT_EPSILON + PSVECMag(&pos)), &pos, &az);
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

	PSVECScale(1.0F / (FLT_EPSILON + PSVECMag(&ax)), &ax, &ax_n);

	sy = displayfunc_col_mag(src, 1);
	sz = displayfunc_col_mag(src, 2);

	// ay starts out as the up vector to cross against
	if (jobj->flags & PBILLBOARD) {
		ay.y = FLT_EPSILON + displayfunc_sqrtf(__fmadds(pos.x, pos.x, pos.z * pos.z));
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
		PSVECScale(-1.0F / (FLT_EPSILON + PSVECMag(&pos)), &pos, &az);
	} else {
		az.x = 0.0F;
		az.y = 0.0F;
		az.z = 1.0F;
	}

	PSVECScale(1.0F / (FLT_EPSILON + sy), &ay, &ay);

	PSVECCrossProduct(&ay, &az, &ax);
	mag = PSVECMag(&ax);
	if (mag >= FLT_EPSILON) {
		sx /= mag;
		PSVECCrossProduct(&az, &ax, &ay);
		sy /= FLT_EPSILON + PSVECMag(&ay);
	} else {
		ay.y = FLT_EPSILON + displayfunc_sqrtf(az.x * az.x + az.z * az.z);
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

	if (jobj->flags & 0xE00) {
		PSMTXConcat(vmtx, jobj->mtx, sp8);
		switch (jobj->flags & 0xE00) {
		case 0x200:
			mkBillBoardMtx(jobj, sp8, pmtx);
			break;
		case 0x400:
			mkVBillBoardMtx(jobj, sp8, pmtx);
			break;
		case 0x600:
			mkHBillBoardMtx(jobj, sp8, pmtx);
			break;
		case 0x800:
			mkRBillBoardMtx(jobj, sp8, pmtx);
			break;
		default:
			HSD_Panic("displayfunc.c", 0x170, "unkown type of billboard.\n");
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
static inline HSD_JObj* displayfunc_find_skeleton(HSD_JObj* jobj)
{
	if (jobj == NULL) {
		__assert("displayfunc.c", 0x184, "jobj");
	}
	for (; jobj != NULL; jobj = jobj->parent) {
		if (jobj->flags & 3) {
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
	x = displayfunc_find_skeleton(jobj);
	if (x == NULL) {
		__assert("displayfunc.c", 0x1D4, "x");
	}

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

	dobj_trsp = trsp_mask << 1;

	if (!(rendermode & 0x4000000)) {
		if (jobj->flags & SPECULAR) {
			HSD_LObjSetupSpecularInit(pmtx);
		}
	}

	HSD_PObjClearMtxMark(NULL, 0);
	for (dobj = jobj->u.dobj; dobj; dobj = dobj->next) {
		if (dobj->flags & 1) {
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

	mask = jobj->flags & (trsp_mask << 18);
	if (mask == 0) {
		return;
	}

	HSD_JObjSetupMatrix(jobj);
	if (vmtx == NULL) {
		vmtx = HSD_CObjGetViewingMtxPtrDirect(HSD_CObjGetCurrent());
	}

	HSD_JOBJ_METHOD(jobj)->make_pmtx(jobj, vmtx, pmtx);

	if (mask & 0x40000) {
		HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, 1, rendermode);
	}

	if (zsort_listing == 0) {
		if (mask & 0x100000) {
			HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, 4, rendermode);
		}
		if (mask & 0x80000) {
			HSD_JOBJ_METHOD(jobj)->disp(jobj, vmtx, pmtx, 2, rendermode);
		}
	} else if (mask & 0x180000) {
		entry = HSD_ObjAlloc(&zlist_alloc_data);
		memset(&entry->vmtx, 0, 0x18);
		PSMTXCopy(pmtx, entry->pmtx);
		if (vmtx != NULL) {
			entry->vmtx = HSD_MtxAlloc();
			PSMTXCopy(vmtx, entry->vmtx);
		}
		entry->jobj       = jobj;
		entry->rendermode = rendermode;
		*zlist_bottom     = entry;
		zlist_bottom      = &entry->next_all;
		if (mask & 0x100000) {
			*zlist_texedge_bottom = entry;
			zlist_texedge_bottom  = &entry->next_opa;
			zlist_texedge_nb++;
		}
		if (mask & 0x80000) {
			*zlist_xlu_bottom = entry;
			zlist_xlu_bottom  = &entry->next_xlu;
			zlist_xlu_nb++;
		}
	}
}

/**
 * @note Address: 0x801CA498
 * @note Size: 0x16C
 */
HSD_ZList* zlist_sort(HSD_ZList* list, s32 count, s32 next_offset)
{
	HSD_ZList* left;
	HSD_ZList* right;
	HSD_ZList** tail;
	s32 left_count;
	s32 right_count;
	s32 i;

	if (count <= 1) {
		if (list != NULL) {
			HSD_ZLIST_NEXT(list, next_offset) = NULL;
		}
		return list;
	}

	left_count  = count / 2;
	right_count = count - left_count;
	right       = list;
	for (i = 0; i < left_count; i++) {
		right = HSD_ZLIST_NEXT(right, next_offset);
	}

	left  = zlist_sort(list, left_count, next_offset);
	right = zlist_sort(right, right_count, next_offset);

	list = NULL;
	tail = &list;
	while (left != NULL && right != NULL) {
		if (HSD_ZLIST_DEPTH(left) <= HSD_ZLIST_DEPTH(right)) {
			*tail = left;
			left  = HSD_ZLIST_NEXT(left, next_offset);
		} else {
			*tail = right;
			right = HSD_ZLIST_NEXT(right, next_offset);
		}
		tail = &HSD_ZLIST_NEXT(*tail, next_offset);
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
		zlist_texedge_top = zlist_sort(zlist_texedge_top, zlist_texedge_nb, 0x3C);
		zlist_xlu_top     = zlist_sort(zlist_xlu_top, zlist_xlu_nb, 0x40);
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
		HSD_JOBJ_METHOD(list->jobj)->disp(list->jobj, (list->vmtx) ? list->vmtx : vmtx, list->pmtx, 4, list->rendermode);
		list = list->next_opa;
	}

	list = zlist_xlu_top;
	while (list != NULL) {
		HSD_JOBJ_METHOD(list->jobj)->disp(list->jobj, (list->vmtx) ? list->vmtx : vmtx, list->pmtx, 2, list->rendermode);
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
				if ((u32)sp->data & 0x80000000) {
					u32 bank   = (u32)sp->data & 0x3F;
					u32 offset = ((u32)sp->data >> 6) & 0xFFFFFF;

					(*sptcl_callback)(0, bank, offset, jobj);
				}
				sp->data = (void*)((u32)sp->data & 0x7FFFFFFF);
			}
		}
	}
}

/**
 * @note Address: 0x801CA874
 * @note Size: 0x8
 */
void HSD_JObjSetSPtclCallback(void (*func)(s32, s32, s32, HSD_JObj*))
{
	sptcl_callback = func;
}

// The game never erases a rectangle, so the linker dropped the two functions below together with their data. Only
// the depth texture's 32 byte alignment is left: it is why retail aligns this file's .data to 32.
static GXColor erase_color = { 0, 0, 0, 0 };

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_SetEraseColor(u8 r, u8 g, u8 b, u8 a)
{
	erase_color.r = r;
	erase_color.g = g;
	erase_color.b = b;
	erase_color.a = a;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_EraseRect(f32 top, f32 bottom, f32 left, f32 right, f32 z, s32 enable_color, s32 enable_alpha, s32 enable_depth)
{
	// a 4x4 Z8 texture at the far plane; GX wants textures 32 byte aligned
	static u8 depth_image[4 * 4] __attribute__((aligned(32))) = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};
	GXTexObj texobj;
	GXColor color;

	if (!enable_color && !enable_alpha && !enable_depth) {
		return;
	}

	GXSetNumTevStages(1);
	if (enable_depth) {
		GXInitTexObj(&texobj, depth_image, 4, 4, GX_TF_Z8, GX_REPEAT, GX_REPEAT, GX_DISABLE);
		GXLoadTexObj(&texobj, GX_TEXMAP0);
		GXSetNumTexGens(1);
		GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_NONE, GX_PTIDENTITY);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GXSetZTexture(GX_ZT_REPLACE, GX_TF_Z8, 0);
	} else {
		GXSetNumTexGens(0);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	}
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	GXSetCullMode(GX_CULL_NONE);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZCompLoc(GX_TRUE);
	GXSetZMode(GX_ENABLE, GX_ALWAYS, enable_depth ? GX_ENABLE : GX_DISABLE);
	GXSetBlendMode(GX_BM_LOGIC, GX_BL_ONE, GX_BL_ZERO, GX_LO_COPY);
	GXSetColorUpdate(enable_color ? GX_ENABLE : GX_DISABLE);
	GXSetAlphaUpdate(enable_alpha ? GX_ENABLE : GX_DISABLE);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
	GXLoadPosMtxImm(HSD_identityMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);

	color = erase_color;
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(left, top, z);
	GXColor4u8(color.r, color.g, color.b, color.a);
	GXTexCoord2u8(0, 0);
	GXPosition3f32(right, top, z);
	GXColor4u8(color.r, color.g, color.b, color.a);
	GXTexCoord2u8(1, 0);
	GXPosition3f32(right, bottom, z);
	GXColor4u8(color.r, color.g, color.b, color.a);
	GXTexCoord2u8(1, 1);
	GXPosition3f32(left, bottom, z);
	GXColor4u8(color.r, color.g, color.b, color.a);
	GXTexCoord2u8(0, 1);
	GXEnd();

	GXSetZTexture(GX_ZT_DISABLE, GX_TF_Z8, 0);
	HSD_StateInvalidate(HSD_STATE_ALL);
}

/**
 * @note Address: N/A
 * @note Size: 0x34
 */
void _HSD_DispForgetMemory(void)
{
	reset_zlist();
}
