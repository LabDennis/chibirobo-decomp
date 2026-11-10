#ifndef _GAME_HSD_JOBJ_H
#define _GAME_HSD_JOBJ_H

// c++ jobj.h inlines, kept apart from the c header's copies. the assert file and
// line numbers are typed in from the binary, __FILE__ would give this header
// todo: copied inlines, clean up

// c++ callers test the dirty flag as a bool, the c header returns BOOL
#define HSD_JObjMtxIsDirty  HSD_JObjMtxIsDirty_c
#define HSD_JObjSetupMatrix HSD_JObjSetupMatrix_c
#define HSD_JObjSetMtxDirty HSD_JObjSetMtxDirty_c
extern "C" {
#include <dolphin/mtx/mtx.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/mtx_transform.h>
}
#undef HSD_JObjMtxIsDirty
#undef HSD_JObjSetupMatrix
#undef HSD_JObjSetMtxDirty

// HSD_JObjWalkTree passes the flag mask in its callback argument.
extern "C" {
void JObjSetMObjFlags(HSD_JObj* jobj, f32** flags, s32 type);
void JObjClearMObjFlags(HSD_JObj* jobj, f32** flags, s32 type);
void JObjSetPObjFlags(HSD_JObj* jobj, f32** flags, s32 type);
void JObjClearPObjFlags(HSD_JObj* jobj, f32** flags, s32 type);
}

#ifndef JOBJ_H_ASSERT
#define JOBJ_H_ASSERT(line, cond) ((cond) ? ((void)0) : __assert("jobj.h", line, #cond))
#endif
#ifndef JOBJ_H_ASSERT_TEXT
#define JOBJ_H_ASSERT_TEXT(line, cond, text) ((cond) ? ((void)0) : __assert("jobj.h", line, text))
#endif

#ifndef JOBJ_EULER_CONDITION
#define JOBJ_EULER_CONDITION "!(jobj->flags & JOBJ_USE_QUATERNION)"
#endif
#ifndef JOBJ_TRANSLATION_CONDITION
#define JOBJ_TRANSLATION_CONDITION "translate"
#endif

#ifndef JOBJ_H_ASSERT_JOINT
#define JOBJ_H_ASSERT_JOINT(line, joint) JOBJ_H_ASSERT(line, joint)
#endif

#ifndef JOBJ_H_ASSERT_ROTATION
#define JOBJ_H_ASSERT_ROTATION(line, rotate) JOBJ_H_ASSERT(line, rotate)
#endif

inline bool HSD_JObjMtxIsDirty(HSD_JObj* jobj)
{
	bool result;
	JOBJ_H_ASSERT_JOINT(605, jobj);
	result = false;
	if (!(jobj->flags & USER_DEF_MTX) && (jobj->flags & MTX_DIRTY)) {
		result = true;
	}
	return result;
}

inline void HSD_JObjSetupMatrix(HSD_JObj* jobj)
{
	if (jobj != NULL && HSD_JObjMtxIsDirty(jobj)) {
		HSD_JObjSetupMatrixSub(jobj);
	}
}

inline void HSD_JObjSetMtxDirty(HSD_JObj* jobj)
{
	if (jobj == NULL || HSD_JObjMtxIsDirty(jobj)) {
		return;
	}
	HSD_JObjSetMtxDirtySub(jobj);
}

inline void HSD_JObjSetRotationX(HSD_JObj* jobj, f32 x)
{
	JOBJ_H_ASSERT(676, jobj);
	JOBJ_H_ASSERT_TEXT(677, !(jobj->flags & USE_QUATERNION), JOBJ_EULER_CONDITION);
	jobj->rotate.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetRotationY(HSD_JObj* jobj, f32 y)
{
	JOBJ_H_ASSERT(696, jobj);
	JOBJ_H_ASSERT_TEXT(697, !(jobj->flags & USE_QUATERNION), JOBJ_EULER_CONDITION);
	jobj->rotate.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetRotationZ(HSD_JObj* jobj, f32 z)
{
	JOBJ_H_ASSERT(716, jobj);
	JOBJ_H_ASSERT_TEXT(717, !(jobj->flags & USE_QUATERNION), JOBJ_EULER_CONDITION);
	jobj->rotate.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjGetRotation(HSD_JObj* jobj, Quaternion* rotate)
{
	JOBJ_H_ASSERT_JOINT(733, jobj);
	JOBJ_H_ASSERT_ROTATION(734, rotate);
	*rotate = jobj->rotate;
}

inline f32 HSD_JObjGetRotationX(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(748, jobj);
	return jobj->rotate.x;
}

inline f32 HSD_JObjGetRotationY(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(762, jobj);
	return jobj->rotate.y;
}

inline f32 HSD_JObjGetRotationZ(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(776, jobj);
	return jobj->rotate.z;
}

inline void HSD_JObjSetScaleX(HSD_JObj* jobj, f32 x)
{
	JOBJ_H_ASSERT(805, jobj);
	jobj->scale.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetScaleY(HSD_JObj* jobj, f32 y)
{
	JOBJ_H_ASSERT(819, jobj);
	jobj->scale.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetScaleZ(HSD_JObj* jobj, f32 z)
{
	JOBJ_H_ASSERT(833, jobj);
	jobj->scale.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjGetScale(HSD_JObj* jobj, Vec* scale)
{
	JOBJ_H_ASSERT(849, jobj);
	JOBJ_H_ASSERT(850, scale);
	*scale = jobj->scale;
}

inline f32 HSD_JObjGetScaleX(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(899, jobj);
	return jobj->scale.x;
}

inline f32 HSD_JObjGetScaleY(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(911, jobj);
	return jobj->scale.y;
}

inline f32 HSD_JObjGetScaleZ(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(923, jobj);
	return jobj->scale.z;
}

inline void HSD_JObjSetTranslateX(HSD_JObj* jobj, f32 x)
{
	JOBJ_H_ASSERT(952, jobj);
	jobj->translate.x = x;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetTranslateY(HSD_JObj* jobj, f32 y)
{
	JOBJ_H_ASSERT(966, jobj);
	jobj->translate.y = y;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetTranslateZ(HSD_JObj* jobj, f32 z)
{
	JOBJ_H_ASSERT(980, jobj);
	jobj->translate.z = z;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline MtxPtr HSD_JObjGetMtxPtr(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(1148, jobj);
	HSD_JObjSetupMatrix(jobj);
	return jobj->mtx;
}

inline void HSD_JObjCopyMtx(HSD_JObj* jobj, Mtx mtx)
{
	JOBJ_H_ASSERT(1173, jobj);
	JOBJ_H_ASSERT(1174, mtx);
	PSMTXCopy(mtx, jobj->mtx);
	jobj->flags |= USER_DEF_MTX | MTX_INDEP_PARENT | MTX_INDEP_SRT;
	HSD_JObjSetMtxDirty(jobj);
}

inline void HSD_JObjClearMtxFlags(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT_JOINT(1199, jobj);
	jobj->flags &= ~(USER_DEF_MTX | MTX_INDEP_PARENT | MTX_INDEP_SRT);
	HSD_JObjSetMtxDirty(jobj);
}

inline void HSD_JObjGetTranslation(HSD_JObj* jobj, Vec* translate)
{
	JOBJ_H_ASSERT(996, jobj);
	JOBJ_H_ASSERT_TEXT(997, translate, JOBJ_TRANSLATION_CONDITION);
	*translate = jobj->translate;
}

inline f32 HSD_JObjGetTranslationX(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(1009, jobj);
	return jobj->translate.x;
}

inline f32 HSD_JObjGetTranslationY(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(1021, jobj);
	return jobj->translate.y;
}

inline f32 HSD_JObjGetTranslationZ(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(1033, jobj);
	return jobj->translate.z;
}

inline void HSD_JObjSetRotation(HSD_JObj* jobj, Quaternion* rotate)
{
	JOBJ_H_ASSERT_JOINT(656, jobj);
	JOBJ_H_ASSERT_ROTATION(657, rotate);
	jobj->rotate = *rotate;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetScale(HSD_JObj* jobj, Vec* scale)
{
	JOBJ_H_ASSERT(790, jobj);
	JOBJ_H_ASSERT(791, scale);
	jobj->scale = *scale;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline void HSD_JObjSetTranslate(HSD_JObj* jobj, Vec* translate)
{
	JOBJ_H_ASSERT(937, jobj);
	JOBJ_H_ASSERT_TEXT(938, translate, JOBJ_TRANSLATION_CONDITION);
	jobj->translate = *translate;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

inline HSD_JObj* HSD_JObjGetChild(HSD_JObj* jobj)
{
	if (jobj == NULL) {
		return NULL;
	}
	return jobj->child;
}

inline HSD_JObj* HSD_JObjGetNext(HSD_JObj* jobj)
{
	if (jobj == NULL) {
		return NULL;
	}
	return jobj->next;
}

// walks child then next the given number of times per level
inline HSD_JObj* GetJointByPath(HSD_JObj* jobj, s32* path, s32 depth)
{
	s32 i;
	s32 j;
	if (jobj == NULL) {
		return NULL;
	}
	for (i = 0; i < depth; i++) {
		jobj = HSD_JObjGetChild(jobj);
		for (j = 0; j < path[i]; j++) {
			jobj = HSD_JObjGetNext(jobj);
			if (jobj == NULL) {
				return NULL;
			}
		}
	}
	return jobj;
}

// world position out of the joint's matrix
inline Vec GetJointPosition(HSD_JObj* jobj)
{
	Vec position;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(jobj), &position);
	return position;
}

// same walk stepping the path pointer, the joint helpers with a fixed depth use this one
inline HSD_JObj* FindJointByPath(HSD_JObj* jobj, s32* path, s32 depth)
{
	if (!jobj) {
		return NULL;
	}
	for (s32 i = 0; i < depth; i++, path++) {
		jobj = HSD_JObjGetChild(jobj);
		for (s32 j = 0; j < *path; j++) {
			jobj = HSD_JObjGetNext(jobj);
			if (!jobj) {
				return NULL;
			}
		}
	}
	return jobj;
}

#endif
