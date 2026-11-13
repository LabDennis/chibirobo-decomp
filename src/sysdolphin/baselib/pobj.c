#include <global.h>

#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <kar/math.h>
#include <sysdolphin/pobj.h>

#include <sysdolphin/class_alloc.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/id.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/initialize.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/perf.h>
#include <sysdolphin/shadow.h>
#include <sysdolphin/tev.h>

extern BOOL hsdIsDescendantOf(void* info, void* base);

extern void memset(void*, int, int);
extern void memcpy(void*, const void*, int);

extern struct {
	u8 pad[0xC];
	s32 unkC;
} HSD_PerfCurrentStat;

extern s32 HSD_Index2PosNrmMtx(s32 index);
extern u32 PSMTXInvXpose(Mtx src, Mtx xpose);

void PObjInfoInit(void);

HSD_PObjInfo hsdPObj = { PObjInfoInit };

static HSD_VtxDescList* prev_vtxdesc;
static HSD_VtxDescList* prev_vtxdesclist_array;
static u32 normal_buffer_size;
static u32 vertex_buffer_size;
static f32 (*normal_buffer)[3];
static f32 (*vertex_buffer)[3];
static HSD_PObjInfo* default_class;

static struct {
	void* obj;
	u32 mark;
} mtx_mark[2];

#define ASSERT_POBJ(line, cond)          ((cond) ? ((void)0) : __assert("pobj.c", line, #cond))
#define ASSERT_POBJ_MSG(line, cond, msg) ((cond) ? ((void)0) : __assert("pobj.c", line, msg))

#define GX_WGPIPE ((volatile u8*)0xCC008000)

/**
 * @note Address: 0x801AEB88
 * @note Size: 0x18
 */
void HSD_PObjSetFlags(HSD_PObj* pobj, u32 flags)
{
	if (pobj != NULL) {
		pobj->flags |= flags;
	}
}

/**
 * @note Address: 0x801AEB70
 * @note Size: 0x18
 */
void HSD_PObjClearFlags(HSD_PObj* pobj, u32 flags)
{
	if (pobj != NULL) {
		pobj->flags &= ~flags;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PObjRemoveAnimByFlags(HSD_PObj* pobj, u32 flags)
{
	if (pobj != NULL && (flags & POBJ_ANIM)) {
		HSD_AObjRemove(pobj->aobj);
		pobj->aobj = NULL;
	}
}

/**
 * @note Address: 0x801AEAFC
 * @note Size: 0x74
 */
void HSD_PObjRemoveAnimAllByFlags(HSD_PObj* pobj, u32 flags)
{
	HSD_PObj* pp;

	if (pobj == NULL) {
		return;
	}

	for (pp = pobj; pp != NULL; pp = pp->next) {
		HSD_PObjRemoveAnimByFlags(pp, flags);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PObjAddAnim(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
	if (pobj != NULL) {
		if (pobj->aobj != NULL) {
			HSD_AObjRemove(pobj->aobj);
		}
		pobj->aobj = HSD_AObjLoadDesc(shapeanim->aobjdesc);
	}
}

/**
 * @note Address: 0x801AEA6C
 * @note Size: 0x90
 */
void HSD_PObjAddAnimAll(HSD_PObj* pobj, HSD_ShapeAnim* shapeanim)
{
	HSD_PObj* po;
	HSD_ShapeAnim* sa;

	if (pobj == NULL || shapeanim == NULL) {
		return;
	}

	for (po = pobj, sa = shapeanim; po != NULL; po = po->next, sa = next_p(sa)) {
		HSD_PObjAddAnim(po, sa);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PObjReqAnimByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
	if (pobj != NULL && (flags & POBJ_ANIM)) {
		HSD_AObjReqAnim(pobj->aobj, startframe);
	}
}

/**
 * @note Address: 0x801AE9F8
 * @note Size: 0x74
 */
void HSD_PObjReqAnimAllByFlags(HSD_PObj* pobj, f32 startframe, u32 flags)
{
	HSD_PObj* pp;

	if (pobj == NULL) {
		return;
	}

	for (pp = pobj; pp != NULL; pp = pp->next) {
		HSD_PObjReqAnimByFlags(pp, startframe, flags);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void ShapeSetSetAnimResult(HSD_ShapeSet* shape_set, u32 type, HSD_ObjData* val)
{
	if (shape_set->flags & SHAPESET_ADDITIVE) {
		shape_set->blend.bp[type - HSD_A_S_W0] = val->fv;
	} else {
		shape_set->blend.bl = val->fv;
	}
}

/**
 * @note Address: 0x801AE9AC
 * @note Size: 0x4C
 */
void PObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val)
{
	HSD_PObj* pobj = HSD_POBJ(obj);

	if (pobj == NULL) {
		return;
	}

	if (pobj_type(pobj) == POBJ_SHAPEANIM) {
		ShapeSetSetAnimResult(pobj->u.shape_set, type, val);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PObjAnim(HSD_PObj* pobj)
{
	if (pobj != NULL) {
		HSD_AObjInterpretAnim(pobj->aobj, pobj, HSD_POBJ_METHOD(pobj)->update);
	}
}

/**
 * @note Address: 0x801AE950
 * @note Size: 0x5C
 */
void HSD_PObjAnimAll(HSD_PObj* pobj)
{
	HSD_PObj* pp;

	if (pobj != NULL) {
		for (pp = pobj; pp != NULL; pp = pp->next) {
			HSD_PObjAnim(pp);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXPosition3f32(f32 x, f32 y, f32 z)
{
	*(volatile f32*)GX_WGPIPE = x;
	*(volatile f32*)GX_WGPIPE = y;
	*(volatile f32*)GX_WGPIPE = z;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXNormal3f32(f32 x, f32 y, f32 z)
{
	*(volatile f32*)GX_WGPIPE = x;
	*(volatile f32*)GX_WGPIPE = y;
	*(volatile f32*)GX_WGPIPE = z;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXMatrixIndex1u8(u8 idx)
{
	*(volatile u8*)GX_WGPIPE = idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXTexCoord1x8(u8 idx)
{
	*(volatile u8*)GX_WGPIPE = idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXTexCoord1x16(u16 idx)
{
	*(volatile u16*)GX_WGPIPE = idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXColor1x8(u8 idx)
{
	*(volatile u8*)GX_WGPIPE = idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXColor1x16(u16 idx)
{
	*(volatile u16*)GX_WGPIPE = idx;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXColor1u16(u16 clr)
{
	*(volatile u16*)GX_WGPIPE = clr;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXColor3u8(u8 r, u8 g, u8 b)
{
	*(volatile u8*)GX_WGPIPE = r;
	*(volatile u8*)GX_WGPIPE = g;
	*(volatile u8*)GX_WGPIPE = b;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXColor4u8(u8 r, u8 g, u8 b, u8 a)
{
	*(volatile u8*)GX_WGPIPE = r;
	*(volatile u8*)GX_WGPIPE = g;
	*(volatile u8*)GX_WGPIPE = b;
	*(volatile u8*)GX_WGPIPE = a;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void GXEnd(void)
{
}

#define pobj_min(x, y) (x < y ? x : y)
#define MAX(a, b)      (((a) > (b)) ? (a) : (b))

// the anim functions live in tobj.c for now, pobj's split ends too early
void PObjUpdateFunc(void* obj, enum_t type, FObjData* val);

extern HSD_ObjAllocData shadow_alloc_data;

#define ASSERT_REF_COUNT(line, cond) ((cond) ? ((void)0) : __assert("object.h", line, "HSD_OBJ(o)->ref_count_individual != 0"))

// the hsd gx headers dont have these
#define GX_POS_XY        0
#define GX_TEX_S         0
#define GX_QUADS         0x80
#define GX_TRIANGLES     0x90
#define GX_TRIANGLESTRIP 0x98
#define GX_TRIANGLEFAN   0xA0

s32 PObjLoad(HSD_PObj* pobj, HSD_PObjDesc* desc);
void PObjAmnesia(HSD_ClassInfo* info);
void PObjRelease(HSD_Class* o);
void PObjSetupMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
void SetupEnvelopeModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
void SetupSharedVtxModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode);
void drawShapeAnim(HSD_PObj* pobj);
void interpretShapeAnimDisplayList(HSD_PObj* pobj, f32 (*vertex)[3], f32 (*normal)[3]);
void get_shape_nbt_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32* dst);
void get_shape_normal_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32 dst[3]);
void get_shape_vertex_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32 dst[3]);
void HSD_PObjRemoveAll(HSD_PObj* pobj);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_Envelope* HSD_EnvelopeAlloc(void)
{
	HSD_Envelope* envelope = hsdAllocMemPiece(sizeof(HSD_Envelope));
	ASSERT_POBJ(0x1AE, envelope);
	memset(envelope, 0, sizeof(HSD_Envelope));
	return envelope;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_SList* loadEnvelopeDesc(HSD_EnvelopeDesc** edesc_p)
{
	HSD_SList* list    = NULL;
	HSD_SList** list_p = &list;

	if (edesc_p == NULL) {
		return NULL;
	}

	while (*edesc_p) {
		HSD_Envelope* envelope  = NULL;
		HSD_Envelope** env_p    = &envelope;
		HSD_EnvelopeDesc* edesc = *edesc_p;

		while (edesc->joint) {
			*env_p           = HSD_EnvelopeAlloc();
			(*env_p)->weight = edesc->weight;
			env_p            = &(*env_p)->next;
			edesc++;
		}

		(*list_p)       = HSD_SListAlloc();
		(*list_p)->data = envelope;
		list_p          = &(*list_p)->next;
		edesc_p++;
	}
	return list;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_ShapeSet* ShapeSetAlloc(void)
{
	HSD_ShapeSet* shape_set = hsdAllocMemPiece(sizeof(HSD_ShapeSet));

	ASSERT_POBJ(0x1E7, shape_set);
	memset(shape_set, 0, sizeof(HSD_ShapeSet));
	return shape_set;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_ShapeSet* loadShapeSetDesc(HSD_ShapeSetDesc* sdesc)
{
	s32 i;
	HSD_ShapeSet* shape_set    = ShapeSetAlloc();
	shape_set->flags           = sdesc->flags;
	shape_set->nb_shape        = sdesc->nb_shape;
	shape_set->nb_vertex_index = sdesc->nb_vertex_index;
	shape_set->vertex_desc     = sdesc->vertex_desc;
	shape_set->vertex_idx_list = sdesc->vertex_idx_list;
	shape_set->nb_normal_index = sdesc->nb_normal_index;
	shape_set->normal_desc     = sdesc->normal_desc;
	shape_set->normal_idx_list = sdesc->normal_idx_list;
	if (shape_set->flags & SHAPESET_ADDITIVE) {
		shape_set->blend.bp = HSD_Alloc(shape_set->nb_shape * sizeof(f32));
		for (i = 0; i < shape_set->nb_shape; i++) {
			shape_set->blend.bp[i] = 0.0f;
		}
	} else {
		shape_set->blend.bl = 0.0f;
	}
	return shape_set;
}

/**
 * @note Address: 0x801AE68C
 * @note Size: 0x2C4
 */
s32 PObjLoad(HSD_PObj* pobj, HSD_PObjDesc* desc)
{
	pobj->next      = HSD_PObjLoadDesc(desc->next);
	pobj->verts     = desc->verts;
	pobj->flags     = desc->flags;
	pobj->n_display = desc->n_display;
	pobj->display   = desc->display;

	switch (pobj_type(pobj)) {
	case POBJ_SHAPEANIM:
		pobj->u.shape_set = loadShapeSetDesc(desc->u.shape_set);
		break;

	case POBJ_ENVELOPE:
		pobj->u.envelope_list = loadEnvelopeDesc(desc->u.envelope_p);
		break;

	case POBJ_SKIN:
		break;

	default:
		HSD_Panic(__FILE__, 0x22F, "pobj: unexected type.\n");
	}

	_HSD_NeedCacheInvalidate(1);

	return 0;
}

/**
 * @note Address: 0x801AE5C8
 * @note Size: 0xC4
 */
HSD_PObj* HSD_PObjLoadDesc(HSD_PObjDesc* pobjdesc)
{
	if (pobjdesc != NULL) {
		HSD_PObj* pobj;
		HSD_ClassInfo* info;

		if (!pobjdesc->class_name || !(info = (HSD_ClassInfo*)hsdSearchClassInfo(pobjdesc->class_name))) {
			pobj = HSD_PObjAlloc();
		} else {
			pobj = hsdNew(info);
			ASSERT_POBJ_MSG(0x24C, pobj, "pobj");
		}
		HSD_POBJ_METHOD(pobj)->load(pobj, pobjdesc);
		return pobj;
	} else {
		return NULL;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x5C
 */
HSD_PObj* HSD_PObjAlloc(void)
{
	HSD_PObj* pobj = hsdNew((HSD_ClassInfo*)HSD_PObjGetCurrent());
	ASSERT_POBJ_MSG(0x2AE, pobj, "pobj");
	return pobj;
}

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
HSD_PObjInfo* HSD_PObjGetCurrent(void)
{
	return default_class ? default_class : &hsdPObj;
}

/**
 * @note Address: N/A
 * @note Size: 0x58
 */
void HSD_PObjSetCurrent(HSD_PObjInfo* info)
{
	if (info != NULL) {
		ASSERT_POBJ_MSG(0x28E, hsdIsDescendantOf(info, &hsdPObj), "hsdIsDescendantOf(info, &hsdPObj)");
	}
	default_class = info;
}

/**
 * @note Address: 0x801AE554
 * @note Size: 0x74
 */
void HSD_PObjRemoveAll(HSD_PObj* pobj)
{
	HSD_PObj* next;

	while (pobj != NULL) {
		next = pobj->next;
		if (pobj != NULL) {
			HSD_CLASS_METHOD(pobj)->release((HSD_Class*)pobj);
			HSD_CLASS_METHOD(pobj)->destroy((HSD_Class*)pobj);
		}
		pobj = next;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjRefThis(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		HSD_OBJ(jobj)->ref_count_individual++;
		ASSERT_REF_COUNT(0x9E, HSD_OBJ(jobj)->ref_count_individual != 0);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void resolveEnvelope(HSD_SList* list, HSD_EnvelopeDesc** edesc_p)
{
	if (list == NULL || edesc_p == NULL) {
		return;
	}

	for (; list != NULL && *edesc_p != NULL; list = list->next, edesc_p++) {
		HSD_Envelope* env       = list->data;
		HSD_EnvelopeDesc* edesc = *edesc_p;

		while (env != NULL && edesc->joint != NULL) {
			HSD_JObjUnrefThis(env->jobj);
			env->jobj = HSD_IDGetDataFromTable(NULL, (u32)edesc->joint, NULL);
			ASSERT_POBJ(0x2CF, env->jobj);
			HSD_JObjRefThis(env->jobj);
			env = env->next;
			edesc++;
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_PObjResolveRefs(HSD_PObj* pobj, HSD_PObjDesc* pdesc)
{
	if (pobj == NULL || pdesc == NULL) {
		return;
	}

	switch (pobj_type(pobj)) {
	case POBJ_ENVELOPE:
		resolveEnvelope(pobj->u.envelope_list, pdesc->u.envelope_p);
		break;

	case POBJ_SKIN:
		HSD_JObjUnrefThis(pobj->u.jobj);
		pobj->u.jobj = NULL;
		if (pdesc->u.joint != NULL) {
			pobj->u.jobj = HSD_IDGetDataFromTable(NULL, (u32)pdesc->u.joint, NULL);
			ASSERT_POBJ(0x2EA, pobj->u.jobj);
			HSD_JObjRefThis(pobj->u.jobj);
		}
		break;

	default:
		break;
	}
}

/**
 * @note Address: 0x801AE388
 * @note Size: 0x1CC
 */
void HSD_PObjResolveRefsAll(HSD_PObj* pobj, HSD_PObjDesc* pdesc)
{
	for (; pobj != NULL && pdesc != NULL; pobj = pobj->next, pdesc = pdesc->next) {
		HSD_PObjResolveRefs(pobj, pdesc);
	}
}

/**
 * @note Address: 0x801AE0FC
 * @note Size: 0x28C
 */
void HSD_PObjCountVertices(HSD_PObj* pobj, u32* vertices, u32* polygons)
{
	u32 nv = 0;
	u32 np = 0;

	for (; pobj != NULL; pobj = pobj->next) {
		HSD_VtxDescList* desc;
		int attr;
		s32 stride = 0;
		s32 width;

		for (desc = pobj->verts; (attr = desc->attr) != GX_VA_NULL; desc++) {
			switch (desc->attr_type) {
			case GX_INDEX8:
				stride++;
				break;
			case GX_INDEX16:
				stride += 2;
				break;
			case GX_DIRECT:
				if (attr == GX_VA_CLR0 || attr == GX_VA_CLR1) {
					switch (desc->comp_type) {
					case GX_RGB565:
					case GX_RGBA4:
						width = 2;
						break;
					case GX_RGB8:
					case GX_RGBA6:
						width = 3;
						break;
					case GX_RGBX8:
					case GX_RGBA8:
						width = 4;
						break;
					}
				} else {
					switch (desc->comp_type) {
					case GX_U8:
					case GX_S8:
						width = 1;
						break;
					case GX_U16:
					case GX_S16:
						width = 2;
						break;
					case GX_F32:
						width = 4;
						break;
					default:
						goto next_pobj;
					}
				}
			switch (desc->attr) {
				case GX_VA_PNMTXIDX:
					stride++;
					break;
				case GX_VA_POS:
					if (desc->comp_cnt == GX_POS_XY) {
						stride += 2 * width;
					} else {
						stride += 3 * width;
					}
					break;
				case GX_VA_NRM:
					if (desc->comp_cnt == GX_NRM_XYZ) {
						stride += 3 * width;
					}
					break;
				case GX_VA_CLR0:
				case GX_VA_CLR1:
					stride += width;
					break;
				case GX_VA_TEX0:
				case GX_VA_TEX1:
				case GX_VA_TEX2:
				case GX_VA_TEX3:
				case GX_VA_TEX4:
				case GX_VA_TEX5:
				case GX_VA_TEX6:
				case GX_VA_TEX7:
					if (desc->comp_cnt == GX_TEX_S) {
						stride += width;
					} else {
						stride += 2 * width;
					}
					break;
				default:
					goto next_pobj;
				}
				break;
			}
		}
		{
			s32 offset;
			s32 length = pobj->n_display << 5;
			u8* dl     = pobj->display;
			if (dl != NULL) {
				offset = 0;

				while (offset < length) {
					s32 primitive = dl[offset++] & GX_OPCODE_MASK;
					if (primitive != 0) {
						u32 count = (u16)dl[offset];
						count     = (count << 8) + dl[offset + 1];
						nv += count;
						offset += count * stride + 2;
						switch (primitive) {
						case GX_TRIANGLES:
							np += count / 3;
							break;
						case GX_TRIANGLESTRIP:
						case GX_TRIANGLEFAN:
							np += count - 2;
							break;
						case GX_QUADS:
							np += count / 2;
							break;
						}
					}
				}
			}
		}
	next_pobj:;
	}
	if (vertices != NULL) {
		*vertices = nv;
	}
	if (polygons != NULL) {
		*polygons = np;
	}
}

/**
 * @note Address: 0x801AE0D0
 * @note Size: 0x2C
 */
void HSD_ClearVtxDesc(void)
{
	GXClearVtxDesc();
	prev_vtxdesclist_array = 0;
	prev_vtxdesc           = 0;
}

/**
 * @note Address: 0x801ADD44
 * @note Size: 0x38C
 */
void get_shape_vertex_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32 dst[3])
{
	u8* index_array = shape_set->vertex_idx_list[shape_id];
	s32 i, idx;
	void* src_base;
	int type;

	if (shape_set->vertex_desc->attr_type == GX_INDEX16) {
		idx = index_array[arrayidx * 2];
		idx = (idx << 8) + index_array[arrayidx * 2 + 1];
	} else {
		idx = index_array[arrayidx];
	}

	ASSERT_POBJ_MSG(0x449, shape_set->vertex_desc->comp_cnt == GX_POS_XYZ, "shape_set->vertex_desc->comp_cnt == GX_POS_XYZ");
	src_base = ((u8*)shape_set->vertex_desc->vertex) + idx * shape_set->vertex_desc->stride;
	type     = shape_set->vertex_desc->comp_type;

	if (type == GX_F32) {
		memcpy(dst, src_base, sizeof(f32[3]));
	} else {
		s32 decimal_point = 1 << shape_set->vertex_desc->frac;
		switch (type) {
		case GX_U8:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((u8*)src_base)[i] / decimal_point;
			}
			break;

		case GX_S8:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((s8*)src_base)[i] / decimal_point;
			}
			break;

		case GX_U16:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((u16*)src_base)[i] / decimal_point;
			}
			break;

		case GX_S16:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((s16*)src_base)[i] / decimal_point;
			}
			break;

		default:
			HSD_Panic("pobj.c", 0x474, "unexpected vertex type.\n");
		}
	}
}

/**
 * @note Address: 0x801AD9B8
 * @note Size: 0x38C
 */
void get_shape_normal_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32 dst[3])
{
	u8* index_array = shape_set->normal_idx_list[shape_id];
	s32 i, idx;
	void* src_base;
	int type;

	if (shape_set->normal_desc->attr_type == GX_INDEX16) {
		idx = index_array[arrayidx * 2];
		idx = (idx << 8) + index_array[arrayidx * 2 + 1];
	} else {
		idx = index_array[arrayidx];
	}

	ASSERT_POBJ_MSG(0x488, shape_set->normal_desc->comp_cnt == GX_NRM_XYZ, "shape_set->normal_desc->comp_cnt == GX_NRM_XYZ");
	src_base = ((u8*)shape_set->normal_desc->vertex) + idx * shape_set->normal_desc->stride;
	type     = shape_set->normal_desc->comp_type;

	if (type == GX_F32) {
		memcpy(dst, src_base, sizeof(f32[3]));
	} else {
		s32 decimal_point = 1 << shape_set->normal_desc->frac;
		switch (type) {
		case GX_U8:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((u8*)src_base)[i] / decimal_point;
			}
			break;
		case GX_S8:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((s8*)src_base)[i] / decimal_point;
			}
			break;
		case GX_U16:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((u16*)src_base)[i] / decimal_point;
			}
			break;
		case GX_S16:
			for (i = 0; i < 3; i++) {
				dst[i] = (f32)((s16*)src_base)[i] / decimal_point;
			}
			break;
		default:
			HSD_Panic("pobj.c", 0x4B3, "unexpected normal type.");
		}
	}
}

/**
 * @note Address: 0x801AD5B8
 * @note Size: 0x400
 */
void get_shape_nbt_xyz(HSD_ShapeSet* shape_set, s32 shape_id, s32 arrayidx, f32* dst)
{
	u8* index_array = shape_set->normal_idx_list[shape_id];
	s32 idx;
	int i;
	void* src_base;
	int type;

	ASSERT_POBJ(0x4C0, shape_set->normal_desc->attr == GX_VA_NBT);

	if (shape_set->normal_desc->attr_type == GX_INDEX16) {
		idx = index_array[arrayidx * 2];
		idx = (idx << 8) + index_array[arrayidx * 2 + 1];
	} else {
		idx = index_array[arrayidx];
	}

	ASSERT_POBJ_MSG(0x4C9, shape_set->normal_desc->comp_cnt == GX_NRM_XYZ, "shape_set->normal_desc->comp_cnt == GX_NRM_XYZ");

	src_base = ((u8*)shape_set->normal_desc->vertex) + idx * shape_set->normal_desc->stride;

	type = shape_set->normal_desc->comp_type;

	if (type == GX_F32) {
		memcpy(dst, src_base, sizeof(f32[9]));
	} else {
		s32 decimal_point = 1 << shape_set->normal_desc->frac;
		switch (type) {
		case GX_U8:
			for (i = 0; i < 9; i++) {
				dst[i] = (f32)((u8*)src_base)[i] / decimal_point;
			}
			break;
		case GX_S8:
			for (i = 0; i < 9; i++) {
				dst[i] = (f32)((s8*)src_base)[i] / decimal_point;
			}
			break;
		case GX_U16:
			for (i = 0; i < 9; i++) {
				dst[i] = (f32)((u16*)src_base)[i] / decimal_point;
			}
			break;
		case GX_S16:
			for (i = 0; i < 9; i++) {
				dst[i] = (f32)((s16*)src_base)[i] / decimal_point;
			}
			break;
		default:
			HSD_Panic("pobj.c", 0x4E8, "unexpected normal type.");
		}
	}
}

/**
 * @note Address: 0x801AD29C
 * @note Size: 0x31C
 */
void interpretShapeAnimDisplayList(HSD_PObj* pobj, f32 (*vertex)[3], f32 (*normal)[3])
{
	s32 i, j;
	s32 n, m;
	s32 l;
	s32 length = pobj->n_display << 5;
	u8* dl     = pobj->display;

	for (l = 0; l + 3 < length;) {
		n = dl[1] << 8 | dl[2];
		m = 3;

		if ((dl[0] & GX_OPCODE_MASK) == GX_NOP) {
			break;
		}
		GXBegin(dl[0] & GX_OPCODE_MASK, dl[0] & GX_VAT_MASK, n);
		for (i = 0; i < n; i++) {
			for (j = 0;; j++) {
				HSD_VtxDescList* desc = &pobj->verts[j];
				if (desc->attr == GX_VA_NULL) {
					break;
				} else {
					u16 idx = dl[m++];
					switch (desc->attr) {
					case GX_VA_PNMTXIDX:
					case GX_VA_TEX0MTXIDX:
					case GX_VA_TEX1MTXIDX:
					case GX_VA_TEX2MTXIDX:
					case GX_VA_TEX3MTXIDX:
					case GX_VA_TEX4MTXIDX:
					case GX_VA_TEX5MTXIDX:
					case GX_VA_TEX6MTXIDX:
					case GX_VA_TEX7MTXIDX:
						GXMatrixIndex1u8(idx);
						break;

					case GX_VA_POS:
						if (desc->attr_type == GX_INDEX16) {
							idx = (idx << 8) | dl[m++];
						}
						GXPosition3f32(vertex[idx][0], vertex[idx][1], vertex[idx][2]);
						break;

					case GX_VA_NRM:
						if (desc->attr_type == GX_INDEX16) {
							idx = (idx << 8) | dl[m++];
						}
						GXNormal3f32(normal[idx][0], normal[idx][1], normal[idx][2]);
						break;

					case GX_VA_NBT:
						if (desc->attr_type == GX_INDEX16) {
							idx = (idx << 8) | dl[m++];
						}
						idx *= 3;
						GXNormal3f32(normal[idx + 0][0], normal[idx + 0][1], normal[idx + 0][2]);
						GXNormal3f32(normal[idx + 1][0], normal[idx + 1][1], normal[idx + 1][2]);
						GXNormal3f32(normal[idx + 2][0], normal[idx + 2][1], normal[idx + 2][2]);
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
							idx = (idx << 8) | dl[m++];
							GXTexCoord1x16(idx);
						} else {
							GXTexCoord1x8(idx);
						}
						break;

					case GX_VA_CLR0:
					case GX_VA_CLR1:
						if (desc->attr_type == GX_INDEX16) {
							idx = (idx << 8) | dl[m++];
							GXColor1x16(idx);
						} else if (desc->attr_type == GX_INDEX8) {
							GXColor1x8(idx);
						} else {
							switch (desc->comp_type) {
							case GX_RGB565:
							case GX_RGBA4:
								GXColor1u16((idx << 8) | dl[m++]);
								break;
							case GX_RGB8:
							case GX_RGBA6:
								GXColor3u8(idx, dl[m], dl[m + 1]);
								m += 2;
								break;
							case GX_RGBA8:
							case GX_RGBX8:
								GXColor4u8(idx, dl[m], dl[m + 1], dl[m + 2]);
								m += 3;
								break;
							}
						}
						break;
					default:
						if (desc->attr_type == GX_INDEX16) {
							idx = (idx << 8) | dl[m++];
						}
						OSReport("attr(%d) is not supported by sysdolphin\n", desc->attr);
						break;
					}
				}
			}
		}
		GXEnd();
		l += m;
		dl += m;
	}
}

/**
 * @note Address: 0x801ACB7C
 * @note Size: 0x720
 */
// NONMATCHING: register allocation only; retail keeps pobj, vertex and normal in r29-r31
void drawShapeAnim(HSD_PObj* pobj)
{
	// NONMATCHING: register allocation only; retail gives the loop counters the lowest saved registers
	HSD_ShapeSet* shape_set = pobj->u.shape_set;
	f32 blend;
	s32 shape_id, i;
	s32 blend_nbt;

	if (vertex_buffer_size == 0) {
		vertex_buffer_size = HSD_DEFAULT_MAX_SHAPE_VERTICES;
		vertex_buffer      = HSD_Alloc(vertex_buffer_size * sizeof(f32[3]));
	}
	ASSERT_POBJ(0x582, vertex_buffer_size >= shape_set->nb_vertex_index);
	if (shape_set->normal_desc && normal_buffer_size == 0) {
		normal_buffer_size = HSD_DEFAULT_MAX_SHAPE_NORMALS;
		normal_buffer      = HSD_Alloc(normal_buffer_size * sizeof(f32[3]));
	}

	if (shape_set->normal_desc) {
		if (shape_set->normal_desc->attr == GX_VA_NRM) {
			ASSERT_POBJ(0x58B, normal_buffer_size >= shape_set->nb_normal_index);
			blend_nbt = 0;
		} else {
			ASSERT_POBJ(0x58E, normal_buffer_size >= shape_set->nb_normal_index * 3);
			blend_nbt = 1;
		}
	}

	if (shape_set->flags & SHAPESET_AVERAGE) {
		s32 blend_int;

		blend     = shape_set->blend.bl;
		blend_int = (s32)blend;
		shape_id  = pobj_min((blend_int < 0 ? 0 : blend_int), shape_set->nb_shape - 1);
		blend     = pobj_min(MAX(0.0, blend - (f32)shape_id), 1.0f);
		for (i = 0; i < shape_set->nb_vertex_index; i++) {
			f32 s0[3], s1[3];

			get_shape_vertex_xyz(shape_set, shape_id, i, s0);
			get_shape_vertex_xyz(shape_set, pobj_min(shape_id + 1, shape_set->nb_shape - 1), i, s1);
			vertex_buffer[i][0] = blend * (s1[0] - s0[0]) + s0[0];
			vertex_buffer[i][1] = blend * (s1[1] - s0[1]) + s0[1];
			vertex_buffer[i][2] = blend * (s1[2] - s0[2]) + s0[2];
		}
		if (shape_set->nb_normal_index) {
			if (blend_nbt) {
				for (i = 0; i < shape_set->nb_normal_index; i++) {
					f32 s0[9], s1[9];
					int j, idx = i * 3;

					get_shape_nbt_xyz(shape_set, shape_id, i, s0);
					get_shape_nbt_xyz(shape_set, pobj_min(shape_id + 1, shape_set->nb_shape - 1), i, s1);
					for (j = 0; j < 9; j++) {
						normal_buffer[idx][j] = blend * (s1[j] - s0[j]) + s0[j];
					}
				}
			} else {
				for (i = 0; i < shape_set->nb_normal_index; i++) {
					f32 s0[3], s1[3];

					get_shape_normal_xyz(shape_set, shape_id, i, s0);
					get_shape_normal_xyz(shape_set, pobj_min(shape_id + 1, shape_set->nb_shape - 1), i, s1);
					normal_buffer[i][0] = blend * (s1[0] - s0[0]) + s0[0];
					normal_buffer[i][1] = blend * (s1[1] - s0[1]) + s0[1];
					normal_buffer[i][2] = blend * (s1[2] - s0[2]) + s0[2];
				}
			}
		}
	} else {
		s32 j;
		f32* blend_bp = shape_set->blend.bp;
		for (i = 0; i < shape_set->nb_vertex_index; i++) {
			get_shape_vertex_xyz(shape_set, 0, i, vertex_buffer[i]);
			for (j = 0; j < shape_set->nb_shape; j++) {
				f32 b = MAX(0.0, blend_bp[j]);
				f32 s[3];

				get_shape_vertex_xyz(shape_set, j + 1, i, s);
				vertex_buffer[i][0] += s[0] * b;
				vertex_buffer[i][1] += s[1] * b;
				vertex_buffer[i][2] += s[2] * b;
			}
		}
		if (shape_set->nb_normal_index) {
			if (blend_nbt) {
				for (i = 0; i < shape_set->nb_normal_index; i++) {
					s32 idx = i * 3;
					get_shape_nbt_xyz(shape_set, 0, i, normal_buffer[idx]);
					for (j = 0; j < shape_set->nb_shape; j++) {
						f32 b = MAX(0.0, blend_bp[j]);
						f32 s[9];

						get_shape_nbt_xyz(shape_set, j + 1, i, s);
						normal_buffer[idx][0] += b * s[0];
						normal_buffer[idx][1] += b * s[1];
						normal_buffer[idx][2] += b * s[2];
						normal_buffer[idx][3] += b * s[3];
						normal_buffer[idx][4] += b * s[4];
						normal_buffer[idx][5] += b * s[5];
						normal_buffer[idx][6] += b * s[6];
						normal_buffer[idx][7] += b * s[7];
						normal_buffer[idx][8] += b * s[8];
					}
				}
			} else {
				for (i = 0; i < shape_set->nb_normal_index; i++) {
					get_shape_normal_xyz(shape_set, 0, i, normal_buffer[i]);
					for (j = 0; j < shape_set->nb_shape; j++) {
						f32 b = MAX(0.0, blend_bp[j]);
						f32 s[3];

						get_shape_normal_xyz(shape_set, j + 1, i, s);
						normal_buffer[i][0] += s[0] * b;
						normal_buffer[i][1] += s[1] * b;
						normal_buffer[i][2] += s[2] * b;
					}
				}
			}
		}
	}
	interpretShapeAnimDisplayList(pobj, vertex_buffer, normal_buffer);
}

/**
 * @note Address: 0x801ACB64
 * @note Size: 0x18
 */
void HSD_PObjClearMtxMark(void* obj, u32 mark)
{
	s32 i;

	for (i = 0; i < 2; i++) {
		mtx_mark[i].obj  = obj;
		mtx_mark[i].mark = mark;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline PObjSetupFlag GetSetupFlags(HSD_JObj* jobj)
{
	PObjSetupFlag flags = SETUP_NONE;

	if (jobj->flags & LIGHTING) {
		flags |= SETUP_NORMAL;
	}
	if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_REFLECTION)) {
		flags |= SETUP_NORMAL | SETUP_REFLECTION;
	}
	if (_HSD_TObjGetCurrentByType(NULL, TEX_COORD_HILIGHT)) {
		flags |= SETUP_NORMAL | SETUP_HIGHLIGHT;
	}
	return flags;
}

/**
 * @note Address: 0x801AC8EC
 * @note Size: 0x278
 */
void SetupSharedVtxModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
	HSD_JObj* jobj;
	Mtx n0, n1, m;
	PObjSetupFlag flags = SETUP_NONE;

	jobj = HSD_JObjGetCurrent();
	{
		void* obj;
		u32 mark;

		obj  = mtx_mark[0].obj;
		mark = mtx_mark[0].mark;
		if (obj != jobj && mark != HSD_MTX_RIGID) {
			flags |= SETUP_JOINT0;
		}

		obj  = mtx_mark[1].obj;
		mark = mtx_mark[1].mark;
		if (obj != pobj->u.jobj && mark != HSD_MTX_RIGID) {
			flags |= SETUP_JOINT1;
		}
	}

	if (flags == SETUP_NONE) {
		return;
	}

	flags |= GetSetupFlags(jobj);

	// always true, the original ors the flag in instead of testing it
	if (flags | SETUP_JOINT0) {
		GXSetCurrentMtx(GX_PNMTX0);
		GXLoadPosMtxImm(pmtx, GX_PNMTX0);
		HSD_PerfCurrentStat.unkC++;

		if (flags & SETUP_NORMAL) {
			if (!PSMTXInvXpose(pmtx, n0)) {
				PSMTXCopy(pmtx, n0);
			}
			if (jobj->flags & LIGHTING) {
				GXLoadNrmMtxImm(n0, GX_PNMTX0);
				HSD_PerfCurrentStat.unkC++;
			}
			if (flags & SETUP_NORMAL_PROJECTION) {
				GXLoadTexMtxImm(n0, GX_TEXMTX0, GX_MTX3x4);
				HSD_PerfCurrentStat.unkC++;
			}
		}
	}
	if (flags | SETUP_JOINT1) {
		HSD_JObjSetupMatrix(pobj->u.jobj);
		PSMTXConcat(vmtx, pobj->u.jobj->mtx, m);
		GXLoadPosMtxImm(m, GX_PNMTX1);
		HSD_PerfCurrentStat.unkC++;

		if (flags & SETUP_NORMAL) {
			if (!PSMTXInvXpose(m, n1)) {
				PSMTXCopy(m, n1);
			}
			if (jobj->flags & LIGHTING) {
				GXLoadNrmMtxImm(n1, GX_PNMTX1);
				HSD_PerfCurrentStat.unkC++;
			}
			if (flags & SETUP_NORMAL_PROJECTION) {
				GXLoadTexMtxImm(n1, GX_TEXMTX1, GX_MTX3x4);
				HSD_PerfCurrentStat.unkC++;
			}
		}
	}
}

/**
 * @note Address: 0x801AC574
 * @note Size: 0x378
 */
void SetupEnvelopeModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
	s32 MtxIdx;
	HSD_JObj* jobj;
	HSD_SList* list;
	MtxPtr right;
	Mtx mtx;
	PObjSetupFlag flags;

	jobj = HSD_JObjGetCurrent();
	HSD_PObjClearMtxMark(NULL, HSD_MTX_ENVELOPE);
	flags = GetSetupFlags(jobj);
	right = _HSD_mkEnvelopeModelNodeMtx(jobj, mtx);

	for (MtxIdx = 0, list = pobj->u.envelope_list; MtxIdx < 10 && list != NULL; MtxIdx++, list = list->next) {
		Mtx tmp2, tmp;
		MtxPtr mtxp;
		HSD_Envelope* envelope = list->data;
		s32 mtx_no             = HSD_Index2PosNrmMtx(MtxIdx);
		s32 perf               = 0;

		ASSERT_POBJ(0x738, envelope);
		if (envelope->weight >= 1.0f) {
			HSD_JObjSetupMatrix(envelope->jobj);
			if (right) {
				PSMTXConcat(envelope->jobj->mtx, envelope->jobj->envelopemtx, tmp2);
				mtxp = tmp2;
			} else {
				mtxp = envelope->jobj->mtx;
			}
		} else {
			tmp2[0][0] = tmp2[0][1] = tmp2[0][2] = tmp2[0][3] = tmp2[1][0] = tmp2[1][1] = tmp2[1][2] = tmp2[1][3] = tmp2[2][0] = tmp2[2][1]
			    = tmp2[2][2] = tmp2[2][3] = 0.0f;
			while (envelope != NULL) {
				HSD_JObj* jp;

				ASSERT_POBJ(0x74C, envelope->jobj);
				jp = envelope->jobj;
				HSD_JObjSetupMatrix(jp);
				ASSERT_POBJ(0x74F, jp->mtx);
				ASSERT_POBJ(0x750, jp->envelopemtx);

				PSMTXConcat(jp->mtx, jp->envelopemtx, tmp);
				HSD_MtxScaledAdd(tmp, tmp2, tmp2, envelope->weight);
				perf++;
				envelope = envelope->next;
			}
			mtxp = tmp2;
		}
		HSD_PerfCountEnvelopeBlending(perf);
		if (right) {
			PSMTXConcat(mtxp, right, tmp2);
		}
		PSMTXConcat(vmtx, mtxp, tmp);
		GXLoadPosMtxImm(tmp, mtx_no);
		HSD_PerfCurrentStat.unkC++;

		if (flags & SETUP_NORMAL) {
			if (!PSMTXInvXpose(tmp, tmp2)) {
				PSMTXCopy(tmp, tmp2);
			}
			if (jobj->flags & LIGHTING) {
				GXLoadNrmMtxImm(tmp2, mtx_no);
				HSD_PerfCurrentStat.unkC++;
			}
			if (flags & SETUP_NORMAL_PROJECTION) {
				GXLoadTexMtxImm(tmp2, HSD_Index2TexMtx(MtxIdx), GX_MTX3x4);
				HSD_PerfCurrentStat.unkC++;
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void SetupRigidModelMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
	HSD_JObj* jobj = HSD_JObjGetCurrent();
	void* mtx_obj  = mtx_mark[0].obj;
	u32 mark       = mtx_mark[0].mark;

	if (mtx_obj != jobj || mark != HSD_MTX_RIGID) {
		PObjSetupFlag flags;
		Mtx n;

		GXSetCurrentMtx(GX_PNMTX0);
		GXLoadPosMtxImm(pmtx, GX_PNMTX0);
		HSD_PerfCurrentStat.unkC++;

		flags = GetSetupFlags(jobj);
		if (flags & SETUP_NORMAL) {
			if (!PSMTXInvXpose(pmtx, n)) {
				PSMTXCopy(pmtx, n);
			}
			if (jobj->flags & LIGHTING) {
				GXLoadNrmMtxImm(n, GX_PNMTX0);
				HSD_PerfCurrentStat.unkC++;
			}
			if (flags & SETUP_NORMAL_PROJECTION) {
				GXLoadTexMtxImm(n, GX_TEXMTX0, GX_MTX3x4);
				HSD_PerfCurrentStat.unkC++;
			}
		}
	}
}

/**
 * @note Address: 0x801AC2E0
 * @note Size: 0x294
 */
void PObjSetupMtx(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
	switch (pobj_type(pobj)) {
	case POBJ_SKIN:
		if (!pobj->u.jobj) {
			SetupRigidModelMtx(pobj, vmtx, pmtx, rendermode);
		} else {
			SetupSharedVtxModelMtx(pobj, vmtx, pmtx, rendermode);
		}
		break;
	case POBJ_SHAPEANIM:
		SetupRigidModelMtx(pobj, vmtx, pmtx, rendermode);
		break;
	case POBJ_ENVELOPE:
		SetupEnvelopeModelMtx(pobj, vmtx, pmtx, rendermode);
		break;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupArrayDesc(HSD_VtxDescList* desc_list)
{
	HSD_VtxDescList* desc;

	if (prev_vtxdesclist_array != desc_list) {
		for (desc = desc_list; desc->attr != GX_VA_NULL; desc++) {
			if (desc->attr_type != GX_DIRECT) {
				GXSetArray(desc->attr, desc->vertex, desc->stride);
			}
		}
		prev_vtxdesclist_array = desc_list;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupVtxDesc(HSD_PObj* pobj)
{
	HSD_VtxDescList* desc;

	if (prev_vtxdesc != pobj->verts) {
		GXClearVtxDesc();
		for (desc = pobj->verts; desc->attr != GX_VA_NULL; desc++) {
			GXSetVtxDesc(desc->attr, desc->attr_type);
			switch (desc->attr) {
			case GX_VA_PNMTXIDX:
			case GX_VA_TEX0MTXIDX:
			case GX_VA_TEX1MTXIDX:
			case GX_VA_TEX2MTXIDX:
			case GX_VA_TEX3MTXIDX:
			case GX_VA_TEX4MTXIDX:
			case GX_VA_TEX5MTXIDX:
			case GX_VA_TEX6MTXIDX:
			case GX_VA_TEX7MTXIDX:
				break;
			default:
				GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt, desc->comp_type, desc->frac);
			}
		}
		prev_vtxdesc = pobj->verts;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void PObjDispSimplePrimitive(HSD_PObj* pobj, u32 rendermode)
{
	setupArrayDesc(pobj->verts);
	setupVtxDesc(pobj);

	GXCallDisplayList(pobj->display, pobj->n_display << 5);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupShapeAnimArrayDesc(HSD_VtxDescList* desc_list)
{
	HSD_VtxDescList* desc;

	for (desc = desc_list; desc->attr != GX_VA_NULL; desc++) {
		if (desc->attr_type != GX_DIRECT) {
			switch (desc->attr) {
			case GX_VA_POS:
			case GX_VA_NRM:
			case GX_VA_NBT:
				break;
			default:
				GXSetArray(desc->attr, desc->vertex, desc->stride);
			}
		}
	}
	prev_vtxdesclist_array = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupShapeAnimVtxDesc(HSD_PObj* pobj)
{
	HSD_VtxDescList* desc;

	GXClearVtxDesc();
	for (desc = pobj->verts; desc->attr != GX_VA_NULL; desc++) {
		switch (desc->attr) {
		case GX_VA_NRM:
		case GX_VA_POS:
		case GX_VA_NBT:
			GXSetVtxDesc(desc->attr, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt, GX_F32, 0);
			break;

		case GX_VA_PNMTXIDX:
		case GX_VA_TEX0MTXIDX:
		case GX_VA_TEX1MTXIDX:
		case GX_VA_TEX2MTXIDX:
		case GX_VA_TEX3MTXIDX:
		case GX_VA_TEX4MTXIDX:
		case GX_VA_TEX5MTXIDX:
		case GX_VA_TEX6MTXIDX:
		case GX_VA_TEX7MTXIDX:
			GXSetVtxDesc(desc->attr, desc->attr_type);
			break;

		default:
			GXSetVtxDesc(desc->attr, desc->attr_type);
			GXSetVtxAttrFmt(GX_VTXFMT0, desc->attr, desc->comp_cnt, desc->comp_type, desc->frac);
		}
	}
	prev_vtxdesc = 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void PObjDispShapeAnim(HSD_PObj* pobj, u32 rendermode)
{
	setupShapeAnimArrayDesc(pobj->verts);
	setupShapeAnimVtxDesc(pobj);

	ASSERT_POBJ_MSG(0x79D, pobj->u.shape_set, "pobj->u.shape_set");
	drawShapeAnim(pobj);
}

/**
 * @note Address: 0x801ABFF8
 * @note Size: 0x2E8
 */
void HSD_PObjDisp(HSD_PObj* pobj, Mtx vmtx, Mtx pmtx, u32 rendermode)
{
	if (pobj->flags & POBJ_HIDE) {
		return;
	}

	switch (pobj->flags & (POBJ_CULLFRONT | POBJ_CULLBACK)) {
	case 0x0:
		HSD_StateSetCullMode(GX_CULL_NONE);
		break;
	case POBJ_CULLFRONT:
		HSD_StateSetCullMode(GX_CULL_FRONT);
		break;
	case POBJ_CULLBACK:
		HSD_StateSetCullMode(GX_CULL_BACK);
		break;
	case POBJ_CULLFRONT | POBJ_CULLBACK:
		return;
	}

	HSD_POBJ_METHOD(pobj)->setup_mtx(pobj, vmtx, pmtx, rendermode);
	if (pobj_type(pobj) == POBJ_SHAPEANIM) {
		PObjDispShapeAnim(pobj, rendermode);
	} else {
		PObjDispSimplePrimitive(pobj, rendermode);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_EnvelopeFree(HSD_Envelope* env)
{
	if (env != NULL) {
		hsdFreeMemPiece(env, sizeof(HSD_Envelope));
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_EnvelopeListFree(HSD_SList* list)
{
	while (list) {
		HSD_Envelope* env = list->data;
		while (env) {
			HSD_Envelope* next = env->next;
			HSD_JObjUnrefThis(env->jobj);
			HSD_EnvelopeFree(env);
			env = next;
		}
		list = HSD_SListRemove(list);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_ShapeSetFree(HSD_ShapeSet* shape_set)
{
	if (!shape_set) {
		return;
	}
	hsdFreeMemPiece(shape_set, sizeof(HSD_ShapeSet));
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_ShapeSetRemove(HSD_ShapeSet* shape_set)
{
	if (shape_set == NULL) {
		return;
	}

	if (shape_set->flags & SHAPESET_ADDITIVE) {
		HSD_Free(shape_set->blend.bp);
	}

	HSD_ShapeSetFree(shape_set);
}

/**
 * @note Address: 0x801ABECC
 * @note Size: 0x12C
 */
void PObjRelease(HSD_Class* o)
{
	HSD_PObj* pobj = HSD_POBJ(o);

	if (pobj->aobj != NULL) {
		HSD_AObjRemove(pobj->aobj);
	}

	switch (pobj_type(pobj)) {
	case POBJ_SHAPEANIM:
		HSD_ShapeSetRemove(pobj->u.shape_set);
		break;
	case POBJ_ENVELOPE:
		HSD_EnvelopeListFree(pobj->u.envelope_list);
		break;
	case POBJ_SKIN:
		HSD_JObjUnrefThis(pobj->u.jobj);
		break;
	default:
		break;
	}
	HSD_PARENT_INFO(&hsdPObj)->release(o);
}

/**
 * @note Address: 0x801ABE20
 * @note Size: 0xAC
 */
void PObjAmnesia(HSD_ClassInfo* info)
{
	if (info == (HSD_ClassInfo*)default_class) {
		default_class = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdPObj)) {
		if (_HSD_MemCheckOwn(vertex_buffer)) {
			vertex_buffer      = NULL;
			vertex_buffer_size = 0;
		}
		if (_HSD_MemCheckOwn(normal_buffer)) {
			normal_buffer      = NULL;
			normal_buffer_size = 0;
		}
		prev_vtxdesclist_array = NULL;
		prev_vtxdesc           = NULL;
	}
	HSD_PARENT_INFO(&hsdPObj)->amnesia(info);
}

/**
 * @note Address: 0x801ABD88
 * @note Size: 0x98
 */
void PObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdPObj), HSD_CLASS_INFO(&hsdClass), "sysdolphin_base_library", "hsd_pobj", sizeof(HSD_PObjInfo),
	                 sizeof(HSD_PObj));
	HSD_CLASS_INFO(&hsdPObj)->release  = PObjRelease;
	HSD_CLASS_INFO(&hsdPObj)->amnesia  = PObjAmnesia;
	HSD_POBJ_INFO(&hsdPObj)->disp      = HSD_PObjDisp;
	HSD_POBJ_INFO(&hsdPObj)->setup_mtx = PObjSetupMtx;
	HSD_POBJ_INFO(&hsdPObj)->load      = PObjLoad;
	HSD_POBJ_INFO(&hsdPObj)->update    = PObjUpdateFunc;
}
