#include "game/xobjfree.h"
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

// XObjFree::OnHitFromPosition
/**
 * @note Address: 0x800DD600
 * @note Size: 0x94
 */
void XObjFree::OnHitFromPosition(const Vec& pos, f32 value)
{
	OnHitFromPosition__4XObjFRC3Vecf(this, &pos, value);
	if (mStatus.mFlags & 0x04000000ULL) {
		f32 remaining = mLife - value;
		mLife         = remaining;
		if (remaining <= 0.0f)
			mStatus.mFlags |= 0x2000000000000000ULL;
	}
}

// XObjFree::SetAnim, keeps the material anim running across anim changes
/**
 * @note Address: 0x800DD694
 * @note Size: 0x2D8
 */
void XObjFree::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
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
				materialFrame  = 0.0f;
				s16 materialId = GetObjFlags();
				mMatAnim       = Material(&mArchive, materialId);
			}
			if (joint != NULL || mMatAnim != NULL || shape != NULL) {
				HSD_JObjRemoveAnimAll(mJObj);
				HSD_JObjAddAnimAll(mJObj, (HSD_AnimJoint*)joint, mMatAnim, (HSD_ShapeAnimJoint*)shape);
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

// XObjFree::vfC8
/**
 * @note Address: 0x800DD96C
 * @note Size: 0x220
 */
void XObjFree::vfC8()
{
	XObj::vfC8();
	if ((mStatus.mFlags & 0x4000000000000000ULL) && mFlash > 0.0f) {
		Vec position = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		ProjectOffsetToScreen(&position, &x, &y, NULL, 0);
		f32 effect      = mFlash;
		f32 innerRadius = 400.0f * effect;
		f32 outerRadius = 400.0f * (1.0f - effect);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
		u32 color = (u32)(255.0f * effect) | 0xFFFFFF00;
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, innerRadius, innerRadius, 0.0f, 0.0f, 0.0f);
		DrawGsCircle(&gGame.mGs, 16, color, 0xFFFFFF00, x, y, 0.0f, outerRadius, outerRadius, 0.0f, 0.0f, 0.0f);
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	}
}
