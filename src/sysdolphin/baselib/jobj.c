#include <global.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <kar/math.h>
#include <math.h>
#include <string.h>
#include <sysdolphin/jobj.h>

#include <sysdolphin/aobj.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/displayfunc.h>
#include <sysdolphin/dobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/id.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/robj.h>
#include <sysdolphin/spline.h>

#define assert_report(line, cond, msg) ((cond) ? ((void)0) : (OSReport(msg), __assert(__FILE__, line, #cond)))

#define assert_line_text(line, cond, text) ((cond) ? ((void)0) : __assert(__FILE__, line, text))

#define assert_jobj_header(line, cond, text) ((cond) ? ((void)0) : __assert("jobj.h", line, text))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define JOBJ_IK_NORM_EPSILON 1e-10F
#define JOBJ_IK_MIN_DISTANCE_SQ 1e-8F

static void JObjInfoInit(void);
HSD_JObjInfo hsdJObj = { JObjInfoInit };

// .sbss comes out last declaration first, so these run backwards
static HSD_JObj* current_jobj;
static void (*ptcltgt_callback)(HSD_JObj*, s32);
static void (*jsound_callback)(s32);
static HSD_JObjParticleCallback dptcl_callback;
static HSD_SList* ufc_callbacks;
static HSD_ClassInfo* default_class;

/**
 * @note Address: 0x801C3B4C
 * @note Size: 0x12C
 */
void HSD_JObjCheckDepend(HSD_JObj* jobj)
{
	if (jobj == NULL || HSD_JObjMtxIsDirty(jobj)) {
		return;
	}
	if (jobj->flags & USER_DEF_MTX) {
		if (!(jobj->flags & MTX_INDEP_PARENT) && jobj->parent != NULL && HSD_JObjMtxIsDirty(jobj->parent)) {
			jobj->flags |= MTX_DIRTY;
		}
	} else if ((jobj->parent != NULL && (jobj->parent->flags & MTX_DIRTY)) || (jobj->flags & JOBJ_JOINT) == JOINT1
	           || (jobj->flags & JOBJ_JOINT) == JOINT2 || (jobj->flags & JOBJ_JOINT) == EFFECTOR || jobj->robj != NULL) {
		jobj->flags |= MTX_DIRTY;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xD4
 */
static void JObjResetRST(HSD_JObj* jobj, HSD_Joint* joint)
{
	if (jobj == NULL || joint == NULL) {
		return;
	}
	jobj->rotate.x  = joint->rotation.x;
	jobj->rotate.y  = joint->rotation.y;
	jobj->rotate.z  = joint->rotation.z;
	jobj->scale     = joint->scale;
	jobj->translate = joint->position;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x308
 */
void HSD_JObjResetRST(HSD_JObj* jobj, HSD_Joint* joint)
{
	HSD_JObj* child_jobj;
	HSD_Joint* child_joint;

	if (jobj == NULL || joint == NULL) {
		return;
	}
	JObjResetRST(jobj, joint);
	if (!(jobj->flags & INSTANCE)) {
		child_jobj  = jobj->child;
		child_joint = joint->child;
		while (child_jobj != NULL) {
			HSD_JObjResetRST(child_jobj, child_joint);
			child_jobj  = child_jobj->next;
			child_joint = child_joint != NULL ? child_joint->next : NULL;
		}
	}
}

/**
 * @note Address: 0x801C3D40
 * @note Size: 0x344
 */
static void HSD_JObjWalkTree0(HSD_JObj* jobj, HSD_JObjWalkTreeCallback cb, void* context)
{
	u32 type;
	HSD_JObj* child;

	if (jobj == NULL) {
		return;
	}
	assert_line(0xAD, jobj->parent);
	type = jobj->parent->child == jobj ? 1 : 2;
	if (cb != NULL) {
		cb(jobj, context, type);
	}
	if (!(jobj->flags & INSTANCE)) {
		child = jobj->child;
		while (child != NULL) {
			HSD_JObjWalkTree0(child, cb, context);
			child = child->next;
		}
	}
}

/**
 * @note Address: 0x801C4084
 * @note Size: 0x90
 */
void HSD_JObjWalkTree(HSD_JObj* jobj, HSD_JObjWalkTreeCallback cb, void* context)
{
	HSD_JObj* child;

	if (jobj == NULL) {
		return;
	}
	if (cb != NULL) {
		cb(jobj, context, 0);
	}
	if (!(jobj->flags & INSTANCE)) {
		child = jobj->child;
		while (child != NULL) {
			HSD_JObjWalkTree0(child, cb, context);
			child = child->next;
		}
	}
}

/**
 * @note Address: 0x801C4114
 * @note Size: 0x2D0
 */
void HSD_JObjMakeMatrix(HSD_JObj* jobj)
{
	Vec* parentScale;

	HSD_JObjSetupMatrix(jobj->parent);
	if (jobj->flags & CLASSICAL_SCALE) {
		if (jobj->parent != NULL && jobj->parent->scl != NULL) {
			if (jobj->scl == NULL) {
				jobj->scl = HSD_VecAlloc();
			}
			*jobj->scl = *jobj->parent->scl;
		} else {
			if (jobj->scl != NULL) {
				HSD_VecFree(jobj->scl);
				jobj->scl = NULL;
			}
		}
	} else {
		if (jobj->scl == NULL) {
			jobj->scl = HSD_VecAlloc();
		}
		if (jobj->parent != NULL && jobj->parent->scl != NULL) {
			jobj->scl->x = jobj->scale.x * jobj->parent->scl->x;
			jobj->scl->y = jobj->scale.y * jobj->parent->scl->y;
			jobj->scl->z = jobj->scale.z * jobj->parent->scl->z;
		} else {
			*jobj->scl = jobj->scale;
		}
	}
	parentScale = jobj->parent != NULL ? jobj->parent->scl : NULL;
	if (jobj->flags & USE_QUATERNION) {
		HSD_MtxSRTQuat(jobj->mtx, &jobj->scale, &jobj->rotate, &jobj->translate, parentScale);
	} else {
		Vec euler;

		euler.x = jobj->rotate.x;
		euler.y = jobj->rotate.y;
		euler.z = jobj->rotate.z;
		HSD_MtxSRT(jobj->mtx, &jobj->scale, &euler, &jobj->translate, parentScale);
	}
	if (jobj->parent != NULL) {
		PSMTXConcat(jobj->parent->mtx, jobj->mtx, jobj->mtx);
	}
	if (jobj->aobj != NULL && jobj->aobj->hsd_obj != NULL) {
		Vec vec;
		HSD_JObj* aobj_jobj = (HSD_JObj*)jobj->aobj->hsd_obj;
		HSD_JObjSetupMatrix((HSD_JObj*)jobj->aobj->hsd_obj);
		PSMTXMultVec(aobj_jobj->mtx, &jobj->translate, &vec);
		jobj->mtx[0][3] = vec.x;
		jobj->mtx[1][3] = vec.y;
		jobj->mtx[2][3] = vec.z;
	}
}

/**
 * @note Address: 0x801C43E4
 * @note Size: 0x7C
 */
void HSD_JObjRemoveAnimByFlags(HSD_JObj* jobj, u32 flags)
{
	if (jobj != NULL) {
		if (flags & 1) {
			HSD_AObjRemove(jobj->aobj);
			jobj->aobj = NULL;
		}
		if (union_type_dobj(jobj)) {
			HSD_DObjRemoveAnimAllByFlags(jobj->u.dobj, flags);
		}
		HSD_RObjRemoveAnimAllByFlags(jobj->robj, flags);
	}
}

/**
 * @note Address: 0x801C4460
 * @note Size: 0x2A4
 */
void HSD_JObjRemoveAnimAllByFlags(HSD_JObj* jobj, u32 flags)
{
	HSD_JObj* child;

	if (jobj != NULL) {
		HSD_JObjRemoveAnimByFlags(jobj, flags);
		if (!(jobj->flags & INSTANCE)) {
			child = jobj->child;
			while (child != NULL) {
				HSD_JObjRemoveAnimAllByFlags(child, flags);
				child = child->next;
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x68
 */
void HSD_JObjRemoveAnim(HSD_JObj* jobj)
{
	HSD_JObjRemoveAnimByFlags(jobj, 0x7FF);
}

/**
 * @note Address: 0x801C4704
 * @note Size: 0x24
 */
void HSD_JObjRemoveAnimAll(HSD_JObj* jobj)
{
	HSD_JObjRemoveAnimAllByFlags(jobj, 0x7FF);
}

/**
 * @note Address: 0x801C4728
 * @note Size: 0x88
 */
void HSD_JObjReqAnimByFlags(HSD_JObj* jobj, u32 flags, f32 frame)
{
	if (jobj != NULL) {
		if (flags & 1) {
			HSD_AObjReqAnim(jobj->aobj, frame);
		}
		if (union_type_dobj(jobj)) {
			HSD_DObjReqAnimAllByFlags(jobj->u.dobj, frame, flags);
		}
		HSD_RObjReqAnimAllByFlags(jobj->robj, frame, flags);
	}
}

/**
 * @note Address: 0x801C47B0
 * @note Size: 0x2C8
 */
void HSD_JObjReqAnimAllByFlags(HSD_JObj* jobj, u32 flags, f32 frame)
{
	HSD_JObj* child;

	if (jobj != NULL) {
		HSD_JObjReqAnimByFlags(jobj, flags, frame);
		if (!(jobj->flags & INSTANCE)) {
			child = jobj->child;
			while (child != NULL) {
				HSD_JObjReqAnimAllByFlags(child, flags, frame);
				child = child->next;
			}
		}
	}
}

/**
 * @note Address: 0x801C4A78
 * @note Size: 0x24
 */
void HSD_JObjReqAnimAll(HSD_JObj* jobj, f32 frame)
{
	HSD_JObjReqAnimAllByFlags(jobj, 0x7FF, frame);
}

/**
 * @note Address: 0x801C4A9C
 * @note Size: 0x74
 */
void HSD_JObjReqAnim(HSD_JObj* jobj, f32 frame)
{
	HSD_JObjReqAnimByFlags(jobj, 0x7FF, frame);
}

/**
 * @note Address: 0x801C4B10
 * @note Size: 0x58
 */
static void JObjSortAnim(HSD_AObj* aobj)
{
	HSD_FObj* fobj;
	HSD_FObj** fobj_ptr;

	if (aobj == NULL || aobj->fobj == NULL) {
		return;
	}
	for (fobj_ptr = &aobj->fobj; *fobj_ptr != NULL; fobj_ptr = &fobj->next) {
		fobj = *fobj_ptr;
		if (fobj->obj_type == TYPE_JOBJ) {
			*fobj_ptr  = fobj->next;
			fobj->next = aobj->fobj;
			aobj->fobj = fobj;
			break;
		}
	}
}

/**
 * @note Address: 0x801C4B68
 * @note Size: 0x140
 */
void HSD_JObjAddAnim(HSD_JObj* jobj, HSD_AnimJoint* an_joint, HSD_MatAnimJoint* mat_joint, HSD_ShapeAnimJoint* sh_joint)
{
	if (jobj != NULL) {
		if (an_joint != NULL) {
			if (jobj->aobj != NULL) {
				HSD_AObjRemove(jobj->aobj);
			}
			jobj->aobj = HSD_AObjLoadDesc(an_joint->aobjdesc);
			JObjSortAnim(jobj->aobj);
			HSD_RObjAddAnimAll(jobj->robj, an_joint->robj_anim);
			if (an_joint->flags & 1) {
				HSD_JObjSetFlags(jobj, CLASSICAL_SCALE);
			} else {
				HSD_JObjClearFlags(jobj, CLASSICAL_SCALE);
			}
		}
		if (union_type_dobj(jobj)) {
			HSD_DObjAddAnimAll(jobj->u.dobj, mat_joint != NULL ? mat_joint->matanim : NULL,
			                   sh_joint != NULL ? sh_joint->shapeanimdobj : NULL);
		}
	}
}

/**
 * @note Address: 0x801C4CA8
 * @note Size: 0x3DC
 */
void HSD_JObjAddAnimAll(HSD_JObj* jobj, HSD_AnimJoint* ajoint, HSD_MatAnimJoint* mjoint, HSD_ShapeAnimJoint* sjoint)
{
	HSD_JObj* jp;
	HSD_AnimJoint* aj;
	HSD_MatAnimJoint* mj;
	HSD_ShapeAnimJoint* sj;

	if (jobj != NULL) {
		HSD_JObjAddAnim(jobj, ajoint, mjoint, sjoint);
		if (!(jobj->flags & INSTANCE)) {
			jp = jobj->child;
			aj = ajoint != NULL ? ajoint->child : NULL;
			mj = mjoint != NULL ? mjoint->child : NULL;
			sj = sjoint != NULL ? sjoint->child : NULL;
			while (jp != NULL) {
				HSD_JObjAddAnimAll(jp, aj, mj, sj);
				jp = jp->next;
				aj = aj != NULL ? aj->next : NULL;
				mj = mj != NULL ? mj->next : NULL;
				sj = sj != NULL ? sj->next : NULL;
			}
		}
	}
}

typedef void (*JObjUserFunction)(HSD_JObj* jobj, enum_t channel, f32 value);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetRotationX(HSD_JObj* jobj, f32 x)
{
	assert_jobj_header(0x2A4, jobj, "jobj");
	assert_jobj_header(0x2A5, !(jobj->flags & USE_QUATERNION), "!(jobj->flags & JOBJ_USE_QUATERNION)");
	jobj->rotate.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetRotationY(HSD_JObj* jobj, f32 y)
{
	assert_jobj_header(0x2B8, jobj, "jobj");
	assert_jobj_header(0x2B9, !(jobj->flags & USE_QUATERNION), "!(jobj->flags & JOBJ_USE_QUATERNION)");
	jobj->rotate.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetRotationZ(HSD_JObj* jobj, f32 z)
{
	assert_jobj_header(0x2CC, jobj, "jobj");
	assert_jobj_header(0x2CD, !(jobj->flags & USE_QUATERNION), "!(jobj->flags & JOBJ_USE_QUATERNION)");
	jobj->rotate.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetScaleX(HSD_JObj* jobj, f32 x)
{
	assert_jobj_header(0x325, jobj, "jobj");
	jobj->scale.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetScaleY(HSD_JObj* jobj, f32 y)
{
	assert_jobj_header(0x333, jobj, "jobj");
	jobj->scale.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetScaleZ(HSD_JObj* jobj, f32 z)
{
	assert_jobj_header(0x341, jobj, "jobj");
	jobj->scale.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetTranslateX(HSD_JObj* jobj, f32 x)
{
	assert_jobj_header(0x3B8, jobj, "jobj");
	jobj->translate.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetTranslateY(HSD_JObj* jobj, f32 y)
{
	assert_jobj_header(0x3C6, jobj, "jobj");
	jobj->translate.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void HSD_JObjSetTranslateZ(HSD_JObj* jobj, f32 z)
{
	assert_jobj_header(0x3D4, jobj, "jobj");
	jobj->translate.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: 0x801C5084
 * @note Size: 0x944
 */
static void JObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val)
{
	HSD_JObj* jobj = obj;
	u32 channel = type;
	Vec p;
	HSD_JObj* jp;
	HSD_RObj* robj;
	Mtx mtx;

	if (jobj != NULL) {
		switch (channel) {
		case HSD_A_J_PATH:
			if (val->fv < 0.0) {
				val->fv = 0.0F;
			}
			if (1.0 < val->fv) {
				val->fv = 1.0F;
			}
			assert_line(0x24E, jobj->aobj);
			jp = (HSD_JObj*)jobj->aobj->hsd_obj;
			assert_line(0x250, jp);
			assert_line(0x251, jp->u.spline);
			splArcLengthPoint(&p, jp->u.spline, val->fv);
			HSD_JObjSetTranslateX(jobj, p.x);
			HSD_JObjSetTranslateY(jobj, p.y);
			HSD_JObjSetTranslateZ(jobj, p.z);
			break;
		case HSD_A_J_ROTX:
			if (jobj->flags & JOINT1) {
				robj = HSD_RObjGetByType(jobj->robj, REFTYPE_IKHINT, 0);
				if (robj != NULL) {
					robj->u.ik_hint.rotate_x = val->fv;
				}
			}
			HSD_JObjSetRotationX(jobj, val->fv);
			break;
		case HSD_A_J_ROTY:
			HSD_JObjSetRotationY(jobj, val->fv);
			break;
		case HSD_A_J_ROTZ:
			HSD_JObjSetRotationZ(jobj, val->fv);
			break;
		case HSD_A_J_TRAX:
			HSD_JObjSetTranslateX(jobj, val->fv);
			break;
		case HSD_A_J_TRAY:
			HSD_JObjSetTranslateY(jobj, val->fv);
			break;
		case HSD_A_J_TRAZ:
			HSD_JObjSetTranslateZ(jobj, val->fv);
			break;
		case HSD_A_J_SCAX:
			HSD_JObjSetScaleX(jobj, val->fv);
			break;
		case HSD_A_J_SCAY:
			HSD_JObjSetScaleY(jobj, val->fv);
			break;
		case HSD_A_J_SCAZ:
			HSD_JObjSetScaleZ(jobj, val->fv);
			break;
		case HSD_A_J_BRANCH:
			if (val->fv > 0.5) {
				HSD_JObjClearFlagsAll(jobj, HIDDEN);
			} else {
				HSD_JObjSetFlagsAll(jobj, HIDDEN);
			}
			break;
		case HSD_A_J_NODE:
			if (val->fv > 0.5) {
				HSD_JObjClearFlags(jobj, HIDDEN);
			} else {
				HSD_JObjSetFlags(jobj, HIDDEN);
			}
			break;
		case HSD_A_J_SETBYTE0:
		case HSD_A_J_SETBYTE1:
		case HSD_A_J_SETBYTE2:
		case HSD_A_J_SETBYTE3:
		case HSD_A_J_SETBYTE4:
		case HSD_A_J_SETBYTE5:
		case HSD_A_J_SETBYTE6:
		case HSD_A_J_SETBYTE7:
		case HSD_A_J_SETBYTE8:
		case HSD_A_J_SETBYTE9: {
			HSD_SList* callback_node;

			for (callback_node = ufc_callbacks; callback_node != NULL; callback_node = callback_node->next) {
				JObjUserFunction callback = (JObjUserFunction)callback_node->data;
				callback(jobj, type, val->iv);
			}
			break;
		}
		case HSD_A_J_SETFLOAT0:
		case HSD_A_J_SETFLOAT1:
		case HSD_A_J_SETFLOAT2:
		case HSD_A_J_SETFLOAT3:
		case HSD_A_J_SETFLOAT4:
		case HSD_A_J_SETFLOAT5:
		case HSD_A_J_SETFLOAT6:
		case HSD_A_J_SETFLOAT7:
		case HSD_A_J_SETFLOAT8:
		case HSD_A_J_SETFLOAT9: {
			HSD_SList* callback_node;

			for (callback_node = ufc_callbacks; callback_node != NULL; callback_node = callback_node->next) {
				JObjUserFunction callback = (JObjUserFunction)callback_node->data;
				callback(jobj, type, val->fv);
			}
			break;
		}
		case HSD_A_J_TRIGGER_PARTICLE: {
			s32 iv = val->iv;
			s32 lo = iv & 0x3F;
			s32 hi = iv >> 6 & 0xFFFFFF;
			if (dptcl_callback != NULL) {
				dptcl_callback(0, lo, hi, jobj);
			}
		} break;
		case HSD_A_J_TRIGGER_SOUND:
			if (jsound_callback != NULL) {
				jsound_callback(val->iv);
			}
			break;
		case HSD_A_J_SET_PARTICLE_TARGET:
			if (ptcltgt_callback != NULL) {
				ptcltgt_callback(jobj, val->iv);
			}
			break;
		case HSD_A_J_MTX_COL_X:
			jobj->mtx[0][0] = val->p.x;
			jobj->mtx[1][0] = val->p.y;
			jobj->mtx[2][0] = val->p.z;
			break;
		case HSD_A_J_MTX_COL_Y:
			jobj->mtx[0][1] = val->p.x;
			jobj->mtx[1][1] = val->p.y;
			jobj->mtx[2][1] = val->p.z;
			break;
		case HSD_A_J_MTX_COL_Z:
			jobj->mtx[0][2] = val->p.x;
			jobj->mtx[1][2] = val->p.y;
			jobj->mtx[2][2] = val->p.z;
			break;
		case HSD_A_J_MTX_COL_TRANSLATION:
			jobj->mtx[0][3] = val->p.x;
			jobj->mtx[1][3] = val->p.y;
			jobj->mtx[2][3] = val->p.z;
			break;
		case HSD_A_J_MTX_TO_SRT:
		case HSD_A_J_MTX_TO_ROTATION:
		case HSD_A_J_MTX_TO_TRANSLATION:
		case HSD_A_J_MTX_TO_SCALE:
			if (jobj->parent != NULL) {
				HSD_MtxInverseConcat(jobj->parent->mtx, jobj->mtx, mtx);
			} else {
				PSMTXCopy(jobj->mtx, mtx);
			}
			if (channel == HSD_A_J_MTX_TO_SRT || channel == HSD_A_J_MTX_TO_TRANSLATION) {
				HSD_MtxGetTranslate(mtx, &jobj->translate);
			}
			if (channel == HSD_A_J_MTX_TO_SRT || channel == HSD_A_J_MTX_TO_ROTATION) {
				HSD_MtxGetRotation(mtx, (Vec*)&jobj->rotate);
			}
			if (channel == HSD_A_J_MTX_TO_SRT || channel == HSD_A_J_MTX_TO_SCALE) {
				HSD_MtxGetScale(mtx, &jobj->scale);
			}
			break;
		}
	}
}

/**
 * @note Address: 0x801C59C8
 * @note Size: 0x164
 */
void HSD_JObjAnim(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		HSD_JObjCheckDepend(jobj);
		HSD_AObjInterpretAnim(jobj->aobj, jobj, HSD_JOBJ_METHOD(jobj)->update);
		HSD_RObjAnimAll(jobj->robj);
		if (union_type_dobj(jobj)) {
			HSD_DObjAnimAll(jobj->u.dobj);
		}
	}
}

/**
 * @note Address: 0x801C5B2C
 * @note Size: 0x348
 */
static void JObjAnimAll(HSD_JObj* jobj)
{
	HSD_JObj* child;

	if (jobj != NULL) {
		HSD_JObjAnim(jobj);
		if (!(jobj->flags & INSTANCE)) {
			child = jobj->child;
			while (child != NULL) {
				JObjAnimAll(child);
				child = child->next;
			}
		}
	}
}

/**
 * @note Address: 0x801C5E74
 * @note Size: 0x3C
 */
void HSD_JObjAnimAll(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		HSD_AObjInitEndCallBack();
		JObjAnimAll(jobj);
		HSD_AObjInvokeCallBacks();
	}
}

/**
 * @note Address: 0x801C5EB0
 * @note Size: 0x130
 */
static void setupInstanceMtx(Mtx vmtx, HSD_JObj* jobj, Mtx mtx)
{
	HSD_CObj* cobj;

	HSD_JObjSetupMatrix(jobj);
	HSD_JObjSetupMatrix(jobj->child);
	PSMTXInverse(jobj->child->mtx, mtx);
	PSMTXConcat(jobj->mtx, mtx, mtx);
	if (vmtx) {
		PSMTXConcat(vmtx, mtx, mtx);
	} else {
		cobj = HSD_CObjGetCurrent();
		if (cobj != NULL) {
			PSMTXConcat(HSD_CObjGetViewingMtxPtrDirect(cobj), mtx, mtx);
		}
	}
}

/**
 * @note Address: 0x801C5FE0
 * @note Size: 0x544
 */
void HSD_JObjDispAll(HSD_JObj* jobj, Mtx vmtx, HSD_TrspMask trsp_mask, u32 rendermode)
{
	if (jobj != NULL) {
		if (jobj->flags & INSTANCE) {
			if (!(jobj->flags & HIDDEN)) {
				Mtx mtx;
				setupInstanceMtx(vmtx, jobj, mtx);
				HSD_JObjDispAll(jobj->child, mtx, trsp_mask, rendermode);
			}
		} else {
			if (jobj->flags & (trsp_mask << JOBJ_TRSP_SHIFT)) {
				HSD_JObjDisp(jobj, vmtx, trsp_mask, rendermode);
			}
			if (jobj->flags & (trsp_mask << JOBJ_ROOT_TRSP_SHIFT)) {
				jobj = jobj->child;
				while (jobj != NULL) {
					HSD_JObjDispAll(jobj, vmtx, trsp_mask, rendermode);
					jobj = jobj->next;
				}
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x58
 */
void HSD_JObjSetDefaultClass(HSD_ClassInfo* info)
{
	if (info != NULL) {
		if (!hsdIsDescendantOf(info, &hsdJObj.parent.parent)) {
			__assert(__FILE__, 0x3B2, "hsdIsDescendantOf(info, &hsdJObj)");
		}
	}
	default_class = info;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_JObj* JObjLoadJointSub(HSD_Joint* joint, HSD_JObj* parent)
{
	HSD_JObj* jobj;
	HSD_ClassInfo* info;

	if (joint == NULL) {
		jobj = NULL;
	} else {
		if (joint->class_name == NULL || (info = hsdSearchClassInfo(joint->class_name)) == NULL) {
			jobj = HSD_JObjAlloc();
		} else {
			jobj = (HSD_JObj*)hsdNew(info);
			assert_line(0x3D5, jobj);
		}
		HSD_JOBJ_METHOD(jobj)->load(jobj, joint, parent);
	}
	return jobj;
}

/**
 * @note Address: 0x801C652C
 * @note Size: 0x70
 */
// HSD_JObjSetupMatrix, the jobj.h inline, its out of line copy lands here

/**
 * @note Address: 0x801C659C
 * @note Size: 0x1FC
 */
static s32 JObjLoad(HSD_JObj* jobj, HSD_Joint* joint, HSD_JObj* jobj_2)
{
	HSD_Joint* childjoint;
	HSD_JObj* child;

	jobj->child  = NULL;
	jobj->next   = NULL;
	jobj->parent = NULL;
	jobj->flags |= joint->flags;

	if (union_type_spline(jobj)) {
		jobj->u.spline = joint->u.spline;
	} else if (union_type_ptcl(jobj)) {
		HSD_SList* slist;

		jobj->u.ptcl = joint->u.ptcl;
		slist        = joint->u.ptcl;
		while (slist != NULL) {
			slist->uint_value |= JOBJ_STATIC_PARTICLE_PENDING;
			slist = slist->next;
		}
	} else {
		jobj->u.dobj = HSD_DObjLoadDesc(joint->u.dobjdesc);
	}

	jobj->robj      = HSD_RObjLoadDesc(joint->robjdesc);
	jobj->rotate.x  = joint->rotation.x;
	jobj->rotate.y  = joint->rotation.y;
	jobj->rotate.z  = joint->rotation.z;
	jobj->scale     = joint->scale;
	jobj->translate = joint->position;
	PSMTXIdentity(jobj->mtx);
	jobj->scl = NULL;

	if (joint->mtx != NULL) {
		jobj->envelopemtx = HSD_MtxAlloc();
		memcpy(jobj->envelopemtx, joint->mtx, sizeof(Mtx));
	}

	HSD_IDInsertToTable(NULL, (u32)joint, jobj);
	jobj->id = (u32)joint;

	if (!(joint->flags & INSTANCE)) {
		for (childjoint = joint->child; childjoint != NULL; childjoint = childjoint->next) {
			child = JObjLoadJointSub(childjoint, jobj);
			if (child == NULL) {
				return -1;
			}
			HSD_JObjAddChild(jobj, child);
		}
	}
	return 0;
}

/**
 * @note Address: 0x801C6798
 * @note Size: 0xA8
 */
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* joint)
{
	HSD_JObj* jobj = JObjLoadJointSub(joint, NULL);
	HSD_JObjResolveRefsAll(jobj, joint);
	return jobj;
}

/**
 * @note Address: 0x801C6C0C
 * @note Size: 0x54
 */
void HSD_JObjRef(HSD_JObj* jobj)
{
	ref_INC(jobj);
}

/**
 * @note Address: 0x801C6840
 * @note Size: 0xF8
 */
void HSD_JObjResolveRefs(HSD_JObj* jobj, HSD_Joint* joint)
{
	if (jobj == NULL || joint == NULL) {
		return;
	}

	HSD_RObjResolveRefsAll(jobj->robj, joint->robjdesc);
	if (jobj->flags & INSTANCE) {
		HSD_JObjUnref(jobj->child);
		jobj->child = HSD_IDGetData((u32)joint->child, NULL);
		assert_line(0x45F, jobj->child);
		HSD_JObjRef(jobj->child);
	}
	if (union_type_dobj(jobj)) {
		HSD_DObjResolveRefsAll(jobj->u.dobj, joint->u.dobjdesc);
	}
}

/**
 * @note Address: 0x801C6938
 * @note Size: 0x2D4
 */
void HSD_JObjResolveRefsAll(HSD_JObj* jobj, HSD_Joint* joint)
{
	while (jobj != NULL && joint != NULL) {
		HSD_JObjResolveRefs(jobj, joint);
		if (!(jobj->flags & INSTANCE)) {
			HSD_JObjResolveRefsAll(jobj->child, joint->child);
		}
		jobj  = jobj->next;
		joint = joint->next;
	}
}

/**
 * @note Address: 0x801C6CD8
 * @note Size: 0x148
 */
void HSD_JObjUnref(HSD_JObj* jobj)
{
	if (jobj != NULL && ref_DEC(jobj)) {
		if (iref_CNT(jobj) - 1 < 0) {
			hsdDelete(jobj);
		} else {
			iref_INC(jobj);
			HSD_JOBJ_METHOD(jobj)->release_child(jobj);
			if (iref_DEC(jobj)) {
				hsdDelete(jobj);
			}
		}
	}
}

/**
 * @note Address: 0x801C6E20
 * @note Size: 0xA8
 */
void HSD_JObjUnrefThis(HSD_JObj* jobj)
{
	if (jobj != NULL && iref_DEC(jobj) && ref_CNT(jobj) < 0) {
		hsdDelete(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x208
 */
HSD_JObj* HSD_JObjRemove(HSD_JObj* jobj)
{
	HSD_JObj* child;
	HSD_JObj* next;
	HSD_JObj* prev;

	if (jobj == NULL) {
		return NULL;
	}
	child = jobj->child;
	if (child != NULL) {
		assert_line(0x4E4, child->next == NULL);
	}

	next = child != NULL ? child : jobj->next;

	prev = HSD_JObjGetPrev(jobj);
	if (prev != NULL) {
		prev->next = next;
	} else if (jobj->parent != NULL) {
		jobj->parent->child = next;
	}
	if (next != NULL && next == child) {
		next->next   = jobj->next;
		next->parent = jobj->parent;
	}
	jobj->parent = NULL;
	jobj->child  = NULL;
	jobj->next   = NULL;
	HSD_JObjUnref(jobj);
	return child;
}

/**
 * @note Address: N/A
 * @note Size: 0x1A8
 */
void HSD_JObjRemoveAll(HSD_JObj* jobj)
{
	HSD_JObj* prev;
	HSD_JObj* next;

	if (jobj == NULL) {
		return;
	}
	if (jobj->parent != NULL) {
		prev = HSD_JObjGetPrev(jobj);
		if (prev != NULL) {
			prev->next = NULL;
		} else {
			jobj->parent->child = NULL;
		}
	}
	while (jobj != NULL) {
		next         = jobj->next;
		jobj->parent = NULL;
		jobj->next   = NULL;
		HSD_JObjUnref(jobj);
		jobj = next;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void UpdateParentTrspBits(HSD_JObj* jobj, HSD_JObj* child)
{
	u32 flags = (child->flags | (child->flags << (JOBJ_ROOT_TRSP_SHIFT - JOBJ_TRSP_SHIFT))) & JOBJ_ROOT_MASK;
	while (jobj != NULL) {
		if (!(flags & ~jobj->flags)) {
			break;
		}
		jobj->flags |= flags;
		jobj = jobj->parent;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void jobj_recalc_parent_trsp_bits(HSD_JObj* jobj)
{
	HSD_JObj* child;
	u32 flags;

	while (jobj != NULL) {
		child = jobj->child;
		flags = ~JOBJ_ROOT_MASK;
		while (child != NULL) {
			flags |= (child->flags | (child->flags << (JOBJ_ROOT_TRSP_SHIFT - JOBJ_TRSP_SHIFT))) & JOBJ_ROOT_MASK;
			child = child->next;
		}
		if (!(jobj->flags & ~flags)) {
			break;
		}
		jobj->flags &= flags;
		jobj = jobj->next;
	}
}

/**
 * @note Address: 0x801C6EC8
 * @note Size: 0x14C
 */
void HSD_JObjAddChild(HSD_JObj* jobj, HSD_JObj* child)
{
	HSD_JObj* last;

	if (jobj == NULL || child == NULL) {
		return;
	}
	assert_report(0x549, child->parent == NULL, "child should be a orphan.\n");
	assert_report(0x54A, child->next == NULL, "child should not have siblings");
	if (jobj->child == NULL) {
		jobj->child = child;
	} else {
		assert_line_text(0x550, !(jobj->flags & INSTANCE), "!(jobj->flags & JOBJ_INSTANCE)");
		last = jobj->child;
		while (last->next != NULL) {
			assert_line(0x553, last != child);
			last = last->next;
		}
		last->next = child;
	}
	child->parent = jobj;
	UpdateParentTrspBits(jobj, child);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
HSD_JObj* HSD_JObjReparent(HSD_JObj* jobj, HSD_JObj* parent)
{
	HSD_JObj* next;
	HSD_JObj* prev;

	if (jobj == NULL) {
		return NULL;
	}
	next = jobj->next;
	if (jobj->parent != NULL) {
		if (jobj->parent->child == jobj) {
			jobj->parent->child = next;
		} else {
			prev = HSD_JObjGetPrev(jobj);
			assert_line(0x572, prev);
			prev->next = next;
		}
		jobj_recalc_parent_trsp_bits(jobj->parent);
		jobj->parent = NULL;
	}
	jobj->next = NULL;
	HSD_JObjAddChild(parent, jobj);
	return next;
}

/**
 * @note Address: N/A
 * @note Size: 0x8C
 */
HSD_JObj* HSD_JObjGetPrev(HSD_JObj* jobj)
{
	HSD_JObj* cur;

	if (jobj == NULL || jobj->parent == NULL) {
		return NULL;
	}
	if (jobj == jobj->parent->child) {
		return NULL;
	}
	cur = jobj->parent->child;
	while (cur != NULL) {
		if (cur->next == jobj) {
			return cur;
		}
		cur = cur->next;
	}
	HSD_Panic(__FILE__, 0x5EF, "can not find specified jobj. maybe jobj tree is broken.\n");
	return NULL;
}

/**
 * @note Address: 0x801C7014
 * @note Size: 0x2C
 */
HSD_DObj* HSD_JObjGetDObj(HSD_JObj* jobj)
{
	if (jobj == NULL || !union_type_dobj(jobj)) {
		return NULL;
	}
	return jobj->u.dobj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_RObj* robj_set_next(HSD_RObj* robj, HSD_RObj* next)
{
	if (robj == NULL) {
		return next;
	}
	robj->next = next;
	return robj;
}

/**
 * @note Address: N/A
 * @note Size: 0x30
 */
void HSD_JObjPrependRObj(HSD_JObj* jobj, HSD_RObj* robj)
{
	if (jobj == NULL || robj == NULL) {
		return;
	}
	jobj->robj = robj_set_next(robj, jobj->robj);
}

/**
 * @note Address: N/A
 * @note Size: 0x50
 */
void HSD_JObjDeleteRObj(HSD_JObj* jobj, HSD_RObj* robj)
{
	HSD_RObj** cur_ptr;
	HSD_RObj* cur;

	if (jobj == NULL || robj == NULL) {
		return;
	}
	if (robj != NULL) {
		cur_ptr = &jobj->robj;
		while (*cur_ptr != NULL) {
			cur = *cur_ptr;
			if (cur == robj) {
				*cur_ptr   = cur->next;
				robj->next = NULL;
				return;
			}
			cur_ptr = &cur->next;
		}
	}
}

/**
 * @note Address: 0x801C7040
 * @note Size: 0x18
 */
u32 HSD_JObjGetFlags(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		return jobj->flags;
	}
	return 0;
}

/**
 * @note Address: 0x801C7058
 * @note Size: 0x9C
 */
void HSD_JObjSetFlags(HSD_JObj* jobj, u32 flags)
{
	if (jobj != NULL) {
		if ((jobj->flags ^ flags) & CLASSICAL_SCALE) {
			HSD_JObjSetMtxDirty(jobj);
		}
		jobj->flags |= flags;
	}
}

/**
 * @note Address: 0x801C70F4
 * @note Size: 0x2D8
 */
void HSD_JObjSetFlagsAll(HSD_JObj* jobj, u32 flags)
{
	HSD_JObj* i;

	if (jobj != NULL) {
		HSD_JObjSetFlags(jobj, flags);
		if (!(jobj->flags & INSTANCE)) {
			for (i = jobj->child; i != NULL; i = i->next) {
				HSD_JObjSetFlagsAll(i, flags);
			}
		}
	}
}

/**
 * @note Address: 0x801C73CC
 * @note Size: 0x9C
 */
void HSD_JObjClearFlags(HSD_JObj* jobj, u32 flags)
{
	if (jobj != NULL) {
		if ((jobj->flags ^ flags) & CLASSICAL_SCALE) {
			HSD_JObjSetMtxDirty(jobj);
		}
		jobj->flags &= ~flags;
	}
}

/**
 * @note Address: 0x801C7468
 * @note Size: 0x2DC
 */
void HSD_JObjClearFlagsAll(HSD_JObj* jobj, u32 flags)
{
	HSD_JObj* i;

	if (jobj != NULL) {
		HSD_JObjClearFlags(jobj, flags);
		if (!(jobj->flags & INSTANCE)) {
			for (i = jobj->child; i != NULL; i = i->next) {
				HSD_JObjClearFlagsAll(i, flags);
			}
		}
	}
}

/**
 * @note Address: 0x801C7744
 * @note Size: 0xAC
 */
void HSD_JObjModifyFlags(HSD_JObj* jobj, u32 flags, u32 mask)
{
	if (jobj != NULL) {
		flags &= mask;
		if ((jobj->flags ^ flags) & CLASSICAL_SCALE) {
			HSD_JObjSetMtxDirty(jobj);
		}
		jobj->flags = flags | (jobj->flags & ~mask);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x278
 */
void HSD_JObjModifyFlagsAll(HSD_JObj* jobj, u32 flags, u32 mask)
{
	HSD_JObj* i;

	if (jobj != NULL) {
		HSD_JObjModifyFlags(jobj, flags, mask);
		if (!(jobj->flags & INSTANCE)) {
			for (i = jobj->child; i != NULL; i = i->next) {
				HSD_JObjModifyFlagsAll(i, flags, mask);
			}
		}
	}
}

/**
 * @note Address: 0x801C77F0
 * @note Size: 0x5C
 */
HSD_JObj* HSD_JObjAlloc(void)
{
	HSD_JObj* jobj = (HSD_JObj*)hsdNew(default_class != NULL ? default_class : HSD_CLASS_INFO(&hsdJObj));
	assert_line(0x7D6, jobj);
	return jobj;
}

/**
 * @note Address: 0x801C784C
 * @note Size: 0x190
 */
void HSD_JObjSetCurrent(HSD_JObj* jobj)
{
	HSD_JObjRef(jobj);
	HSD_JObjUnref(current_jobj);
	current_jobj = jobj;
}

/**
 * @note Address: 0x801C79DC
 * @note Size: 0x8
 */
HSD_JObj* HSD_JObjGetCurrent(void)
{
	return current_jobj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_JObj* jobj_get_joint2(HSD_JObj* jobj)
{
	while (jobj != NULL) {
		if ((jobj->flags & JOBJ_JOINT) == JOINT2) {
			return jobj;
		}
		jobj = jobj->next;
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_JObj* jobj_get_effector(HSD_JObj* jobj)
{
	while (jobj != NULL) {
		if ((jobj->flags & JOBJ_JOINT) == EFFECTOR) {
			return jobj;
		}
		jobj = jobj->next;
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static HSD_JObj* jobj_get_effector_checked(HSD_JObj* eff)
{
	eff = jobj_get_effector(eff);
	assert_line(0x824, eff);
	if (HSD_RObjGetByType(eff->robj, REFTYPE_JOBJ, ROBJ_JOINT_POSITION) != NULL) {
		return eff;
	} else {
		return NULL;
	}
}

/**
 * @note Address: 0x801C79E4
 * @note Size: 0x7F0
 */
static void resolveIKJoint1(HSD_JObj* jobj)
{
	f32 y_len, y_scale;
	HSD_JObj* eff;
	f32 rot_x;
	f32 bone1_len;
	f32 bone2_len    = 0.0F;
	s32 flip         = 0;
	Vec scl          = { 1.0F, 1.0F, 1.0F };
	HSD_JObj* joint2 = jobj_get_joint2(jobj->child);
	Vec root_pos     = { 0.0F, 0.0F, 0.0F };
	Vec* jobj_scl;
	HSD_RObj* robj;
	Vec bone_dir;
	Vec to_effector;
	Vec unit_normal;
	Vec unit_bend;
	Vec dir;
	Vec bend;
	Vec normal;
	Mtx rotmtx;
	f32 dist2;
	f32 bone1_sq, bone2_sq;
	f32 diff, chord2;
	f32 y2;
	f32 inv_length;

	jobj_scl = jobj->scl;
	if (jobj_scl != NULL) {
		scl = *jobj_scl;
	}
	robj = HSD_RObjGetByType(jobj->robj, REFTYPE_IKHINT, 0);
	assert_line(0x853, robj);
	rot_x     = robj->u.ik_hint.rotate_x;
	bone1_len = robj->u.ik_hint.bone_length * scl.x;
	if (joint2 != NULL) {
		robj = HSD_RObjGetByType(joint2->robj, REFTYPE_IKHINT, 0);
		assert_line(0x85E, robj);
		bone2_len = robj->u.ik_hint.bone_length * joint2->scale.x * scl.x;
		flip      = (robj->flags & ROBJ_IK_FLIP) ? 1 : 0;
		eff       = jobj_get_effector_checked(joint2->child);
	} else {
		eff = jobj_get_effector_checked(jobj->child);
	}
	if (eff != NULL) {
		if (HSD_RObjGetByType(jobj->robj, REFTYPE_JOBJ, ROBJ_JOINT_UP) == NULL && jobj != NULL) {
			if (jobj->robj != NULL) {
				HSD_RObjUpdateAll(jobj->robj, jobj, HSD_JOBJ_METHOD(jobj)->update);
				if (HSD_JObjMtxIsDirty(jobj)) {
					HSD_JOBJ_METHOD(jobj)->make_mtx(jobj);
					jobj->flags &= ~MTX_DIRTY;
				}
			}
		}
		if (jobj->parent != NULL) {
			HSD_MtxGetTranslate(jobj->parent->mtx, &root_pos);
		}
		HSD_RObjGetGlobalPosition(eff->robj, ROBJ_JOINT_POSITION, &eff->translate);
		PSVECSubtract(&eff->translate, &root_pos, &to_effector);
		dist2 = PSVECDotProduct(&to_effector, &to_effector);

		if (dist2 > JOBJ_IK_MIN_DISTANCE_SQ) {
			dir = to_effector;
			if (HSD_RObjGetGlobalPosition(jobj->robj, ROBJ_JOINT_UP, &bend)) {
				PSVECSubtract(&bend, &root_pos, &bend);
				if (rot_x != 0.0F) {
					PSMTXRotAxisRad(rotmtx, &dir, rot_x);
					PSMTXMultVec(rotmtx, &bend, &bend);
				}
				PSVECCrossProduct(&dir, &bend, &normal);
				PSVECCrossProduct(&normal, &dir, &bend);
			} else {
				normal.x = jobj->mtx[0][2];
				normal.y = jobj->mtx[1][2];
				normal.z = jobj->mtx[2][2];
				PSVECCrossProduct(&normal, &dir, &bend);
				PSVECCrossProduct(&dir, &bend, &normal);
			}
			inv_length = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&normal, &normal)));
			PSVECScale(&normal, &unit_normal, inv_length);
			inv_length = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&bend, &bend)));
			PSVECScale(&bend, &unit_bend, inv_length);
			bone1_sq = bone1_len * bone1_len;
			bone2_sq = bone2_len * bone2_len;
			diff     = bone1_sq - bone2_sq;
			chord2   = 0.25F * (__fmsubs(2.0F, bone1_sq + bone2_sq, dist2) - ((diff * diff) / dist2));
			if (chord2 < 0.0F) {
				chord2 = 0.0F;
			}
			y2         = (bone1_sq - chord2) / dist2;
			inv_length = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + y2));
			y_len      = y2 * inv_length;
			inv_length = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + chord2));
			y_scale    = chord2 * inv_length;
		} else {
			y_len   = 0.0F;
			y_scale = bone1_len;
		}
		if (flip != 0) {
			y_scale = -y_scale;
		}
		if ((bone2_sq - chord2) < dist2) {
			PSVECScale(&to_effector, &bone_dir, y_len);
		} else {
			PSVECScale(&to_effector, &bone_dir, -y_len);
		}
		PSVECScale(&unit_bend, &bend, y_scale);
		PSVECAdd(&bone_dir, &bend, &bone_dir);
		inv_length = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&bone_dir, &bone_dir)));
		PSVECScale(&bone_dir, &bone_dir, inv_length);
		jobj->mtx[0][0] = bone_dir.x * scl.x;
		jobj->mtx[1][0] = bone_dir.y * scl.x;
		jobj->mtx[2][0] = bone_dir.z * scl.x;
		PSVECCrossProduct(&unit_normal, &bone_dir, &bend);
		jobj->mtx[0][1] = bend.x * scl.y;
		jobj->mtx[1][1] = bend.y * scl.y;
		jobj->mtx[2][1] = bend.z * scl.y;
		jobj->mtx[0][2] = unit_normal.x * scl.z;
		jobj->mtx[1][2] = unit_normal.y * scl.z;
		jobj->mtx[2][2] = unit_normal.z * scl.z;
		jobj->mtx[0][3] = root_pos.x;
		jobj->mtx[1][3] = root_pos.y;
		jobj->mtx[2][3] = root_pos.z;
	}
}

static const Vec sJObjIKUnitScale = { 1.0F, 1.0F, 1.0F };

/**
 * @note Address: 0x801C81D4
 * @note Size: 0x55C
 */
static void resolveIKJoint2(HSD_JObj* jobj)
{
	Vec scl = sJObjIKUnitScale;
	Vec up;
	Vec parent_pos;
	Vec parent_x;
	Vec left;
	Vec dir;
	Mtx rotmtx;
	Vec parent_x2;
	Vec parent_z;
	HSD_JObj* eff;
	f32 angle;
	f32 bone_scale;
	HSD_RObj* limit_lo;
	HSD_RObj* limit_hi;
	s32 limited;
	s32 flip;
	HSD_RObj* robj;
	f32 x_scale;
	MtxPtr mtx;

	bone_scale = 1.0F;
	eff        = jobj_get_effector_checked(jobj->child);
	if (eff == NULL || jobj->parent == NULL) {
		return;
	}
	if (jobj->scl != NULL) {
		scl = *jobj->scl;
	}
	mtx          = jobj->parent->mtx;
	parent_pos.x = mtx[0][3];
	parent_pos.y = mtx[1][3];
	parent_pos.z = mtx[2][3];
	mtx          = jobj->parent->mtx;
	parent_x.x   = mtx[0][0];
	parent_x.y   = mtx[1][0];
	parent_x.z   = mtx[2][0];
	x_scale      = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&parent_x, &parent_x)));
	PSVECScale(&parent_x, &parent_x, x_scale);
	if (jobj->parent->scl != NULL) {
		bone_scale = jobj->parent->scl->x;
	}
	robj = HSD_RObjGetByType(jobj->parent->robj, REFTYPE_IKHINT, 0);
	assert_line(0x8FC, robj);
	PSVECScale(&parent_x, &parent_x, robj->u.ik_hint.bone_length * bone_scale);
	PSVECAdd(&parent_pos, &parent_x, &up);
	PSVECSubtract(&eff->translate, &up, &parent_x);
	PSVECScale(&parent_x, &parent_x, dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&parent_x, &parent_x))));
	limit_lo = HSD_RObjGetByType(jobj->robj, REFTYPE_LIMIT, ROBJ_LIMIT_ROT_Z_MIN);
	limit_hi = HSD_RObjGetByType(jobj->robj, REFTYPE_LIMIT, ROBJ_LIMIT_ROT_Z_MAX);
	if (limit_lo != NULL || limit_hi != NULL) {
		limited = 0;
		robj    = HSD_RObjGetByType(jobj->robj, REFTYPE_IKHINT, 0);
		assert_line(0x91E, robj);
		flip        = (robj->flags & ROBJ_IK_FLIP) ? 1 : 0;
		mtx         = jobj->parent->mtx;
		parent_x2.x = mtx[0][0];
		parent_x2.y = mtx[1][0];
		parent_x2.z = mtx[2][0];
		PSVECNormalize(&parent_x2, &parent_x2);
		x_scale = PSVECDotProduct(&parent_x2, &parent_x);
		{
			f32 a;

			if (x_scale >= 1.0F) {
				a = 0.0F;
			} else if (x_scale <= -1.0F) {
				a = M_PI;
			} else {
				a = acos(x_scale);
			}
			angle = a;
			if (flip == 0) {
				angle = -a;
			}
		}
		if (limit_lo != NULL && angle < limit_lo->u.limit) {
			angle   = limit_lo->u.limit;
			limited = 1;
		} else if (limit_hi != NULL) {
			if (limit_hi->u.limit < angle) {
				angle   = limit_hi->u.limit;
				limited = 1;
			}
		}
		if (limited != 0) {
			mtx        = jobj->parent->mtx;
			parent_z.x = mtx[0][2];
			parent_z.y = mtx[1][2];
			parent_z.z = mtx[2][2];
			PSMTXRotAxisRad(rotmtx, &parent_z, angle);
			PSMTXMultVec(rotmtx, &parent_x2, &parent_x);
		}
	}
	mtx   = jobj->parent->mtx;
	dir.x = mtx[0][2];
	dir.y = mtx[1][2];
	dir.z = mtx[2][2];
	PSVECCrossProduct(&dir, &parent_x, &left);
	x_scale = dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&left, &left)));
	PSVECScale(&left, &left, x_scale);
	PSVECCrossProduct(&parent_x, &left, &dir);
	jobj->mtx[0][0] = parent_x.x * scl.x;
	jobj->mtx[1][0] = parent_x.y * scl.x;
	jobj->mtx[2][0] = parent_x.z * scl.x;
	jobj->mtx[0][1] = left.x * scl.y;
	jobj->mtx[1][1] = left.y * scl.y;
	jobj->mtx[2][1] = left.z * scl.y;
	jobj->mtx[0][2] = dir.x * scl.z;
	jobj->mtx[1][2] = dir.y * scl.z;
	jobj->mtx[2][2] = dir.z * scl.z;
	jobj->mtx[0][3] = up.x;
	jobj->mtx[1][3] = up.y;
	jobj->mtx[2][3] = up.z;
}

/**
 * @note Address: 0x801C8730
 * @note Size: 0x278
 */
void HSD_JObjSetupMatrixSub(HSD_JObj* jobj)
{
	Vec p1;
	Vec p2;
	Vec p3;
	HSD_JObj* parent;
	HSD_RObj* robj;
	f32 x_scale;

	HSD_JOBJ_METHOD(jobj)->make_mtx(jobj);
	jobj->flags &= ~MTX_DIRTY;
	if (!(jobj->flags & USER_DEF_MTX)) {
		switch (jobj->flags & JOBJ_JOINT) {
		case JOINT1:
			resolveIKJoint1(jobj);
			break;
		case JOINT2:
			resolveIKJoint2(jobj);
			break;
		case EFFECTOR:
			parent  = jobj->parent;
			x_scale = 1.0F;
			if (parent != NULL) {
				robj = HSD_RObjGetByType(parent->robj, REFTYPE_IKHINT, 0);
				if (robj != NULL) {
					p2.x = parent->mtx[0][3];
					p2.y = parent->mtx[1][3];
					p2.z = parent->mtx[2][3];
					p3.x = parent->mtx[0][0];
					p3.y = parent->mtx[1][0];
					p3.z = parent->mtx[2][0];
					PSVECScale(&p3, &p3, dolsqrtf(1.0F / (JOBJ_IK_NORM_EPSILON + PSVECDotProduct(&p3, &p3))));
					if (parent->scl != NULL) {
						x_scale = parent->scl->x;
					}
					PSVECScale(&p3, &p3, robj->u.ik_hint.bone_length * x_scale);
					PSVECAdd(&p2, &p3, &p1);
					jobj->mtx[0][3] = p1.x;
					jobj->mtx[1][3] = p1.y;
					jobj->mtx[2][3] = p1.z;
				}
			}
			break;
		default:
			if (jobj->robj != NULL && jobj != NULL && jobj->robj != NULL) {
				HSD_RObjUpdateAll(jobj->robj, jobj, HSD_JOBJ_METHOD(jobj)->update);
				if (HSD_JObjMtxIsDirty(jobj)) {
					HSD_JOBJ_METHOD(jobj)->make_mtx(jobj);
					jobj->flags &= ~MTX_DIRTY;
				}
			}
			break;
		}
		jobj->flags &= ~MTX_DIRTY;
	}
}

/**
 * @note Address: 0x801C89A8
 * @note Size: 0x334
 */
void HSD_JObjSetMtxDirtySub(HSD_JObj* jobj)
{
	HSD_JObj* child;

	jobj->flags |= MTX_DIRTY;
	if (!(jobj->flags & INSTANCE)) {
		child = jobj->child;
		while (child != NULL) {
			if (!(child->flags & MTX_INDEP_PARENT)) {
				if (!HSD_JObjMtxIsDirty(child)) {
					HSD_JObjSetMtxDirtySub(child);
				}
			}
			child = child->next;
		}
	}
}

/**
 * @note Address: 0x801C8CDC
 * @note Size: 0x8
 */
void HSD_JObjSetDPtclCallback(HSD_JObjParticleCallback callback)
{
	dptcl_callback = callback;
}

/**
 * @note Address: 0x801C8CE4
 * @note Size: 0x8
 */
void HSD_JObjSetJSoundCallback(void (*callback)(s32))
{
	jsound_callback = callback;
}

/**
 * @note Address: 0x801C8CEC
 * @note Size: 0x8
 */
void HSD_JObjSetPtclTargetCallback(void (*cb)(HSD_JObj*, s32))
{
	ptcltgt_callback = cb;
}

/**
 * @note Address: 0x801C8CF4
 * @note Size: 0x64
 */
static int JObjInit(HSD_Class* o)
{
	int status = HSD_OBJECT_PARENT_INFO(&hsdJObj)->init(o);
	if (status >= 0) {
		HSD_JObj* jobj = (HSD_JObj*)o;
		status         = 0;
		jobj->flags    = MTX_DIRTY;
		jobj->scale.x  = 1.0F;
		jobj->scale.y  = 1.0F;
		jobj->scale.z  = 1.0F;
	}
	return status;
}

/**
 * @note Address: 0x801C8D58
 * @note Size: 0x4D4
 */
static void JObjReleaseChild(HSD_JObj* jobj)
{
	HSD_JObj* child;

	if ((child = jobj->child) != NULL) {
		if (jobj->flags & INSTANCE) {
			HSD_JObjUnref(child);
		} else {
			child->parent = NULL;
			HSD_JObjRemoveAll(jobj->child);
		}
		jobj->child = NULL;
	}
	if (jobj->parent != NULL) {
		HSD_JObjReparent(jobj, NULL);
	}
	if (union_type_dobj(jobj)) {
		if (jobj->u.dobj != NULL) {
			HSD_DObjRemoveAll(jobj->u.dobj);
			jobj->u.dobj = NULL;
		}
	}
	if (jobj->robj != NULL) {
		HSD_RObjRemoveAll(jobj->robj);
		jobj->robj = NULL;
	}
	if (jobj->aobj != NULL) {
		HSD_AObjRemove(jobj->aobj);
		jobj->aobj = NULL;
	}
}

/**
 * @note Address: 0x801C922C
 * @note Size: 0x98
 */
static void JObjRelease(HSD_Class* o)
{
	HSD_JObj* jobj = (HSD_JObj*)o;

	HSD_JOBJ_METHOD(jobj)->release_child(jobj);

	if (HSD_IDGetDataFromTable(NULL, jobj->id, NULL) == jobj) {
		HSD_IDRemoveByIDFromTable(NULL, jobj->id);
	}
	if (jobj->scl != NULL) {
		HSD_VecFree(jobj->scl);
	}
	if (jobj->envelopemtx != NULL) {
		HSD_MtxFree(jobj->envelopemtx);
	}
	HSD_OBJECT_PARENT_INFO(&hsdJObj)->release(o);
}

/**
 * @note Address: 0x801C92C4
 * @note Size: 0x64
 */
static void JObjAmnesia(HSD_ClassInfo* info)
{
	if (info == HSD_CLASS_INFO(default_class)) {
		default_class = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdJObj)) {
		ufc_callbacks = NULL;
		current_jobj  = NULL;
	}
	HSD_OBJECT_PARENT_INFO(&hsdJObj)->amnesia(info);
}

/**
 * @note Address: 0x801C9328
 * @note Size: 0xBC
 */
static void JObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdJObj), HSD_CLASS_INFO(&hsdObj), "sysdolphin_base_library", "hsd_jobj", sizeof(HSD_JObjInfo),
	                 sizeof(HSD_JObj));
	HSD_CLASS_INFO(&hsdJObj)->init         = JObjInit;
	HSD_CLASS_INFO(&hsdJObj)->release      = JObjRelease;
	HSD_CLASS_INFO(&hsdJObj)->amnesia      = JObjAmnesia;
	HSD_JOBJ_INFO(&hsdJObj)->make_mtx      = HSD_JObjMakeMatrix;
	HSD_JOBJ_INFO(&hsdJObj)->make_pmtx     = HSD_JObjMakePositionMtx;
	HSD_JOBJ_INFO(&hsdJObj)->disp          = HSD_JObjDispSub;
	HSD_JOBJ_INFO(&hsdJObj)->load          = JObjLoad;
	HSD_JOBJ_INFO(&hsdJObj)->release_child = JObjReleaseChild;
	HSD_JOBJ_INFO(&hsdJObj)->update        = JObjUpdateFunc;
}
