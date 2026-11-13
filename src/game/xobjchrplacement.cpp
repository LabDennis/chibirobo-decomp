#include "game/xchr.h"
#include "game/cgame.h"
#include "game/xobjmap.h"

extern "C" {
CVec GetPlayerJointLocalTranslationByPath(CJObj* object, s32 depth, s32* path);
void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z);
BOOL IsStageDataLoaded();
void EnableJointMaterialTranslucency(HSD_JObj* joint, f32** args, s32 type);
void ShowIdlePlayerCopter(XObjCopter* copter);
void ShowIdlePlayerRadar(XObjRadar* radar);
}

static inline void SamplePlayerPose(XObjChr* player, s32 anim)
{
	player->SetAnim(anim, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	if (anim == 0x13) {
		HSD_AObj* jointAnim = GetAObj(player->mJObj);
		f32 end             = jointAnim != NULL ? HSD_AObjGetEndFrame(jointAnim) : 0.0f;
		player->SetAnim(anim, TRUE, NULL, NULL, NULL, end, 1.0f, 1.0f, 10.0f);
	}
	player->UpdateAnim(0);
}

static inline void SampleRootOffset(XObjChr* player, s32 anim, CVec& offset)
{
	SamplePlayerPose(player, anim);
	s32 root[] = { 0 };
	offset.Set(GetPlayerJointLocalTranslationByPath(player, 1, root));
	SetObjectJointTranslationByPath(player, 1, root, 0.0f, 0.0f, 0.0f);
}

static inline BOOL LoadPlayerModel(XObj& model, s32 id)
{
	return model.LoadFromPlacement(CoordInfo(GetSpawnData(id)));
}

static inline void SetAttachmentParent(XObjChr* player, XObjAttachment& attachment, s32* path, int depth)
{
	attachment.SetParent(FindJointByPath(player->mJObj, path, depth), TRUE);
}

/**
 * @note Address: 0x800B97FC
 * @note Size: 0x1584
 */
BOOL XObjChr::LoadFromPlacement(const CoordInfo& info)
{
	u32 status  = (u32)mStatus.mFlags;
	u32 flags[] = { status & 0x800000, status & 0x200000, status & 0x40, status & 0x20000, status & 0x400000 };
	// Each ready model falls through to the next load. A pending one resumes at the same step.
	switch (mLoadStep) {
	case 0: {
		if (!LoadFromPlacement__4XObjFRC9CoordInfo(this, &info))
			break;
		CJObj::SetRotationY(0.0f, TRUE);
		SamplePlayerPose(this, 8);
		mLift._10.Set(GetPlayerCarryJointLocalTranslation(this));
		SampleRootOffset(this, 0x13, mHang._10);
		SampleRootOffset(this, 0x8A, mRope._10);
		SamplePlayerPose(this, 0x36);
		s32 tumble[]          = { 0, 1, 0, 1, 2, 0, 1 };
		mTumble.mFade.mOffset = GetEventJointWorldPosition(this, 7, tumble) - mMoveTranslate;
		mTumble.mFade.mActive = FALSE;
		mTumble.mFade.mAmount = 0.0f;
		CJObj::SetRotationY(mMoveRotate.y, TRUE);
		SetAnim(0x11, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		mStandTimer = 3;
		{
			s32 branch1Child0[] = { 0, 1, 0, 1, 2, 0, 0, 1, 0 };
			s32 branch1Child1[] = { 0, 1, 0, 1, 2, 0, 0, 1, 1 };
			s32 branch3Child0[] = { 0, 1, 0, 1, 2, 0, 0, 3, 0 };
			s32 branch3Child1[] = { 0, 1, 0, 1, 2, 0, 0, 3, 1 };
			struct JointPath {
				s32 depth;
				s32* nodes;
			};
			JointPath joints[] = { { 9, branch1Child0 }, { 9, branch1Child1 }, { 9, branch3Child0 }, { 9, branch3Child1 } };
			for (int i = 0; i < 4; ++i) {
				XObjChrIdleJoint& idle = mIdleJoints[i];
				idle.mJObj             = FindJointByPath(mJObj, joints[i].nodes, joints[i].depth);
				idle.mAnimJoint        = mArchive.GetAnimJoint(14, joints[i].depth, joints[i].nodes);
				idle.mLastAdHead       = NULL;
			}
			s32 attachment[]     = { 0, 1, 0, 1, 2, 0, 0, 5 };
			mAttachmentJoint     = FindJointByPath(mJObj, attachment, 8);
			s32 animationPath[]  = { 0, 1, 0, 1, 2, 0, 0, 5 };
			mAttachmentJointAnim = mArchive.GetAnimJoint(0x4A, 8, animationPath);
			if (!mSuits.mJoints[0]) {
				s32 suit[]        = { 0, 1, 0, 0 };
				mSuits.mJoints[0] = FindJointByPath(mJObj, suit, 4);
			}
		}
		mStatus.mFlags |= 0x0000200000000000ULL;
		++mLoadStep;
	}
	case 1:
		if (!LoadPlayerModel(mPlug, 15))
			break;
		++mLoadStep;
	case 2: {

		if (!LoadPlayerModel(mCopterObj, 17))
			break;
		s32 path[] = { 0, 1, 0, 1, 2, 0, 0, 4 };
		SetAttachmentParent(this, mCopterObj, path, 8);
		++mLoadStep;
	}
	case 3: {

		if (!LoadPlayerModel(mPopperObj, 18))
			break;
		s32 path[] = { 0, 1, 0, 1, 3 };
		SetAttachmentParent(this, mPopperObj, path, 5);
		++mLoadStep;
	}
	case 4: {

		if (!LoadPlayerModel(mRadarObj, 19))
			break;
		s32 path[] = { 0, 1, 0, 1, 2, 0, 0, 4 };
		SetAttachmentParent(this, mRadarObj, path, 8);
		++mLoadStep;
	}
	case 5: {

#if defined(VERSION_JP)
		const s32 modelId = 20;
#else
		const s32 modelId = 44;
#endif
		if (!LoadPlayerModel(mOXObj, modelId))
			break;
		s32 path[] = { 0, 1, 0, 1, 2, 0, 0, 4 };
		SetAttachmentParent(this, mOXObj, path, 8);
		++mLoadStep;
	}
	case 6:
#if !defined(VERSION_JP)
		switch (mOXMenu._00) {
		case 0:
			if (!LoadPlayerModel(mOXMenu.mMark[0].mObj, 45))
				break;
			if (mOXMenu.mMark[0].mObj.mJObj)
				HSD_JObjWalkTree(mOXMenu.mMark[0].mObj.mJObj, EnableJointMaterialTranslucency, NULL);
			++mOXMenu._00;
		case 1:
			if (!LoadPlayerModel(mOXMenu.mMark[1].mObj, 46))
				break;
			if (mOXMenu.mMark[1].mObj.mJObj)
				HSD_JObjWalkTree(mOXMenu.mMark[1].mObj.mJObj, EnableJointMaterialTranslucency, NULL);
			++mOXMenu._00;
		case 2:
			++mOXMenu._00;
		}
		if ((s32)mOXMenu._00 <= 1)
			break;
#endif
		++mLoadStep;
	case 7:
		++mLoadStep;
	case 8:
		++mLoadStep;
	case 9:
		vfD8(flags);
		if ((mStatus.mFlags & 0x01000000) && IsStageDataLoaded())
			vfE0(GetEnvData()[9] / 100.0f);
		++mLoadStep;
	case 10: {
		CTpl& tpl               = gGame.mTpl;
		TPLDescriptor* textures = ((TPLPalette*)tpl.mBuffer)->descriptorArray;
		mTex[0]                 = tpl.mTex.Init(&textures[122 - StatusTextureOffset], 0);
		mTex[1]                 = tpl.mTex.Init(&((TPLPalette*)tpl.mBuffer)->descriptorArray[123 - StatusTextureOffset], 0);
		++mLoadStep;
	}
	case 11:
		++mLoadStep;
	case 12:
		if (GetAttachment() == 1)
			ShowIdlePlayerCopter(&mCopterObj);
		else if (GetAttachment() == 2)
			mPopper.mState = 3;
		else if (GetAttachment() == 3)
			ShowIdlePlayerRadar(&mRadarObj);
		++mLoadStep;
	}
	if (mLoadStep > 12) {
		mLoadStep = 0;
		return TRUE;
	}
	return FALSE;
}
