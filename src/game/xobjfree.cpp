#include "game/xobjfree.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/hsd_cobj.h"
#include <math.h>

/**
 * @note Address: 0x800DD600
 * @note Size: 0x94
 */
void XObjFree::OnHitFromPosition(const Vec& pos, f32 value)
{
	XObj::OnHitFromPosition(pos, value);
	if (mStatus.mFlags & 0x04000000ULL) {
		f32 remaining = mLife - value;
		mLife         = remaining;
		if (remaining <= 0.0f)
			mStatus.mFlags |= 0x2000000000000000ULL;
	}
}

// Keeps the material anim running across anim changes
/**
 * @note Address: 0x800DD694
 * @note Size: 0x2D8
 */
void XObjFree::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (mStatus.mFlags & 0x1000ULL) {
		if (force || id != mAnimId) {
			mAnimId     = id;
			void* joint = jointAnim != NULL ? jointAnim : mArchive.GetAnimJointRoot(id);
			void* shape = shapeAnim != NULL ? shapeAnim : mArchive.GetShapeAnimJointRoot(id);
			f32 materialFrame;
			if (mMatAnim != NULL) {
				HSD_AObj* animation = FindMaterialOrTextureAnimationInJointTree(mJObj);
				materialFrame       = animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
			} else {
				materialFrame  = 0.0f;
				s16 materialId = GetObjFlags();
				mMatAnim       = mArchive.GetMatAnimJointRoot(materialId);
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

/**
 * @note Address: 0x800DD96C
 * @note Size: 0x220
 */
void XObjFree::DrawScreenOverlay()
{
	XObj::DrawScreenOverlay();
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
