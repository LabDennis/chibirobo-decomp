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
void XObjDust::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x1000ULL) {
		if (force || id != mAnimId) {
			mAnimId     = id;
			void* joint = jointAnim != NULL ? jointAnim : Joint(&mArchive, id);
			void* shape = shapeAnim != NULL ? shapeAnim : Shape(&mArchive, id);
			f32 materialFrame;
			if (mMatAnim != NULL) {
				HSD_AObj* animation = FindMaterialOrTextureAnimationInJointTree(mJObj);
				materialFrame       = animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
			} else {
				materialFrame = 0.0f;
				mMatAnim      = Material(&mArchive, 0);
			}
			if (joint != NULL || mMatAnim != NULL || shape != NULL) {
				HSD_JObjRemoveAnimAll(mJObj);
				HSD_JObjAddAnimAll(mJObj, (HSD_AnimJoint*)joint, mMatAnim, (HSD_ShapeAnimJoint*)shape);
				HSD_ForeachAnim(mJObj, (HSD_Type)6, (HSD_TypeMask)0x484, HSD_AObjSetFlags, (AObj_Arg_Type)3, 0x20000000);
				HSD_JObjReqAnimAllByFlags(mJObj, 0x189, frame);
				HSD_JObjReqAnimAllByFlags(mJObj, 0x16, materialFrame);
			}
			force = TRUE;
		}
		if (rate != mAnimRate || force) {
			mAnimRate     = rate;
			mLastAnimRate = 0.01f + rate;
		}
	} else {
		XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	}
}

// XObjDust::DrawObjectEffects
/**
 * @note Address: 0x800DEAA0
 * @note Size: 0x208
 */
void XObjDust::DrawObjectEffects()
{
	if (GetObjFlags() & 1) {
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		gGame.mGs.mBillboardEnabled = 1;
		f32 height;
		f32 width = 8.0f * mMoveScale.x;
		height    = 8.0f * mMoveScale.y;
		u32 color;
		if (__fpclassifyf(mHeading) != FP_NAN) {
			color = 0xC8C8C800;
		} else if (mStatus.mFlags & 0x4000000000000000ULL) {
			color = 0xC8646400;
		} else {
			color = 0xC8C8C800;
		}
		s32 alpha = (s32)(255.0f * mFade);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		Vec rotation = { 0.0f, 0.0f, 0.0f };
		DrawGsCircle(&gGame.mGs, 16, color | (u8)alpha, color, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z, width, height,
		             rotation.x, rotation.y, rotation.z);
		gGame.mGs.mBillboardEnabled = 0;
	}
}

// XObjDust::DrawWithView
/**
 * @note Address: 0x800DECA8
 * @note Size: 0x7C
 */
void XObjDust::DrawWithView(Mtx view)
{
	if (GetObjFlags() & 2) {
		DrawWithView__4XObjFPA4_f(this, view);
	} else if (GetObjFlags() & 4) {
		DrawWithView__4XObjFPA4_f(this, view);
	}
}
