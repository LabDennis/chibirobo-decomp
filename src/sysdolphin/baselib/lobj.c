#include <global.h>
#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <float.h>
#include <math.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/constants.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/wobj.h>
#include <sysdolphin/aobj.h>
#include <sysdolphin/list.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/util.h>

#define MIN_LIGHT_ATTENUATION_REFERENCE 1.1920929e-07F
#define INFINITE_LIGHT_DISTANCE 1048576.0F

enum {
	GX_LIGHT_COUNT = 8,
	AMBIENT_LIGHT_INDEX = GX_LIGHT_COUNT,
	ACTIVE_LIGHT_SLOTS = GX_LIGHT_COUNT + 1,
};

static void LObjInfoInit(void);
HSD_LObjInfo hsdLObj = { LObjInfoInit };

static HSD_LObjInfo* default_class = NULL;

static HSD_SList* current_lights = NULL;
static HSD_LObj* active_lights[ACTIVE_LIGHT_SLOTS];
static s32 nb_active_lights = 0;

static s32 lightmask_diffuse  = 0;
static s32 lightmask_specular = 0;
static s32 lightmask_attnfunc = 0;
static s32 lightmask_alpha    = 0;

void HSD_LObjDeleteCurrent(HSD_LObj*);

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
u32 HSD_LObjGetFlags(HSD_LObj* lobj)
{
	if (lobj) {
		return lobj->flags;
	}
	return 0;
}

/**
 * @note Address: 0x801BD040
 * @note Size: 0x18
 */
void HSD_LObjSetFlags(HSD_LObj* lobj, u32 flags)
{
	if (lobj == NULL) {
		return;
	}
	lobj->flags |= flags;
}

/**
 * @note Address: N/A
 * @note Size: 0x18
 */
void HSD_LObjClearFlags(HSD_LObj* lobj, u32 flags)
{
	if (lobj == NULL) {
		return;
	}
	lobj->flags &= ~flags;
}

/**
 * @note Address: 0x801BD058
 * @note Size: 0x8
 */
s32 HSD_LObjGetLightMaskDiffuse(void)
{
	return lightmask_diffuse;
}

/**
 * @note Address: 0x801BD060
 * @note Size: 0x8
 */
s32 HSD_LObjGetLightMaskAttnFunc(void)
{
	return lightmask_attnfunc;
}

/**
 * @note Address: 0x801BD068
 * @note Size: 0x8
 */
s32 HSD_LObjGetLightMaskAlpha(void)
{
	return lightmask_alpha;
}

/**
 * @note Address: 0x801BD070
 * @note Size: 0x8
 */
s32 HSD_LObjGetLightMaskSpecular(void)
{
	return lightmask_specular;
}

/**
 * @note Address: 0x801BD078
 * @note Size: 0x8
 */
s32 HSD_LObjGetNbActive(void)
{
	return nb_active_lights;
}

/**
 * @note Address: 0x801BD080
 * @note Size: 0x48
 */
HSD_LObj* HSD_LObjGetActiveByID(u32 id)
{
	s32 idx = HSD_LightID2Index(id);
	if (idx >= 0 && idx < ACTIVE_LIGHT_SLOTS) {
		return active_lights[idx];
	}
	return NULL;
}

/**
 * @note Address: 0x801BD0C8
 * @note Size: 0x2C
 */
HSD_LObj* HSD_LObjGetActiveByIndex(s32 idx)
{
	if (idx >= 0 && idx < GX_LIGHT_COUNT) {
		return active_lights[idx];
	}
	return NULL;
}

/**
 * @note Address: 0x801BD0F4
 * @note Size: 0x24C
 */
static void LObjUpdateFunc(void* obj, enum_t type, FObjData* val)
{
	HSD_LObj* lobj = obj;

	if (lobj == NULL) {
		return;
	}
	switch (type) {
	case HSD_A_L_VIS:
		if (val->fv >= 0.5) {
			lobj->flags &= ~LOBJ_HIDDEN;
		} else {
			lobj->flags |= LOBJ_HIDDEN;
		}
		break;
	case HSD_A_L_A0:
	case HSD_A_L_CUTOFF:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.a0 = val->fv;
		} else {
			lobj->u.params.cutoff = val->fv;
		}
		break;
	case HSD_A_L_A1:
	case HSD_A_L_REFDIST:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.a1 = val->fv;
		} else {
			lobj->u.params.ref_dist = val->fv;
		}
		break;
	case HSD_A_L_A2:
	case HSD_A_L_REFBRIGHT:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.a2 = val->fv;
		} else {
			lobj->u.params.ref_br = val->fv;
		}
		break;
	case HSD_A_L_K0:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.k0 = val->fv;
		}
		break;
	case HSD_A_L_K1:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.k1 = val->fv;
		}
		break;
	case HSD_A_L_K2:
		if (lobj->flags & LOBJ_RAW_PARAM) {
			lobj->u.attn.k2 = val->fv;
		}
		break;
	case HSD_A_L_LITC_R:
		lobj->color.r = 255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	case HSD_A_L_LITC_G:
		lobj->color.g = 255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	case HSD_A_L_LITC_B:
		lobj->color.b = 255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	case HSD_A_L_LITC_A:
		lobj->color.a = 255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	}
}

/**
 * @note Address: 0x801BD340
 * @note Size: 0x58
 */
void HSD_LObjAnim(HSD_LObj* lobj)
{
	if (lobj != NULL) {
		HSD_AObjInterpretAnim(lobj->aobj, lobj, HSD_LOBJ_METHOD(lobj)->update);
		HSD_WObjInterpretAnim(HSD_LObjGetPositionWObj(lobj));
		HSD_WObjInterpretAnim(HSD_LObjGetInterestWObj(lobj));
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x74
 */
void HSD_LObjAnimAll(HSD_LObj* lobj)
{
	HSD_LObj* lp;

	if (lobj == NULL) {
		return;
	}

	for (lp = lobj; lp; lp = lp->next) {
		HSD_LObjAnim(lp);
	}
}

/**
 * @note Address: 0x801BD398
 * @note Size: 0x60
 */
void HSD_LObjReqAnim(HSD_LObj* lobj, f32 startframe)
{
	if (lobj == NULL) {
		return;
	}

	HSD_AObjReqAnim(lobj->aobj, startframe);
	HSD_WObjReqAnim(HSD_LObjGetPositionWObj(lobj), startframe);
	HSD_WObjReqAnim(HSD_LObjGetInterestWObj(lobj), startframe);
}

/**
 * @note Address: N/A
 * @note Size: 0x80
 */
void HSD_LObjReqAnimAll(HSD_LObj* lobj, f32 startframe)
{
	HSD_LObj* lp;

	if (lobj == NULL) {
		return;
	}

	for (lp = lobj; lp; lp = lp->next) {
		HSD_LObjReqAnim(lp, startframe);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 NormalizeLightDirection(Vec* src, Vec* dst)
{
	if (!src || !dst) {
		return -1;
	}
	if (fabs(src->x) <= FLT_MIN && fabs(src->y) <= FLT_MIN && fabs(src->z) <= FLT_MIN) {
		return -1;
	}
	PSVECNormalize(src, dst);
	return 0;
}

/**
 * @note Address: 0x801BD3F8
 * @note Size: 0x124
 */
void HSD_LObjGetLightVector(HSD_LObj* lobj, Vec* v)
{
	Vec position = { 0.0F, 0.0F, 0.0F };
	Vec interest = { 0.0F, 0.0F, 0.0F };

	if (lobj == NULL) {
		return;
	}
	HSD_LObjGetPosition(lobj, &position);
	HSD_LObjGetInterest(lobj, &interest);
	PSVECSubtract(&interest, &position, v);
	if (NormalizeLightDirection(v, v) != 0) {
		v->x = 0.0F;
		v->y = 0.0F;
		v->z = 1.0F;
	}
}

/**
 * @note Address: 0x801BD51C
 * @note Size: 0x164
 */
void HSD_LObjSetup(HSD_LObj* lobj, GXColor* color, f32 shininess)
{
	if (lobj->flags & LOBJ_HIDDEN || HSD_LObjGetType(lobj) == LOBJ_AMBIENT) {
		return;
	}
	if (lobj->flags & (LOBJ_DIFFUSE | LOBJ_ALPHA)) {
		if (lobj->hw_color.r != color->r || lobj->hw_color.g != color->g || lobj->hw_color.b != color->b || lobj->hw_color.a != color->a) {
			GXInitLightColor(&lobj->lightobj, *color);
			lobj->hw_color = *color;
			lobj->flags |= LOBJ_DIFF_DIRTY;
		}
		if (lobj->flags & LOBJ_DIFF_DIRTY) {
			GXLoadLightObjImm(&lobj->lightobj, lobj->id);
			lobj->flags &= ~LOBJ_DIFF_DIRTY;
		}
	}
	if (lobj->spec_id != GX_LIGHT_NULL) {
		if (lobj->shininess != shininess) {
			f32 k0 = shininess;

			k0 *= 0.5F;
			lobj->shininess = shininess;
			GXInitLightAttn(&lobj->spec_lightobj, 0.0F, 0.0F, 1.0F, k0, 0.0F, 1.0F - k0);
			lobj->flags |= LOBJ_SPEC_DIRTY;
		}
		if (lobj->flags & LOBJ_SPEC_DIRTY) {
			GXLoadLightObjImm(&lobj->spec_lightobj, lobj->spec_id);
			lobj->flags &= ~LOBJ_SPEC_DIRTY;
		}
	}
}

/**
 * @note Address: 0x801BD680
 * @note Size: 0x278
 */
void HSD_LObjSetupSpecularInit(Mtx pmtx)
{
	s32 i;
	s32 num;
	Vec cdir;
	Vec jpos;

	jpos.x = pmtx[0][3];
	jpos.y = pmtx[1][3];
	jpos.z = pmtx[2][3];

	if (NormalizeLightDirection(&jpos, &cdir) != 0) {
		cdir.x = 0.0F;
		cdir.y = 0.0F;
		cdir.z = -1.0F;
	}

	num = HSD_LObjGetNbActive();
	for (i = 0; i < num; i++) {
		Vec half, ldir;
		HSD_LObj* lobj = HSD_LObjGetActiveByIndex(i);

		if (lobj->spec_id == GX_LIGHT_NULL) {
			continue;
		}

		switch (HSD_LObjGetType(lobj)) {
		case LOBJ_POINT:
		case LOBJ_SPOT:
			PSVECSubtract(&jpos, &lobj->lvec, &ldir);
			if (NormalizeLightDirection(&ldir, &ldir) == 0) {
				PSVECAdd(&ldir, &cdir, &half);
				break;
			}
			half.x = 0.0F;
			half.y = 0.0F;
			half.z = 1.0F;
			goto init_light_dir;

		case LOBJ_INFINITE:
			PSVECAdd(&lobj->lvec, &cdir, &half);
			break;

		default:
			assert_line(634, 0);
		}

		if (NormalizeLightDirection(&half, &half) != 0) {
			half.x = 0.0F;
			half.y = 0.0F;
			half.z = 1.0F;
		}

	init_light_dir:
		GXInitLightDir(&lobj->spec_lightobj, half.x, half.y, half.z);
		lobj->flags |= LOBJ_SPEC_DIRTY;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void LObjSetActive(HSD_LObj* lobj)
{
	s32 idx;

	if (HSD_LObjGetType(lobj) == LOBJ_AMBIENT) {
		idx = AMBIENT_LIGHT_INDEX;
		if (active_lights[idx]) {
			return;
		}
	} else {
		idx = nb_active_lights++;
	}
	active_lights[idx] = lobj;
	lobj->id           = HSD_Index2LightID(idx);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setup_infinite_lightobj(HSD_LObj* lobj, MtxPtr vmtx)
{
	Vec lpos;

	HSD_LObjGetPosition(lobj, &lpos);
	lpos.x *= INFINITE_LIGHT_DISTANCE;
	lpos.y *= INFINITE_LIGHT_DISTANCE;
	lpos.z *= INFINITE_LIGHT_DISTANCE;
	PSMTXMultVec(vmtx, &lpos, &lpos);
	if (lobj->flags & LOBJ_DIFFUSE) {
		GXInitLightPos(&lobj->lightobj, lpos.x, lpos.y, lpos.z);
		GXInitLightAttn(&lobj->lightobj, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F);
	}
	if (lobj->flags & LOBJ_SPECULAR) {
		GXInitLightPos(&lobj->spec_lightobj, lpos.x, lpos.y, lpos.z);
	}
}

static inline void clampAttenuationReference(f32* value)
{
	if (*value < MIN_LIGHT_ATTENUATION_REFERENCE) {
		*value = MIN_LIGHT_ATTENUATION_REFERENCE;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setup_point_lightobj(HSD_LObj* lobj, Mtx mtx)
{
	Vec lpos;

	GXInitLightColor(&lobj->lightobj, lobj->color);
	lobj->hw_color = lobj->color;
	HSD_LObjGetPosition(lobj, &lpos);
	PSMTXMultVec(mtx, &lpos, &lpos);
	GXInitLightPos(&lobj->lightobj, lpos.x, lpos.y, lpos.z);
	GXInitLightPos(&lobj->spec_lightobj, lpos.x, lpos.y, lpos.z);
	if (lobj->flags & LOBJ_RAW_PARAM) {
		GXInitLightAttn(&lobj->lightobj, 1.0F, 0.0F, 0.0F, lobj->u.attn.k0, lobj->u.attn.k1, lobj->u.attn.k2);
	} else {
		f32 ref_br = lobj->u.params.ref_br;
		f32 ref_dist = lobj->u.params.ref_dist;
		GXDistAttnFn dist_func = lobj->u.params.dist_func;

		clampAttenuationReference(&ref_dist);
		clampAttenuationReference(&ref_br);
		GXInitLightDistAttn(&lobj->lightobj, ref_dist, ref_br, dist_func);
		GXInitLightSpot(&lobj->lightobj, 0.0F, GX_SP_OFF);
		GXInitLightDistAttn(&lobj->spec_lightobj, ref_dist, ref_br, dist_func);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setup_spot_lightobj(HSD_LObj* lobj, Mtx mtx)
{
	Vec lpos;
	Vec ldir;

	HSD_LObjGetPosition(lobj, &lpos);
	PSMTXMultVec(mtx, &lpos, &lpos);
	HSD_LObjGetLightVector(lobj, &ldir);
	PSMTXMultVecSR(mtx, &ldir, &ldir);
	PSVECNormalize(&ldir, &ldir);
	GXInitLightPos(&lobj->lightobj, lpos.x, lpos.y, lpos.z);
	GXInitLightPos(&lobj->spec_lightobj, lpos.x, lpos.y, lpos.z);
	GXInitLightDir(&lobj->lightobj, ldir.x, ldir.y, ldir.z);
	if (lobj->flags & LOBJ_RAW_PARAM) {
		GXInitLightAttn(&lobj->lightobj, lobj->u.attn.a0, lobj->u.attn.a1, lobj->u.attn.a2, lobj->u.attn.k0, lobj->u.attn.k1,
		                lobj->u.attn.k2);
	} else {
		f32 ref_br = lobj->u.params.ref_br;
		f32 ref_dist = lobj->u.params.ref_dist;
		f32 cutoff = lobj->u.params.cutoff;
		GXSpotFn spot_func = lobj->u.params.spot_func;
		GXDistAttnFn dist_func = lobj->u.params.dist_func;

		clampAttenuationReference(&ref_dist);
		clampAttenuationReference(&ref_br);
		GXInitLightDistAttn(&lobj->lightobj, ref_dist, ref_br, dist_func);
		GXInitLightDistAttn(&lobj->spec_lightobj, ref_dist, ref_br, dist_func);
		GXInitLightSpot(&lobj->lightobj, cutoff, spot_func);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setup_diffuse_lightobj(HSD_LObj* lobj)
{
	GXInitLightColor(&lobj->lightobj, lobj->color);
	lobj->hw_color = lobj->color;
	lobj->flags |= LOBJ_DIFF_DIRTY;

	switch (HSD_LObjGetType(lobj)) {
	case LOBJ_SPOT:
	case LOBJ_POINT:
		lightmask_attnfunc |= lobj->id;
		break;
	case LOBJ_INFINITE:
		break;
	default:
		assert_line(664, 0);
	}

	if (lobj->flags & LOBJ_DIFFUSE) {
		lightmask_diffuse |= lobj->id;
	}

	if (lobj->flags & LOBJ_ALPHA) {
		lightmask_alpha |= lobj->id;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static void setup_spec_lightobj(HSD_LObj* lobj, Mtx mtx, s32 spec_id)
{
	f32 x;

	lobj->spec_id = spec_id;
	if (spec_id != GX_LIGHT_NULL) {
		GXInitLightColor(&lobj->spec_lightobj, lobj->color);
		lobj->shininess = 50.0F;

		x = lobj->shininess;
		x *= 0.5F;
		GXInitLightAttn(&lobj->spec_lightobj, 0.0F, 0.0F, 1.0F, x, 0.0F, 1.0F - x);
		switch (HSD_LObjGetType(lobj)) {
		case LOBJ_POINT:
		case LOBJ_SPOT:
			HSD_LObjGetPosition(lobj, &lobj->lvec);
			PSMTXMultVec(mtx, &lobj->lvec, &lobj->lvec);
			break;
		case LOBJ_INFINITE:
			HSD_LObjGetLightVector(lobj, &lobj->lvec);
			PSMTXMultVecSR(mtx, &lobj->lvec, &lobj->lvec);
			PSVECNormalize(&lobj->lvec, &lobj->lvec);
			break;
		default:
			assert_line(704, 0);
		}
		lobj->flags |= LOBJ_SPEC_DIRTY;
		lightmask_specular |= spec_id;
	}
}

static inline void LObjClearActive(void)
{
	int i;

	for (i = 0; i < ACTIVE_LIGHT_SLOTS; i++) {
		active_lights[i] = NULL;
	}
}

/**
 * @note Address: 0x801BD8F8
 * @note Size: 0xBBC
 */
void HSD_LObjSetupInit(HSD_CObj* cobj)
{
	MtxPtr view_mtx;
	s32 light_index, active_count, next_light_index;
	HSD_SList* list;

	lightmask_diffuse = GX_LIGHT_NULL;
	list = current_lights;
	view_mtx = cobj->view_mtx;

	LObjClearActive();
	lightmask_specular = GX_LIGHT_NULL;
	lightmask_attnfunc = GX_LIGHT_NULL;
	lightmask_alpha = GX_LIGHT_NULL;
	nb_active_lights = 0;
	next_light_index = 0;

	for (; next_light_index < GX_LIGHT_COUNT && list; list = list->next) {
		HSD_LObj* lobj = list->data;
		u32 flags;
		u32 type;
		s32 ref_type;

		if (lobj == NULL) {
			continue;
		}

		flags = lobj->flags;

		if (flags & LOBJ_HIDDEN) {
			continue;
		}

		type     = HSD_LObjGetType(lobj);
		ref_type = flags & (LOBJ_DIFFUSE | LOBJ_SPECULAR | LOBJ_ALPHA);

		if (ref_type == 0) {
			continue;
		}

		LObjSetActive(lobj);
		next_light_index = HSD_LObjGetNbActive();

		lobj->spec_id = GX_LIGHT_NULL;

		switch (type) {
		case LOBJ_INFINITE:
			setup_infinite_lightobj(lobj, view_mtx);
			break;
		case LOBJ_POINT:
			setup_point_lightobj(lobj, view_mtx);
			break;
		case LOBJ_SPOT:
			setup_spot_lightobj(lobj, view_mtx);
			break;
		case LOBJ_AMBIENT:
			continue;
		}

		if (ref_type & (LOBJ_DIFFUSE | LOBJ_ALPHA)) {
			setup_diffuse_lightobj(lobj);
		} else if (ref_type & LOBJ_SPECULAR) {
			setup_spec_lightobj(lobj, view_mtx, lobj->id);
		}
	}

	if (!HSD_LObjGetActiveByID(HSD_LIGHT_AMBIENT)) {
		for (; list; list = list->next) {
			HSD_LObj* lobj = list->data;
			u32 flags;

			if (lobj == NULL) {
				continue;
			}
			flags = lobj->flags;
			if (flags & LOBJ_HIDDEN) {
				continue;
			}
			if (HSD_LObjGetType(lobj) != LOBJ_AMBIENT) {
				continue;
			}
			if (!(flags & (LOBJ_DIFFUSE | LOBJ_ALPHA))) {
				continue;
			}
			LObjSetActive(lobj);
			break;
		}
	}

	active_count = HSD_LObjGetNbActive();
	for (light_index = 0; next_light_index < GX_LIGHT_COUNT && light_index < active_count; light_index++) {
		HSD_LObj* lobj = HSD_LObjGetActiveByIndex(light_index);

		if (lobj == NULL) {
			continue;
		}
		if (!(lobj->flags & LOBJ_SPECULAR)) {
			continue;
		}
		if (!(lobj->flags & (LOBJ_DIFFUSE | LOBJ_ALPHA))) {
			continue;
		}
		setup_spec_lightobj(lobj, view_mtx, HSD_Index2LightID(next_light_index++));
	}

	for (light_index = 0; light_index < active_count; light_index++) {
		HSD_LObj* lobj;
		GXColor color;

		if (!(lobj = HSD_LObjGetActiveByIndex(light_index))) {
			continue;
		}
		color = lobj->color;
		HSD_LObjSetup(lobj, &color, lobj->shininess);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x120
 */
void HSD_LObjAddCurrent(HSD_LObj* lobj)
{
	HSD_SList* node;
	HSD_SList** p;

	if (lobj != NULL) {
		node = current_lights;
		while (node != NULL) {
			if (node->data == lobj) {
				HSD_LObjDeleteCurrent(lobj);
				break;
			}
			node = node->next;
		}
		ref_INC(lobj);
		for (p = &current_lights; *p != NULL; p = &(*p)->next) {
			if (HSD_LObjGetPriority((*p)->data) > HSD_LObjGetPriority(lobj)) {
				break;
			}
		}
		*p = HSD_SListPrepend(*p, lobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_WObjUnref(HSD_WObj* wobj)
{
	if (wobj != NULL && ref_DEC(wobj)) {
		hsdDelete(wobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjUnrefThis(HSD_LObj* lobj)
{
	if (lobj != NULL && ref_DEC(lobj)) {
		hsdDelete(lobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x178
 */
void HSD_LObjDeleteCurrent(HSD_LObj* lobj)
{
	if (lobj != NULL) {
		HSD_SList** p;
		for (p = &current_lights; *p != NULL; p = &(*p)->next) {
			if ((*p)->data == lobj) {
				s32 i;
				for (i = 0; i < ACTIVE_LIGHT_SLOTS; i++) {
					if (lobj == active_lights[i]) {
						active_lights[i] = NULL;
					}
				}
				*p = HSD_SListRemove(*p);
				HSD_LObjUnrefThis(lobj);
				return;
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void LObjRemoveAll(void)
{
	LObjClearActive();
	nb_active_lights = 0;
	while (current_lights != NULL) {
		HSD_LObj* lp = current_lights->data;
		HSD_LObjUnrefThis(lp);
		current_lights = HSD_SListRemove(current_lights);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x258
 */
void HSD_LObjDeleteCurrentAll(HSD_LObj* lobj)
{
	if (lobj != NULL) {
		while (lobj != NULL) {
			HSD_LObjDeleteCurrent(lobj);
			lobj = lobj->next;
		}
		return;
	}
	LObjRemoveAll();
}

/**
 * @note Address: 0x801BE4B4
 * @note Size: 0x340
 */
void HSD_LObjSetCurrentAll(HSD_LObj* lobj)
{
	HSD_LObj* cur;

	LObjRemoveAll();
	for (cur = lobj; cur != NULL; cur = cur->next) {
		HSD_LObjAddCurrent(cur);
	}
}

/**
 * @note Address: 0x801BE7F4
 * @note Size: 0x34
 */
HSD_LObj* HSD_LObjGetCurrentByType(u16 flags)
{
	u32 type       = flags & LOBJ_TYPE_MASK;
	HSD_SList* cur = current_lights;
	while (cur != NULL) {
		HSD_LObj* lobj = cur->data;
		if (type == HSD_LObjGetType(lobj)) {
			return lobj;
		}
		cur = cur->next;
	}
	return NULL;
}

/**
 * @note Address: 0x801BE828
 * @note Size: 0xEC
 */
s32 HSD_LightID2Index(u32 id)
{
	s32 idx;

	switch (id) {
	case GX_LIGHT0:
		idx = 0;
		break;
	case GX_LIGHT1:
		idx = 1;
		break;
	case GX_LIGHT2:
		idx = 2;
		break;
	case GX_LIGHT3:
		idx = 3;
		break;
	case GX_LIGHT4:
		idx = 4;
		break;
	case GX_LIGHT5:
		idx = 5;
		break;
	case GX_LIGHT6:
		idx = 6;
		break;
	case GX_LIGHT7:
		idx = 7;
		break;
	case HSD_LIGHT_AMBIENT:
		idx = AMBIENT_LIGHT_INDEX;
		break;
	default:
		assert_line(1192, 0);
	}
	return idx;
}

/**
 * @note Address: 0x801BE914
 * @note Size: 0x70
 */
s32 HSD_Index2LightID(s32 index)
{
	switch (index) {
	case 0:
		return GX_LIGHT0;
	case 1:
		return GX_LIGHT1;
	case 2:
		return GX_LIGHT2;
	case 3:
		return GX_LIGHT3;
	case 4:
		return GX_LIGHT4;
	case 5:
		return GX_LIGHT5;
	case 6:
		return GX_LIGHT6;
	case 7:
		return GX_LIGHT7;
	case AMBIENT_LIGHT_INDEX:
		return HSD_LIGHT_AMBIENT;
	default:
		return GX_LIGHT_NULL;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x20C
 */
void HSD_LObjRemoveAll(HSD_LObj* lobj)
{
	HSD_LObj* next;
	HSD_LObj* cur;

	cur = lobj;
	while (cur != NULL) {
		next = cur->next;
		HSD_LObjDeleteCurrent(cur);
		HSD_LObjUnrefThis(cur);
		cur = next;
	}
}

/**
 * @note Address: 0x801BE984
 * @note Size: 0xC
 */
void HSD_LObjSetColor(HSD_LObj* lobj, GXColor color)
{
	lobj->color = color;
}

/**
 * @note Address: 0x801BE990
 * @note Size: 0xC
 */
void HSD_LObjGetColor(HSD_LObj* lobj, GXColor* color)
{
	*color = lobj->color;
}

/**
 * @note Address: 0x801BE99C
 * @note Size: 0x14
 */
void HSD_LObjSetSpot(HSD_LObj* lobj, f32 cutoff, GXSpotFn spot_func)
{
	if (lobj != NULL) {
		lobj->u.params.cutoff = cutoff;
		lobj->u.params.spot_func = spot_func;
	}
}

/**
 * @note Address: 0x801BE9B0
 * @note Size: 0x18
 */
void HSD_LObjSetDistAttn(HSD_LObj* lobj, f32 ref_dist, f32 ref_br, GXDistAttnFn dist_func)
{
	if (lobj != NULL) {
		lobj->u.params.ref_dist = ref_dist;
		lobj->u.params.ref_br = ref_br;
		lobj->u.params.dist_func = dist_func;
	}
}

/**
 * @note Address: 0x801BE9C8
 * @note Size: 0x88
 */
void HSD_LObjSetPosition(HSD_LObj* lobj, Vec* position)
{
	assert_line(1391, lobj);
	if (lobj->position == NULL) {
		lobj->position = HSD_WObjAlloc();
		assert_line(1394, lobj->position);
	}
	HSD_WObjSetPosition(lobj->position, position);
}

/**
 * @note Address: 0x801BEA50
 * @note Size: 0x40
 */
BOOL HSD_LObjGetPosition(HSD_LObj* lobj, Vec* pos)
{
	if (lobj != NULL && lobj->position != NULL) {
		HSD_WObjGetPosition(lobj->position, pos);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801BEA90
 * @note Size: 0x88
 */
void HSD_LObjSetInterest(HSD_LObj* lobj, Vec* interest)
{
	assert_line(1427, lobj);
	if (lobj->interest == NULL) {
		lobj->interest = HSD_WObjAlloc();
		assert_line(1430, lobj->interest);
	}
	HSD_WObjSetPosition(lobj->interest, interest);
}

/**
 * @note Address: 0x801BEB18
 * @note Size: 0x40
 */
BOOL HSD_LObjGetInterest(HSD_LObj* lobj, Vec* interest)
{
	if (lobj != NULL && lobj->interest != NULL) {
		HSD_WObjGetPosition(lobj->interest, interest);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801BEB58
 * @note Size: 0x18
 */
HSD_WObj* HSD_LObjGetPositionWObj(HSD_LObj* lobj)
{
	if (lobj != NULL) {
		return lobj->position;
	}
	return NULL;
}

/**
 * @note Address: 0x801BEB70
 * @note Size: 0x18
 */
HSD_WObj* HSD_LObjGetInterestWObj(HSD_LObj* lobj)
{
	if (lobj != NULL) {
		return lobj->interest;
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjSetPositionWObj(HSD_LObj* lobj, HSD_WObj* wobj)
{
	if (lobj == NULL) {
		return;
	}
	HSD_WObjUnref(lobj->position);
	lobj->position = wobj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjSetInterestWObj(HSD_LObj* lobj, HSD_WObj* wobj)
{
	if (lobj == NULL) {
		return;
	}
	HSD_WObjUnref(lobj->interest);
	lobj->interest = wobj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjSetAttnA(HSD_LObj* lobj, f32 a0, f32 a1, f32 a2)
{
	if (lobj == NULL) {
		return;
	}
	lobj->u.attn.a0 = a0;
	lobj->u.attn.a1 = a1;
	lobj->u.attn.a2 = a2;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjSetAttnK(HSD_LObj* lobj, f32 k0, f32 k1, f32 k2)
{
	if (lobj == NULL) {
		return;
	}
	lobj->u.attn.k0 = k0;
	lobj->u.attn.k1 = k1;
	lobj->u.attn.k2 = k2;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_LObjSetAttn(HSD_LObj* lobj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2)
{
	HSD_LObjSetAttnA(lobj, a0, a1, a2);
	HSD_LObjSetAttnK(lobj, k0, k1, k2);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
HSD_LObjInfo* HSD_LObjGetDefaultClass(void)
{
	return default_class ? default_class : &hsdLObj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
HSD_LObj* HSD_LObjAlloc(void)
{
	HSD_LObj* new = hsdNew(HSD_CLASS_INFO(HSD_LObjGetDefaultClass()));
	assert_line(1500, new);
	return new;
}

/**
 * @note Address: 0x801BEB88
 * @note Size: 0x414
 */
static int LObjLoad(HSD_LObj* lobj, HSD_LightDesc* ldesc)
{
	HSD_LObjSetColor(lobj, ldesc->color);
	HSD_LObjSetFlags(lobj, ldesc->flags);
	switch (ldesc->flags & LOBJ_TYPE_MASK) {
	case LOBJ_AMBIENT:
		break;
	case LOBJ_INFINITE:
		HSD_LObjSetPositionWObj(lobj, HSD_WObjLoadDesc(ldesc->position));
		break;
	case LOBJ_POINT:
		HSD_LObjSetPositionWObj(lobj, HSD_WObjLoadDesc(ldesc->position));
		if (ldesc->attnflags & LOBJ_LIGHT_ATTN) {
			HSD_LObjSetFlags(lobj, LOBJ_RAW_PARAM);
			HSD_LObjSetAttnK(lobj, ldesc->u.attn->k0, ldesc->u.attn->k1, ldesc->u.attn->k2);
		} else {
			HSD_LObjSetDistAttn(lobj, ldesc->u.point->ref_dist, ldesc->u.point->ref_br, ldesc->u.point->dist_func);
		}
		break;
	case LOBJ_SPOT:
		HSD_LObjSetPositionWObj(lobj, HSD_WObjLoadDesc(ldesc->position));
		HSD_LObjSetInterestWObj(lobj, HSD_WObjLoadDesc(ldesc->interest));
		if (ldesc->attnflags & LOBJ_LIGHT_ATTN) {
			HSD_LObjSetFlags(lobj, LOBJ_RAW_PARAM);
			HSD_LObjSetAttn(lobj, ldesc->u.attn->a0, ldesc->u.attn->a1, ldesc->u.attn->a2, ldesc->u.attn->k0, ldesc->u.attn->k1,
			                ldesc->u.attn->k2);
		} else {
			HSD_LObjSetDistAttn(lobj, ldesc->u.spot->ref_dist, ldesc->u.spot->ref_br, ldesc->u.spot->dist_func);
			HSD_LObjSetSpot(lobj, ldesc->u.spot->cutoff, ldesc->u.spot->spot_func);
		}
		break;
	default:
		OSReport("unexpected lightdesc flags (%x)\n", ldesc->flags);
		HSD_Panic(__FILE__, 0x660, "");
		break;
	}
	return 0;
}

/**
 * @note Address: 0x801BEF9C
 * @note Size: 0xF0
 */
HSD_LObj* HSD_LObjLoadDesc(HSD_LightDesc* ldesc)
{
	HSD_LObj *top, **p = &top;

	for (; ldesc; ldesc = ldesc->next) {
		HSD_ClassInfo* info;

		if (!ldesc->class_name || !(info = hsdSearchClassInfo(ldesc->class_name))) {
			*p = HSD_LObjAlloc();
		} else {
			*p = hsdNew(info);
			assert_line(1666, *p);
		}
		HSD_LOBJ_METHOD(*p)->load(*p, ldesc);
		p = &(*p)->next;
	}
	*p = NULL;

	return top;
}

/**
 * @note Address: 0x801BF08C
 * @note Size: 0x94
 */
void HSD_LObjAddAnim(HSD_LObj* lobj, HSD_LightAnim* lanim)
{
	if (lobj == NULL) {
		return;
	}

	if (lanim != NULL) {
		if (lobj->aobj) {
			HSD_AObjRemove(lobj->aobj);
		}
		lobj->aobj = HSD_AObjLoadDesc(lanim->aobjdesc);
		HSD_WObjAddAnim(HSD_LObjGetPositionWObj(lobj), lanim->position_anim);
		HSD_WObjAddAnim(HSD_LObjGetInterestWObj(lobj), lanim->interest_anim);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xD4
 */
void HSD_LObjAddAnimAll(HSD_LObj* lobj, HSD_LightAnim* lanim)
{
	HSD_LObj* lp;
	HSD_LightAnim* la;

	if (lobj == NULL) {
		return;
	}

	for (lp = lobj, la = lanim; lp; lp = next_p(lp), la = next_p(la)) {
		HSD_LObjAddAnim(lp, la);
	}
}

/**
 * @note Address: 0x801BF120
 * @note Size: 0x164
 */
static void LObjRelease(HSD_Class* o)
{
	HSD_LObj* lobj = HSD_LOBJ(o);

	HSD_AObjRemove(lobj->aobj);
	HSD_WObjUnref(HSD_LObjGetPositionWObj(lobj));
	HSD_WObjUnref(HSD_LObjGetInterestWObj(lobj));

	HSD_OBJECT_PARENT_INFO(&hsdLObj)->release(o);
}

/**
 * @note Address: 0x801BF284
 * @note Size: 0x60
 */
static void LObjAmnesia(HSD_ClassInfo* info)
{
	if (info == HSD_CLASS_INFO(default_class)) {
		default_class = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdLObj)) {
		current_lights = NULL;
	}
	HSD_OBJECT_PARENT_INFO(&hsdLObj)->amnesia(info);
}

/**
 * @note Address: 0x801BF2E4
 * @note Size: 0x80
 */
static void LObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdLObj), &hsdObj, "sysdolphin_base_library", "hsd_lobj", sizeof(HSD_LObjInfo), sizeof(HSD_LObj));
	HSD_CLASS_INFO(&hsdLObj)->release = LObjRelease;
	HSD_CLASS_INFO(&hsdLObj)->amnesia = LObjAmnesia;
	hsdLObj.load                     = LObjLoad;
	hsdLObj.update                   = LObjUpdateFunc;
}
