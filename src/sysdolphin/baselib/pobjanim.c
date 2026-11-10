#include <global.h>
#include <sysdolphin/pobj.h>

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
