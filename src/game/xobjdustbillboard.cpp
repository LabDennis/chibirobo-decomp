#include "game/xobjflags.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/hsd_cobj.h"
#include <math.h>

// todo: cleanup, move these onto CJArchive in cobj.h with GetAnimJointRoot
static inline HSD_AnimJoint* Joint(CJArchive* archive, s32 id)
{
	if (id >= 0) {
		s32 index = id % 1000;
		if (index < archive->mNum[0]) {
			return archive->mAnimJoint != NULL ? archive->mAnimJoint[index] : NULL;
		}
	}
	return NULL;
}

static inline HSD_MatAnimJoint* Material(CJArchive* archive, s32 id)
{
	if (id >= 0) {
		s32 index = id % 1000;
		if (index < archive->mNum[0]) {
			return archive->mMatAnimJoint != NULL ? archive->mMatAnimJoint[index] : NULL;
		}
	}
	return NULL;
}

static inline HSD_ShapeAnimJoint* Shape(CJArchive* archive, s32 id)
{
	if (id >= 0) {
		s32 index = id % 1000;
		if (index < archive->mNum[0]) {
			return archive->mShapeAnimJoint != NULL ? archive->mShapeAnimJoint[index] : NULL;
		}
	}
	return NULL;
}

// XObjDust::SetAnim, XObjFree's with the material anim looping
/**
 * @note Address: 0x800DE7F4
 * @note Size: 0x2AC
 */
extern "C" void SetAnim__8XObjDustFliPvPvPvffff(XObjDust* object, s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim,
                                                f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (object->mStatus.mFlags & 0x1000ULL) {
		if (force || id != object->mAnimId) {
			object->mAnimId = id;
			void* joint     = jointAnim != NULL ? jointAnim : Joint(&object->mArchive, id);
			void* shape     = shapeAnim != NULL ? shapeAnim : Shape(&object->mArchive, id);
			f32 materialFrame;
			if (object->mMatAnim != NULL) {
				HSD_AObj* animation = FindMaterialOrTextureAnimationInJointTree(object->mJObj);
				materialFrame       = animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
			} else {
				materialFrame    = 0.0f;
				object->mMatAnim = Material(&object->mArchive, 0);
			}
			if (joint != NULL || object->mMatAnim != NULL || shape != NULL) {
				HSD_JObjRemoveAnimAll(object->mJObj);
				HSD_JObjAddAnimAll(object->mJObj, (HSD_AnimJoint*)joint, object->mMatAnim, (HSD_ShapeAnimJoint*)shape);
				HSD_ForeachAnim(object->mJObj, (HSD_Type)6, (HSD_TypeMask)0x484, HSD_AObjSetFlags, (AObj_Arg_Type)3, 0x20000000);
				HSD_JObjReqAnimAllByFlags(object->mJObj, 0x189, frame);
				HSD_JObjReqAnimAllByFlags(object->mJObj, 0x16, materialFrame);
			}
			force = TRUE;
		}
		if (rate != object->mAnimRate || force) {
			object->mAnimRate     = rate;
			object->mLastAnimRate = 0.01f + rate;
		}
	} else {
		SetAnim__4XObjFliPvPvPvffff(object, id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	}
}

// XObjDust::DrawObjectEffects
/**
 * @note Address: 0x800DEAA0
 * @note Size: 0x208
 */
extern "C" void DrawObjectEffects__8XObjDustFv(XObjDust* object)
{
	if (object->GetObjFlags() & 1) {
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		gGame.mGs.mBillboardEnabled = 1;
		f32 height;
		f32 width = 8.0f * object->mMoveScale.x;
		height    = 8.0f * object->mMoveScale.y;
		u32 color;
		if (__fpclassifyf(object->mHeading) != FP_NAN) {
			color = 0xC8C8C800;
		} else if (object->mStatus.mFlags & 0x4000000000000000ULL) {
			color = 0xC8646400;
		} else {
			color = 0xC8C8C800;
		}
		s32 alpha = (s32)(255.0f * object->mFade);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 16, color | (u8)alpha, color, object->mMoveTranslate.x, object->mMoveTranslate.y, object->mMoveTranslate.z,
		             width, height, rotation.x, rotation.y, rotation.z);
		gGame.mGs.mBillboardEnabled = 0;
	}
}

// XObjDust::DrawWithView
/**
 * @note Address: 0x800DECA8
 * @note Size: 0x7C
 */
extern "C" void DrawWithView__8XObjDustFPA4_f(XObjDust* object, Mtx view)
{
	if (object->GetObjFlags() & 2) {
		DrawWithView__4XObjFPA4_f(object, view);
	} else if (object->GetObjFlags() & 4) {
		DrawWithView__4XObjFPA4_f(object, view);
	}
}
