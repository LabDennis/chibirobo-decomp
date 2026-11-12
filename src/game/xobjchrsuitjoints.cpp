#include "game/xchr.h"
#include "game/animframe.h"
#include "game/cgame.h"
#include "game/xobjchr.h"

extern "C" void SetPlayerPopperMountHidden(XObj* player, BOOL hidden);

static inline void MountSuitSkeleton(XObjChr* player, s32 suit)
{
	s32 suitPath[]  = { 0, 1, 0 };
	HSD_JObj* mount = FindJointByPath(player->mJObj, suitPath, 3);
	mount->child    = player->mSuits.mJoints[suit];
	for (HSD_JObj* child = mount->child; child; child = child->next)
		child->parent = mount;
}

static inline HSD_JObj* FindPlayerToolMount(HSD_JObj* root)
{
	s32 tool[] = { 0, 1, 0, 1, 2, 0, 0, 4 };
	return FindJointByPath(root, tool, 8);
}

static inline void BindRotationJoint(CJointRot& rotation)
{
	if (rotation.mObj) {
		s32 path[]      = { 0, 1, 0, 1, 2 };
		rotation.mJoint = FindJointByPath(rotation.mObj->mJObj, path, 5);
	}
}

/**
 * @note Address: 0x800B7D54
 * @note Size: 0xAFC
 */
extern "C" void ApplyPlayerSuit(XObjChr* player)
{
	if (!(player->mStatus.mFlags & 1) || !player->mSuits.mJoints[player->mSuits.mCurrent])
		return;
	player->ClearAnim();
	player->ApplyTransform();
	if (player->GetAttachment() == 2)
		SetPlayerPopperMountHidden(player, FALSE);
	MountSuitSkeleton(player, player->mSuits.mCurrent);
	struct JointPath {
		s32 depth;
		s32* indices;
	};
	JointPath joints[] = { { 9, NULL }, { 9, NULL }, { 9, NULL }, { 9, NULL } };
	s32 path3[]        = { 0, 1, 0, 1, 2, 0, 0, 3, 1 };
	joints[3].indices  = path3;
	s32 path2[]        = { 0, 1, 0, 1, 2, 0, 0, 3, 0 };
	joints[2].indices  = path2;
	s32 path1[]        = { 0, 1, 0, 1, 2, 0, 0, 1, 1 };
	joints[1].indices  = path1;
	s32 path0[]        = { 0, 1, 0, 1, 2, 0, 0, 1, 0 };
	joints[0].indices  = path0;
	for (s32 i = 0; i < 4; ++i)
		player->_9500[i].mJObj = FindJointByPath(player->mJObj, joints[i].indices, joints[i].depth);
	{
		s32 attachment[]         = { 0, 1, 0, 1, 2, 0, 0, 5 };
		player->mAttachmentJoint = FindJointByPath(player->mJObj, attachment, 8);
		player->mCopterObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
		s32 popper[] = { 0, 1, 0, 1, 3 };
		player->mPopperObj.SetParent(FindJointByPath(player->mJObj, popper, 5), FALSE);
		player->mRadarObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
		player->mOXObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
	}
	BindRotationJoint(player->mJointRot);
	BindRotationJoint(player->mNeckRot);
	BindRotationJoint(player->mHeadRot);
	if (!(gGame.mStatus.mFlags & 0x100))
		SetPlayerEyeCameraJointsHidden(player, FALSE);
	if (player->GetAttachment() == 2)
		SetPlayerPopperMountHidden(player, TRUE);
	player->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
	player->_A2C = 3;
}

/**
 * @note Address: 0x800B8850
 * @note Size: 0x674
 */
extern "C" void RemovePlayerSuit(XObjChr* player)
{
	s32 anim  = player->mAnimId;
	f32 frame = GetAnimFrame(player->mJObj);
	player->ClearAnim();
	player->ApplyTransform();

	MountSuitSkeleton(player, 0);
	player->SetAnim(anim, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);

	{
		s32 attachment[]         = { 0, 1, 0, 1, 2, 0, 0, 5 };
		player->mAttachmentJoint = FindJointByPath(player->mJObj, attachment, 8);
		player->mCopterObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
		s32 popper[] = { 0, 1, 0, 1, 3 };
		player->mPopperObj.SetParent(FindJointByPath(player->mJObj, popper, 5), FALSE);
		player->mRadarObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
		player->mOXObj.SetParent(FindPlayerToolMount(player->mJObj), FALSE);
	}

	XObj& suit = player->mSuits.mObjects[player->mSuits.mCurrent];
	if (suit.mStatus.mFlags & 1)
		suit.Release();
	if (player->mSuits.mJoints[player->mSuits.mCurrent])
		player->mSuits.mJoints[player->mSuits.mCurrent] = NULL;
	player->mSuitAnim.Release();
}
