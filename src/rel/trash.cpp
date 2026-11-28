#include "game/reltrash.h"
#include "Version.h"
#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/cgame.h"
#include "game/gmath.h"
#include "game/xchr.h"
#include "game/xobjflags.h"

extern "C" double sin(double x);
extern "C" double cos(double x);
extern "C" double atan2(double y, double x);
extern "C" f32 PSVECDistance(Vec* a, Vec* b);

// first aobj on the jobj or below it
static inline HSD_AObj* FindAnim(HSD_JObj* jobj)
{
	if (jobj != NULL) {
		HSD_AObj* aobj = FindJointAnimationInTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
		aobj = FindMaterialOrTextureAnimationInJointTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
		aobj = FindPolygonAnimationInJointTree(jobj);
		if (aobj != NULL) {
			return aobj;
		}
	}
	return NULL;
}

static inline f32 GetAnimFrame(HSD_JObj* jobj)
{
	HSD_AObj* aobj = FindAnim(jobj);
	if (aobj != NULL) {
		return HSD_AObjGetCurrentFrame(aobj);
	}
	return 0.0f;
}

static XObj* trashTarget;
static s32 trashStackNum;
static f32 trashStackScale;
static XObj* trashObj;
static s32 trashState;
static Vec trashPos;
static Vec trashDest;
static Vec trashStep;
static f32 trashArc;
// 0 while it runs, 2 the arc finished over the box and the piece was released, 3 it hit the box low and fell beside it, 4 cancelled on B
static s32 trashResult;
static u8 trashActive;
static s32 trashboxIndex;

/**
 * @note Address: stage04 .text 0x183C
 * @note Size: 0x5B8
 */
extern "C" void DrawTrashStack(void)
{
	// NONMATCHING: regalloc
	XObj* actor;
	HSD_JObj* jobj;
	f32 a;
	f32 angle;
	f32 x;
	f32 y;
	f32 z;
	s32 count;
	s32 i;

	if (trashTarget == NULL) {
		return;
	}
	actor = trashObj;
	if (actor == NULL || !(actor->mStatus.mFlags & 1)) {
		return;
	}
	if (!trashActive) {
		return;
	}

	count   = trashStackNum;
	Vec pos = actor->mMoveTranslate;
	angle   = THIRD_PI + actor->mMoveRotate.y;
	jobj    = actor->mJObj;
	if (count < 0) {
		count = 0;
	}
	if (count > 10) {
		count = 10;
	}

	for (i = 1; i < count; i++) {
		int step = i - 1;
		if (step % 4 == 0) { // every fourth piece sits on the axis
			x = 0.0f;
			z = x;
			y = 2.0f * step / 4.0f + 2.0f;
		} else {
			a = DEG2RAD * (step % 4 * 120) + angle; // the other three at 120 degrees
			x = 2.0f * (f32)sin(a);
			y = 2.0f * (i - 1) / 4.0f + 1.0f; // half a unit higher each step
			z = 2.0f * (f32)cos(a);
		}
		HSD_JObjSetTranslateX(jobj, x * trashStackScale + pos.x);
		HSD_JObjSetTranslateY(jobj, y * trashStackScale + pos.y);
		HSD_JObjSetTranslateZ(jobj, z * trashStackScale + pos.z);
		HSD_JObjDispAll(jobj, NULL, 7, 0);
	}
	HSD_JObjSetTranslateX(jobj, pos.x);
	HSD_JObjSetTranslateY(jobj, pos.y);
	HSD_JObjSetTranslateZ(jobj, pos.z);
}
// DrawTrashStack

/**
 * @note Address: stage04 .text 0x1DF4
 * @note Size: 0x7E0
 */
extern "C" void UpdateTrash(u32 frames)
{
	// NONMATCHING: regalloc
	Vec impactPos;
	XObjChr* robot;

	if (trashTarget != NULL && trashActive) {
		CPad pad(0);
		robot = gGame.GetPlayer();
		switch (trashState) {
		case 0:
			trashObj = robot->mLift.mTarget; // the lifted object is the one to throw
			if (pad.mPad->mButton & 0x100) { // A throws
				robot->SetAnim(9, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
				trashState++;
			} else if (pad.mPad->mTrigger & 0x200) { // B cancels
				trashResult = 4;
				trashState  = 99;
			}
			break;
		case 1:
			// wait for the robot's turn
			if (robot->mMoveRotate.y == robot->mMoveRotate.mTarget.y) {
				robot->mLift.mState = 6;
				trashTarget->mStatus.mFlags |= 0x10000000000ULL;
				trashState++;
			}
			break;
		case 2:
			if (15.0f <= GetAnimFrame(robot->mJObj)) { // the toss anim's release frame
				robot->mLift.mTarget = NULL;
				trashObj->mStatus.mFlags &= ~0x400000000000000ULL; // no longer carried
				trashDest = trashTarget->mMoveTranslate;
				trashDest.y += 30.0f; // aim above the box
				trashPos    = trashObj->mMoveTranslate;
				trashStep.x = (trashDest.x - trashPos.x) / 36.0f; // 36 frames of flight
				trashArc    = 0.0f;
				trashState++;
				trashStep.y = (trashDest.y - trashPos.y) / 36.0f;
				trashStep.z = (trashDest.z - trashPos.z) / 36.0f;
			}
			break;
		case 3: {
			if (30.0f <= GetAnimFrame(robot->mJObj) && GetAnimFrame(robot->mJObj) < 33.0f) {
				// the toss sound's window
				XSound* obj = &gGame.mSound;
				if (!IsGameSoundPlaying(obj, 0x1B9)) {
					PlayGameSound(obj, 0x1B9, 0xFF);
				}
			}
			trashArc += DEG2RAD * 5 * frames;
			if ((f32)sin(trashArc) < 0.0f) { // the arc is over (half a turn, 36 frames): release the piece
				trashResult = 2;
				if (trashObj->GetCategory() == 8) {
					static_cast<XObjFlags*>(trashObj)->SetFlag();
				}
				trashObj->Release();
				trashObj   = NULL;
				trashState = 99;
			} else {
				Vec pos = trashObj->mMoveTranslate;
				pos.y   = trashTarget->mMoveTranslate.y;
				if (PSVECDistance(&trashTarget->mMoveTranslate, &pos) < 15.0f && trashObj->mMoveTranslate.y < 30.0f) {
					trashState = 10; // it hit the box low: bounce it off
				} else {
					trashPos.x += trashStep.x * frames;
					trashPos.y += trashStep.y * frames;
					trashPos.z += trashStep.z * frames;
					pos.x = trashPos.x;
					pos.y = trashPos.y;
					pos.z = trashPos.z;
					pos.y += 10.0f * (f32)sin(trashArc); // the arc's height
					trashObj->SetMoveDir(&pos, 10.0f);
				}
			}
			break;
		}
		case 10: {
			f32 angle;
			trashResult = 3;
			// push it 2 units further out from the box, down to the ground
			impactPos = trashObj->mMoveTranslate;
			angle     = atan2(impactPos.x - trashTarget->mMoveTranslate.x, impactPos.z - trashTarget->mMoveTranslate.z);
			impactPos.x += 2.0f * (f32)sin(angle);
			impactPos.y = 0.0f;
			impactPos.z += 2.0f * (f32)cos(angle);
			trashObj->SetMoveDir(&impactPos, 1.0f);
			trashState = 99;
			break;
		}
		case 99:
			break;
		}
	}
}
// UpdateTrash

/**
 * @note Address: stage04 .text 0x25D4
 * @note Size: 0x48
 */
// CPad::~CPad

/**
 * @note Address: stage04 .text 0x261C
 * @note Size: 0xCC
 */
extern "C" void InitTrash(void)
{
	XObj* actor;
	CGame* game = &gGame;
	s32 count;
	s32 i;

	trashTarget   = NULL;
	trashObj      = NULL;
	trashboxIndex = -1;
	trashActive   = 0;
	count         = game->mXObjs.mObjectCount;
	for (i = 0; trashboxIndex < 0 && i < count; i++) {
		actor = game->mXObjs.mObjects[i];
		if (actor->mStatus.mFlags & 1) { // live objects only
			// the trashbox's object ids
			if (actor->mCoord.mPlacement->mObjectId == 0x12D) {
				trashboxIndex = i;
			}
			if (actor->mCoord.mPlacement->mObjectId == 0x477) {
				trashboxIndex = i;
			}
		}
	}
}

/**
 * @note Address: stage04 .text 0x26E8
 * @note Size: 0x100
 */
extern "C" s32 CmdIsNearTrashbox(s32 cmd, s32 count, s32* args)
{
	CGame* game;
	XObjChr* robot;
	XObj* box;

	if (trashboxIndex < 0) {
		return 0;
	}
	game  = &gGame;
	robot = game->GetPlayer();
	if (trashboxIndex != robot->mFloorContacts[0].objectIndex) { // the box must be the contacted object
		return 0;
	}
	box = game->mXObjs.mObjects[trashboxIndex];
	CVec from(robot->mMoveTranslate);
	CVec to(box->mMoveTranslate);
	from.y = to.y = 0.0f;
	return PSVECDistance(&from, &to) > 11.5f ? 0 : 1;
}
// CmdIsNearTrashbox

/**
 * @note Address: stage04 .text 0x27E8
 * @note Size: 0x9C
 */
extern "C" s32 CmdCancelTrash(s32 cmd, s32 count, s32* args)
{
	XObjChr* robot = gGame.GetPlayer();

	if (robot->mLift.mTarget != NULL) {
		robot->mLift.mState = 6;
	}
	if (trashObj != NULL && (trashObj->mStatus.mFlags & 1)) {
		trashObj->Release();
		trashObj = NULL;
	}
	return 0;
}

/**
 * @note Address: stage04 .text 0x2884
 * @note Size: 0x2C
 */
extern "C" s32 CmdEndTrash(s32 cmd, s32 count, s32* args)
{
	if (trashTarget != NULL) {
		trashObj    = NULL;
		trashActive = 0;
	}
	return 0;
}

/**
 * @note Address: stage04 .text 0x28B0
 * @note Size: 0x10
 */
extern "C" s32 CmdGetTrashResult(s32 cmd, s32 count, s32* args)
{
	return trashResult;
}

/**
 * @note Address: stage04 .text 0x28C0
 * @note Size: 0xA8
 */
extern "C" s32 CmdStartTrash(s32 cmd, s32 count, s32* args)
{
	CGame* game     = &gGame;
	f32 scale       = args[2] / 100.0f; // percent
	s32 idx         = args[0];          // the trashbox's object index
	s32 num         = args[1];          // pieces in the stack
	trashTarget     = game->mXObjs.mObjects[idx];
	trashStackNum   = num;
	trashStackScale = scale;
	if (trashTarget != NULL) {
		trashResult = 0;
		trashState  = 0;
		trashActive = 1;
	}
	return 0;
}
