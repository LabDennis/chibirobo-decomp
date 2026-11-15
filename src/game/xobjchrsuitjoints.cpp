#include "game/xchr.h"
#include "game/animframe.h"
#include "game/cgame.h"
#include "game/xobjchr.h"

extern "C" void SetPlayerPopperMountHidden(XObj* player, BOOL hidden);

// the joint a rotation turns, found from its object's root
static inline void BindRotationJoint(CJointRot& rotation, s32* path, s32 depth)
{
	if (rotation.mObj) {
		rotation.mJoint = FindJointByPath(rotation.mObj->mJObj, path, depth);
	}
}

/**
 * @note Address: 0x800B7D54
 * @note Size: 0xAFC
 */
// NONMATCHING: regalloc, the path walks hold the joint and the path pointer in each other's register; the stack matches
extern "C" void ApplyPlayerSuit(XObjChr* player)
{
	if ((player->mStatus.mFlags & 1) && player->mSuits.mJoints[player->mSuits.mCurrent]) {
		player->ClearAnim();
		player->ApplyTransform();
		if (player->GetAttachment() == 2)
			SetPlayerPopperMountHidden(player, FALSE);
		HSD_JObj* mount = FindJointByPath(player->mJObj, (s32[3]) { 0, 1, 0 }, 3);
		mount->child    = player->mSuits.mJoints[player->mSuits.mCurrent]; // graft the suit's joint tree
		for (HSD_JObj* child = mount->child; child; child = child->next)
			child->parent = mount;
		struct JointPath {
			s32 depth;
			s32* indices;
		};
		JointPath joints[] = { { 9, NULL }, { 9, NULL }, { 9, NULL }, { 9, NULL } };
		s32 path0[]        = { 0, 1, 0, 1, 2, 0, 0, 1, 0 };
		joints[0].indices  = path0;
		s32 path1[]        = { 0, 1, 0, 1, 2, 0, 0, 1, 1 };
		joints[1].indices  = path1;
		s32 path2[]        = { 0, 1, 0, 1, 2, 0, 0, 3, 0 };
		joints[2].indices  = path2;
		s32 path3[]        = { 0, 1, 0, 1, 2, 0, 0, 3, 1 };
		joints[3].indices  = path3;
		for (s32 i = 0; i < 4; ++i) // the suit's four idle joints
			player->mIdleJoints[i].mJObj = FindJointByPath(player->mJObj, joints[i].indices, joints[i].depth);
		{ // the suit's attachment, tool and popper mount points
			player->mAttachmentJoint = FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 5 }, 8);
			player->mCopterObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
			player->mPopperObj.SetParent(FindJointByPath(player->mJObj, (s32[5]) { 0, 1, 0, 1, 3 }, 5), FALSE);
			player->mRadarObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
			player->mOXObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
		}
		BindRotationJoint(player->mJointRot, (s32[5]) { 0, 1, 0, 1, 2 }, 5);
		BindRotationJoint(player->mHeadTurn.mNeck, (s32[5]) { 0, 1, 0, 1, 2 }, 5);
		BindRotationJoint(player->mHeadTurn.mHead, (s32[5]) { 0, 1, 0, 1, 2 }, 5);
		if (!(gGame.mStatus.mFlags & 0x100))
			SetPlayerEyeCameraJointsHidden(player, FALSE);
		if (player->GetAttachment() == 2) // hide the popper mount again
			SetPlayerPopperMountHidden(player, TRUE);
		player->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
		player->mIdleLoops = 3;
	}
}

/**
 * @note Address: 0x800B8850
 * @note Size: 0x674
 */
// NONMATCHING: identical instruction stream; only regalloc and scheduling differ
extern "C" void RemovePlayerSuit(XObjChr* player)
{
	s32 anim  = player->mAnimId;
	f32 frame = GetAnimFrame(player->mJObj);
	player->ClearAnim();
	player->ApplyTransform();

	HSD_JObj* mount = FindJointByPath(player->mJObj, (s32[3]) { 0, 1, 0 }, 3);
	mount->child    = player->mSuits.mJoints[0]; // restore slot 0's joints
	for (HSD_JObj* child = mount->child; child; child = child->next)
		child->parent = mount;
	player->SetAnim(anim, TRUE, NULL, NULL, NULL, frame, 1.0f, 1.0f, 10.0f);

	{
		player->mAttachmentJoint = FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 5 }, 8);
		player->mCopterObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
		player->mPopperObj.SetParent(FindJointByPath(player->mJObj, (s32[5]) { 0, 1, 0, 1, 3 }, 5), FALSE);
		player->mRadarObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
		player->mOXObj.SetParent(FindJointByPath(player->mJObj, (s32[8]) { 0, 1, 0, 1, 2, 0, 0, 4 }, 8), FALSE);
	}

	XObj& suit = player->mSuits.mObjects[player->mSuits.mCurrent];
	if (suit.mStatus.mFlags & 1) // release the suit object if loaded
		suit.Release();
	if (player->mSuits.mJoints[player->mSuits.mCurrent])
		player->mSuits.mJoints[player->mSuits.mCurrent] = NULL;
	player->mSuitAnim.Release();
}
