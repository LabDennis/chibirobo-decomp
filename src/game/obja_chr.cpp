#include "game/hsd_jobj.h"
#include "game/xchr.h"
#include "game/playeranimationdata.h"
#include "game/cgame.h"
#include "game/stagedata.h"
#include "game/xobjmap.h"
#include "game/gmath.h"
#include "game/video.h"
#include <math.h>

extern "C" {
s32 GetObjectAnimationId(const CXObj* object);
s32 ClassifyFloatBits(f32 value);
CObjVec* GetObjectMoveRotation(CXObj* object);
BOOL IsActionTargetFloorId(const XAction* action, s32 wanted);
void SetObjectYAndTarget(CObjVec* value, f32 y);
f32 GetVectorDistance(const Vec* a, const Vec* b);
u64 GetMaskedGameStatus(const CGame* game, u64 mask);
f32 GetVectorMagnitude(const Vec* value);
f32 PlayerActionSine(f32 angle);
f32 PlayerActionCosine(f32 angle);
BOOL IsCopterActionState(const XAction* action, s32 state);
BOOL IsLadderActionState(const XAction* action, s32 state);
BOOL IsRopeActionState(const XAction* action, s32 state);
BOOL IsHangActionState(const XActHang* act, s32 wanted);
BOOL IsPushActionState(const XActPush* act, s32 wanted);
BOOL HasPlayerActionTarget(const XAction* action);
XObj* GetPlayerActionTarget(const XAction* action);
XObj* GetGameObjectByIndex(const CGame* game, s32 index);
CVec* GetObjectPreviousMoveTranslation(XObj* object);
CObjVec* GetObjectMoveTranslation(CXObj* object);
void fn_800A2C98(Vec* dst, const Vec* src);
f32 fn_800B30FC(const XObjChr* chr);
void SetObjectVectorTargetX(CObjVec* value, f32 x);
void SetObjectVectorTargetZ(CObjVec* value, f32 z);
CVec* GetObjectVectorTarget(CObjVec* value);
CVec* GetObjectVectorSpeed(CObjVec* value);
void SetObjectVectorSpeedX(CObjVec* value, f32 x);
void SetObjectVectorSpeedZ(CObjVec* value, f32 z);
void SetVectorX(CVec* value, f32 x);
void SetVectorZ(CVec* value, f32 z);
void SetObjectVectorAndTargetXYZ(CObjVec* value, f32 x, f32 y, f32 z);
Vec* fn_800B305C(XActHang* act);
f32 fn_800B3054(const XActHang* act);
u32 fn_800B304C(const XAction* action);
BOOL IsLiftActionState(const XActLift* act, s32 wanted);
BOOL IsTumbleActionState(const XActTumble* act, s32 wanted);
BOOL IsJumpActionState(const XAction* action, s32 state);
CVec* fn_800B2D54(XObjPlug* plug);
BOOL fn_800B2D64(const XActPopper*);
BOOL fn_800B2D5C(const XActRadar*);
BOOL fn_800B2A80(const XActLift*);
BOOL IsHangActionIdle(const XActHang* act);
BOOL fn_800B2A4C(const XActJump* act);
BOOL fn_800B2A28(const XActLadder* act);
BOOL fn_800B2A04(const XActRope* act);
BOOL fn_800B29FC(const XActPush*);
BOOL fn_800B29F4(const XActTumble*);
BOOL fn_800B29C8(const XActCopter* act);
BOOL fn_800B29C0(const XActPopper*);
BOOL fn_800B29B8(const XActRadar*);
void SetObjectVelocity(CObjVec* value, f32 x, f32 y, f32 z);
void SetObjectVectorTarget(CObjVec* value, const Vec& target);
void SetVectorComponents(CVec* value, f32 x, f32 y, f32 z);
BOOL IsRadarActionState(const XActRadar* act, s32 wanted);
BOOL IsPopperActionState(const XActPopper* act, s32 wanted);
s32 IsGameMenuActive(const CGame* game);
u32 CheckCurrentStageFlags(const CGame* game, u32 mask);
BOOL fn_800B2E80(const XActHang* act);
BOOL fn_800B2E44(const XActJump* act);
BOOL fn_800B2E18(const XActLadder* act);
BOOL fn_800B2DE4(const XActRope* act);
BOOL fn_800B2DC0(const XActTumble* act);
BOOL fn_800B2D6C(const XActCopter* act);
XSound* GetGameSound(CGame* game);
void SpawnPlayerDust(XObjChr* player, const Vec* origin);
void SetTumbleStateAndFade(XObjChr* chr, s32 state, BOOL restart);
BOOL IsObjectActive(XObj* object);
BOOL fn_800B26A0(const XAction* action, s32 wanted);
u32 fn_800B253C(const XObjChr* chr, u32 mask);
u64 GetMaskedObjectStatus(const XObj* object, u64 mask);
u32 GetObjectId(const CXObj* object);
void SetObjectStatusFlags(XObj* object, u64 mask);
void ClearObjectStatusFlags(XObj* object, u64 mask);
XObj* GetDrawListObject(const CGame* game, s32 list, s32 index);
s32 GetDrawListCount(const CGame* game, s32 list);
s32 GetDrawListIndex(const CGame* game, s32 list, s32 index);
f32 GetObjectFloorXZExtent(const XObj* object);
f32 GetObjectHitFloorHeight(const XObj* object);
s32 GetActionTargetFloor(const XAction* action);
XObjFloor** GetObjectFloorTableAddress(XObj* object);
CObjVec* GetObjectMoveScale(CXObj* object);
void SetObjectVerticalSpeed(CObjVec* value, f32 y);
void SetObjectTargetY(CObjVec* value, f32 y);
f32 GetCopterDescentAcceleration();
f32 GetCopterDescentSpeedLimit();
BOOL IsPlayerRingMenuOpen(const XObjChrMenu* menu);
void ClosePlayerEquipmentRings(XObjChrMenu* menu, s32 result);
void SetCopterActionState(XAction* action, s32 state);
BOOL fn_800B2294(const XObj* object);
s32 fn_800B2284(const XObj* object, s32 index);
BOOL fn_800B2250(const XObj* object);
BOOL fn_800B21B8(const XObj* object);
BOOL fn_800B2164(const XObj* object);
void ReleasePlayerLiftTarget(XObjChr* chr);
void SetPlayerActionState(XAction* action, s32 state);
void SetPlayerHangState(XObjChr* chr, s32 state, XObj* target);
void SetPlayerLadderState(XObjChr* chr, s32 state, XObj* target);
void SetPlayerRopeState(XObjChr* chr, s32 state, XObj* target);
s32 GetObjectPoolUsedCount(const CGame* game, s32 index);
void* GetObjectFromPool(const CGame* game, s32 pool, s32 index);
CoordInfo* GetObjectCoordInfo(XObj* object);
UnkStageObj* GetIndexedCoordPlacement(const CoordInfo* info, s32 index);
s32 GetStageObjectCount(const CGame* game);
f32 GetSelectedFloorHeight(const XObj* object);
void DisablePlayerLook(XObjChrLook* look);
bool IsPlayerLookMarkerVisible(const XObjChrLook* look);
void EnablePlayerLook(XObjChrLook* look);
void SetPlayerLookMode(XObjChrLook* look, s32 mode);
void SetLookTargetPosition(XObjChrLook* look, f32 x, f32 y, f32 z);
BOOL fn_800B200C(const XObj*);
s32 GetSelectedFloorIndex(const XObj* object);
f32 GetObjectEdgeOutwardYaw(const XObj* object, Vec* point, s32 index);
s32 fn_800B1E04(void*);
f32 GetObjectMaxFloorHeight(XObj* object);
BOOL fn_800B1D50(XObj* object);
CVec* GetCameraAngles(CCamera* camera);
f32 GetControlStickMagnitude(const CPadOne* pad);
f32 fn_800B1C14();
f32 fn_800B1BCC();
f32 fn_800B1B84();
BOOL fn_800B1B24(const XObjChr* chr);
void SetHangActionState(XActHang* act, s32 state);
void TryLeaveHang(XActHang*);
void StopPlayerMotion(XObjChr*);
void SetPushActionState(XActPush* act, s32 state);
void SetPlayerActionTarget(XAction* action, XObj* target);
void fn_800B1B0C(XActPush* act, s32 value);
f32 WrapPlayerActionAngle(f32 angle);
extern s32 lbl_8065B268;
f32 GetPlayerAnimationRateMultiplier(XObjChr* chr);
Vec* fn_800B2FC0(CObjVec* value);
void* GetMapPoolEntry(const CGame* game, s32 index);
void SetObjectXAndTarget(CObjVec* value, f32 x);
void SetObjectZAndTarget(CObjVec* value, f32 z);
BOOL IsEventScriptActive(const CGame* game);
BOOL IsPlayerAttachmentEquipped(const XObjChr* chr, s32 wanted);
CPadOne* GetPadForPort(CGame* game, u8 port);
u32 GetHeldPadButtons(const CPadOne* pad, u32 mask);
void SetObjectVectorAndTargetFromVec(CObjVec* value, const Vec& v);
u32 fn_800B2EB0(const XObjPlug* plug);
CVec* NormalizeVectorInPlace(CVec* value);
BOOL fn_800B2ABC(const XObjMap* map, const Vec* pos, XObjMapHit* hit, Vec* tri);
void ScaleVectorInPlace(Vec* value, f32 scale);
Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index);
Vec GetFloorExtent(XObjFloor* const* floors, s32 index);
HSD_JObj* GetObjectJointByPath(const CJObj* object, s32 depth, s32* path);
void GetJointScale(CJObj*, HSD_JObj* jobj, Vec* scale);
f32 GetHorizontalDistance(const Vec* a, const Vec* b);
s32 GetObjectPoolId(const XObj* object);
f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index);
f32 SquareRootFloat(f32 value);
BOOL fn_800B2390(const XActPush*);
void SelectHighestActiveFloor(XObj*);
f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to);
BOOL fn_800B2048(const XObj* object);
s32 GetCharacterObjectApproachResult(const XObjChr* chr, const XObj* target);
BOOL fn_800B2014(const XObj* object);
BOOL IsPositionBehindObject(void*, const Vec* pos, CXObj* object);
u32 fn_800B1DFC(XObj* object);
u32 fn_800B1DF0(XObj* object, u32 mask);
Vec& __as__3VecFRC3Vec(Vec* dst, const Vec* src);
u32 fn_800B31AC(const XObjChr* chr, u32 mask);
}

struct PushContactBounds {
	Vec center, rear, front;
	f32 radius, height, depth;
};

// Update the player movement, contacts and context-sensitive actions.
extern "C" void fn_800AC660(XObjChr* player, u32 frames, f32 speed, f32 direction)
{
	if (fn_800B31AC(player, 0x80000000))
		return;
	CVec floor(0, 0, 0);
	{
		if (!IsCopterActionState(&player->mCopter, 3) && !IsCopterActionState(&player->mCopter, 4)
		    && !IsCopterActionState(&player->mCopter, 7) && !IsCopterActionState(&player->mCopter, 8)
		    && !IsCopterActionState(&player->mCopter, 5) && !(IsPushActionState(&player->mPush, 2) && HasPlayerActionTarget(&player->mPush))
		    && player->mFloorContacts[0].objectIndex >= 0
		    && player->mFloorContacts[0].objectIndex == player->mFloorContacts[1].objectIndex) {
			{
				XObj* support     = GetGameObjectByIndex(&gGame, player->mFloorContacts[0].objectIndex);
				Vec previous      = *GetObjectPreviousMoveTranslation(support);
				const CVec& delta = *GetObjectMoveTranslation(support) - previous;
				fn_800A2C98(&floor, &delta);
			}
		}
		player->mFloorContacts[1].objectIndex = player->mFloorContacts[0].objectIndex;
		player->mFloorContacts[0].objectIndex = -1;
	}
	CVec hang(0, 0, 0);
	{
		if (!IsHangActionState(&player->mHang, 0) && HasPlayerActionTarget(&player->mHang)) {
			XObj* support     = GetPlayerActionTarget(&player->mHang);
			Vec previous      = *GetObjectPreviousMoveTranslation(support);
			const CVec& delta = *GetObjectMoveTranslation(support) - previous;
			fn_800A2C98(&hang, &delta);
		}
	}
	CVec ladder(0, 0, 0);
	{
		const XActLadder& ladderAction = player->mLadder;
		if (!IsLadderActionState(&ladderAction, 0) && !IsLadderActionState(&ladderAction, 2) && !IsLadderActionState(&ladderAction, 3)
		    && HasPlayerActionTarget(&ladderAction)) {
			XObj* target = GetPlayerActionTarget(&ladderAction);
			if (IsLadderActionState(&ladderAction, 1)) {
				f32 previous = GetObjectPreviousMoveTranslation(target)->y;
				ladder.y     = GetObjectMoveTranslation(target)->y - previous;
			} else {
				Vec previous      = *GetObjectPreviousMoveTranslation(target);
				const CVec& delta = *GetObjectMoveTranslation(target) - previous;
				fn_800A2C98(&ladder, &delta);
			}
		}
	}
	CVec rope(0, 0, 0);
	{
		const XActRope& ropeAction = player->mRope;
		if (!IsRopeActionState(&ropeAction, 0) && !IsRopeActionState(&ropeAction, 2) && HasPlayerActionTarget(&ropeAction)) {
			XObj* target = GetPlayerActionTarget(&ropeAction);
			if (IsRopeActionState(&ropeAction, 1) || IsRopeActionState(&ropeAction, 3)) {
				f32 previous = GetObjectPreviousMoveTranslation(target)->y;
				rope.y       = GetObjectMoveTranslation(target)->y - previous;
			} else {
				Vec previous      = *GetObjectPreviousMoveTranslation(target);
				const CVec& delta = *GetObjectMoveTranslation(target) - previous;
				fn_800A2C98(&rope, &delta);
			}
		}
	}
	u32 movementFrames = frames > 2 ? 2 : frames;
	f32 targetX, targetZ;
	{
		targetX = targetZ = 0.0f;
		if (ClassifyFloatBits(direction) != FP_NAN) {
			f32 angle = direction >= 0.0f ? direction : -direction;
			if (angle > 6.2831855f)
				angle -= 6.2831855f;
			if (lbl_8065B268 >= 1) {
				f32 scale = 1.0f - 0.15f * lbl_8065B268;
				if (scale < 0.7f)
					scale = 0.7f;
				f32 factor = fn_800B30FC(player);
				targetX    = movementFrames * (scale * ((speed * PlayerActionSine(angle)) * factor));
				factor     = fn_800B30FC(player);
				targetZ    = movementFrames * (scale * ((speed * PlayerActionCosine(angle)) * factor));
			} else {
				f32 factor = fn_800B30FC(player);
				f32 xSpeed = (speed * PlayerActionSine(angle)) * factor;
				targetX    = movementFrames * (GetPlayerAnimationRateMultiplier(player) * xSpeed);
				factor     = fn_800B30FC(player);
				f32 zSpeed = (speed * PlayerActionCosine(angle)) * factor;
				targetZ    = movementFrames * (GetPlayerAnimationRateMultiplier(player) * zSpeed);
			}
		}
		SetObjectVectorTargetX(&player->_988, targetX);
		SetObjectVectorTargetZ(&player->_988, targetZ);
		player->_9B8 = GetVectorMagnitude(GetObjectVectorTarget(&player->_988)) / movementFrames;
		if (ClassifyFloatBits(player->_12DF8) != FP_NAN)
			player->_9B8 = 2.8f * GetVectorMagnitude(GetObjectVectorSpeed(&player->mMoveTranslate));
	}
	Vec savedRotation = player->mMoveRotate;
	{
		CObjVec& motion      = player->_988;
		XActHang& hangAction = player->mHang;
		if (!IsHangActionState(&hangAction, 0)) {
			if (IsHangActionState(&hangAction, 5)) {
				SetObjectVectorSpeedX(&motion, 0);
				SetObjectVectorSpeedZ(&motion, 0);
				if (HasPlayerActionTarget(&hangAction) && IsActionTargetFloorId(&hangAction, 0)) {
					Vec center = *fn_800B305C(&hangAction);
					f32 radius = fn_800B3054(&hangAction);
					f32 period = (6.2831855f * radius) / (0.3f * speed);
					f32 angle  = movementFrames * ((6.2831855f / period) * (s32)fn_800B304C(&hangAction)) + player->mMoveRotate.y;
					player->SetRotationY(WrapPlayerActionAngle(angle), 0);
					f32 facing = 3.1415927f + angle;
					SetVectorX(&motion, (radius * PlayerActionSine(facing) + center.x) - player->mMoveTranslate.x);
					SetVectorZ(&motion, (radius * PlayerActionCosine(facing) + center.z) - player->mMoveTranslate.z);
					PlayerActionSine(facing);
					PlayerActionCosine(facing);
				} else {
					SetVectorX(&motion, targetX);
					SetVectorZ(&motion, targetZ);
				}
			} else {
				SetObjectVectorAndTargetXYZ(&motion, 0, 0, 0);
			}
		} else if (IsPushActionState(&player->mPush, 2)) {
			SetObjectVectorSpeedX(&motion, 0);
			SetObjectVectorSpeedZ(&motion, 0);
			s32 animation = GetObjectAnimationId(player);
			if (animation == 64 || animation == 65) {
				SetVectorX(&motion, targetX);
				SetVectorZ(&motion, targetZ);
			} else {
				SetVectorX(&motion, 0);
				SetVectorZ(&motion, 0);
			}
		} else if (!IsRopeActionState(&player->mRope, 0)) {
			if (IsRopeActionState(&player->mRope, 5)) {
				SetObjectVectorSpeedX(&motion, 0);
				SetObjectVectorSpeedZ(&motion, 0);
				SetVectorX(&motion, 0);
				SetVectorZ(&motion, 0);
				f32 sign = (s32)fn_800B304C(&player->mRope);
				if (sign > 0)
					sign = 1;
				if (sign < 0)
					sign = -1;
				SetObjectYAndTarget(&player->mMoveTranslate, player->mMoveTranslate.y + sign * (speed * movementFrames));
			} else {
				SetObjectVectorAndTargetXYZ(&motion, 0, 0, 0);
			}
		} else if (!IsLadderActionState(&player->mLadder, 0)) {
			if (IsLadderActionState(&player->mLadder, 5)) {
				SetObjectVectorSpeedX(&motion, 0);
				SetObjectVectorSpeedZ(&motion, 0);
				SetVectorX(&motion, 0);
				SetVectorZ(&motion, 0);
				f32 climbDir = (s32)fn_800B304C(&player->mLadder);
				SetObjectYAndTarget(&player->mMoveTranslate, player->mMoveTranslate.y + speed * climbDir);
			} else {
				SetObjectVectorAndTargetXYZ(&motion, 0, 0, 0);
			}
		} else {
			f32 step = 1.56f * (0.05f * movementFrames);
			if (!(GetMaskedGameStatus(&gGame, 0x100))) {
				Vec* current = fn_800B2FC0(&motion);
				Vec* target  = GetObjectVectorTarget(&motion);
				f32 rate     = GetPlayerAnimationRateMultiplier(player);
				f32 distance = GetVectorDistance(current, target) / rate;
				if (distance >= 3.0f) {
					f32 difference = distance - 8.0f;
					if (difference < 0)
						difference = -difference;
					step *= difference / 2.5f;
				}
			}
			SetObjectVectorSpeedX(&motion, step);
			SetObjectVectorSpeedZ(&motion, step);
		}
	}
	player->_988.Step(frames);
	{
		if (IsHangActionState(&player->mHang, 0) && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
		    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)) {
			if (IsEventScriptActive(&gGame)) {
				if (ClassifyFloatBits(direction) != FP_NAN && direction >= 0.0f)
					player->SetRotateY(direction, 0.20943952f);
			} else {
				f32 rate;
				{
					f32 current    = WrapPlayerActionAngle(player->mMoveRotate.y);
					f32 desired    = ClassifyFloatBits(direction) != FP_NAN && direction >= 0.0f
					                   ? WrapPlayerActionAngle(direction)
					                   : WrapPlayerActionAngle(GetObjectVectorTarget(&player->mMoveRotate)->y);
					f32 difference = desired - current;
					if (difference < 0)
						difference = -difference;
					if (difference <= 0.008726646f) {
						rate = 0.017453292f;
					} else {
						if (difference >= TWO_PI)
							difference -= TWO_PI;
						if (difference > PI)
							desired = desired < current ? desired + TWO_PI : desired - TWO_PI;
						f32 turn = desired - current;
						if (turn < 0)
							turn = -turn;
						if (GetMaskedGameStatus(&gGame, 0x400))
							rate = turn * 0.15f;
						else if (GetMaskedGameStatus(&gGame, 0x800))
							rate = turn * 0.2f;
						else if (IsPlayerAttachmentEquipped(player, 2))
							rate = turn * 0.1f;
						else
							rate = turn * 0.2f;
					}
				}
				XActLift& lift  = player->mLift;
				CObjVec& motion = player->_988;
				if (GetObjectAnimationId(player) == 0x895
				    && (IsLiftActionState(&lift, 27) || IsLiftActionState(&lift, 28) || IsLiftActionState(&lift, 4))) {
					SetObjectXAndTarget(&motion, 0);
					SetObjectZAndTarget(&motion, 0);
					if (ClassifyFloatBits(direction) != FP_NAN && direction >= 0.0f)
						player->SetRotateY(direction, 0.25f * rate);
				} else if (GetHeldPadButtons(GetPadForPort(&gGame, 0), 0x100) && !(GetMaskedGameStatus(&gGame, 0x400))
				           && IsJumpActionState(&player->mJump, 0) && IsLiftActionState(&lift, 4)
				           && (IsCarryingToothbrush(&lift) || IsCarryingSpoon(&lift) || IsCarryingSquirter(&lift))
				           && GetObjectAnimationId(player) != 0x8ae && GetObjectAnimationId(player) != 0x8af) {
					SetObjectXAndTarget(&motion, 0.7f * motion.x);
					SetObjectZAndTarget(&motion, 0.7f * motion.z);
					player->_9B8 = GetPlayerAnimationRateMultiplier(player);
					if (ClassifyFloatBits(direction) != FP_NAN && direction >= 0.0f) {
						f32 diff = WrapPlayerActionAngle(player->mMoveRotate.y - WrapPlayerActionAngle(direction));
						if (diff < 1.5707964f || diff >= 4.712389f)
							player->SetRotateY(direction, 0.25f * rate);
						if (player->_9B8 > 0) {
							if (diff > 1.5707964f && diff < 4.712389f)
								player->SetAnim(0x12a, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
							else
								player->SetAnim(3, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
						} else {
							player->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
						}
					} else {
						SetObjectVectorAndTargetFromVec(&player->mMoveRotate, player->mMoveRotate);
						player->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
					}
				} else {
					if (ClassifyFloatBits(direction) != FP_NAN && direction >= 0.0f)
						player->SetRotateY(direction, rate);
					else
						SetObjectVerticalSpeed(&player->mMoveRotate, rate);
					if (GetObjectAnimationId(player) == 0x12a)
						player->SetAnim(17, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
				}
			}
		}
	}

	Vec position;
	void* map;
	{
		player->mMoveTranslate.y += rope.y + (ladder.y + (floor.y + hang.y));
		player->_960C.Step(frames);
		map      = GetMapPoolEntry(&gGame, 0);
		position = player->mMoveTranslate;
		f32 dx   = floor.x + 0.3f * player->_988.x;
		f32 dz   = floor.z + 0.3f * player->_988.z;
		position.x += player->_960C.x + (rope.x + (ladder.x + (hang.x + dx)));
		position.z += player->_960C.z + (rope.z + (ladder.z + (hang.z + dz)));
	}
	{
		if (fn_800B2EB0(&player->mPlug)) {
			if (!IsEventScriptActive(&gGame) && !IsGameMenuActive(&gGame) && fn_800B2E80(&player->mHang) && fn_800B2E44(&player->mJump)
			    && fn_800B2E18(&player->mLadder) && fn_800B2DE4(&player->mRope) && IsPushActionState(&player->mPush, 0)
			    && fn_800B2DC0(&player->mTumble) && fn_800B2D6C(&player->mCopter) && fn_800B2D64(&player->mPopper)
			    && fn_800B2D5C(&player->mRadar)) {
				CVec pull((const Vec&)*fn_800B2D54(&player->mPlug));
				CVec combined = player->_988 + pull;
				combined.y    = 0;
				f32 strength  = GetVectorMagnitude(&pull);
				if (strength > 0.2f) {
					position.x += 0.2f * (pull.x * frames);
					position.z += 0.2f * (pull.z * frames);
					if (strength > 1 && GetVectorMagnitude(&combined) > 1
					    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
					    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
					    && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
					    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
					    && IsCopterActionState(&player->mCopter, 0)
					    && (IsPopperActionState(&player->mPopper, 0) || IsPopperActionState(&player->mPopper, 8)
					        || IsPopperActionState(&player->mPopper, 4))
					    && IsRadarActionState(&player->mRadar, 0))
						SpawnPlayerDust(player, GetObjectMoveTranslation(player));
				}
			}
			if (CheckCurrentStageFlags(&gGame, 1)) {
				s32 path[2] = { 0, 0 };
				f32 distance;
				{
					const CVec& joint = GetEventJointWorldPosition(&player->mPlug, 2, path);
					distance          = GetVectorDistance(GetObjectMoveTranslation(player), &joint);
				}
				if (distance >= 34 && !IsEventScriptActive(&gGame)) {
					PlayGameSound(GetGameSound(&gGame), 0x37, 255);
					ResetPlayerPlugState(&player->mPlug, TRUE);
					if (IsLiftActionState(&player->mLift, 0) && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
					    && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
					    && IsPushActionState(&player->mPush, 0) && !IsPlayerAttachmentEquipped(player, 1)
					    && !IsPlayerAttachmentEquipped(player, 2) && !IsPlayerAttachmentEquipped(player, 3))
						SetTumbleStateAndFade(player, 7, FALSE);
				}
			}
		}
	}
	{
		if (GetVectorMagnitude(&player->_158C4) > 0 && !IsEventScriptActive(&gGame) && !IsGameMenuActive(&gGame)
		    && fn_800B2E80(&player->mHang) && fn_800B2E44(&player->mJump) && fn_800B2E18(&player->mLadder) && fn_800B2DE4(&player->mRope)
		    && IsPushActionState(&player->mPush, 0) && fn_800B2DC0(&player->mTumble) && fn_800B2D6C(&player->mCopter)
		    && fn_800B2D64(&player->mPopper) && fn_800B2D5C(&player->mRadar)) {
			CVec extPush  = player->_158C4;
			extPush.y     = 0;
			CVec combined = player->_988 + extPush;
			combined.y    = 0;
			position.x += 0.1f * (extPush.x * frames);
			position.z += 0.1f * (extPush.z * frames);
			if (GetVectorMagnitude(&extPush) > 1 && GetVectorMagnitude(&combined) > 1 && IsLiftActionState(&player->mLift, 0)
			    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
			    && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
			    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
			    && IsRadarActionState(&player->mRadar, 0))
				SpawnPlayerDust(player, GetObjectMoveTranslation(player));
		}
	}
	{
		BOOL hit = fn_800B2ABC(static_cast<const XObjMap*>(map), &position, &player->mFloorHit, reinterpret_cast<Vec*>(player->_95E8));
		if (hit) {
			SetVectorX(&player->mMoveTranslate, position.x);
			SetVectorZ(&player->mMoveTranslate, position.z);
			Vec* triangle = reinterpret_cast<Vec*>(player->_95E8);
			CVec normal;
			CalcTriangleNormal(triangle, triangle + 1, triangle + 2, &normal);
			NormalizeVectorInPlace(&normal);
			Vec down   = { 0, -1, 0 };
			Vec origin = { 0, 0, 0 };
			GetOutwardPerpendicularToEdge(&down, &origin, &normal, &player->_963C);
			{
				if (!fn_800B2A80(&player->mLift) || !IsHangActionIdle(&player->mHang) || !fn_800B2A4C(&player->mJump)
				    || !fn_800B2A28(&player->mLadder) || !fn_800B2A04(&player->mRope) || !fn_800B29FC(&player->mPush)
				    || !fn_800B29F4(&player->mTumble) || !fn_800B29C8(&player->mCopter) || !fn_800B29C0(&player->mPopper)
				    || !fn_800B29B8(&player->mRadar)) {
					SetObjectVectorAndTargetXYZ(&player->_960C, 0, 0, 0);
				} else {
					CVec slopeDir = player->_963C;
					if (GetVectorMagnitude(&slopeDir) > 0.3f) {
						f32 vertical = slopeDir.y < 0 ? -slopeDir.y : slopeDir.y;
						slopeDir.y   = 0;
						NormalizeVectorInPlace(&slopeDir);
						ScaleVectorInPlace(&slopeDir, -vertical);
						f32 slopeSpeed = 0.02f * GetVectorMagnitude(&slopeDir);
						if (slopeSpeed < 0.01f)
							slopeSpeed = 0.01f;
						SetObjectVelocity(&player->_960C, slopeSpeed, slopeSpeed, slopeSpeed);
					} else {
						SetVectorComponents(&slopeDir, 0, 0, 0);
					}
					SetObjectVectorTarget(&player->_960C, slopeDir);
				}
			}
			if (player->mFloorHit.mHeight > 1.0f + player->mMoveTranslate.y) {
				player->mFloorHit.mTriangle = -1;
				player->mFloorHit.mTrigon   = -1;
				player->mFloorHit.mHeight   = -1;
				SetVectorX(&player->mMoveTranslate, position.x);
				SetVectorZ(&player->mMoveTranslate, position.z);
				SetObjectVectorAndTargetXYZ(&player->_960C, 0, 0, 0);
				SetVectorComponents(&player->_963C, 0, 0, 0);
			}
		} else {
			{
				player->mFloorHit.mTriangle = -1;
				player->mFloorHit.mTrigon   = -1;
				player->mFloorHit.mHeight   = -1;
				SetVectorX(&player->mMoveTranslate, position.x);
				SetVectorZ(&player->mMoveTranslate, position.z);
				SetObjectVectorAndTargetXYZ(&player->_960C, 0, 0, 0);
				SetVectorComponents(&player->_963C, 0, 0, 0);
			}
		}
	}

	f32 radius, height;
	s32 liftState22;
	Vec offset;
	{
		radius      = player->mMoveScale.x * GetFloorExtent(&player->mFloors, 0).x;
		f32 extentY = GetFloorExtent(&player->mFloors, 0).y;
		height      = player->mMoveScale.y * (GetFloorCenterOffset(&player->mFloors, 0).y + extentY);
		liftState22 = 0;
		if (IsCarryingMug(&player->mLift)) {
			s32 boundsPath[2] = { 0, 0 };
			Vec scale;
			GetJointScale(player, GetObjectJointByPath(player, 2, boundsPath), &scale);
			radius += 2.5f * scale.x;
			liftState22 = IsLiftActionState(&player->mLift, 22);
		}
		Mtx rotation;
		PSMTXRotRad(rotation, 'y', player->mMoveRotate.y);
		offset = player->mMoveScale * GetFloorCenterOffset(&player->mFloors, 0);
		PSMTXMultVec(rotation, &offset, &offset);
	}
	__as__3VecFRC3Vec(&position, &player->mMoveTranslate);
	u32 queryFlags   = IsEventScriptActive(&gGame) ? 0 : 2;
	u32 contactFlags = 0;
	f32 floorHeight  = player->mFloorHit.mHeight;
	s32 floorIndex   = -1;
	BOOL canRise     = FALSE;
	CVec push, contact;
	lbl_8065B268 = 0;
	if (player->_146D8) {
		if (fn_800B2E80(&player->mHang) || IsHangActionState(&player->mHang, 9)) {
			for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
				XObj* object = GetDrawListObject(&gGame, 0, i);
				{
					BOOL checkable = IsObjectActive(object) && (GetMaskedObjectStatus(object, 0x10000))
					              && !(GetMaskedObjectStatus(object, 0x0004000000100000ULL)) && GetObjectId(object) != GetObjectId(player);
					if (checkable && (GetMaskedObjectStatus(object, 0x08000000)) && IsPushActionState(&player->mPush, 2)
					    && fn_800B26A0(&player->mPush, 1)) {
						s32 floorIdx = player->mFloorContacts[1].objectIndex;
						if (floorIdx < 0 || GetGameObjectByIndex(&gGame, floorIdx) != object)
							checkable = false;
					}
					if (checkable) {
						f32 range = 100.0f + (radius + GetObjectFloorXZExtent(object));
						checkable = !(GetHorizontalDistance(GetObjectMoveTranslation(object), &position) > range);
					}
					if (!checkable)
						continue;
				}
				BOOL touched = false;
				for (s32 floor = 0; floor != -1;) {
					position.x = player->mMoveTranslate.x + offset.x;
					position.z = player->mMoveTranslate.z + offset.z;
					u32 hit;
					{
						CVec from(position), to(position);
						hit = object->vfFC(&from, &to, radius, height, &push, &contact, (s32)&floor, queryFlags | 1);
					}
					if (hit & 1) {
						touched = true;
						f32 top = GetObjectHitFloorHeight(object);
						BOOL blocked;
						if (ClassifyFloatBits(top) == FP_NAN) {
							if (GetMaskedObjectStatus(object, 0x0002000000000000ULL)) {
								blocked = false;
							} else {
								if (liftState22 && GetMaskedObjectStatus(object, 0x4000) && !GetMaskedObjectStatus(object, 0x10))
									SetObjectStatusFlags(object, 0x10);
								blocked = true;
							}
						} else if (player->mMoveTranslate.y + 1.0f >= top) {
							blocked = false;
							if (top >= floorHeight) {
								floorHeight = top;
								floorIndex  = GetDrawListIndex(&gGame, 0, i);
							}
						} else {
							if (GetMaskedObjectStatus(object, 0x0002000000000000ULL)) {
								blocked = false;
							} else {
								if (liftState22 && GetMaskedObjectStatus(object, 0x4000) && !GetMaskedObjectStatus(object, 0x10))
									SetObjectStatusFlags(object, 0x10);
								blocked = true;
							}
						}
						if (blocked && fn_800B253C(player, 1) && fn_800B2E18(&player->mLadder) && fn_800B2DE4(&player->mRope)) {
							if (IsPushActionState(&player->mPush, 2) && HasPlayerActionTarget(&player->mPush)) {
								SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264);
							} else {
								SetVectorX(&player->mMoveTranslate, contact.x - offset.x);
								SetVectorZ(&player->mMoveTranslate, contact.z - offset.z);
							}
						}
						if (blocked)
							++lbl_8065B268;
					}
					contactFlags |= hit;
				}
				if (!touched && GetMaskedObjectStatus(object, 0x0002000000000000ULL))
					ClearObjectStatusFlags(object, 0x0002000000000000ULL);
			}
			if (liftState22) {
				for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
					XObj* object = GetDrawListObject(&gGame, 0, i);
					if (!(GetMaskedObjectStatus(object, 0x10)))
						continue;
					ClearObjectStatusFlags(object, 0x10ULL);
					for (s32 floor = 0; floor != -1;) {
						u32 hit = object->vfFC(&player->mMoveTranslate, &player->mMoveTranslate, radius, height, &push, &contact,
						                       (s32)&floor, 0);
						if ((hit & 3) != 3)
							continue;
						f32 top = GetObjectHitFloorHeight(object);
						if (ClassifyFloatBits(top) != FP_NAN && player->mMoveTranslate.y + 1.0f >= top)
							continue;
						BOOL separated;
						{
							const CVec& difference = player->mMoveTranslate - contact;
							separated              = GetVectorMagnitude(&difference) > 1.5f;
						}
						if (separated)
							SetObjectStatusFlags(object, 0x0002000000000000ULL);
					}
				}
			}
		} else {
			if (IsHangActionState(&player->mHang, 5) && HasPlayerActionTarget(&player->mHang)) {
				f32 side  = player->mMoveRotate.y + HALF_PI * (s32)(0u - fn_800B304C(&player->mHang));
				s32 path  = 0;
				Vec joint = GetEventJointWorldPosition(player, 1, &path);
				Vec body;
				body.x = joint.x + 3.0f * PlayerActionSine(side) - 2.5f * PlayerActionSine(player->mMoveRotate.y);
				body.z = joint.z + 3.0f * PlayerActionCosine(side) - 2.5f * PlayerActionCosine(player->mMoveRotate.y);
				body.y = player->mMoveTranslate.y + 1.0f;
				Vec lower, upper;
				lower.x        = joint.x + 3.0f * PlayerActionSine(side);
				lower.z        = joint.z + 3.0f * PlayerActionCosine(side);
				lower.y        = joint.y - 1.0f;
				upper.x        = joint.x + 3.0f * PlayerActionSine(side);
				upper.z        = joint.z + 3.0f * PlayerActionCosine(side);
				upper.y        = joint.y + 0.5f;
				BOOL supported = false;
				BOOL restored  = false;
				for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
					XObj* object = GetDrawListObject(&gGame, 0, i);

					if (!IsObjectActive(object) || !(GetMaskedObjectStatus(object, 0x10000))
					    || (GetMaskedObjectStatus(object, 0x0004000000100000ULL)))
						continue;
					if ((s16)GetObjectPoolId(object) == 7 || (s16)GetObjectPoolId(object) == 20
					    || GetObjectId(object) == GetObjectId(player))
						continue;
					f32 range = 100.0f + (radius + GetObjectFloorXZExtent(object));
					if (GetHorizontalDistance(GetObjectMoveTranslation(object), &body) <= range) {
						u32 bodyHit;
						{
							CVec from(body), to(body);
							bodyHit = object->vfFC(&from, &to, 0.5f, height, &push, &contact, 0, 0);
						}
						if ((bodyHit & 3) == 3) {
							SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264 + hang);
							SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
							restored = true;
							break;
						}
						u32 upperHit;
						{
							CVec from(upper), to(upper);
							upperHit = object->vfFC(&from, &to, 0.1f, 0.1f, &push, &contact, 0, 0);
						}
						if ((upperHit & 3) == 3) {
							SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264 + hang);
							SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
							restored = true;
							break;
						}
					}
					range = 100.0f + (radius + GetObjectFloorXZExtent(object));
					if (GetHorizontalDistance(GetObjectMoveTranslation(object), &lower) <= range) {
						u32 lowerHit;
						{
							CVec from(lower), to(lower);
							lowerHit = object->vfFC(&from, &to, 0.5f, 0.5f, &push, &contact, 0, 0);
						}
						if ((lowerHit & 3) == 3)
							supported = true;
					}
				}
				if (!supported && !restored) {
					SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264 + hang);
					SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
				}
			}
		}
		PushContactBounds pushed;
		BOOL gotBounds = false;
		{
			XActPush& pushAction = player->mPush;
			if (IsPushActionState(&pushAction, 2) && !fn_800B26A0(&pushAction, 0) && HasPlayerActionTarget(&pushAction)) {
				XObj* target  = GetPlayerActionTarget(&pushAction);
				Vec targetPos = *GetObjectMoveTranslation(target);
				f32 angle     = 0;
				if (fn_800B26A0(&pushAction, 1))
					angle = GetObjectMoveRotation(player)->y;
				else if (fn_800B26A0(&pushAction, -1))
					angle = PI + GetObjectMoveRotation(player)->y;
				targetPos.x += 2.5f * (0.4f * PlayerActionSine(angle));
				targetPos.z += 2.5f * (0.4f * PlayerActionCosine(angle));
				s32 index        = GetActionTargetFloor(&pushAction);
				Vec centerOffset = GetFloorCenterOffset(GetObjectFloorTableAddress(target), index);
				Vec scaledOffset = *GetObjectMoveScale(target) * centerOffset;
				index            = GetActionTargetFloor(&pushAction);
				Vec shape        = GetFloorExtent(GetObjectFloorTableAddress(target), index);
				Vec extent       = *GetObjectMoveScale(target) * shape;
				index            = GetActionTargetFloor(&pushAction);
				f32 floorYaw     = GetFloorYawRadians(GetObjectFloorTableAddress(target), index);
				f32 yaw          = GetObjectMoveRotation(target)->y + floorYaw;
				scaledOffset.y   = 0;
				if (IsActionTargetFloorId(&pushAction, 2)) {
					extent.y *= (2.0f / SquareRootFloat(3.0f)) * 0.5f;
					extent.x *= (2.0f / SquareRootFloat(3.0f)) * 0.5f;
					extent.z *= (2.0f / SquareRootFloat(3.0f)) * 0.5f;
				}
				pushed.height = 2.0f * extent.y;
				pushed.radius = extent.x;
				pushed.depth  = extent.z;
				Mtx translation, center, scale, rotation, transform;
				PSMTXTrans(translation, targetPos.x, targetPos.y, targetPos.z);
				PSMTXTrans(center, scaledOffset.x, scaledOffset.y, scaledOffset.z);
				PSMTXScale(scale, extent.x, extent.y, extent.z);
				PSMTXRotRad(rotation, 'y', yaw);
				PSMTXConcat(translation, rotation, transform);
				PSMTXConcat(transform, center, transform);
				PSMTXConcat(transform, scale, transform);
				pushed.center = (Vec) { 0, 0, 0 };
				PSMTXMultVec(transform, &pushed.center, &pushed.center);
				f32 ratio   = extent.x / extent.z;
				pushed.rear = (Vec) { 0, 0, ratio - 1.0f };
				PSMTXMultVec(transform, &pushed.rear, &pushed.rear);
				pushed.front = (Vec) { 0, 0, 1.0f - ratio };
				PSMTXMultVec(transform, &pushed.front, &pushed.front);
				gotBounds = true;
			}
		}
		if (gotBounds) {
			for (s32 i = 0; i < GetDrawListCount(&gGame, 0); ++i) {
				XObj* object = GetDrawListObject(&gGame, 0, i);

				if (!IsObjectActive(object) || !(GetMaskedObjectStatus(object, 0x10000)) || (GetMaskedObjectStatus(object, 0x08000000)))
					continue;
				if ((GetMaskedObjectStatus(object, 0x0004000000100000ULL)) && !(GetMaskedObjectStatus(object, 0x40000000)))
					continue;
				if ((s16)GetObjectPoolId(object) == 7 || (s16)GetObjectPoolId(object) == 20 || object == player)
					continue;
				u32 id = GetObjectId(object);
				if (GetObjectId(GetPlayerActionTarget(&player->mPush)) == id)
					continue;
				f32 range = 100.0f + (pushed.radius + GetObjectFloorXZExtent(object));
				if (GetHorizontalDistance(GetObjectMoveTranslation(object), &pushed.center) > range)
					continue;
				{
					BOOL centerBlocked = false;
					{
						u32 hit;
						{
							CVec from(pushed.center), to(pushed.center);
							hit = object->vfFC(&from, &to, pushed.radius, pushed.height, &push, &contact, 0, 0);
						}
						if ((hit & 3) == 3) {
							f32 top   = GetObjectHitFloorHeight(object);
							f32 delta = pushed.center.y - top;
							if (delta < 0)
								delta = -delta;
							if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top, 0);
							} else {
								SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264);
								SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
								centerBlocked = true;
							}
						}
					}
					if (centerBlocked)
						break;
				}
				if (pushed.depth / pushed.radius > 8.0f) {
					BOOL rearBlocked = false;
					{
						u32 hit;
						{
							CVec from(pushed.rear), to(pushed.rear);
							hit = object->vfFC(&from, &to, pushed.radius, pushed.height, &push, &contact, 0, 0);
						}
						if ((hit & 3) == 3) {
							f32 top   = GetObjectHitFloorHeight(object);
							f32 delta = pushed.rear.y - top;
							if (delta < 0)
								delta = -delta;
							if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top, 0);
							} else {
								SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264);
								SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
								rearBlocked = true;
							}
						}
					}
					BOOL frontBlocked = false;
					if (!rearBlocked) {
						u32 hit;
						{
							CVec from(pushed.front), to(pushed.front);
							hit = object->vfFC(&from, &to, pushed.radius, pushed.height, &push, &contact, 0, 0);
						}
						if ((hit & 3) == 3) {
							f32 top   = GetObjectHitFloorHeight(object);
							f32 delta = pushed.front.y - top;
							if (delta < 0)
								delta = -delta;
							if (delta <= 1 && !GetMaskedObjectStatus(object, 0x40000000)) {
								GetPlayerActionTarget(&player->mPush)->SetTranslateY(top, 0);
							} else {
								SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264);
								SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
								frontBlocked = true;
							}
						}
					}
					if (rearBlocked || frontBlocked)
						break;
				}
			}
		} else {
			XActRope& ropeAction = player->mRope;
			if (IsRopeActionState(&ropeAction, 5) && fn_800B26A0(&ropeAction, -1) && HasPlayerActionTarget(&ropeAction)) {
				s32 path   = 0;
				CVec joint = GetEventJointWorldPosition(player, 1, &path);
				Vec point;
				point.x = joint.x;
				point.z = joint.z;
				point.y = 9.0f + GetObjectMoveTranslation(player)->y;
				u32 hit;
				{
					CVec from(point), to(point);
					hit = GetPlayerActionTarget(&ropeAction)->vfFC(&from, &to, 0.5f, 0.5f, &push, &contact, 0, 0);
				}
				if ((hit & 3) != 3) {
					SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264 + hang);
					SetObjectVectorAndTargetFromVec(&player->mMoveRotate, savedRotation);
				}
			}
		}
	} else {
		SetObjectVectorAndTargetXYZ(&player->_960C, 0, 0, 0);
		SetVectorComponents(&player->_963C, 0, 0, 0);
	}
	if (1.0f + player->mMoveTranslate.y >= floorHeight)
		canRise = TRUE;
	if (!contactFlags)
		canRise = TRUE;
	if (floorIndex >= 0)
		player->mFloorContacts[3].object = GetGameObjectByIndex(&gGame, floorIndex);
	else
		player->mFloorContacts[3].object = static_cast<XObj*>(map);
	f32 distance = player->mMoveTranslate.y - floorHeight;
	if (distance < 0)
		distance = -distance;
	if (distance <= 1 && floorIndex >= 0)
		player->mFloorContacts[0].objectIndex = floorIndex;
	if (floorHeight > player->mFloorHit.mHeight) {
		player->mFloorHit.mHeight = floorHeight;
		SetObjectVectorAndTargetXYZ(&player->_960C, 0, 0, 0);
		SetVectorComponents(&player->_963C, 0, 0, 0);
	}
	if (player->_146D8) {
		CObjVec& motion = player->mMoveTranslate;
		f32 floor       = player->mFloorHit.mHeight;
		if (motion.y == floor) {
			SetObjectYAndTarget(&motion, floor);
			SetObjectVerticalSpeed(&motion, 0);
		} else if (motion.y > floor && fn_800B2E80(&player->mHang) && fn_800B2E44(&player->mJump) && fn_800B2E18(&player->mLadder)
		           && fn_800B2DE4(&player->mRope) && fn_800B2390(&player->mPush) && fn_800B2DC0(&player->mTumble)
		           && fn_800B2D6C(&player->mCopter) && fn_800B2D64(&player->mPopper) && fn_800B2D5C(&player->mRadar)
		           && fn_800B253C(player, 2)) {
			SetObjectTargetY(&motion, player->mFloorHit.mHeight);
			f32 speed;
			if (IsCopterActionState(&player->mCopter, 4) || IsCopterActionState(&player->mCopter, 7)
			    || IsCopterActionState(&player->mCopter, 8)) {
				f32 acceleration = GetCopterDescentAcceleration();
				f32 limit        = GetCopterDescentSpeedLimit();
				speed            = acceleration + GetObjectVectorSpeed(&motion)->y;
				if (speed > limit)
					speed = limit;
			} else {
				speed = 0.15f + GetObjectVectorSpeed(&motion)->y;
				if (speed > 10.0f)
					speed = 10.0f;
			}
			SetObjectVerticalSpeed(&motion, speed);
			f32 drop = motion.y - player->mFloorHit.mHeight;
			if (speed > 0.1f && drop > 1) {
				if (!(player->mFloorContacts[0].objectIndex >= 0
				      || (!IsLiftActionState(&player->mLift, 0) && !IsLiftActionState(&player->mLift, 4))
				      || !IsHangActionState(&player->mHang, 0) || !IsJumpActionState(&player->mJump, 0)
				      || !IsLadderActionState(&player->mLadder, 0) || !IsRopeActionState(&player->mRope, 0)
				      || !IsTumbleActionState(&player->mTumble, 0)
				      || (!IsCopterActionState(&player->mCopter, 0) && !IsCopterActionState(&player->mCopter, 3))
				      || (!IsPopperActionState(&player->mPopper, 0) && !IsPopperActionState(&player->mPopper, 8)
				          && !IsPopperActionState(&player->mPopper, 4))
				      || !IsRadarActionState(&player->mRadar, 0) || player->mAttachmentRequest >= 0)) {
					if (IsCopterActionState(&player->mCopter, 3)) {
						SetObjectVerticalSpeed(&player->mMoveTranslate, 0);
					} else {
						if (IsPlayerRingMenuOpen(&player->mMenu))
							ClosePlayerEquipmentRings(&player->mMenu, -1);
						if (!IsPushActionState(&player->mPush, 0))
							ReleasePushAction(&player->mPush);
						if (IsPlayerAttachmentEquipped(player, 1)) {
							SetCopterActionState(&player->mCopter, 4);
							SetObjectVerticalSpeed(&player->mMoveTranslate, 0);
						} else {
							XObj* support = NULL;
							if (player->mFloorContacts[1].objectIndex >= 0) {
								support       = GetGameObjectByIndex(&gGame, player->mFloorContacts[1].objectIndex);
								s32 supHeight = fn_800B2294(support) ? fn_800B2284(support, 0) : 12;
								if (!(drop > 0.5f + supHeight))
									support = NULL;
							}
							if (!support || !fn_800B2250(support)) {
								SetPlayerActionState(&player->mJump, 2);
								SetObjectVerticalSpeed(&player->mMoveTranslate, 0);
							} else if (ClassifyFloatBits(player->_12DF8) == FP_NAN) {
								if (IsEventScriptActive(&gGame) || player->mSuits.mChangeStep) {
									SetObjectVectorAndTargetFromVec(&player->mMoveTranslate, player->_264);
									SetObjectVerticalSpeed(&player->mMoveTranslate, 0);
									player->mFloorContacts[0].objectIndex = player->mFloorContacts[1].objectIndex;
								} else if (GetObjectAnimationId(player) == 4 || GetObjectAnimationId(player) == 13
								           || GetObjectAnimationId(player) == 0x898 || GetObjectAnimationId(player) == 0x819) {
									SetPlayerHangState(player, 4, support);
								} else if (IsPlayerAttachmentEquipped(player, 2) || IsPlayerAttachmentEquipped(player, 3)
								           || !IsLiftActionState(&player->mLift, 0)) {
									SetPlayerHangState(player, 4, support);
								} else {
									if (!IsLiftActionState(&player->mLift, 0))
										ReleasePlayerLiftTarget(player);
									if (fn_800B21B8(support))
										SetPlayerLadderState(player, 3, support);
									else if (fn_800B2164(support))
										SetPlayerRopeState(player, 3, support);
									else
										SetPlayerHangState(player, 3, support);
								}
							}
						}
					}
				}
			}
		} else if (motion.y < player->mFloorHit.mHeight && canRise) {
			if (IsRopeActionState(&player->mRope, 5)) {
				SetObjectYAndTarget(&motion, player->mFloorHit.mHeight);
				SetObjectVerticalSpeed(&motion, 0);
			} else if (IsLadderActionState(&player->mLadder, 5)) {
				SetObjectYAndTarget(&motion, player->mFloorHit.mHeight);
				SetObjectVerticalSpeed(&motion, 0);
			} else {
				SetObjectTargetY(&motion, player->mFloorHit.mHeight);
				f32 speed = 0.2f + GetObjectVectorSpeed(&motion)->y;
				SetObjectVerticalSpeed(&motion, speed > 5 ? 5 : speed);
			}
		} else {
			SetObjectVerticalSpeed(&motion, 0);
		}
	}
	{
		if (!IsGameMenuActive(&gGame) && !(GetMaskedGameStatus(&gGame, 0x100))
		    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4)) && IsHangActionState(&player->mHang, 0)
		    && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
		    && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0) && IsCopterActionState(&player->mCopter, 0)
		    && (IsPopperActionState(&player->mPopper, 0) || IsPopperActionState(&player->mPopper, 8)
		        || IsPopperActionState(&player->mPopper, 4))
		    && IsRadarActionState(&player->mRadar, 0)) {
			const Vec* current    = fn_800B2FC0(&player->_988);
			const Vec* dustTarget = GetObjectVectorTarget(&player->_988);
			f32 rate              = GetPlayerAnimationRateMultiplier(player);
			f32 dustDistance      = GetVectorDistance(current, dustTarget) / rate;
			if (dustDistance >= 3.0f) {
				SpawnPlayerDust(player, GetObjectMoveTranslation(player));
				if (dustDistance >= 5.0f && !IsGameSoundPlaying(GetGameSound(&gGame), 0x1b))
					PlayGameSound(GetGameSound(&gGame), 0x1b, 255);
			}
		}
	}
	{
		f32 reach = 0.5f * radius;
		Vec probePoint;
		probePoint.x            = (player->mMoveTranslate.x + offset.x) + reach * PlayerActionSine(player->mMoveRotate.y);
		probePoint.z            = (player->mMoveTranslate.z + offset.z) + reach * PlayerActionCosine(player->mMoveRotate.y);
		probePoint.y            = player->mMoveTranslate.y;
		f32 probeRadius         = 0.9f * radius;
		player->mHitObjectIndex = player->_9D0 = player->mFloorContacts[2].objectIndex = -1;
		f32 best = NAN, bestTagged = NAN;
		for (s32 i = 0; i < GetStageObjectCount(&gGame); ++i) {
			XObj* object = GetGameObjectByIndex(&gGame, i);

			if (!IsObjectActive(object) || !(GetMaskedObjectStatus(object, 0x10000)) || GetMaskedObjectStatus(object, 0x0004000000000000ULL)
			    || GetMaskedObjectStatus(object, 0x0002000000000000ULL) || GetObjectId(object) == GetObjectId(player))
				continue;
			if (GetHorizontalDistance(GetObjectMoveTranslation(object), &probePoint)
			    > 100.0f + (probeRadius + GetObjectFloorXZExtent(object)))
				continue;
			u32 hit;
			{
				CVec from(probePoint), to(probePoint);
				hit = object->vfFC(&from, &to, probeRadius, height, &push, &contact, 0, queryFlags);
			}
			if ((hit & 3) != 3) {
				ClearObjectStatusFlags(object, 0x8000000000000000ULL);
				continue;
			}
			SelectHighestActiveFloor(object);
			bool highest = player->mHitObjectIndex < 0 || ClassifyFloatBits(best) == FP_NAN || GetSelectedFloorHeight(object) > best;
			if (highest) {
				player->mHitObjectIndex = i;
				best                    = GetSelectedFloorHeight(object);
				if ((object->vf128()
				     && (!GetMaskedObjectStatus(object, 0x20)
				         || (IsLiftTargetPlug(&player->mLift) && player->mHitObjectIndex != player->mFloorContacts[0].objectIndex))))
					player->mFloorContacts[2].objectIndex = i;
			} else if (player->mFloorContacts[2].objectIndex < 0
			           && (object->vf128()
			               && (!GetMaskedObjectStatus(object, 0x20)
			                   || (IsLiftTargetPlug(&player->mLift)
			                       && player->mFloorContacts[2].objectIndex != player->mFloorContacts[0].objectIndex)))) {
				player->mFloorContacts[2].objectIndex = i;
			}
			if ((player->_9D0 < 0 || ClassifyFloatBits(bestTagged) == FP_NAN || GetSelectedFloorHeight(object) > bestTagged)
			    && (GetMaskedObjectStatus(object, 0x000000ff00000000ULL))) {
				player->_9D0 = i;
				bestTagged   = GetSelectedFloorHeight(object);
			}
		}
		if (player->mFloorContacts[0].objectIndex >= 0 && player->mHitObjectIndex < 0) {
			player->mHitObjectIndex = player->mFloorContacts[0].objectIndex;
			if (GetMaskedObjectStatus(GetGameObjectByIndex(&gGame, player->mHitObjectIndex), 0x000000ff00000000ULL))
				player->_9D0 = player->mHitObjectIndex;
		}
	}
	{
		player->_A10 = -1;
		s32 floorIdx = player->mFloorContacts[0].objectIndex;
		if (!((player->mHitObjectIndex >= 0 && player->mHitObjectIndex != floorIdx)
		      || (player->mFloorContacts[2].objectIndex >= 0 && player->mFloorContacts[2].objectIndex != floorIdx)
		      || !IsLiftActionState(&player->mLift, 4) || !IsLiftTargetPlug(&player->mLift))) {
			for (s32 i = 0; i < GetObjectPoolUsedCount(&gGame, 9); ++i) {
				XObj* object = (XObj*)GetObjectFromPool(&gGame, 9, i);
				if (!IsObjectActive(object) || !GetMaskedObjectStatus(object, 0x10000) || !GetMaskedObjectStatus(object, 0x20)
				    || !object->vf128())
					continue;
				if (GetVectorDistance(GetObjectMoveTranslation(object), GetObjectMoveTranslation(player)) >= 18.0f)
					continue;
				f32 yaw   = GetObjectMoveRotation(object)->y;
				f32 angle = WrapPlayerActionAngle(
				    GetHeadingBetweenPositions(GetObjectMoveTranslation(object), GetObjectMoveTranslation(player)) - yaw);
				if (angle < 0.52359879f || angle > 5.7595863f || (angle > 1.04719758f && angle < 2.09439516f)
				    || (angle > 2.61799383f && angle < 3.66519141f) || (angle > 4.18879032f && angle < 5.23598766f)) {
					player->_A10 = GetIndexedCoordPlacement(GetObjectCoordInfo(object), 0) - GetStagePlacementByIndex(0);
					break;
				}
			}
		}
	}
	u32 available     = 0;
	XObj* target      = NULL;
	s32 actionMode    = -1;
	u32 marker        = 0;
	player->mLook._30 = 0;
	if (IsPlayerLookMarkerVisible(&player->mLook))
		++player->mLook._30;
	DisablePlayerLook(&player->mLook);
	if (!IsEventScriptActive(&gGame) && !(GetMaskedGameStatus(&gGame, 0x500)) && player->mAttachmentRequest < 0
	    && (IsLiftActionState(&player->mLift, 0) || (IsLiftActionState(&player->mLift, 4) && !IsCarryingMug(&player->mLift)))
	    && IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
	    && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
	    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0) && IsRadarActionState(&player->mRadar, 0)) {
		{
			s32 index = player->mFloorContacts[2].objectIndex;
			available = 0;
			if (index >= 0 && index != player->mFloorContacts[0].objectIndex) {
				XObj* object = GetGameObjectByIndex(&gGame, index);
				if (!player->_A14 || !GetMaskedObjectStatus(object, 0x100000)) {
					if (fn_800B2048(object))
						available |= 4;
					else if (GetMaskedObjectStatus(object, 0x0000010000000000ULL))
						available |= 1;
				}
				if (GetMaskedObjectStatus(object, 0x20) && IsLiftTargetPlug(&player->mLift)
				    && GetCharacterObjectApproachResult(player, object))
					available |= 0x80;
			} else if (index >= 0 && player->mFloorContacts[2].objectIndex == player->mFloorContacts[0].objectIndex
			           && player->mHitObjectIndex == player->mFloorContacts[2].objectIndex && !player->_A14) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContacts[2].objectIndex);
				if (GetMaskedObjectStatus(object, 0x0000010000000000ULL) && !GetMaskedObjectStatus(object, 0x0000100000000000ULL))
					available |= 1;
			}
		}
		{
			s32 index = player->_9D0;
			if (index >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, index);
				if (fn_800B2048(object) && index != player->mFloorContacts[0].objectIndex)
					available |= 4;
				if (fn_800B2014(object) && index != player->mFloorContacts[0].objectIndex) {
					if (IsLiftActionState(&player->mLift, 0) || (IsLiftActionState(&player->mLift, 4) && IsLiftTargetPlug(&player->mLift)))
						available |= 2;
#if defined(VERSION_JP)
					if (GetMaskedGameStatus(&gGame, 0x80000000000ULL) != 0) {
						target     = object;
						actionMode = 9;
					}
#endif
				} else if (fn_800B200C(object) && index != player->mFloorContacts[0].objectIndex) {
					f32 yaw   = GetObjectMoveRotation(object)->y;
					s32 edge  = GetSelectedFloorIndex(object);
					f32 angle = GetObjectEdgeOutwardYaw(object, GetObjectMoveTranslation(player), edge) - yaw;
					if (angle > 3.13984752f && angle < 3.14333797f)
						available |= 2;
#if defined(VERSION_JP)
					if (GetMaskedGameStatus(&gGame, 0x80000000000ULL) != 0) {
						target     = object;
						actionMode = 9;
					}
#endif
				}
				if (fn_800B2250(object)) {
					f32 climbHeight = GetSelectedFloorHeight(object) - player->mMoveTranslate.y;
					if (climbHeight < 0)
						climbHeight = -climbHeight;
					if (climbHeight != 0) {
						if (climbHeight >= 12 && climbHeight <= 12.5f) {
							if (IsLiftActionState(&player->mLift, 0)
							    || (IsLiftActionState(&player->mLift, 4) && !IsCarryingToothbrush(&player->mLift)
							        && !IsCarryingSpoon(&player->mLift) && !IsCarryingMug(&player->mLift)
							        && !IsCarryingSquirter(&player->mLift))) {
#if defined(VERSION_JP)
								if (GetMaskedGameStatus(&gGame, 0x80000000000ULL) != 0)
									available |= 8;
#endif
#if defined(VERSION_JP)
								if (!GetMaskedGameStatus(&gGame, 0x80000000000ULL))
#endif
								{
									target     = object;
									actionMode = 4;
								}
							}
						} else {
#if defined(VERSION_JP)
							if (GetMaskedGameStatus(&gGame, 0x80000000000ULL) != 0)
								available |= 8;
#endif
							if (climbHeight > 1) {
								if (climbHeight <= 4)
#if defined(VERSION_JP)
									if (!GetMaskedGameStatus(&gGame, 0x80000000000ULL))
#endif
									{
										target     = object;
										actionMode = 1;
									} else if (climbHeight <= 8)
#if defined(VERSION_JP)
										if (!GetMaskedGameStatus(&gGame, 0x80000000000ULL))
#endif
										{
											target     = object;
											actionMode = 2;
										} else if (climbHeight <= 12.5f)
#if defined(VERSION_JP)
											if (!GetMaskedGameStatus(&gGame, 0x80000000000ULL))
#endif
											{
												target     = object;
												actionMode = 3;
											}
							}
						}
					}
				} else if (fn_800B21B8(object)) {
					{
						BOOL inReach = GetSelectedFloorIndex(object) == 0;
						if (inReach) {
							if (index == player->mFloorContacts[0].objectIndex) {
								f32 facing = GetObjectMoveRotation(object)->y;
								f32 angle  = WrapPlayerActionAngle(GetObjectMoveRotation(player)->y - facing);
								inReach    = angle <= HALF_PI || angle >= 1.5f * PI;
							} else {
								f32 playerHeight = GetObjectMaxFloorHeight(player);
								inReach          = GetSelectedFloorHeight(object) > playerHeight
								       && (!true || IsPositionBehindObject(&player->mLadder, GetObjectMoveTranslation(player), object));
							}
						}
						if (inReach)
							available |= 0x10;
					}
				} else if (fn_800B2164(object)) {
					{
						BOOL inReach = GetSelectedFloorIndex(object) == 0;
						if (inReach) {
							if (index == player->mFloorContacts[0].objectIndex) {
								f32 facing = GetObjectMoveRotation(object)->y;
								f32 angle  = WrapPlayerActionAngle(GetObjectMoveRotation(player)->y - facing);
								inReach    = angle <= HALF_PI || angle >= 1.5f * PI;
							} else {
								f32 playerHeight = GetObjectMaxFloorHeight(player);
								inReach          = GetSelectedFloorHeight(object) > playerHeight
								       && (!false || IsPositionBehindObject(&player->mLadder, GetObjectMoveTranslation(player), object));
							}
						}
						if (inReach)
							available |= 0x20;
					}
				} else if (fn_800B1E04(object)) {
					available |= 0x40;
				}
				if ((GetMaskedObjectStatus(object, 0x20)) && IsLiftTargetPlug(&player->mLift)
				    && index != player->mFloorContacts[0].objectIndex && GetCharacterObjectApproachResult(player, object))
					available |= 0x80;
				if (player->_9AC0)
					player->mHitObjectIndex = index;
			}
		}
		{
			if (player->_A10 >= 0)
				available |= 0x80;
			if (!available && player->mHitObjectIndex >= 0
			    && !(player->mFloorContacts[4].objectIndex == player->mHitObjectIndex
			         && player->mLastContactFloorIndex == GetSelectedFloorIndex(GetGameObjectByIndex(&gGame, player->mHitObjectIndex)))
			    && player->mFloorContacts[2].objectIndex >= 0
			    && !(player->mFloorContacts[4].objectIndex == player->mFloorContacts[2].objectIndex
			         && player->mLastContactFloorIndex
			                == GetSelectedFloorIndex(GetGameObjectByIndex(&gGame, player->mFloorContacts[2].objectIndex)))) {
				player->mFloorContacts[4].objectIndex = -1;
				player->mNearDistance                 = 2;
			}
			if (player->mFloorContacts[2].objectIndex >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContacts[2].objectIndex);
				if (0xff & 1)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 1))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 1)))
						marker |= 1;
				if (0xff & 2)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 2))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 2)))
						marker |= 2;
				if (0xff & 4)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 4))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 4)))
						marker |= 4;
				if (0xff & 8)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 8))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 8)))
						marker |= 8;
				if (0xff & 0x10)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x10))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x10)))
						marker |= 0x10;
				if (0xff & 0x20)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x20))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x20)))
						marker |= 0x20;
				if (0xff & 0x40)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x40))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x40)))
						marker |= 0x40;
				if (0xff & 0x80)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x80))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x80)))
						marker |= 0x80;
			}
			if (player->mHitObjectIndex >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
				if (0xfe & 1)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 1))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 1)))
						marker |= 1;
				if (0xfe & 2)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 2))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 2)))
						marker |= 2;
				if (0xfe & 4)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 4))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 4)))
						marker |= 4;
				if (0xfe & 8)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 8))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 8)))
						marker |= 8;
				if (0xfe & 0x10)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x10))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x10)))
						marker |= 0x10;
				if (0xfe & 0x20)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x20))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x20)))
						marker |= 0x20;
				if (0xfe & 0x40)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x40))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x40)))
						marker |= 0x40;
				if (0xfe & 0x80)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x80))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x80)))
						marker |= 0x80;
			}
			if (player->_A10 >= 0) {
				XObj* object = GetGameObjectByIndex(&gGame, player->_A10);
				if (0x80 & 1)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 1))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 1)))
						marker |= 1;
				if (0x80 & 2)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 2))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 2)))
						marker |= 2;
				if (0x80 & 4)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 4))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 4)))
						marker |= 4;
				if (0x80 & 8)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 8))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 8)))
						marker |= 8;
				if (0x80 & 0x10)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x10))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x10)))
						marker |= 0x10;
				if (0x80 & 0x20)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x20))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x20)))
						marker |= 0x20;
				if (0x80 & 0x40)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x40))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x40)))
						marker |= 0x40;
				if (0x80 & 0x80)
					if ((fn_800B1DFC(object) == 0x80000000 && (available & 0x80))
					    || (fn_800B1DFC(object) != 0x80000000 && fn_800B1DF0(object, 0x80)))
						marker |= 0x80;
			}
		}
	}
	{
		u32 count = 0;
		if (marker & 0x1)
			++count;
		if (marker & 0x2)
			++count;
		if (marker & 0x4)
			++count;
		if (marker & 0x10)
			++count;
		if (marker & 0x20)
			++count;
		if (marker & 0x40)
			++count;
		if (marker & 0x80)
			++count;
		if (marker & 0x8)
			++count;
		if (count) {
			EnablePlayerLook(&player->mLook);
#if defined(VERSION_JP)
			const s32 normalMode = 0x6e;
#else
			const s32 normalMode = 0x70;
#endif
			if (marker & 0x80) {
				SetPlayerLookMode(&player->mLook, normalMode + 1);
				SetLookTargetPosition(&player->mLook, 0, 0, 0);
			} else {
				SetPlayerLookMode(&player->mLook, normalMode);
				SetLookTargetPosition(&player->mLook, 0, 0, -0.52359879f);
			}
			if (count > player->mLook._30)
				player->mLook._30 = count;
		} else {
			DisablePlayerLook(&player->mLook);
		}
	}
	if (!marker) {
		if (!IsHangActionState(&player->mHang, 0)) {
			if (IsHangActionState(&player->mHang, 5) && (player->_9BC[0] > 0) && HasPlayerActionTarget(&player->mHang)) {
				f32 angle = WrapPlayerActionAngle((player->_9BC[2] + GetCameraAngles(gGame.GetCamera())->y) - player->mMoveRotate.y);
				if ((angle < 0.52359879f || angle > 5.7595863f) && fn_800B1D50(GetPlayerActionTarget(&player->mHang))) {
					target     = GetPlayerActionTarget(&player->mHang);
					actionMode = 5;
				}
				if (angle > 2.61799383f && angle < 3.66519141f) {
#if defined(VERSION_JP)
					if (!GetMaskedGameStatus(&gGame, 0x80000000000ULL))
#endif
					{
						target     = GetPlayerActionTarget(&player->mHang);
						actionMode = 6;
					}
				}
			}
		} else {
			if (!IsCopterActionState(&player->mCopter, 0))
				IsCopterActionState(&player->mCopter, 3);
			else if (!IsRopeActionState(&player->mRope, 0))
				IsRopeActionState(&player->mRope, 5);
			else if (!IsLadderActionState(&player->mLadder, 0))
				IsLadderActionState(&player->mLadder, 5);
			else if (!IsPushActionState(&player->mPush, 0))
				IsPushActionState(&player->mPush, 2);
			else if (IsPlayerAttachmentEquipped(player, 1))
				IsCopterActionState(&player->mCopter, 0);
			else if (IsPlayerAttachmentEquipped(player, 2))
				IsPopperActionState(&player->mPopper, 0);
			else
				IsPlayerAttachmentEquipped(player, 3);
		}
	}
	{
		if (player->_146D8 && !GetMaskedGameStatus(&gGame, 0x20)
		    && (IsPlayerAttachmentEquipped(player, 0) || IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2)
		        || IsPlayerAttachmentEquipped(player, 3) || IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5)
		        || IsPlayerAttachmentEquipped(player, 6) || IsPlayerAttachmentEquipped(player, 7)
		        || IsPlayerAttachmentEquipped(player, 8))) {
			f32 magnitude = GetControlStickMagnitude(&player->mPad);
			if (magnitude == 0 && player->mFloorContacts[4].objectIndex >= 0)
				magnitude = 0.8f;
			if (target && !GetMaskedObjectStatus(target, 0x0002000000000000ULL) && !GetHeldPadButtons(&player->mPad, 0x200)
			    && magnitude > 0) {
				f32 threshold = FRAME_RATE * fn_800B1C14();
				if ((s32)target == player->_9AC0) {
					if (fn_800B1BCC() > 0)
						player->_9AC4 += frames * fn_800B1BCC();
					else
						player->_9AC4 += magnitude * frames;
					f32 cap = fn_800B1B84();
					if (cap > 0 && magnitude <= 0.7f && player->_9AC4 > threshold * cap)
						player->_9AC4 = threshold * cap;
				} else {
					player->_9AC0 = (s32)target;
					player->_9AC4 = 0;
				}
				if (player->_9AC4 >= threshold) {
					XObj* hit = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
					if (actionMode == 1) {
						SetPlayerActionState(&player->mJump, 8);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 2) {
						SetPlayerActionState(&player->mJump, 11);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 3) {
						SetPlayerActionState(&player->mJump, 14);
						SetPlayerActionTarget(&player->mJump, hit);
					} else if (actionMode == 4) {
						if (fn_800B1B24(player))
							ReleasePlayerLiftTarget(player);
						SetHangActionState(&player->mHang, 1);
						SetPlayerActionTarget(&player->mHang, hit);
					} else if (actionMode == 5) {
						if (fn_800B1B24(player))
							ReleasePlayerLiftTarget(player);
						TryLeaveHang(&player->mHang);
						StopPlayerMotion(player);
					} else if (actionMode == 6) {
						if (fn_800B1B24(player))
							ReleasePlayerLiftTarget(player);
						SetHangActionState(&player->mHang, 9);
						StopPlayerMotion(player);
					} else if (actionMode == 9) {
						if (fn_800B1B24(player))
							ReleasePlayerLiftTarget(player);
						StopPlayerMotion(player);
						SetPushActionState(&player->mPush, 1);
						SetPlayerActionTarget(&player->mPush, hit);
						fn_800B1B0C(&player->mPush, 0);
					}
				}
			}
		}
		player->_9AC0 = 0;
		player->_9AC4 = 0;
	}
}

#include "game/video.h"
#include "game/xobjchrstate.h"
#include "game/playeritemanim.h"
#include "game/cmdline.h"
#include "game/hsd_dobj.h"
#include "game/xobjmap.h"
#include "game/texturecopy.h"
#include "game/cvec.h"
#include "game/cjobjinlineattach.h"
#include "game/cjobjset.h"
#include "game/globals.h"
#include "game/pointptcl.h"
#include "game/animframe.h"
#include <stdlib.h>
#include <string.h>
#include "game/drawhelptext.h"

extern "C" {
f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to);
HSD_JObj* GetObjectJointByPath(const CJObj* object, s32 depth, s32* path);
s32 GetObjectPoolId(const XObj* object);
BOOL IsEventScriptActive(const CGame* game);
BOOL IsLiftActionState(const XActLift* act, s32 wanted);
BOOL IsPlayerAttachmentEquipped(const XObjChr* chr, s32 wanted);
BOOL IsPopperActionState(const XActPopper* act, s32 wanted);
BOOL IsRadarActionState(const XActRadar* act, s32 wanted);
BOOL IsTumbleActionState(const XActTumble* act, s32 wanted);
void SetObjectVectorTarget(CObjVec* value, const Vec& target);
void SetPlayerActionTarget(XAction* action, XObj* target);
void SetPushActionState(XActPush* act, s32 state);
void UpdatePlayerEquipmentRings(XObjChrMenu* menu, u32 frames);
BOOL IsCopterActionState(const XAction* action, s32 state);
BOOL IsJumpActionState(const XAction* action, s32 state);
BOOL IsLadderActionState(const XAction* action, s32 state);
BOOL IsRopeActionState(const XAction* action, s32 state);
BOOL IsObjectActive(XObj* object);
void fn_800B1B0C(XActPush* act, s32 value);
BOOL fn_800B1B24(const XObjChr* chr);
BOOL IsPositionBehindObject(void*, const Vec* pos, CXObj* object);
BOOL fn_800B2014(const XObj* object);
BOOL fn_800B2048(const XObj* object);
BOOL fn_800B2164(const XObj* object);
BOOL fn_800B21B8(const XObj* object);
void ReleasePlayerLiftTarget(XObjChr* chr);
BOOL fn_800B2250(const XObj* object);
void SetCopterActionState(XAction* action, s32 state);
f32 WrapPlayerActionAngle(f32 angle);
BOOL IsHangActionState(const XActHang* act, s32 wanted);
XObj* GetGameObjectByIndex(const CGame* game, s32 index);
BOOL IsPushActionState(const XActPush* act, s32 wanted);
u32 fn_800B31AC(const XObjChr* chr, u32 mask);
void UpdatePlayerAttachmentRequest(XObjChr* chr, f32* speed);
void OpenPlayerEquipmentRings(XObjChr* chr);
BOOL IsGameRunning(CGame*);
void SetHangActionState(XActHang*, s32);
void SetPlayerActionState(XAction*, s32);
void UpdateOXMenuTransition(OXMenu*, u32);
void TryStartObjectEvent(CGame*, const void*, void*, s16, s16);
void SetLiftActionState(XActLift*, s32);
void SetLadderActionState(XActLadder*, s32);
void SetRopeActionState(XActRope*, s32);
void SetRadarActionState(XActRadar*, s32);
void SetPopperActionState(XActPopper*, s32);
void fn_800B503C(XActLift*, s32);
void RequestPlayerUnequipAttachment(XObjChr*);
BOOL VectorsDiffer(const Vec*, const Vec*);
Vec GetJointMatrixTranslation(void*, HSD_JObj*);
BOOL IsPopperFireAnimationSelected(XObjPopper*);
void fn_800D6340(XObjRadar*);
void fn_800D7B18(XObjCopter*);
void HandleLiftActionInput(XActLift*, const CPadOne*, f32*);
void HandleHangActionInput(XActHang*, const CPadOne*, f32*);
void SuppressMovementDuringJump(XActJump*, const CPadOne*, f32*);
void HandleLadderActionInput(XActLadder*, const CPadOne*, f32*);
void HandleRopeActionInput(XActRope*, const CPadOne*, f32*);
void HandlePushActionInput(XActPush*, const CPadOne*, f32*);
void HandleTumbleActionInput(XActTumble*, const CPadOne*, f32*);
void HandleCopterActionInput(XActCopter*, const CPadOne*, f32*);
void ApplyPopperMovementWeight(XActPopper*, const CPadOne*, f32*);
void ApplyRadarMovementWeight(XActRadar*, const CPadOne*, f32*);
}

static inline void ReleaseCarriedTarget(XObjChr* player)
{
	if (fn_800B1B24(player))
		ReleasePlayerLiftTarget(player);
}
static inline void LiftInteractionTarget(XObjChr* player, XObj* target)
{
	SetLiftActionState(&player->mLift, 1);
	SetPlayerActionTarget(&player->mLift, target);
	fn_800B503C(&player->mLift, 0);
}
static inline void StartInteractionEvent(XObj* target)
{
	TryStartObjectEvent(&gGame, target->vf128(), target, -1, -1);
}
static inline void ChooseClimbAction(XObjChr* player, XObj* object, s32 mode, bool ladder)
{
	s32 dir = 0;
	if (object->mSelectedFloorIndex == 0) {
		if (player->mHitObjectIndex == player->mFloorContacts[0].objectIndex) {
			f32 angle = WrapPlayerActionAngle(player->mMoveRotate.y - object->mMoveRotate.y);
			if (angle <= PI * .5f || angle >= PI * 1.5f)
				dir = -1;
		} else if (object->mSelectedFloorHeight > player->mMaxFloorHeight
		           && (!ladder || IsPositionBehindObject(&player->mLadder, &player->mMoveTranslate, object)))
			dir = 1;
	}
	if (dir != 0) {
		if (mode == 1) {
			player->_95E0 = 0;
			player->_95E4 = ladder ? 1 : 2;
			RequestPlayerUnequipAttachment(player);
		} else if (mode == 2) {
			ReleaseCarriedTarget(player);
			if (ladder) {
				if (dir > 0)
					SetLadderActionState(&player->mLadder, 1);
				else
					SetLadderActionState(&player->mLadder, 2);
				SetPlayerActionTarget(&player->mLadder, object);
			} else {
				if (dir > 0)
					SetRopeActionState(&player->mRope, 1);
				else
					SetRopeActionState(&player->mRope, 2);
				SetPlayerActionTarget(&player->mRope, object);
			}
		}
		StopPlayerMotion(player);
	}
}

/**
 * @note Address: 0x800B35CC
 */
extern "C" f32 UpdatePlayerActions(XObjChr* player, u32 frames)
{
	if (fn_800B31AC(player, 0x80000000))
		return 0;
	f32 speed          = -1;
	const CPadOne* pad = &player->mPad;
	if (IsGameRunning(&gGame) && !IsEventScriptActive(&gGame) && (player->_980.mFlags & 0x20000000)
	    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
	    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
	        && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
	        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
	        && IsRadarActionState(&player->mRadar, 0))) {
		CPad controller(0);
		if (!gGame.mMenu.mActive) {
			if ((controller.mPad->mTrigger & 0x400) && !(controller.mPad->mButton & 0x1000) && (gGame.mStatus.mFlags & 0x800000000000ULL)
			    && player->mAttachmentRequest < 0) {
				if (!(gGame.mStatus.mFlags & 0x100))
					OpenPlayerEquipmentRings(player);
				else
					PlayGameSound(&gGame.mSound, 0x85, 255);
			} else if (!IsPlayerAttachmentEquipped(player, 0) && (pad->mTrigger & 0x200) && !(pad->mButton & 0x1000)
			           && (!(gGame.mStatus.mFlags & 0x100) && player->mAttachmentRequest < 0)) {
				RequestPlayerUnequipAttachment(player);
				return 0;
			}
		}
		if (!(gGame.mStatus.mFlags & 0x400)
		    && (IsPlayerAttachmentEquipped(player, 0) || IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2)
		        || IsPlayerAttachmentEquipped(player, 3) || IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5)
		        || IsPlayerAttachmentEquipped(player, 6) || IsPlayerAttachmentEquipped(player, 7)
		        || IsPlayerAttachmentEquipped(player, 8))) {
			BOOL press = (pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x500) && player->mAttachmentRequest < 0;
			s32 mode;
			if (IsPlayerAttachmentEquipped(player, 1) || IsPlayerAttachmentEquipped(player, 2) || IsPlayerAttachmentEquipped(player, 3)) {
				switch (player->mAttachmentActionType) {
				case 1:
					mode = 1;
					break;
				case 2:
					mode = 2;
					break;
				default:
					mode = 0;
				}
			} else if (IsPlayerAttachmentEquipped(player, 6))
				mode = 0;
			else if (IsPlayerAttachmentEquipped(player, 4) || IsPlayerAttachmentEquipped(player, 5) || IsPlayerAttachmentEquipped(player, 7)
			         || IsPlayerAttachmentEquipped(player, 8))
				if (player->mToolActionType == 1)
					mode = 1;
				else
					mode = 0;
			else
				mode = 2;
			{
				BOOL emptyHands = IsPlayerAttachmentEquipped(player, 0);
				if (press && player->_A10 >= 0) {
					XObj* object = GetGameObjectByIndex(&gGame, player->_A10);
					if (object->mStatus.mFlags & 0x10000000000ULL)
						StartInteractionEvent(object);
				} else if (press
				           || (player->mFloorContacts[4].objectIndex >= 0
				               && player->mFloorContacts[4].objectIndex == player->mHitObjectIndex)
				           || player->_95E0) {
					BOOL handled  = FALSE;
					player->_95E0 = 0;
					if (emptyHands && player->mFloorContacts[2].objectIndex >= 0
					    && player->mFloorContacts[2].objectIndex != player->mFloorContacts[0].objectIndex) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContacts[2].objectIndex);
						if (fn_800B2048(object)) {
							if ((IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
							    && object->mMaxFloorHeight > 1.f + player->mMoveTranslate.y) {
								ReleaseCarriedTarget(player);
								LiftInteractionTarget(player, object);
								handled = true;
							}
						} else if (object->mStatus.mFlags & 0x10000000000ULL) {
							StartInteractionEvent(object);
							handled = true;
						}
					} else if (emptyHands && player->mFloorContacts[2].objectIndex >= 0
					           && player->mFloorContacts[2].objectIndex == player->mFloorContacts[0].objectIndex
					           && player->mHitObjectIndex == player->mFloorContacts[2].objectIndex) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContacts[2].objectIndex);
						if ((object->mStatus.mFlags & 0x10000000000ULL) && !(object->mStatus.mFlags & 0x100000000000ULL)
						    && (s16)GetObjectPoolId(object) != 8) {
							StartInteractionEvent(object);
							handled = true;
						}
					}
					if (!handled && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
					    && player->mHitObjectIndex >= 0) {
						XObj* object = GetGameObjectByIndex(&gGame, player->mHitObjectIndex);
						if (emptyHands && fn_800B2048(object) && player->mHitObjectIndex != player->mFloorContacts[0].objectIndex
						    && object->mMaxFloorHeight > 1.f + player->mMoveTranslate.y) {
							ReleaseCarriedTarget(player);
							SetLiftActionState(&player->mLift, 1);
							SetPlayerActionTarget(&player->mLift, GetGameObjectByIndex(&gGame, player->mHitObjectIndex));
							fn_800B503C(&player->mLift, 0);
						} else if (emptyHands && fn_800B2250(object)) {
// Only the Japanese release starts a hang or a jump from this height check.
#if defined(VERSION_JP)
							f32 height = Abs(object->mSelectedFloorHeight - player->mMoveTranslate.y);
							if (height != 0 && (gGame.mStatus.mFlags & 0x80000000000ULL)) {
								if (height >= 12 && height <= 12.5f) {
									ReleaseCarriedTarget(player);
									SetHangActionState(&player->mHang, 1);
									SetPlayerActionTarget(&player->mHang, object);
								} else if (height <= 4) {
									ReleaseCarriedTarget(player);
									SetPlayerActionState(&player->mJump, 8);
									SetPlayerActionTarget(&player->mJump, object);
								} else if (height <= 8) {
									ReleaseCarriedTarget(player);
									SetPlayerActionState(&player->mJump, 11);
									SetPlayerActionTarget(&player->mJump, object);
								} else if (height <= 12.5f) {
									ReleaseCarriedTarget(player);
									SetPlayerActionState(&player->mJump, 14);
									SetPlayerActionTarget(&player->mJump, object);
								}
							}
#endif
						} else if (fn_800B21B8(object))
							ChooseClimbAction(player, object, mode, true);
						else if (fn_800B2164(object))
							ChooseClimbAction(player, object, mode, false);
#if defined(VERSION_JP)
						if (!(gGame.mStatus.mFlags & 0x80000000000ULL))
#endif
							if ((IsLiftActionState(&player->mLift, 0)
							     || (IsLiftActionState(&player->mLift, 4) && IsLiftTargetPlug(&player->mLift)))
							    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0)
							        && IsLadderActionState(&player->mLadder, 0) && IsRopeActionState(&player->mRope, 0)
							        && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
							        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
							        && IsRadarActionState(&player->mRadar, 0))
							    && fn_800B2014(object) && player->mHitObjectIndex != player->mFloorContacts[0].objectIndex) {
								ReleaseCarriedTarget(player);
								StopPlayerMotion(player);
								SetPushActionState(&player->mPush, 1);
								SetPlayerActionTarget(&player->mPush, object);
								fn_800B1B0C(&player->mPush, 0);
							}
					}
				}
			}
		}
		if (!IsEventScriptActive(&gGame) && !(gGame.mStatus.mFlags & 0x400) && player->mAttachmentRequest < 0
		    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
		    && (IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
		        && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
		        && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
		        && IsRadarActionState(&player->mRadar, 0))) {
			if (IsPlayerAttachmentEquipped(player, 0)) {
				if (!(gGame.mStatus.mFlags & 0x100)) {
					if (player->mPlug._D3C && (pad->mTrigger & 0x300)) {
						if (player->mLift.mPlugTarget) {
							StartInteractionEvent(player->mLift.mPlugTarget);
							StopPlayerMotion(player);
							speed = 0;
						}
					} else if (IsLiftActionState(&player->mLift, 0) && IsPushActionState(&player->mPush, 0) && !player->mPlug._D3C
					           && player->mFloorContacts[4].objectIndex < 0 && (pad->mButton & 0x100))
						player->mLiftPlugFlags |= 0x80;
				}
			} else if (IsPlayerAttachmentEquipped(player, 1)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100)) {
					SetCopterActionState(&player->mCopter, 3);
					player->SetAnim(39, FALSE, NULL, NULL, NULL, 0, 1, 0, 10);
					fn_800D7B18(&player->mCopterObj);
					StopPlayerMotion(player);
				}
			} else if (IsPlayerAttachmentEquipped(player, 2)) {
				if (pad->mTrigger & 0x100) {
					SetPopperActionState(&player->mPopper, 7);
					SetObjectVectorTarget(&player->mMoveRotate, player->mMoveRotate);
				}
			} else if (IsPlayerAttachmentEquipped(player, 3)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100)) {
					SetRadarActionState(&player->mRadar, 3);
					fn_800D6340(&player->mRadarObj);
				}
			} else if (IsPlayerAttachmentEquipped(player, 4) && !IsLiftActionState(&player->mLift, 0)
			           && IsCarryingToothbrush(&player->mLift)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100))
					SetLiftActionState(&player->mLift, 27);
			} else if (IsPlayerAttachmentEquipped(player, 5) && !IsLiftActionState(&player->mLift, 0) && IsCarryingSpoon(&player->mLift)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100) && player->mAnimId != 0x8ae)
					SetLiftActionState(&player->mLift, 29);
			} else if (IsPlayerAttachmentEquipped(player, 6) && !IsLiftActionState(&player->mLift, 0) && IsCarryingMug(&player->mLift)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100))
					SetLiftActionState(&player->mLift, 31);
			} else if (IsPlayerAttachmentEquipped(player, 7) && !IsLiftActionState(&player->mLift, 0)
			           && IsCarryingSquirter(&player->mLift)) {
				if ((pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100))
					SetLiftActionState(&player->mLift, 35);
			} else
				IsPlayerAttachmentEquipped(player, 8);
		}
	}
	if (player->mLiftPlugFlags & 0x80) {
		bool lift = false;
		if (player->_A20)
			player->_A1C += frames;
		else {
			player->_A1C = 0;
			player->_A20 = (pad->mTrigger >> 8) & 1;
		}
		if ((player->mLiftPlugFlags & 2) && player->_A1C >= FRAME_RATE * ((s32)fn_8006E9AC(3) / 100.f)) {
			if (player->_A20)
				lift = true;
		} else if (player->mLiftPlugFlags & 1) {
			s32 path[] = { 0 };
			Vec joint  = GetJointMatrixTranslation(player, GetObjectJointByPath(&player->mPlug, 1, path));
			f32 angle  = WrapPlayerActionAngle(player->mMoveRotate.y - GetHeadingBetweenPositions(&player->mMoveTranslate, &joint));
			if ((angle < PI * .5f || angle > PI * 1.5f) && player->_A20)
				lift = true;
		} else if (!(player->mLiftPlugFlags & 2))
			lift = true;
		if (lift) {
			LiftInteractionTarget(player, &player->mPlug);
			player->_A1C = 0;
		}
	} else
		player->_A1C = player->_A20 = 0;
	if (IsPlayerAttachmentEquipped(player, 2) && !IsEventScriptActive(&gGame)
	    && (IsPopperActionState(&player->mPopper, 7) || IsPopperActionState(&player->mPopper, 8))
	    && IsPopperFireAnimationSelected(&player->mPopperObj) && (pad->mTrigger & 0x100))
		SetPopperActionState(&player->mPopper, 5);
	if ((pad->mTrigger & 0x10) && !IsEventScriptActive(&gGame) && !(gGame.mStatus.mFlags & 0x400) && player->mAttachmentRequest < 0
	    && !(gGame.mStatus.mFlags & 0x20) && !(gGame.mStatus.mFlags & 0x100)
	    && (IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
	    && (IsHangActionState(&player->mHang, 0) || IsHangActionState(&player->mHang, 5)) && IsJumpActionState(&player->mJump, 0)
	    && (IsLadderActionState(&player->mLadder, 0) || IsLadderActionState(&player->mLadder, 5))
	    && (IsRopeActionState(&player->mRope, 0) || IsRopeActionState(&player->mRope, 5))
	    && (IsPushActionState(&player->mPush, 0) || IsPushActionState(&player->mPush, 2)) && IsTumbleActionState(&player->mTumble, 0)
	    && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0) && IsRadarActionState(&player->mRadar, 0))
		TryStartObjectEvent(&gGame, GetStageZButtonScript(), NULL, -1, -1);
	if (player->mFloorContacts[4].objectIndex >= 0) {
		bool clear = false;
		if (IsEventScriptActive(&gGame) || (gGame.mStatus.mFlags & 0x400)
		    || !(IsLiftActionState(&player->mLift, 0) || IsLiftActionState(&player->mLift, 4))
		    || !(IsHangActionState(&player->mHang, 0) && IsJumpActionState(&player->mJump, 0) && IsLadderActionState(&player->mLadder, 0)
		         && IsRopeActionState(&player->mRope, 0) && IsPushActionState(&player->mPush, 0) && IsTumbleActionState(&player->mTumble, 0)
		         && IsCopterActionState(&player->mCopter, 0) && IsPopperActionState(&player->mPopper, 0)
		         && IsRadarActionState(&player->mRadar, 0)))
			clear = true;
		else if (player->mHitObjectIndex >= 0) {
			if (player->mHitObjectIndex != player->mFloorContacts[0].objectIndex)
				clear = true;
			else if (player->mFloorContacts[4].objectIndex == player->mHitObjectIndex
			         && player->mLastContactFloorIndex == GetGameObjectByIndex(&gGame, player->mHitObjectIndex)->mSelectedFloorIndex)
				clear = true;
		}
		XObj* object = GetGameObjectByIndex(&gGame, player->mFloorContacts[4].objectIndex);
		if (!IsObjectActive(object) || !(object->mStatus.mFlags & 0x10000))
			clear = true;
		else {
			Vec pos = object->mMoveTranslate;
			if (VectorsDiffer(&player->_9E8, &pos))
				clear = true;
		}
		if (clear) {
			player->mFloorContacts[4].objectIndex = -1;
			player->mNearDistance                 = 2;
		}
	}
	HandleLiftActionInput(&player->mLift, pad, &speed);
	HandleHangActionInput(&player->mHang, pad, &speed);
	SuppressMovementDuringJump(&player->mJump, pad, &speed);
	HandleLadderActionInput(&player->mLadder, pad, &speed);
	HandleRopeActionInput(&player->mRope, pad, &speed);
	HandlePushActionInput(&player->mPush, pad, &speed);
	HandleTumbleActionInput(&player->mTumble, pad, &speed);
	HandleCopterActionInput(&player->mCopter, pad, &speed);
	ApplyPopperMovementWeight(&player->mPopper, pad, &speed);
	ApplyRadarMovementWeight(&player->mRadar, pad, &speed);
	UpdatePlayerAttachmentRequest(player, &speed);
	UpdatePlayerEquipmentRings(&player->mMenu, frames);
	UpdateOXMenuTransition(&player->mOXMenu, frames);
	return speed == -1.f ? 1.f : speed;
}

// XObjChr, chibi robo himself, and the menus he opens. most of the small functions at the end
// are out of line copies of the inlines the player code uses

// the globals label and icon ids are 2 lower in japan
#if defined(VERSION_JP)
#define DRS_LABEL(id) ((id) - 2)
#else
#define DRS_LABEL(id) (id)
#endif

// the globals label and icon ids are 2 lower in japan
#if defined(VERSION_JP)
#define ATC_LABEL(id) ((id) - 2)
#else
#define ATC_LABEL(id) (id)
#endif

extern "C" s32 GetPlayerAttachmentCount(XObjChr* chr, s32 type);

extern "C" const char lbl_8065D4AC[1]; // ""

// todo: cleanup
static inline s32 FindAttachment(const AttachmentValues* values, s32 id)
{
	for (s32 i = 0; i < 9; ++i) {
		if (values->mEntries[i].mId == id)
			return i;
	}
	return -1;
}

// 0.3 seconds
#if defined(VERSION_PAL)
#define LOOK_FADE_STEP (1.0f / (0.3f * gVideoFramesPerSecond))
#else
#define LOOK_FADE_STEP (1.0f / 18.0f)
#endif

#if defined(VERSION_PAL)
#define IDLE_SECOND gVideoFramesPerSecond
#else
#define IDLE_SECOND 60
#endif

// todo: cleanup
static inline u8 DiskBusy(CGame* game)
{
	return (u8)((game->mDvdStatus || game->mPrevDvdStatus) && gGame.mDiskOk == 0);
}

// todo: cleanup
static inline u8 InputEnabled(CGame* game)
{
	return (u8)((game->mStatus.mFlags & 1) && !(game->mStatus.mFlags & 0x22) && !DiskBusy(game));
}

extern "C" {
s32 GetClearLetickerApproachSide(XObj* target, const Vec* position);
void ApplyPlayerHitReaction(XObjChr* chr, BOOL start, BOOL cancel, f32 value);
}

// todo: cleanup
static inline f32 CurrentFrame(HSD_JObj* joint)
{
	HSD_AObj* animation = GetAObj(joint);
	return animation != NULL ? HSD_AObjGetCurrentFrame(animation) : 0.0f;
}

// joints that follow an item's own anim
struct PlayerTurnState {
	f32 heading, previousHeading;
	f32 change, previousChange;
	f32 total;
};

extern "C" PlayerTurnState gPlayerTurnState;
extern "C" f32 GetControlStickMagnitude(const CPadOne*);
extern "C" s32 FindNearbyPlayerInteraction(XObjChr*, f32);
extern "C" const Vec* GetCurrentEdgeWorldOffsetPosition(const XObj*);
extern "C" void UpdatePlayerSuitChange(XObjChr*);
extern "C" void fn_800AC660(XObjChr*, u32, f32, f32);
extern "C" void UpdatePlayerActionAnimations(XObjChr*, u32);
extern "C" void UpdatePlayerHeadOffset(XObjChr*, u32);
extern "C" void UpdatePlayerBattery(XObjChr*, u32);
extern "C" void UpdatePushedObject(XActPush*);
extern "C" s32 GetPlayerGroundType(XObjChr*);
extern "C" void UpdatePlayerFrameEffects(XObjChr*);
extern "C" void UpdatePlayerLookMarker(XObjChrLook*, u32);
extern "C" void UpdateTumbleFade(XActTumbleFade*, BOOL, u32);
extern "C" void UpdateRadarBeamAnimation(XActRadar*, u32);
extern "C" void UpdatePointParticleDisplay(PointPtcl*, u32);
extern "C" void UpdateSpiralTrail(SuitChangeEffect*, u32);

// the joints an item's animation runs on, by item
XObjChrItemAnim gPlayerItemAnimationTable[] = {
	{ 3, 8, { 0, 1, 0, 1, 2, 0, 1, 0 }, 0.0f },
	{ 3, 9, { 0, 1, 0, 1, 2, 0, 1, 0, 0 }, 0.0f },
	{ 4, 6, { 0, 1, 0, 1, 0, 0 }, 0.0f },
	{ 4, 6, { 0, 1, 0, 1, 0, 1 }, 0.0f },
	{ -1, 0, { 0 }, 0.0f },
};

void XObjChr::UpdateObject(u32 frames)
{
	PlayerTurnState& turn = gPlayerTurnState;
	XPad* pad             = &mPad;
	if (pad->mReset && !mMenu.IsOpen() && !mOXMenu.mStarted && !gGame.mMenu.mActive && !mSuits.mChangeStep)
		pad->Update(frames);
	else
		pad->Clear();
	if ((mMenu.mAtc.mOpen > 0 || mMenu.mDrs.mOpen > 0) && mTumble.mState) {
		PlayGameSound(&gGame.mSound, 0x86, 255);
		if (mMenu.mAtc._280 != 3)
			mMenu.mSel = mMenu.mPrevSel;
		mMenu.mAtc.Select(0);
		mMenu.mDrs.Select(0);
	}
	_9BC[1] = _9BC[0];
	_9BC[3] = _9BC[2];
	_9BC[0] = InputEnabled(&gGame) && !gGame.mEvt.mData.mScript ? GetControlStickMagnitude(pad) : 0.0f;
	_9BC[2] = _9BC[0] > 0 ? -(f32)atan2(pad->mStickX, pad->mStickY) : -1.f;
	if (InputEnabled(&gGame) && !gGame.mEvt.mData.mScript && GetAttachment() != 1 && GetAttachment() != 2 && GetAttachment() != 3
	    && (mLift.mState == 0
	        || (mLift.mState == 4 && !IsCarryingToothbrush(&mLift) && !IsCarryingSpoon(&mLift) && !IsCarryingMug(&mLift)
	            && !IsCarryingSquirter(&mLift)))
	    && !mHang.mState && !mJump.mState && !mLadder.mState && !mRope.mState && !mPush.mState && !mTumble.mState && !mCopter.mState
	    && !mPopper.mState && !mRadar.mState) {
		if (mFloorContacts[4].objectIndex < 0 && !_A14 && (pad->mTrigger & 0x100) && !(gGame.mStatus.mFlags & 0x100)
		    && (mHitObjectIndex < 0 || mHitObjectIndex == mFloorContacts[0].objectIndex
		        || (!IsLiftTargetPlug(&mLift) && mHitObjectIndex >= 0 && (gGame.GetObject(mHitObjectIndex)->mStatus.mFlags & 0x20)))) {
			for (mNearDistance = 2; mNearDistance <= 10; ++mNearDistance) {
				mFloorContacts[4].objectIndex = FindNearbyPlayerInteraction(this, mNearDistance);
				if (mFloorContacts[4].objectIndex >= 0) {
					XObj* object = gGame.GetObject(mFloorContacts[4].objectIndex);
					_9E8.Set(object->mMoveTranslate);
					mLastContactFloorIndex = object->mSelectedFloorIndex;
					_9FC                   = CVec(*GetCurrentEdgeWorldOffsetPosition(object));
					break;
				}
			}
		}
		if (mFloorContacts[4].objectIndex >= 0) {
			_9BC[2]
			    = WrapAngle((f32)atan2(_9FC.x - mMoveTranslate.x, _9FC.z - mMoveTranslate.z)) - gGame.mMotionControlCamera.mCamera.mAngle.y;
			_9BC[0] = 0.8f;
		}
	} else {
		mFloorContacts[4].objectIndex = -1;
		mNearDistance                 = 2;
	}
	mLiftPlugFlags &= ~0x80;

	f32 speed = 1.2f * _9BC[0];

	if (!mHang.mState && !mJump.mState && !mLadder.mState && !mRope.mState && !mPush.mState && !mTumble.mState && !mCopter.mState
	    && (mPopper.mState == 0 || mPopper.mState == 8 || mPopper.mState == 4) && !mRadar.mState)
		speed *= 1.2f;
	if (speed > 0 && speed < 0.5f)
		speed = 0.5f;
	if (speed > 1.0f)
		speed *= speed;

	f32 direction;
	if (speed > 0) {

		if (gGame.mStatus.mFlags & 0x100) {
			if (gGame.mStatus.mFlags & 0x200) {
				f32 target = mMoveRotate.mTarget.y;
				if ((_9BC[2] >= 0 && _9BC[2] <= HALF_PI) || (_9BC[2] <= 0 && _9BC[2] >= -HALF_PI))
					direction = WrapAngle(target + (2 * DEG2RAD * frames) * (_9BC[2] / HALF_PI));
				else if (_9BC[2] >= HALF_PI && _9BC[2] <= 130 * DEG2RAD)
					direction = WrapAngle(target + (2 * DEG2RAD * frames) * ((130 * DEG2RAD - _9BC[2]) / (40 * DEG2RAD)));
				else if (_9BC[2] <= -HALF_PI && _9BC[2] >= -130 * DEG2RAD)
					direction = WrapAngle(target + (2 * DEG2RAD * frames) * ((-130 * DEG2RAD - _9BC[2]) / HALF_PI));
				else
					direction = -WrapAngle(_9BC[2] + target);
			} else {
				direction = NAN;
				speed     = 0;
			}
		} else if (mHang.mState == 5) {
			f32 yaw   = mMoveRotate.y;
			f32 angle = WrapAngle((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y) - yaw);
			if (angle >= 30 * DEG2RAD && angle < 150 * DEG2RAD) {
				direction = TWO_PI + WrapAngle(HALF_PI + yaw);
				mHang._0C = -1;
				speed     = 0.5f;
			} else if (angle >= 210 * DEG2RAD && angle < 330 * DEG2RAD) {
				direction = TWO_PI + WrapAngle(yaw - HALF_PI);
				mHang._0C = 1;
				speed     = 0.5f;
			} else {
				direction = NAN;
				speed     = 0;
				mHang._0C = 0;
			}
		} else if (mCopter.mState == 3) {
			mCopter._0C = 1;
			direction   = WrapAngle((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y));
		} else if (mCopter.mState == 4 || mCopter.mState == 7 || mCopter.mState == 8) {
			if ((gGame.mStatus.mFlags & 0x40000000) && mParams.mBattery <= 0 && InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn)) {
				direction = NAN;
				speed     = 0;
			} else {
				mCopter._0C = 1;
				direction   = WrapAngle((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y));
				speed *= 0.75f;
			}
		} else if (mPopper.mState == 3 || mPopper.mState == 4 || mPopper.mState == 8) {
			direction = WrapAngle((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y));
		} else if (mRope.mState == 5) {
			direction = NAN;
			if (mPad.mStickY > 0.1f) {
				mRope._0C = 1;
				if (mAnimId == 139.0f || mAnimId == 138.0f)
					speed = 0.32f;
				else if (mAnimId == 141.0f || mAnimId == 140.0f)
					speed = 0.12f;
				else
					speed = 0;
			} else if (mPad.mStickY < -0.1f) {
				mRope._0C = -1;
				speed     = 0.4f;
			} else {
				mRope._0C = 0;
			}
		} else if (mLadder.mState == 5) {
			direction = NAN;
			if (mPad.mStickY > 0.1f || mPad.mStickY < -0.1f) {
				mLadder._0C = mPad.mStickY > 0 ? 1 : -1;
				speed       = 0;
			} else {
				mLadder._0C = 0;
			}
		} else if (mPush.mState == 2) {
			f32 yaw      = mMoveRotate.y;
			f32 relative = WrapAngle(HALF_PI + ((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y) - yaw));
			if (relative >= 0 && relative < PI) {
				mPush._0C = 1;
			} else {
				mPush._0C = -1;
				yaw += PI;
			}
			direction = TWO_PI + WrapAngle(yaw);
			speed     = mPush.mTarget && !mPush.mPushPositionClamped ? 1.5f : 0;
		} else {
			direction = WrapAngle((_9BC[2] + gGame.mMotionControlCamera.mCamera.mAngle.y));
		}
		if (__fpclassifyf(_9AA8) != FP_NAN)
			direction = _9AA8 + (_9BC[2] - _9AB0);
		_9AAC = direction;
	} else {
		direction = NAN;
		if (mHang.mState == 5 && mHang._0C)
			mHang._0C = 0;
		else if (mCopter.mState == 3 && mCopter._0C)
			mCopter._0C = 0;
		else if (mRope.mState == 5) {
			if (mAnimId == 139) {
				speed     = 0.32f;
				mRope._0C = 2;
			} else if (mAnimId == 141) {
				speed     = 0.12f;
				mRope._0C = 2;
			} else if (mRope._0C != 0) {
				mRope._0C = 0;
			}
		} else if (mLadder.mState == 5 && mLadder._0C)
			mLadder._0C = 0;
		else if (mPush.mState == 2 && mPush._0C)
			mPush._0C = 0;
		_9AAC = _9AA8 = NAN;
	}
	if (__fpclassifyf(_9AA8) == FP_NAN)
		_9AB0 = _9BC[2];
	else if (Abs(_9BC[2] - _9AB0) >= 3 * DEG2RAD) {
		_9AAC = _9AA8 = NAN;
		_9AB0         = _9BC[2];
	}
	if (__fpclassifyf(_12DF8) != FP_NAN)
		direction = _12DF8;
	f32 scale = UpdatePlayerActions(this, frames);
	if (scale == 0.0f) {
		speed     = 0.0f;
		direction = NAN;
	} else {
		speed *= scale;
	}
	UpdatePlayerSuitChange(this);
	fn_800AC660(this, frames, speed, direction);
	UpdatePlayerActionAnimations(this, frames);
	XObj::UpdateObject(frames);

	static_cast<XObj&>(mCopterObj).UpdateObject(frames);
	static_cast<XObj&>(mPopperObj).UpdateObject(frames);
	static_cast<XObj&>(mRadarObj).UpdateObject(frames);
	static_cast<XObj&>(mOXObj).UpdateObject(frames);
	for (s32 i = 0; i < 9; ++i)
		mSuits.mObjects[i].vfA8(frames);

	if (IsCarryingToothbrush(&mLift) || IsCarryingSpoon(&mLift) || IsCarryingSquirter(&mLift)) {
		for (s32 i = 0; i < 2; ++i) {
			const Vec& rgb                = mColorStep[i].mValue;
			mLift.mTarget->mColors[1 - i] = ((u32)rgb.x << 24) | ((u32)rgb.y << 16) | ((u32)rgb.z << 8);
		}
	}
	for (XObjChrItemAnim* entry = gPlayerItemAnimationTable; entry->mItem >= 0; ++entry) {
		if (entry->mItem == mSuits.mCurrent) {
			HSD_AObj* anim = GetAObj(GetJointByPath(mJObj, entry->mPath, entry->mDepth));
			if (anim)
				entry->mFrame = HSD_AObjGetCurrentFrame(anim);
		}
	}
	if (__fpclassifyf(_12DF8) != FP_NAN) {
		CObjVec& pos = mMoveTranslate;
		if (pos.x == pos.mTarget.x && pos.y == pos.mTarget.y && pos.z == pos.mTarget.z) {
			pos.mSpeed.Set(0, 0, 0);
			_12DF8 = NAN;
		} else if (PSVECDistance(&pos, &pos.mTarget) <= 0.000001f) {
			SetPos(pos.mTarget, 0);
			pos.mSpeed.Set(0, 0, 0);
			_12DF8 = NAN;
		}
	}
	if (mJump.mState == 2 || mJump.mState == 3 || mHang.mState == 10 || mRope.mState == 9 || mLadder.mState == 10 || mCopter.mState == 5)
		_9A90 += _264.y - mMoveTranslate.y;
	else
		_9A90 = 0.0f;
	UpdatePlayerBattery(this, frames);
	UpdateHeldObject(&mLift, frames);
	UpdatePushedObject(&mPush);
	_158C0 = GetPlayerGroundType(this);
	UpdatePlayerFrameEffects(this);
	s32 path[]    = { 0, 1, 0, 1, 2, 0, 1 };
	f32 jointY    = GetEventJointWorldPosition(this, 7, path).y;
	mLook.mOffset = (Vec) { 0, (jointY - mMoveTranslate.y) + 3.0f * mMoveScale.y, 0 };
	UpdatePlayerLookMarker(&mLook, frames);
	XObj* plug = &mPlug;
	plug->UpdateObject(frames);
	UpdateParticlePool(&mDust.mPtcl, frames);
	UpdateTumbleFade(&mTumble.mFade, mTumble.mState == 2 || mTumble.mState == 16, frames);
	UpdateRadarBeamAnimation(&mRadar, frames);

	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticleSlots[i]) {
			UpdatePointParticleDisplay(gPointParticleSlots[i], frames);
			if (gPointParticleSlots[i]->IsFree())
				gPointParticleSlots[i] = NULL;
		}
	}
	UpdateSpiralTrail(&gPlayerSuitChangeEffect, frames);
	if (mPad.mReset) {
		turn.previousHeading = turn.heading;
		f32 heading          = mMoveRotate.y;
		while (heading < 0.0f)
			heading += TWO_PI;
		while (heading >= TWO_PI)
			heading -= TWO_PI;
		turn.heading        = heading;
		turn.previousChange = turn.change;
		turn.change         = heading - turn.previousHeading;
		if (turn.change > PI)
			turn.change -= TWO_PI;
		else if (turn.change < -PI)
			turn.change += TWO_PI;

		if (mLift.mState || mCopter.mState || mPopper.mState == 4) {
			turn.total = 0.0f;
		} else {
			turn.total += turn.change;
			if (turn.change * turn.previousChange <= 0.0f || Abs(turn.change) < 10.0f * DEG2RAD)
				turn.total = 0.0f;
		}
		s32 turns = (s32)(turn.total / TWO_PI);
		if ((turns < 0 ? -turns : turns) >= 5) {
			mTumble.mState = 10;
			turn.total     = 0.0f;
		}
	}
	f32& margin = mCameraTargetMargin;
	if (margin < mCameraTargetMarginSetting) {
		margin += 0.01f * frames;
		if (margin > mCameraTargetMarginSetting)
			margin = mCameraTargetMarginSetting;
	} else if (margin > mCameraTargetMarginSetting) {
		margin -= 0.01f * frames;
		if (margin < mCameraTargetMarginSetting)
			margin = mCameraTargetMarginSetting;
	}
	if (_158BC > 0) {
		_158BC -= frames;
		if (_158BC < 0)
			_158BC = 0;
	}
	if (_158B8 > 0 && _158BC == 0) {
		switch (_158B8) {
		case 1:
			mHeadTurn.mNeck.mMode   = 2;
			mHeadTurn.mNeck.mTarget = 0.0f;
			mHeadTurn.mNeck.mLookAt = NULL;
			break;
		case 2:
			mTumble.mState = 13;
			break;
		}
		_158B8 = 0;
	}
	UpdatePlayerHeadOffset(this, frames);
}

// anims from 3000 up are the suit's, from 2000 up the attachment's, from 1000 up XObj's second archive
static inline s32 SuitAnimGroup(XObjChr* chr, s32 id)
{
	bool ret = false;
	if (id >= 3000 && chr->mSuitAnim.mArchive != NULL) {
		ret = true;
	}
	return ret ? id / 3000 : 0;
}

static inline s32 AtcAnimGroup(XObjChr* chr, s32 id)
{
	bool ret = false;
	if (id >= 2000 && chr->mAttachmentAnim.mArchive != NULL) {
		ret = true;
	}
	return ret ? id / 2000 : 0;
}

static inline s32 AnimGroup(XObjChr* chr, s32 id)
{
	bool extra = id >= 1000;
	if (extra)
		extra = chr->mAnim.mArchive != NULL;
	return extra ? id / 1000 : 0;
}

// a particle an anim frame table asks for
struct PlayerFrameParticle {
	s32 mCode;      // _00, -1 for none
	Vec mTranslate; // _04
	Vec mScale;     // _10
	Vec mRotate;    // _1C
};
STATIC_ASSERT(sizeof(PlayerFrameParticle) == 0x28);

extern "C" {
PlayerFrameParticle ProcessPlayerFrameEvents(XObj*, const FrameEventTime*, const FrameEvent*);
}

// todo: cleanup
static inline u8* AnimationHead(HSD_JObj* joint)
{
	HSD_AObj* animation = joint->aobj;
	HSD_FObj* frame     = animation ? animation->fobj : NULL;
	return frame ? frame->ad_head : NULL;
}

extern "C" {
extern Vec lbl_80378258;
extern Vec lbl_8037824C;
}

static inline Vec FirstEdgePoint(const XObj* object, s32 index, f32 height)
{
	Vec point = lbl_80378258;
	point.x   = object->mMoveTranslate.x + object->mEdges[index].mFrom.x;
	point.y   = height;
	point.z   = object->mMoveTranslate.z + object->mEdges[index].mFrom.z;
	return point;
}

static inline Vec SecondEdgePoint(const XObj* object, s32 index, f32 height)
{
	Vec point = lbl_8037824C;
	point.x   = object->mMoveTranslate.x + object->mEdges[index].mTo.x;
	point.y   = height;
	point.z   = object->mMoveTranslate.z + object->mEdges[index].mTo.z;
	return point;
}

// out of line copies of the small inlines the player code uses, on XObjChr, its actions
// and the objects around it

extern "C" {
u32 CheckStageFlag(s32 stage, u32 mask);
}

// todo: cleanup
static inline bool TestStatus(const XObj* object, u64 mask)
{
	return (u8)((object->mStatus.mFlags & mask) != 0);
}

extern "C" s32 lbl_8065B260;

// out of battery, IsPlayerStopped
static inline bool IsStopped(const XObjChr* chr)
{
	return chr->mParams.mBattery <= 0.0f && InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn);
}

// out of battery and slumped over
static inline bool IsDown(const XObjChr* chr)
{
	bool ret = false;
	if (IsStopped(chr)) {
		if (chr->_158A4 != 0) {
			ret = true;
		}
	}
	return ret;
}

static inline bool IsEventHeld(const XObjChr* chr)
{
	bool ret = false;
	if ((gGame.mStatus.mFlags & 0x40000000) && IsDown(chr)) {
		ret = true;
	}
	return ret;
}

static inline bool IsTumbling(const XObjChr* chr)
{
	bool ret = false;
	if (chr->mTumble.mState == 4 || chr->mTumble.mState == 5 || chr->mTumble.mState == 6) {
		ret = true;
	}
	return ret;
}

// events wait while he lies there out of battery
static inline bool IsEventBlocked(const XObjChr* chr)
{
	bool ret = false;
	if (IsEventHeld(chr) && IsTumbling(chr)) {
		ret = true;
	}
	return ret;
}

static inline u8 IsMenuUp(XObjChr* chr)
{
	return (u8)(chr->mMenu.mAtc.mOpen > 0 || chr->mMenu.mDrs.mOpen > 0);
}

// he stops where he is, and lets go of what he pushes
static inline void StopMoving(XObjChr* chr)
{
	chr->_988.Set(0.0f, 0.0f, 0.0f);
	chr->_988.mTarget.Set(0.0f, 0.0f, 0.0f);
	chr->_988.mSpeed.Set(0.0f, 0.0f, 0.0f);
	chr->mMoveRotate.Set(chr->mMoveRotate.mTarget);
	if (chr->mPush.mState != 0) {
		ReleasePushAction(&chr->mPush);
		chr->mPush.mState = 0;
		chr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		chr->_A2C = 3;
	}
}

// the y angle a direction faces
static inline f32 GetYAngle(const Vec& dir)
{
	return atan2(dir.x, dir.z);
}

// OXMenu's marks start at random turns
static inline void SpinMarks(XObjChr& chr)
{
	for (int i = 0; i < 2; i++) {
		chr.mOXMenu.mMark[i].mSpin  = DEG2RAD * (rand() % 360);
		chr.mOXMenu.mMark[i].mTilt  = DEG2RAD * (rand() % 360);
		chr.mOXMenu.mMark[i].mScale = 0.5f;
	}
}

static inline void OpenMenus(XObjChr* chr)
{
	gGame.mStatus.mFlags |= 0x400ULL;
	PlayGameSound(&gGame.mSound, 0x86, 255);
	if (chr->mMenu.mSel == 0) {
		chr->mMenu.mAtc.Open(0);
		chr->mMenu.mDrs.Open(1);
	} else {
		chr->mMenu.mAtc.Open(1);
		chr->mMenu.mDrs.Open(0);
	}
}

extern "C" {
void vfD8__4XObjFPUl(CXObj* object, u32* flags);
void vf120__4XObjFl(CXObj* object, s32 mode);
extern PointPtcl gPointParticlePool[10];
extern PointPtcl* gPointParticleSlots[10];
}

// todo: cleanup
static inline u8 IsRopeState(XObjChr* chr, s32 state)
{
	return chr->mRope.mState == state;
}

extern "C" {
// stage.cpp returns a bool, this file tests the whole word
BOOL IsGlobalDataLoaded();
void ResetPopperCharge(XActPopperCharge* charge);
void ResetPopperShot(XActPopperShot* shot);
}

// the item glows brighter and darker while the cursor is on it
static inline void AddPulse(s32& red, s32& green, s32& blue, const OctaMenu* menu, s32 index)
{
	red += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index]._08) + 25.0f);
	green += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index]._08) + 25.0f);
	blue += (s32)(25.0f * (f32)sin(1.2f * menu->mItems[index]._08) + 25.0f);
}

// he has the attachment on, or asked for it
static inline BOOL IsAttachment(const XObjChr* chr, s32 id)
{
	return id == chr->GetAttachment();
}

// DrsMenu::vf2C, he wears the item's costume
static inline BOOL IsWorn(DrsMenu* menu, s32 index)
{
	return menu->mItems[index].mType == menu->mChr->mSuits.mCurrent;
}

// he carries the item
static inline BOOL HasItem(XObjChr* chr, s16 id)
{
	return FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems, id) < 0 ? FALSE : TRUE;
}

// stops every popper shot he has flying
static inline void StopShots(XObjChr* chr)
{
	XActPopperShot* shot = chr->mPopper.mShots.mShot;
	for (int i = 0; i < 10; i++, shot++) {
		ResetPopperShot(shot);
	}
}

// puts the joint at an angle and keeps it there
static inline void SetTurn(CJointRot* rot, f32 angle)
{
	rot->mMode   = 1;
	rot->mTarget = angle;
	rot->mAngle  = angle;
}

// turns the joint to an angle
static inline void TurnJoint(f32 angle, CJointRot* rot)
{
	rot->mMode   = 1;
	rot->mTarget = angle;
}

// how the head turns for each cursor, from no cursor through the eight items
extern "C" {
f32 gEquipmentHeadYaw[9]      = { 0, -20, 0, 20, 20, 20, 0, -20, -20 };
f32 gEquipmentHeadPitch[9]    = { -20, -20, -30, -20, -10, 0, 10, 0, -10 };
f32 gEquipmentRowY[8]         = { 0, 0, 0, 0, 0, 0, 0, 0 };
f32 gEquipmentRowX[8]         = { -3, -2, -1, 0, 1, 2, 3, 4 };
f32 gEquipmentRingY[8]        = { -1, -1, -1, 0, 1, 1, 1, 0 };
f32 gEquipmentRingX[8]        = { -1, 0, 1, 1, 1, 0, -1, -1 };
f32 gEquipmentOutlineX[4]     = { -1, 1, 1, -1 };
f32 gEquipmentOutlineY[4]     = { -1, -1, 1, 1 };
f32 gEquipmentHintOutlineX[4] = { -1, 1, 1, -1 };
f32 gEquipmentHintOutlineY[4] = { -1, -1, 1, 1 };
}

// one of his tools is out
static inline BOOL IsToolOut(XObjChr* chr, s16 item)
{
	switch (item) {
	case 0x10:
		return IsCarryingToothbrush(&chr->mLift);
	case 0x16:
		return IsCarryingMug(&chr->mLift);
	case 0x15:
		return IsCarryingSpoon(&chr->mLift);
	default:
		return IsCarryingSquirter(&chr->mLift);
	}
}

// he can use the tool, he has it or it is out
static inline bool CanUseTool(XObjChr* chr, s16 item)
{
	u8 has = 1;
	if (FindOwnedInventoryItemIndex(chr->mParams.mInventory.mItems, item) < 0 && !IsToolOut(chr, item))
		has = 0;
	return has;
}

static inline void SetAttachmentOn(AttachmentValues* values, s32 id, bool on)
{
	s32 index = FindAttachment(values, id);
	if (index >= 0)
		values->mEntries[index].mValue = on;
}

// the lift's timer ran out
static inline bool IsLiftDone(const XObjChr* chr)
{
	bool ret = false;
	if (chr->mLift.mState == 4 && IsLiftTargetPlug((XActLift*)&chr->mLift)) {
		ret = true;
	}
	return ret;
}

// he takes a tool out, or puts it away when he came back to it
static inline void TakeOutTool(XObjChr* chr, s16 item)
{
	if (chr->_95DC == 0) {
		chr->mLift.mState = 20;
		chr->mLift._48    = item;
	} else {
		chr->mLift.mState = 24;
		chr->mLift._48    = item;
	}
}

// the anim archive each attachment needs
static const char* sAttachmentAnims[9] = {
	NULL, "cb_robo_6.dat", "cb_robo_7.dat", "cb_robo_8.dat", "cb_robo_9.dat", "cb_robo_10.dat", "cb_robo_11.dat", "cb_robo_12.dat", NULL,
};

extern "C" {
void StartPointParticles(PointPtcl* ptcl, s32 type, s32 amount);
}

// closes both rings, 0 cancels, above 0 confirms, below 0 backs out
/**
 * @note Address: 0x80097284
 * @note Size: 0xF4
 */
extern "C" void ClosePlayerEquipmentRings(XObjChrMenu* menu, s32 result)
{
	if (result == 0) {
		PlayGameSound(&gGame.mSound, 0x86, 255);
		if (menu->mAtc._280 != 3)
			menu->mSel = menu->mPrevSel;
	} else if (result > 0) {
		PlayGameSound(&gGame.mSound, 0x88, 255);
	} else if (result < 0) {
		PlayGameSound(&gGame.mSound, 0x86, 255);
		if (menu->mAtc._280 != 3)
			menu->mSel = menu->mPrevSel;
	}
	menu->mAtc.Select(result);
	menu->mDrs.Select(result);
}

extern "C" {
void DrawEquipmentRing(OctaMenu* ring);
}

#if defined(VERSION_JP)
#define HINT_ICON_WIDTH (1.0f / 14.0f)
#else
#define HINT_ICON_WIDTH (1.0f / 17.0f)
#endif

/**
 * @note Address: 0x80097378
 * @note Size: 0x7D8
 */
extern "C" void DrawPlayerEquipmentRings(XObjChrMenu* menu)
{
	if (menu->mSel == 0) {
		DrawEquipmentRing(&menu->mDrs);
		DrawEquipmentRing(&menu->mAtc);
	} else {
		DrawEquipmentRing(&menu->mAtc);
		DrawEquipmentRing(&menu->mDrs);
	}
	CGs* gs   = &gGame.mGs;
	CTpl* tpl = &gGame.mTpl;
	if (menu->mAtc._280 == 3) {
		if (menu->mSel == 0) {
			menu->mTargetY = menu->mAtc.mCursorY;
			menu->mTargetX = menu->mAtc.mCursorX;
		} else if (menu->mSel == 1) {
			menu->mTargetY = menu->mDrs.mCursorY;
			menu->mTargetX = menu->mDrs.mCursorX;
		} else {
			menu->mTargetY = 0.0f;
			menu->mTargetX = 0.0f;
		}
		if (menu->mFade < 1.0f) {
			menu->mCursorY = menu->mTargetY;
			menu->mCursorX = menu->mTargetX;
		} else {
			menu->mCursorY += 0.7f * (menu->mTargetY - menu->mCursorY);
			menu->mCursorX += 0.7f * (menu->mTargetX - menu->mCursorX);
		}
		f32 radius = 40.0f * menu->mFade;
		f32 y      = menu->mCursorY;
		f32 x      = menu->mCursorX;
		f32 left   = 8.0f + (x - radius);
		f32 top    = 8.0f + (y - radius);
		f32 right  = (x + radius) - 8.0f;
		f32 bottom = (y + radius) - 8.0f;
		Vec quad[] = { { left, top, 0.0f }, { right, top, 0.0f }, { right, bottom, 0.0f }, { left, bottom, 0.0f }, { left, top, 0.0f } };
		SetGsDepthMode(gs->mCurRenderMode, 1, 5, 0);
		u32 color = (u32)(255.0f * menu->mFade);
		DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x, quad[2].y,
		                 quad[2].z, color, quad[3].x, quad[3].y, quad[3].z, color);
		SetGsDepthMode(gs->mCurRenderMode, 1, 1, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj tex = tpl->mTex.Init(&((TPLPalette*)tpl->mBuffer)->descriptorArray[110 - StatusTextureOffset], 0);
		color       = 0xFFFFFF00 | (u32)(255.0f * menu->mFade);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, 2.0f * radius, 2.0f * radius, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gs->mCurRenderMode, 1, 3, 0);
		return;
	}
	struct Hint {
		s32 icon;
		const char* label;
	};
	Hint hints[]    = { { 8, NULL }, { 5, NULL }, { 5, NULL } };
	hints[0].label  = GetLocalizedInterfaceText(ATC_LABEL(9));
	hints[1].label  = GetLocalizedInterfaceText(ATC_LABEL(10));
	hints[2].label  = GetLocalizedInterfaceText(ATC_LABEL(11));
	s32 order[2][2] = { { 0, 1 }, { 0, 2 } };

	f32 edge            = 608.0f + 100.0f * (1.0f - (f32)sin(1.5707964f * menu->mFade));
	const Hint* rowHint = hints;
	for (s32 row = 0; row < 2; ++row, ++rowHint) {
		Hint& hint    = hints[order[menu->mSel][row]];
		f32 x         = edge - MeasureGameTextWidth(&gGame.mGs, rowHint->label, TRUE, 20.0f);
		f32 y         = 46.0f + row * 24.0f;
		const f32* dx = gEquipmentHintOutlineX;
		const f32* dy = gEquipmentHintOutlineY;
		for (s32 j = 0; j < 4; ++j, ++dx, ++dy) {
			u32 alpha = (u32)(255.0f * menu->mFade);
			DrawGsProportionalTextFormat(&gGame.mGs, alpha, hint.label, x + *dx, y + *dy, 0.0f, 20.0f, 20.0f);
		}
		u32 color = 0xFFFFFF00 | (u32)(255.0f * menu->mFade);
		DrawGsProportionalTextFormat(&gGame.mGs, color, hint.label, x, y, 0.0f, 20.0f, 20.0f);
		CTexObj tex = tpl->mTex.Init(&((TPLPalette*)tpl->mBuffer)->descriptorArray[119 - StatusTextureOffset], 0);
		PSMTXScale(tex.mMtx[1], HINT_ICON_WIDTH, 1.0f, 1.0f);
		tex.mMtxDirty = TRUE;
		PSMTXTrans(tex.mMtx[0], HINT_ICON_WIDTH * hint.icon, 0.0f, 0.0f);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		color = 0xFFFFFF00 | (u32)(255.0f * menu->mFade);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 12.0f, y + 10.0f, 0.0f, 24.0f, 24.0f, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

extern "C" s32 HandleEquipmentRingInput(OctaMenu* menu, BOOL active);
extern "C" void UpdateEquipmentRingAnimation(OctaMenu* menu, u32 frames);

static inline u8 RingReady(const OctaMenu& ring)
{
	return ring.mOpen > 1 && (ring._0C == 0.0f || ring._0C == 1.0f);
}

static inline BOOL CanSelectEquipmentRing(const XObjChrMenu* menu)
{
	return RingReady(menu->mAtc) || RingReady(menu->mDrs);
}

static inline void FocusEquipmentRing(XObjChrMenu* menu)
{
	if (menu->mSel == 0) {
		menu->mAtc.mFocus       = 0;
		menu->mDrs.mFocus       = 1;
		menu->mDrs.mCursorIndex = -1;
	} else {
		menu->mAtc.mFocus       = 1;
		menu->mAtc.mCursorIndex = -1;
		menu->mDrs.mFocus       = 0;
	}
	PlayGameSound(&gGame.mSound, 0x86, 255);
}

static inline void FadeEquipmentRingIn(XObjChrMenu* menu, u32 frames)
{
	f32 fade    = menu->mFade + (1.0f / (0.18f * IDLE_SECOND)) * frames;
	menu->mFade = fade;
	if (fade >= 1.0f)
		menu->mFade = 1.0f;
}

static inline void FadeEquipmentRingOut(XObjChrMenu* menu, u32 frames)
{
	f32 fade    = menu->mFade - (1.0f / (0.25f * IDLE_SECOND)) * frames;
	menu->mFade = fade;
	if (fade <= 0.0f)
		menu->mFade = 0.0f;
}

/**
 * @note Address: 0x80097B50
 * @note Size: 0x6B4
 */
extern "C" void UpdatePlayerEquipmentRings(XObjChrMenu* menu, u32 frames)
{
	BOOL active = gGame.mEvt.mData.mScript == NULL;
	if (CanSelectEquipmentRing(menu) && active) {
		CPad pad(0);
		if (menu->mAtc._280 == 3) {
			u8 dir = pad.mPad->mDirButton & 0xF;
			if (dir == 8 && (menu->mSel == 1 || menu->mSel < 0)) {
				menu->mSel = 0;
				FocusEquipmentRing(menu);
			} else if (dir == 4 && (menu->mSel == 0 || menu->mSel < 0)) {
				menu->mSel = 1;
				FocusEquipmentRing(menu);
			}
		} else if (pad.mPad->mTrigger & 0x460) {
			menu->mSel = (menu->mSel + 1) % 2;
			FocusEquipmentRing(menu);
		}
	}
	if (RingReady(menu->mAtc) && RingReady(menu->mDrs)) {
		menu->mAtc.mHeadTurn = NAN;
		menu->mDrs.mHeadTurn = NAN;
	} else {
		menu->mAtc.mHeadTurn = -1.52f;
		menu->mDrs.mHeadTurn = -1.52f;
	}
	s32 result = menu->mSel == 0 ? HandleEquipmentRingInput(&menu->mAtc, active) : HandleEquipmentRingInput(&menu->mDrs, active);
	if (menu->mAtc.mOpen > 0) {
		if (!(gGame.mStatus.mFlags & 0x100)) {
			gGame.GetPlayer()->SetRotateY(PI + gGame.mMotionControlCamera.mCamera.mAngle.y, DEG2RAD * 8.0f);
		}
		if (gGame.mEvt.mData.mScript == NULL && (gGame.mPad.mPad[0].mTrigger & 0x200)) {
			result = -1;
		}
	}
	if (result != 0)
		ClosePlayerEquipmentRings(menu, result);
	UpdateEquipmentRingAnimation(&menu->mAtc, frames);
	UpdateEquipmentRingAnimation(&menu->mDrs, frames);
	if (menu->IsOpen()) {
		if (menu->IsOpening()) {
			if (menu->mFade < 1.0f) {
				if (menu->mAtc._280 == 3) {
					if (menu->mSel >= 0)
						FadeEquipmentRingIn(menu, frames);
					else
						FadeEquipmentRingOut(menu, frames);
				} else {
					FadeEquipmentRingIn(menu, frames);
				}
			}
		} else if (menu->IsClosing() && menu->mFade > 0.0f) {
			FadeEquipmentRingOut(menu, frames);
		}
	} else if (gGame.mStatus.mFlags & 0x400) {
		gGame.mStatus.mFlags &= ~0x400ULL;
		gGame.GetPlayer()->mPad.Update(0);
	}
}

// DrsMenu::GetItemColor, the item's color, pulsing while selected
/**
 * @note Address: 0x80098204
 * @note Size: 0x1B4
 */
u32 DrsMenu::GetItemColor(s32 index)
{
	s32 green, blue, red;
	if (mItems[index].mAvailable == 0) {
		blue  = 32;
		green = 32;
		red   = 32;
	} else if (_280 == 3) {
		if (IsWorn(this, index)) {
			red   = 50;
			green = 200;
			blue  = 200;
		} else {
			red   = 50;
			green = 150;
			blue  = 50;
		}
	} else if (index == mCursorIndex && IsWorn(this, index)) {
		red   = 200;
		green = 100;
		blue  = 150;
	} else if (index == mCursorIndex) {
		red   = 200;
		green = 100;
		blue  = 100;
	} else if (IsWorn(this, index)) {
		red   = 50;
		green = 200;
		blue  = 200;
	} else {
		red   = 50;
		green = 150;
		blue  = 50;
	}
	if (index == mCursorIndex)
		AddPulse(red, green, blue, this, index);
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

// DrsMenu::GetItemLabel, the item's label
/**
 * @note Address: 0x800983B8
 * @note Size: 0x64
 */
const char* DrsMenu::GetItemLabel(s32 index)
{
	if (mItems[index].mAvailable > 0) {
		if (IsWorn(this, index))
			return GetLocalizedInterfaceText(DRS_LABEL(13));
		return (const char*)GetCostumeName(mItems[index].mType);
	}
	return GetLocalizedInterfaceText(DRS_LABEL(14));
}

/**
 * @note Address: 0x8009841C
 * @note Size: 0xB0
 */
s32 DrsMenu::GetItemIconTextureIndex(s32 index)
{
	s32 icons[9] = { DRS_LABEL(126), DRS_LABEL(98),  DRS_LABEL(100), DRS_LABEL(99), DRS_LABEL(101),
		             DRS_LABEL(126), DRS_LABEL(102), DRS_LABEL(103), DRS_LABEL(104) };
	if (index == -1)
		return DRS_LABEL(107);
	if (index == -2)
		return DRS_LABEL(109);
	if (mItems[index].mAvailable > 0)
		return icons[mItems[index].mType];
	return DRS_LABEL(105);
}

// DrsMenu::vf2C
/**
 * @note Address: 0x800984CC
 * @note Size: 0x28
 */
extern "C" BOOL vf2C__7DrsMenuFl(DrsMenu* menu, s32 index)
{
	return IsWorn(menu, index);
}

// puts the selected costume on, or takes the worn one off
/**
 * @note Address: 0x800984F4
 * @note Size: 0xC0
 */
void DrsMenu::ConfirmSelection()
{
	s8 selectable[9] = { 0, 1, 1, 1, 1, 0, 1, 1, 1 };
	s32 type         = mItems[mCursorIndex].mType;
	if (selectable[type]) {
		if (type == mChr->mSuits.mCurrent) {
			XObjChr* chr = mChr;
			if (chr->mSuits.mChangeStep == 0) {
				chr->mSuits.mRequested  = 0;
				chr->mSuits.mChangeStep = 1;
			}
		} else {
			XObjChr* chr = mChr;
			if (chr->mSuits.mChangeStep == 0) {
				chr->mSuits.mRequested  = type;
				chr->mSuits.mChangeStep = 1;
			}
		}
	} else {
		XObjChr* chr = mChr;
		if (chr->mSuits.mChangeStep == 0) {
			chr->mSuits.mRequested  = 0;
			chr->mSuits.mChangeStep = 1;
		}
	}
}

// OctaMenu::Open, ahead of the two menus that inline it
/**
 * @note Address: 0x8009BF14
 * @note Size: 0x2A8
 */
extern "C" void Open__8OctaMenuFl(OctaMenu* menu, s32 focus)
{
	if (menu->mOpen == 0) {
		menu->mOpeningHeading = menu->mChr->mMoveRotate.y;
		SetTurn(&menu->mChr->mHeadTurn.mNeck, 0.0f);
		SetTurn(&menu->mChr->mHeadTurn.mHead, -1.52f);
	}
	menu->mOpen  = 1;
	menu->mFocus = focus;
	if (menu->mFocus != 0) {
		menu->_0C = 1.0f;
	}
	Vec offset = { 0.0f, 8.8f, 0.0f };
	menu->mChr->ProjectOffsetToScreen(&offset, &menu->mOpenScreenX, &menu->mOpenScreenY, NULL, 0);
	for (int i = 0; i < 8; i++) {
		menu->mItems[i]._08 = DEG2RAD * (rand() % 360);
		menu->mItems[i]._0C = DEG2RAD * (rand() % 360);
		menu->mItems[i]._10 = menu->mFocus != 0 ? 0.3f : 0.5f;
	}
	menu->mCursorIndex = -1;
	menu->mLabelLength = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(menu->mLabelChars); i++) {
		menu->mLabelChars[i].mAngle0 = DEG2RAD * (rand() % 360);
		menu->mLabelChars[i].mAngle1 = DEG2RAD * (rand() % 360);
	}
	menu->mChr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

// DrsMenu::Open, a costume is there when he has it
/**
 * @note Address: 0x800985B4
 * @note Size: 0x488
 */
extern "C" void Open__7DrsMenuFl(OctaMenu* menu, s32 focus)
{
	// NONMATCHING: the costume ids are read from the unit's rodata
	if (menu->_280 == 3) {
		s16 ids[8] = { 0x18, 0x20, 0x22, 0x19, 0x1A, 0x1E, 0x1F, 0x21 };
		for (s32 i = 0; i < 8; ++i) {
			if (menu->_280 == 2)
				menu->mItems[i].mAvailable = 1;
			else
				menu->mItems[i].mAvailable = HasItem(menu->mChr, ids[i]);
		}
	} else {
		s16 ids[8] = { 0x19, 0x21, 0x20, 0x18, 0x22, 0x1F, 0x1E, 0x1A };
		for (s32 i = 0; i < 8; ++i) {
			if (menu->_280 == 2)
				menu->mItems[i].mAvailable = 1;
			else
				menu->mItems[i].mAvailable = HasItem(menu->mChr, ids[i]);
		}
	}
	Open__8OctaMenuFl(menu, focus);
	if (menu->_280 == 3) {
		if (menu->mPendingFirstVisibleItem >= 0) {
			menu->mFirstVisibleItem        = menu->mPendingFirstVisibleItem;
			menu->mPendingFirstVisibleItem = -1;
		} else {
			// one past the items
			for (s32 i = 0; i < 9; ++i) {
				if (menu->mItems[i].mType == menu->mChr->mSuits.mCurrent) {
					menu->mFirstVisibleItem = i - 3;
					if (menu->mFirstVisibleItem < 0)
						menu->mFirstVisibleItem += 7;
				}
			}
		}
	}
}

// AtcMenu::GetItemColor, the item's color, pulsing while selected
/**
 * @note Address: 0x80098A3C
 * @note Size: 0x1E4
 */
u32 AtcMenu::GetItemColor(s32 index)
{
	s32 green, blue, red;
	if (mItems[index].mAvailable == 0) {
		blue  = 32;
		green = 32;
		red   = 32;
	} else if (_280 == 3) {
		if (IsAttachment(mChr, mItems[index].mType)) {
			red   = 50;
			green = 200;
			blue  = 200;
		} else {
			red   = 50;
			green = 50;
			blue  = 200;
		}
	} else if (index == mCursorIndex && IsAttachment(mChr, mItems[index].mType)) {
		red   = 200;
		green = 100;
		blue  = 150;
	} else if (index == mCursorIndex) {
		red   = 200;
		green = 100;
		blue  = 100;
	} else if (IsAttachment(mChr, mItems[index].mType)) {
		red   = 50;
		green = 200;
		blue  = 200;
	} else {
		red   = 50;
		green = 50;
		blue  = 200;
	}
	if (index == mCursorIndex)
		AddPulse(red, green, blue, this, index);
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

// AtcMenu::GetItemLabel, the item's label. the last one stays up while a switch is pending or an
// action that holds the attachment is running
/**
 * @note Address: 0x80098C20
 * @note Size: 0xCC
 */
const char* AtcMenu::GetItemLabel(s32 index)
{
	static const char* sLabel = "";
	if (mItems[index].mAvailable > 0) {
		if (mChr->mAttachmentRequest < 0 && mChr->mLift.mState != 20 && mChr->mLift.mState != 21 && mChr->mLift.mState != 22) {
			if (mItems[index].mType == mChr->GetAttachment())
				sLabel = GetLocalizedInterfaceText(ATC_LABEL(12));
			else
				sLabel = (const char*)GetAttachmentName(mItems[index].mType);
		}
	} else {
		sLabel = GetLocalizedInterfaceText(ATC_LABEL(14));
	}
	return sLabel;
}

/**
 * @note Address: 0x80098CEC
 * @note Size: 0xB0
 */
s32 AtcMenu::GetItemIconTextureIndex(s32 index)
{
	s32 icons[9] = { ATC_LABEL(126), ATC_LABEL(91), ATC_LABEL(92), ATC_LABEL(93), ATC_LABEL(94),
		             ATC_LABEL(96),  ATC_LABEL(95), ATC_LABEL(97), ATC_LABEL(126) };
	if (index == -1)
		return ATC_LABEL(106);
	if (index == -2)
		return ATC_LABEL(108);
	if (mItems[index].mAvailable > 0)
		return icons[mItems[index].mType];
	return ATC_LABEL(105);
}

// AtcMenu::vf2C
/**
 * @note Address: 0x80098D9C
 * @note Size: 0x38
 */
extern "C" u8 vf2C__7AtcMenuFl(AtcMenu* menu, s32 index)
{
	return menu->mChr->GetAttachment() == menu->mItems[index].mType;
}

// AtcMenu::ConfirmSelection, switches to the selected attachment, or puts the current one away
/**
 * @note Address: 0x80098DD4
 * @note Size: 0x74
 */
extern "C" void ConfirmSelection__7AtcMenuFv(AtcMenu* menu)
{
	XObjChr* chr = menu->mChr;
	s32 current  = chr->GetAttachment();
	s32 type     = menu->mItems[menu->mCursorIndex].mType;
	if (type == current) {
		if (chr->mAttachment != 0) {
			chr->mAttachmentRequest = 0;
			chr->_95D8              = 0;
		}
	} else {
		if (type != chr->mAttachment) {
			chr->mAttachmentRequest = type;
			chr->_95D8              = 0;
		}
	}
}

// AtcMenu::Open, OctaMenu's plus the item counts and the start item
/**
 * @note Address: 0x80098E48
 * @note Size: 0x3B4
 */
extern "C" void Open__7AtcMenuFl(OctaMenu* menu, s32 focus)
{
	for (s32 i = 0; i < 8; ++i) {
		if (menu->_280 == 2)
			menu->mItems[i].mAvailable = 1;
		else
			menu->mItems[i].mAvailable = GetPlayerAttachmentCount(menu->mChr, menu->mItems[i].mType);
	}
	Open__8OctaMenuFl(menu, focus);
	if (menu->_280 == 3) {
		if (menu->mPendingFirstVisibleItem >= 0) {
			menu->mFirstVisibleItem        = menu->mPendingFirstVisibleItem;
			menu->mPendingFirstVisibleItem = -1;
		} else {
			for (s32 i = 0; i < 8; ++i) {
				if (menu->mItems[i].mType == menu->mChr->GetAttachment()) {
					menu->mFirstVisibleItem = i - 3;
					if (menu->mFirstVisibleItem < 0)
						menu->mFirstVisibleItem += 7;
				}
			}
		}
	}
}

// how many of an attachment chibi robo has
/**
 * @note Address: 0x800991FC
 * @note Size: 0x28
 */
extern "C" s32 GetPlayerAttachmentCount(XObjChr* chr, s32 type)
{
	return GetAttachmentCountById(&chr->mAttachments, type);
}

// OctaMenu::GetItemLabel, the base menu has no labels
/**
 * @note Address: 0x8009AFD4
 * @note Size: 0x8
 */
const char* OctaMenu::GetItemLabel(s32 index)
{
	return lbl_8065D4AC;
}

// OctaMenu::vf2C
/**
 * @note Address: 0x8009AFDC
 * @note Size: 0x8
 */
extern "C" u8 vf2C__8OctaMenuFl(OctaMenu* menu, s32 index)
{
	return 0;
}

/**
 * @note Address: 0x8009AFE4
 * @note Size: 0x8
 */
s32 OctaMenu::GetItemIconTextureIndex(s32 index)
{
	return 0;
}

// OctaMenu::GetItemColor, the item's color, pulsing while selected
/**
 * @note Address: 0x8009AFEC
 * @note Size: 0x130
 */
// NONMATCHING: regalloc
u32 OctaMenu::GetItemColor(s32 index)
{
	s32 green, blue, red;
	if (mItems[index].mAvailable == 0) {
		blue  = 32;
		green = 32;
		red   = 32;
	} else if (_280 == 3) {
		red   = 50;
		green = 50;
		blue  = 200;
	} else if (index == mCursorIndex) {
		red   = 200;
		green = 100;
		blue  = 100;
	} else {
		red   = 50;
		green = 50;
		blue  = 200;
	}
	if (index == mCursorIndex)
		AddPulse(red, green, blue, this, index);
	return ((u32)red << 24) | ((u8)green << 16) | ((u8)blue << 8);
}

static inline void RaiseRingValue(f32& value, f32 target, f32 rate, u32 frames)
{
	const f32 next = value + rate * frames;
	value          = next;
	if (next >= target)
		value = target;
}

static inline void LowerRingValue(f32& value, f32 target, f32 rate, u32 frames)
{
	const f32 next = value - rate * frames;
	value          = next;
	if (next <= target)
		value = target;
}

static inline void ApproachItemScale(f32& value, f32 target, f32 step)
{
	if (value < target) {
		const f32 next = value + step;
		value          = next;
		if (next > target)
			value = target;
	} else if (value > target) {
		const f32 next = value - step;
		value          = next;
		if (next < target)
			value = target;
	}
}

/**
 * @note Address: 0x8009B11C
 * @note Size: 0x8C4
 */
extern "C" void UpdateEquipmentRingAnimation(OctaMenu* ring, u32 frames)
{
	if (ring->mFocus != 0) {
		if (ring->_280 == 3) {
			if (ring->mFade < 1.0f)
				RaiseRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * IDLE_SECOND), frames);
			else if (ring->mFade > 1.0f)
				LowerRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * IDLE_SECOND), frames);
		} else {
			if (ring->mFade < 0.7f)
				RaiseRingValue(ring->mFade, 0.7f, 1.0f / (0.25f * IDLE_SECOND), frames);
			else if (ring->mFade > 0.7f)
				LowerRingValue(ring->mFade, 0.7f, 1.0f / (0.25f * IDLE_SECOND), frames);
		}
		if (ring->_0C < 1.0f) {
			if (ring->_280 == 3)
				ring->_0C = 1.0f;
			else
				RaiseRingValue(ring->_0C, 1.0f, 1.0f / (0.35f * IDLE_SECOND), frames);
		}
	} else {
		if (ring->mOpen > 0) {
			if (ring->mFade < 1.0f)
				RaiseRingValue(ring->mFade, 1.0f, 1.0f / (0.18f * IDLE_SECOND), frames);
		} else if (ring->mOpen < 0) {
			if (ring->mFade > 0.0f) {
				if (ring->_280 == 3)
					LowerRingValue(ring->mFade, 0.0f, 1.0f / (0.18f * IDLE_SECOND), frames);
				else
					LowerRingValue(ring->mFade, 0.0f, 1.0f / (0.25f * IDLE_SECOND), frames);
			}
			if (ring->mFade == 0.0f)
				ring->vf18();
		}
		if (ring->_0C > 0.0f) {
			if (ring->_280 == 3)
				ring->_0C = 0.0f;
			else
				LowerRingValue(ring->_0C, 0.0f, 1.0f / (0.35f * IDLE_SECOND), frames);
		}
	}
	if (ring->mFade > 0.0f) {
		for (s32 i = 0; i < 8; ++i) {
			OctaMenuItem& item = ring->mItems[i];
			if (ring->mFocus != 0) {
				if (ring->_280 == 3)
					ApproachItemScale(item._10, 0.4f, 0.02f * frames);
				else
					ApproachItemScale(item._10, 0.3f, 0.01f * frames);
			} else {
				item._08 += (DEG2RAD * 8.0f) * frames;
				item._0C += (DEG2RAD * 3.0f) * frames;
				if (ring->_280 == 3)
					item._10 = i == ring->mCursorIndex ? 0.5f : 0.4f;
				else if (i == ring->mCursorIndex)
					ApproachItemScale(item._10, 0.7f, 0.03f * frames);
				else
					ApproachItemScale(item._10, 0.5f, 0.02f * frames);
			}
		}
		ring->mPhase += (1.0f / (0.3f * IDLE_SECOND)) * frames;
		if (ring->mPhase >= 2.0f)
			ring->mPhase = fmod(ring->mPhase, 2.0);
	}
	if (ring->mFade > 0.0f) {
		for (s32 i = 0; i < (s32)ARRAY_SIZE(ring->mLabelChars); ++i) {
			ring->mLabelChars[i].mAngle0 += (DEG2RAD * 5.0f) * frames;
			ring->mLabelChars[i].mAngle1 += (DEG2RAD * 3.0f) * frames;
		}
	}
	if (ring->mOpen > 0) {
		ring->mOpen = 2;
		if (ring->mFade >= 0.5f) {
			for (s32 i = 0; i < 8; ++i) {
				f32& glow = ring->mItems[i].mHighlight;
				if (i == ring->mCursorIndex)
					RaiseRingValue(glow, 1.0f, 1.0f / (0.2f * IDLE_SECOND), frames);
				else if (ring->_280 == 3)
					glow = 0.0f;
				else
					LowerRingValue(glow, 0.0f, 1.0f / (0.25f * IDLE_SECOND), frames);
			}
		}
	} else if (ring->mOpen < 0) {
		for (s32 i = 0; i < 8; ++i)
			LowerRingValue(ring->mItems[i].mHighlight, 0.0f, 1.0f / (0.25f * IDLE_SECOND), frames);
	}
}

// OctaMenu::vf14, the stick or the dpad moves the cursor, a picks, b closes. 1 when it picked,
// -1 when it closed
/**
 * @note Address: 0x8009B9E0
 * @note Size: 0x438
 */
extern "C" s32 HandleEquipmentRingInput(OctaMenu* menu, BOOL active)
{
	if (menu->mOpen > 0 && menu->mFocus == 0) {
		CPad pad(0);
		s32 cursor;
		if (menu->_280 == 3) {
			u32 dir = active ? pad.mPad->mDirRepeat & 0xF : 0;
			if (dir & 1) {
				if (--menu->mFirstVisibleItem < 0)
					menu->mFirstVisibleItem = 6;
				PlayGameSound(&gGame.mSound, 0x87, 255);
			} else if (dir & 2) {
				if (++menu->mFirstVisibleItem >= 7)
					menu->mFirstVisibleItem = 0;
				PlayGameSound(&gGame.mSound, 0x87, 255);
			}
			if (menu->mFocus != 0)
				cursor = menu->mCursorIndex = -1;
			else
				cursor = menu->mCursorIndex = (menu->mFirstVisibleItem + 3) % 7;
		} else {
			u32 stick = active ? pad.mPad->mButton & 0xF0000 : 0;
			if (stick == 0x50000)
				cursor = 0;
			else if (stick == 0x10000)
				cursor = 1;
			else if (stick == 0x90000)
				cursor = 2;
			else if (stick == 0x80000)
				cursor = 3;
			else if (stick == 0xA0000)
				cursor = 4;
			else if (stick == 0x20000)
				cursor = 5;
			else if (stick == 0x60000)
				cursor = 6;
			else if (stick == 0x40000)
				cursor = 7;
			else
				cursor = menu->mCursorIndex;
		}
		if (menu->_280 != 3) {
			TurnJoint(DEG2RAD * gEquipmentHeadYaw[cursor + 1], &menu->mChr->mHeadTurn.mNeck);
			if (fpclassify(menu->mHeadTurn) == FP_NAN)
				TurnJoint(DEG2RAD * gEquipmentHeadPitch[cursor + 1] + -1.52f, &menu->mChr->mHeadTurn.mHead);
			else
				TurnJoint(menu->mHeadTurn, &menu->mChr->mHeadTurn.mHead);
		}
		if (menu->mCursorIndex != cursor)
			PlayGameSound(&gGame.mSound, 0x87, 255);
		menu->mCursorIndex = cursor;
		if (active && menu->mFade >= 1.0f) {
			if (pad.mPad->mTrigger & 0x100) {
				if (menu->mCursorIndex < 0) {
					PlayGameSound(&gGame.mSound, 0x85, 255);
				} else if (menu->mItems[menu->mCursorIndex].mAvailable == 0) {
					PlayGameSound(&gGame.mSound, 0x85, 255);
				} else {
					menu->ConfirmSelection();
					StopShots(menu->mChr);
					return 1;
				}
			} else if (pad.mPad->mTrigger & 0x200) {
				return -1;
			}
		}
	}
	return 0;
}

// OctaMenu::ConfirmSelection, nothing to decide on the base menu
/**
 * @note Address: 0x8009BE18
 * @note Size: 0x4
 */
extern "C" void ConfirmSelection__8OctaMenuFv(OctaMenu* menu)
{
}

// OctaMenu::Select, closes the ring and turns chibi robo back
/**
 * @note Address: 0x8009BE1C
 * @note Size: 0xF8
 */
extern "C" void Select__8OctaMenuFl(OctaMenu* menu, s32 result)
{
	if (result > 0 && menu->mFocus == 0)
		menu->mOpen = -1;
	else
		menu->mOpen = -2;
	menu->mPendingFirstVisibleItem = -1;
	menu->mFocus                   = 0;
	if (menu->mChr->mTumble.mState == 0) {
		menu->mChr->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		menu->mChr->SetRotateY(menu->mOpeningHeading, DEG2RAD * 12.0f);
		CJointRot* neck = &menu->mChr->mHeadTurn.mNeck;
		neck->mMode     = 2;
		neck->mTarget   = 0.0f;
		neck->mLookAt   = NULL;
		CJointRot* head = &menu->mChr->mHeadTurn.mHead;
		head->mMode     = 2;
		head->mTarget   = -1.52f;
		head->mLookAt   = NULL;
	}
}

// the count for an attachment id, -1 when it isn't in the list
/**
 * @note Address: 0x8009C1BC
 * @note Size: 0x10C
 */
extern "C" s32 GetAttachmentCountById(AttachmentValues* values, s32 id)
{
	s32 index = FindAttachment(values, id);
	if (index >= 0) {
		UpdateAttachmentAvailability(values);
		return values->mEntries[index].mValue;
	}
	return -1;
}

/**
 * @note Address: 0x8009C2C8
 * @note Size: 0xD0
 */
extern "C" void SetAttachmentCountById(AttachmentValues* values, s32 id, s32 value)
{
	s32 index = FindAttachment(values, id);
	if (index >= 0)
		values->mEntries[index].mValue = value;
}

// which of his tools he can use, he has them or one of them is going
/**
 * @note Address: 0x8009C398
 * @note Size: 0x548
 */
extern "C" void UpdateAttachmentAvailability(AttachmentValues* values)
{
	XObjChr* chr = gGame.GetPlayer();
	SetAttachmentOn(values, 4, CanUseTool(chr, 0x10));
	SetAttachmentOn(values, 6, CanUseTool(chr, 0x16));
	SetAttachmentOn(values, 5, CanUseTool(chr, 0x15));
	SetAttachmentOn(values, 7, CanUseTool(chr, 0x38));
	SetAttachmentOn(values, 8, false);
}

// the first three start at what the stage says, or at what the command line picked
/**
 * @note Address: 0x8009C8E0
 * @note Size: 0x5C0
 */
extern "C" void UpdateAttachmentCounts(AttachmentValues* values)
{
	if (IsGlobalDataLoaded()) {
		SetAttachmentCountById(
		    values, 1, gCmdLineArgs.mCopterLevelOverride != 0 ? gCmdLineArgs.mCopterLevelOverride - 1 : GetStartingAttachmentLevel(0));
		SetAttachmentCountById(
		    values, 2, gCmdLineArgs.mPopperLevelOverride != 0 ? gCmdLineArgs.mPopperLevelOverride - 1 : GetStartingAttachmentLevel(1));
		SetAttachmentCountById(
		    values, 3, gCmdLineArgs.mRadarLevelOverride != 0 ? gCmdLineArgs.mRadarLevelOverride - 1 : GetStartingAttachmentLevel(2));
	} else {
		SetAttachmentCountById(values, 1, gCmdLineArgs.mCopterLevelOverride != 0 ? gCmdLineArgs.mCopterLevelOverride - 1 : 0);
		SetAttachmentCountById(values, 2, gCmdLineArgs.mPopperLevelOverride != 0 ? gCmdLineArgs.mPopperLevelOverride - 1 : 0);
		SetAttachmentCountById(values, 3, gCmdLineArgs.mRadarLevelOverride != 0 ? gCmdLineArgs.mRadarLevelOverride - 1 : 0);
	}
}

/**
 * @note Address: 0x8009CEA0
 * @note Size: 0x34
 */
extern "C" void RefreshAttachmentMenuEntries(AttachmentValues* values)
{
	UpdateAttachmentCounts(values);
	UpdateAttachmentAvailability(values);
}

// the eight attachments in ring order, then a refresh
/**
 * @note Address: 0x8009CED4
 * @note Size: 0x60
 */
extern "C" void InitializeAttachmentMenuEntries(AttachmentValues* values)
{
	values->mEntries[0].mId = 1;
	values->mEntries[1].mId = 2;
	values->mEntries[2].mId = 3;
	values->mEntries[3].mId = 4;
	values->mEntries[4].mId = 5;
	values->mEntries[5].mId = 6;
	values->mEntries[6].mId = 7;
	values->mEntries[7].mId = 8;
	RefreshAttachmentMenuEntries(values);
}

/**
 * @note Address: 0x8009CF34
 * @note Size: 0x6CC
 */
extern "C" void DrawPlayerLookMarker(XObjChrLook* look)
{
	if (gGame.mMotionControlCamera.IsAction(4) || !(look->mFade > 0.0f))
		return;
	CTexObj tex;
	CTpl* tpl  = &gGame.mTpl;
	f32 scale  = look->mFade + 0.5f * (f32)sin(PI * look->mFade);
	tex        = gGame.mTpl.mTex.Init(&((TPLPalette*)tpl->mBuffer)->descriptorArray[111 - StatusTextureOffset], 0);
	f32 width  = scale * GXGetTexObjWidth(&tex);
	f32 height = scale * GXGetTexObjHeight(&tex);
	Vec pos;
	look->mObject->ProjectOffsetToScreen(&look->mOffset, &pos.x, &pos.y, NULL, NULL);
	pos.x += 0.5f * width;
	pos.y -= 0.5f * height;
	pos.z = 0.0f;
	pos.y += 4.0f * (f32)sin(DEG2RAD * look->mPhase);

	CGs* gs = &gGame.mGs;
	SetGsDepthCompareBeforeTexture(gs->mCurRenderMode, 0);
	SetGsAlphaCompare(gs->mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u32 color    = 0xFFFFFF00 | (u32)(255.0f * look->mFade);
	Vec rotation = { 0.0f, 0.0f, 0.0f };
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, pos.x, pos.y, pos.z, width, height, rotation.x, rotation.y,
	                   rotation.z);

	f32 glow = look->mFade * (0.5f + 0.5f * (f32)sin(PI * look->mPulse));
	if (glow < 0.0f)
		glow = 0.0f;
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
	Vec maskRotation = { 0.0f, 0.0f, 0.0f };
	DrawGsTexturedQuad(&gGame.mGs, &tex, 0xFFFFFF01, 0xFFFFFF01, 0xFFFFFF01, 0xFFFFFF01, pos.x, pos.y, pos.z, 0.99f * width, 0.99f * height,
	                   maskRotation.x, maskRotation.y, maskRotation.z);
	SetGsBlendMode(gs->mCurRenderMode, 1, 2, 4, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 2, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	Vec glowRotation = { 0.0f, 0.0f, 0.0f };
	color            = 0xFFFFFF00 | (u32)(255.0f * glow);
	DrawGsQuad(&gGame.mGs, color, color, color, color, pos.x, pos.y, pos.z, width, height, glowRotation.x, glowRotation.y, glowRotation.z);

	if (look->mMode >= 0) {
		tex = gGame.mTpl.mTex.Init(&((TPLPalette*)tpl->mBuffer)->descriptorArray[look->mMode], 0);
		SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gs->mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		color = 0xFFFFFF00 | (u32)(255.0f * look->mFade);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, pos.x, pos.y, pos.z, width, height, look->mTarget.x,
		                   look->mTarget.y, look->mTarget.z);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 3, 0);
	SetGsDepthCompareBeforeTexture(gs->mCurRenderMode, 1);
	InvalidateCaptureGraphics(gs);
}

// steps the marker on what chibi robo looks at
/**
 * @note Address: 0x8009D600
 * @note Size: 0x178
 */
extern "C" void UpdatePlayerLookMarker(XObjChrLook* look, u32 frames)
{
	if (look->mActive) {
		if (look->mFade < 1.0f) {
			f32 fade    = look->mFade + LOOK_FADE_STEP * frames;
			look->mFade = fade;
			if (fade >= 1.0f)
				look->mFade = 1.0f;
			look->mPulse = look->mFade;
		}
	} else if (look->mFade > 0.0f) {
		f32 fade    = look->mFade - LOOK_FADE_STEP * frames;
		look->mFade = fade;
		if (fade <= 0.0f)
			look->mFade = 0.0f;
	}
	if (look->mFade > 0.0f) {
		look->mPhase = fmod(look->mPhase + (f32)(frames * 6), 360.0);
		if (look->mFade >= 1.0f) {
			f32 pulse    = look->mPulse + LOOK_FADE_STEP * frames;
			look->mPulse = pulse;
			if (pulse >= 2.0f)
				look->mPulse = fmod(look->mPulse, 2.0);
		}
	}
}

// XObjChr::vf144, what chibi robo does when he's left alone: looks at the camera first,
// then two longer idles at 3000 and 6000
/**
 * @note Address: 0x8009D778
 * @note Size: 0x330
 */
extern "C" void vf144__7XObjChrFll(XObjChr* chr, s32 input, s32 elapsed)
{
	s32 idle;
	if (gGame.mEvt.mData.mScript != NULL || !InputEnabled(&gGame) || (gGame.mStatus.mFlags & 0xC00) || (gGame.mStatus.mFlags & 0x100)
	    || !(chr->_980.mFlags & 0x20000000) || chr->mCopter.mState != 0 || (chr->mPopper.mState != 0 && chr->mPopper.mState != 4)
	    || chr->mRadar.mState != 0 || (!(idle = (u8)(chr->mLift.mState == 0)) && chr->mLift.mState != 4) || chr->mHang.mState != 0
	    || chr->mJump.mState != 0 || chr->mLadder.mState != 0 || chr->mRope.mState != 0 || chr->mPush.mState != 0
	    || chr->mTumble.mState != 0 || chr->mAttachmentRequest != -1 || chr->mSuits.mChangeStep != 0)
		return;
	if (input != -1)
		return;
	if (elapsed > 6000 && (idle || (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift)))) {
		u8 release = chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift);
		if (release) {
			if (chr->mLift.mTarget) {
				chr->mLift.mTarget->mStatus.mFlags &= 0xFBFFFFFFFFFFFFFFULL;
				chr->mLift.mTarget = NULL;
			}
			chr->mLift.mState = 0;
		}
		chr->mHeadTurn.mNeck.mMode   = 2;
		chr->mHeadTurn.mNeck.mTarget = 0.0f;
		chr->mHeadTurn.mNeck.mLookAt = NULL;
		chr->mTumble.mState          = 15;
		chr->mTumble.mFade.mActive   = 1;
		chr->mTumble.mFade.mAmount   = 0.0f;
		chr->_158BC                  = 3 * IDLE_SECOND;
	} else if (elapsed > 3000) {
		chr->mTumble.mState          = 12;
		chr->mHeadTurn.mNeck.mMode   = 2;
		chr->mHeadTurn.mNeck.mTarget = 0.0f;
		chr->mHeadTurn.mNeck.mLookAt = NULL;
		chr->_158B8                  = 2;
		chr->_158BC                  = IDLE_SECOND;
	} else {
		f32 angle = (f32)atan2(gGame.mMotionControlCamera.mCamera.mEye.x - chr->mMoveTranslate.x,
		                       gGame.mMotionControlCamera.mCamera.mEye.z - chr->mMoveTranslate.z)
		          - chr->mMoveRotate.y;
		chr->mHeadTurn.mNeck.mMode   = 1;
		chr->mHeadTurn.mNeck.mTarget = angle;
		chr->_158B8                  = 1;
		chr->_158BC                  = IDLE_SECOND;
	}
}

// the ground type under chibi robo, from what he hangs or climbs on, the object he stands
// on, or else the map's floor color. 13 and up count as none
/**
 * @note Address: 0x8009DAA8
 * @note Size: 0x1DC
 */
extern "C" s32 GetPlayerGroundType(XObjChr* chr)
{
	s32 type = 0;
	if (chr->mHang.mState == 5 && chr->mHang.mTarget != NULL) {
		XObj* owner = chr->mHang.mTarget;
		type        = owner->mFloors->mType;
	} else if (chr->mRope.mState == 5 && chr->mRope.mTarget != NULL) {
		XObj* owner = chr->mRope.mTarget;
		type        = owner->mFloors->mType;
	} else if (chr->mLadder.mState == 5 && chr->mLadder.mTarget != NULL) {
		XObj* owner = chr->mLadder.mTarget;
		type        = owner->mFloors->mType;
	} else if (chr->mFloorContacts[0].objectIndex >= 0) {
		XObj* owner = gGame.mXObjs.mObjects[chr->mFloorContacts[0].objectIndex];
		f32 best    = NAN;
		for (s32 i = 0; owner->mFloors[i].mId != -1; ++i) {
			if ((owner->_250[0] & (1U << i)) && (__fpclassifyf(best) == FP_NAN || owner->mFloorHeights[i] > best)) {
				f32 height = owner->mFloorHeights[i];
				if (height <= 1.0f + chr->mMoveTranslate.y) {
					best = height;
					type = owner->mFloors[i].mType;
				}
			}
		}
	}
	type = (u8)type;
	if (type == 0 && chr->mFloorHit.mTriangle >= 0 && (s32)chr->mFloorHit.mTrigon >= 0) {
		type = GetMapTriangleFloorType((XObjMap*)gGame.mXObjs.mPools[3].mObjects, (s32)chr->mFloorHit.mTrigon, chr->mFloorHit.mTriangle);
	}
	if (type >= 13)
		type = 0;
	return type;
}

// the highest floor of the object under a floor group that's still below chibi robo,
// stops early on the preferred one, -1 for none
/**
 * @note Address: 0x8009DC84
 * @note Size: 0x134
 */
extern "C" s32 FindPlayerSupportingFloor(XObjChr* chr, s32 group, s32 preferred)
{
	s32 result = -1;
	if (chr->mFloorContacts[group].objectIndex >= 0) {
		XObj* owner = gGame.mXObjs.mObjects[chr->mFloorContacts[group].objectIndex];
		f32 best    = NAN;
		for (int i = 0; owner->mFloors[i].mId != -1; i++) {
			if (owner->_250[group] & (1 << i)) {
				if ((fpclassify(best) == FP_NAN || owner->mFloorHeights[i] >= best)
				    && owner->mFloorHeights[i] <= 1.0f + chr->mMoveTranslate.y) {
					best   = owner->mFloorHeights[i];
					result = i;
					if (i == preferred)
						break;
				}
			}
		}
	}
	return result;
}

extern "C" {
void fn_800CE334(XActLift*, u32, BOOL);
void ResetPlayerAttachment(XObjChr*, s32);
void SetPlugSparkMode(XObjPlug*, s32);
void DisablePlugSparkEffect(XObjPlug*);
void SetObjectJointTranslationByPath(CJObj*, s32, s32*, f32, f32, f32);
}
static inline void ClearPlugState(XObjPlug* plug, BOOL resetJoint)
{
	plug->_D3C = 0;
	if (resetJoint) {
		s32 path        = lbl_8065B260;
		HSD_JObj* joint = GetJointByPath(plug->mJObj, &path, 1);
		HSD_JObjSetRotationX(joint, 0.0f);
	}
}

static inline void RequestScriptAttachment(XObjChr* player, s32 id)
{
	if (player->mAttachment != id) {
		player->mAttachmentRequest = id;
		player->_95D8              = 0;
	}
}

static inline BOOL ScriptLiftReady(XObjChr* player)
{
	s32 request = player->mAttachmentRequest;
	return request != 4 && request != 5 && request != 6 && request != 7 && (player->mLift.mState == 0 || player->mLift.mState == 4);
}

static inline CTexObj& RingTexture(OctaMenu* menu, s32 index)
{
	s32 id = menu->GetItemIconTextureIndex(index);
	return gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[id], 0);
}
extern "C" {
extern f32 gEquipmentIconWidth[8], gEquipmentIconHeight[8];
}

static inline void DrawRingBandLines(f32 x0, f32 x1, f32 x2, f32 x3, f32 y0, f32 y1, u32 ink)
{
	DrawGsLineSegment(&gGame.mGs, NULL, x1, y0, 0, ink, x2, y1, 0, ink);
	DrawGsLineSegment(&gGame.mGs, NULL, x0, y0, 0, 0xffffff00, x1, y1, 0, ink);
	DrawGsLineSegment(&gGame.mGs, NULL, x2, y0, 0, ink, x3, y1, 0, 0xffffff00);
}
static inline void DrawRingQuad(CGs& gs, CTexObj& tex, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 rotZ = 0, f32 rotY = 0)
{
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, y, z, w, h, 0, rotY, rotZ);
}

/**
 * @note Address: 0x80099224
 * @note Size: 0x1DB0 (US/PAL), 0x1DC4 (JP)
 */
extern "C" void DrawEquipmentRing(OctaMenu* menu)
{
	if (!(menu->mFade > 0))
		return;
	f32 originX, originY;
	f32 slotX[8], slotY[8];
	f32 scale;
	if (gGame.mStatus.mFlags & 0x100) {
		originX = 320;
		originY = 276;
	} else if (menu->_280 == 3) {
		originX = 320;
		originY = 228;
	} else {
		Vec offset = { 0, 8.8f, 0 };
		menu->mChr->ProjectOffsetToScreen(&offset, &originX, &originY, 0, NULL);
	}
	f32 fade = menu->mFade;
	if (menu->_280 == 3) {

		for (s32 i = 0; i < 8; ++i) {
			OctaMenuItem& item = menu->mItems[i];
			s32 slot           = i - menu->mFirstVisibleItem;
			if (slot < 0)
				slot += 7;
			item.mTargetSlotOffset = menu->mFade * gEquipmentRowX[slot];
			slotY[i]               = menu->_284 + gEquipmentRowY[slot];
			if (menu->mFade < 1.f) {
				item.mSlotOffset = item.mTargetSlotOffset;
			} else {
				if (item.mSlotOffset < 0 && item.mTargetSlotOffset == 3)
					item.mSlotOffset = 4;
				else if (item.mSlotOffset > 0 && item.mTargetSlotOffset == -3)
					item.mSlotOffset = -4;
				f32 step = item.mTargetSlotOffset - item.mSlotOffset;
				if (step > 1) {
					step             = 1;
					item.mSlotOffset = item.mTargetSlotOffset - 1;
				}
				if (step < -1) {
					step             = -1;
					item.mSlotOffset = item.mTargetSlotOffset + 1;
				}
				item.mSlotOffset += .4f * step;
			}
			slotX[i] = item.mSlotOffset;
		}
		scale = 1.1f;
	} else if (menu->_280 == 0) {
		for (int i = 0; i < 8; ++i) {
			slotY[i] = gEquipmentRingY[i];
			slotX[i] = gEquipmentRingX[i];
		}
		scale = 1.1f;
	} else {
		for (s32 i = 0, j = -1; i < 8; ++i, ++j) {
			f32 angle = (PI / 4.f) * j;
			slotY[i]  = -1.f * (f32)cos(angle);
			slotX[i]  = (f32)sin(angle);
		}
		scale = 1.35f;
	}

	CGs& gs = gGame.mGs;
	for (s32 i = 0; i < 8; ++i) {
		CTexObj tex = RingTexture(menu, i);
		u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
		gEquipmentIconWidth[i]  = 100.f;
		gEquipmentIconHeight[i] = 100.f * ((f32)h / w);
	}
	s32 count = menu->_280 == 3 ? 7 : 8;
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsLineWidth(gs.mCurRenderMode, 12, 5);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
	if (menu->_280 == 3) {
		s32 start = menu->mFirstVisibleItem, last = (start + 6) % 7;
		f32 itemSize = menu->mItems[start]._10 * (f32)sin((PI * .5f) * fade);
		f32 radius   = 60.f + 20.f * itemSize;
		f32 y0       = originY + (scale * slotY[start]) * radius;
		f32 left     = originX + (-3.f * scale) * radius;
		f32 y1       = originY + (scale * slotY[last]) * radius;
		f32 right    = originX + (3.f * scale) * radius;
		f32 half     = .7f * (itemSize * gEquipmentIconHeight[start]);

		f32 labelY    = y0 + 24.f;
		u32 fill      = (u32)(128.f * fade);
		f32 highlight = 0;
		for (s32 i = 0; i < count; ++i) {
			if (menu->mItems[i].mHighlight > highlight)
				highlight = menu->mItems[i].mHighlight;
		}
		// The vertex buffer also contains the strip below the icons for their labels.
		u32 labelAlpha      = (u32)(128.f * highlight);
		u32 clear           = 0;
		PrimVertex band[24] = { { (Vec) { ((left - half) - 2.f * half), (y0 - half), 0 }, clear },
			                    { (Vec) { (left - half), (y0 - half), 0 }, fill },
			                    { (Vec) { (left - half), (y0 + half), 0 }, fill },
			                    { (Vec) { ((left - half) - 2.f * half), (y0 + half), 0 }, clear },
			                    { (Vec) { (left - half), (y0 - half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 - half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 + half), 0 }, fill },
			                    { (Vec) { (left - half), (y0 + half), 0 }, fill },
			                    { (Vec) { (right + half), (y0 - half), 0 }, fill },
			                    { (Vec) { ((right + half) + 2.f * half), (y0 - half), 0 }, clear },
			                    { (Vec) { ((right + half) + 2.f * half), (y0 + half), 0 }, clear },
			                    { (Vec) { (right + half), (y0 + half), 0 }, fill },
			                    { (Vec) { ((left - half) - 2.f * half), labelY, 0 }, clear },
			                    { (Vec) { (left - half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (left - half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { ((left - half) - 2.f * half), labelY + half, 0 }, clear },
			                    { (Vec) { (left - half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { (left - half), labelY + half, 0 }, labelAlpha },
			                    { (Vec) { (right + half), labelY, 0 }, labelAlpha },
			                    { (Vec) { ((right + half) + 2.f * half), labelY, 0 }, clear },
			                    { (Vec) { ((right + half) + 2.f * half), labelY + half, 0 }, clear },
			                    { (Vec) { (right + half), labelY + half, 0 }, labelAlpha } };
		DrawGsTriangleVertices(&gGame.mGs, (GXPrimitive)0xa0, gs.mCamera->mViewMtx, band, 4);
		DrawGsTriangleVertices(&gGame.mGs, (GXPrimitive)0xa0, gs.mCamera->mViewMtx, band + 4, 4);
		DrawGsTriangleVertices(&gGame.mGs, (GXPrimitive)0xa0, gs.mCamera->mViewMtx, band + 8, 4);
		left -= half;
		right += half;
		f32 x0 = left - 2.f * half, x3 = right + 2.f * half;
		f32 top = y0 - half, bottom = y0 + half;
		u32 ink = 0xffffff00 | (u32)(255.f * fade);
		DrawRingBandLines(x0, left, right, x3, top, y1 - half, ink);
		DrawRingBandLines(x0, left, right, x3, bottom, y1 + half, ink);
	}
	CTexObj background = RingTexture(menu, -1);
	for (s32 i = 0; i < count; ++i) {
		CTexObj tex        = RingTexture(menu, i);
		OctaMenuItem& item = menu->mItems[i];
		f32 width = gEquipmentIconWidth[i], height = gEquipmentIconHeight[i];
		sin(item._08);
		sin(item._0C);
		sin(.5f * item._0C);
		sin(.7f * item._0C);
		f32 pulse;
		if (menu->_280 == 3)
			pulse = item._10 * (f32)sin((PI * .5f) * fade);
		else
			pulse = item._10 * (f32)sin(2.0943952f * fade);
		f32 x, y;

		if (menu->_280 == 3) {
			x        = originX + 68.f * (scale * slotX[i]);
			y        = originY + 68.f * (scale * slotY[i]);
			s32 slot = i - menu->mFirstVisibleItem;
			if (slot < 0)
				slot += 7;
			if (slot == 3) {
				menu->mCursorY = y;
				menu->mCursorX = menu->mFade < 1.f ? x : 320.f;
			}
		} else {
			f32 radius = 60.f + 20.f * item._10;
			x          = originX + fade * (radius * (scale * slotX[i]));
			y          = 10.f + ((originY + fade * (radius * (scale * slotY[i]))) - (.5f * width) * .5f);
		}
		if (menu->_280 != 3 && menu->mFocus) {
			if (menu->_280 == 0) {
				y += 80.f * (slotY[i] * (f32)sin(PI * menu->_0C));
				x += 80.f * (slotX[i] * (f32)sin(PI * menu->_0C));
			} else {
				y += 100.f * (slotY[i] * (f32)sin(PI * menu->_0C));
				x += 100.f * (slotX[i] * (f32)sin(PI * menu->_0C));
			}
		}
		f32 slot = item.mSlotOffset;
		f32 alpha;
		if (slot >= 3.02f)
			alpha = Abs(4.f - slot);
		else if (slot <= -3.02f)
			alpha = Abs(4.f + slot);
		else
			alpha = 1.f;
		menu->GetItemColor(i);
		SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
		f32 iconAlpha = alpha;
		if (menu->vf2C(i))
			iconAlpha = alpha * .25f;
		u32 color = 0xffffff00 | (u32)(255.f * alpha);
		f32 size  = 1.2f * pulse;
		SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
		SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 0);
		SetGsTevMode(gs.mCurTevStage, 0);
		DrawRingQuad(gs, background, color, x, y, 0, width * size, height * size);
		DrawRingQuad(gs, tex, 0xffffff00 | (u32)(255.f * iconAlpha), x, y, 0, width * size, height * size);
		SetGsTevMode(gs.mCurTevStage, 4);
		SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 1);
		if (i == menu->mCursorIndex) {
			SetGsBlendMode(gs.mCurRenderMode, 1, 2, 4, 15);
			SetGsDepthMode(gs.mCurRenderMode, 1, 2, 0);
			f32 glow = fade * (.2f * (f32)sin(PI * menu->mPhase) + .2f);
			if (glow < 0)
				glow = 0;
			u32 c = 0xffffff00 | (u32)(255.f * glow);
			DrawGsQuad(&gGame.mGs, c, c, c, c, x, y, 0, width * size, height * size, 0, 0, 0);
			SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
			SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
		}
		if (menu->_280 == 3 && (item.mSlotOffset >= 3.02f || item.mSlotOffset <= -3.02f)) {
			f32 span   = 7.f * scale;
			f32 radius = 20.f * item._10 + 60.f;
			if (item.mSlotOffset >= 3.02f) {
				x     = x - span * radius;
				alpha = Abs(item.mSlotOffset - 3.f);
			} else {
				x     = x + span * radius;
				alpha = Abs(3.f + item.mSlotOffset);
			}
			menu->GetItemColor(i);
			SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
			u32 c = 0xffffff00 | (u32)((255.f * fade) * alpha);
			SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
			SetGsTevMode(gs.mCurTevStage, 0);
			DrawRingQuad(gGame.mGs, background, c, x, y, 0, width * size, height * size);
			DrawRingQuad(gGame.mGs, tex, c, x, y, 0, width * size, height * size);
			SetGsTevMode(gs.mCurTevStage, 4);
		}
	}
	SetGsCullMode(gs.mCurRenderMode, 2);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsDepthMode(gs.mCurRenderMode, 0, 7, 0);
	f32 zero = 0.0f;
	for (s32 i = 0; i < count; ++i) {
		OctaMenuItem& item = menu->mItems[i];
		if (item.mHighlight == zero)
			continue;
		f32 width = gEquipmentIconWidth[i];
		sin(2.0943952f * fade);
		f32 x, y;
		if (menu->_280 == 3) {
			y = originY + 68.f * (scale * slotY[i]);
			x = originX + 68.f * (scale * slotX[i]);
		} else {
			f32 radius = 60.f + 20.f * item._10;
			y          = 10.f + ((originY + fade * (radius * (scale * slotY[i]))) - (.5f * width) * .5f);
			x          = originX + fade * (radius * (scale * slotX[i]));
		}
		f32 highlight      = item.mHighlight;
		const char* source = menu->GetItemLabel(i);
		char label[128];
		strncpy(label, source, sizeof(label) - 1);
		label[sizeof(label) - 1] = 0;
		MeasureGameTextWidth(&gGame.mGs, label, TRUE, 22.f);
		f32 baseline = 52.f + y;
		f32 tileSize = 30.f;
		f32 spacing  = tileSize;
#if !defined(VERSION_JP)
		spacing *= 0.9f;
#endif
		SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
		SetGsTevMode(gs.mCurTevStage, 0);
		CTexObj tile       = RingTexture(menu, -2);
		menu->mLabelLength = 0;
		const char* p      = label;
		for (s32 k = 0; k < (s32)ARRAY_SIZE(menu->mLabelChars) && *p; ++k, ++p) {
			menu->mLabelChars[k].mText = p;
			++menu->mLabelLength;
			if (HelpTextIsJapanese() && (IsSjisLead((u8)*p) & 1) && p[1])
				++p;
		}
		f32 phase = PI * highlight, halfPhase = (PI * .5f) * highlight;
		f32 halfSpacing = 0.5f * spacing;
		u32 opacity     = (u32)(255.f * highlight);
		for (s32 k = 0; k < menu->mLabelLength; ++k) {
			MenuHelpChar& ch = menu->mLabelChars[k];

			if (!HelpTextIsJapanese() && strncmp(ch.mText, " ", 1) == 0)
				continue;
			CFont font;
			s32 width;
			CTexObj glyph = *GetFontGlyphTexture(&font, ch.mText, &width);
			f32 bob       = 2.f * (f32)sin(ch.mAngle0);
			f32 spin      = (f32)sin(.7f * ch.mAngle1);
			f32 glow      = (f32)sin(phase);
			f32 wave      = (f32)sin(halfPhase);
			f32 cy        = baseline + bob;
			f32 scale     = .2f * glow + wave;
			f32 rotation  = -(DEG2RAD * (10.f * spin));

			f32 cx = halfSpacing + (k * spacing + (x - (menu->mLabelLength * spacing) * .5f));
			DrawRingQuad(gGame.mGs, tile, 0xffffff00 | opacity, cx, cy, 1, tileSize * scale, tileSize * scale, rotation, -zero);
			f32 glyphSize = tileSize * (.8f * scale);
			f32 padding   = HelpGlyphPadding(width, glyphSize);

			for (s32 j = 0; j < 4; ++j)
				DrawRingQuad(gGame.mGs, glyph, opacity, .5f * padding + (cx + gEquipmentOutlineX[j]),
				             bob + (baseline + gEquipmentOutlineY[j]), 1, glyphSize, glyphSize, rotation, -zero);
			DrawRingQuad(gGame.mGs, glyph, 0xfffffb00 | opacity, .5f * padding + cx, cy, 1, glyphSize, glyphSize, rotation, -zero);
		}
		// Keep the label references valid after the local render copy expires.
		for (s32 k = 0; k < menu->mLabelLength; ++k) {
			MenuHelpChar& ch = menu->mLabelChars[k];
			ch.mText         = source + (ch.mText - label);
		}
		SetGsTevMode(gs.mCurTevStage, 4);
	}
	SetGsDepthMode(gs.mCurRenderMode, 1, 3, 0);
}

/**
 * @note Address: 0x8009DDB8
 * @note Size: 0x1740
 */
s32 XObjChr::HandleEventCommand(s32 count, s32* args)
{
	s32 result = 0;
	if (count <= 0)
		return result;
	switch (args[0]) {
	case 100:
		return mPlug._D3C;
	case 101: {
		XObj* obj = FindObj(args[1]);
		return obj->GetCategory() == 9 ? GetClearLetickerApproachSide(obj, &mMoveTranslate) : 1;
	}
	case 102: {
		s32 value         = args[2];
		XObj* target      = FindObj(args[1]);
		mLift.mState      = 7;
		mLift.mPlugTarget = target;
		fn_800CE334(&mLift, value, FALSE);
		result = -1;
		break;
	}
	case 103: {
		XObj* target = FindObj(args[1]);
		SetPos(target->mMoveTranslate, 0);
		SetRotationY(PI + target->mMoveRotate.y, 0);
		HSD_AObj* anim = GetAObj(mJObj);
		if (anim) {
			mLift.mState      = 7;
			mLift.mPlugTarget = target;
			fn_800CE334(&mLift, args[2], FALSE);
			do {
				f32 end   = anim ? HSD_AObjGetEndFrame(anim) : 0;
				f32 frame = anim ? HSD_AObjGetCurrentFrame(anim) : 0;
				UpdateLiftActionAnimation(&mLift, mAnimId, anim, 2, frame, end);
				UpdateObject__4XObjFUl(this, 2);
				mPlug.UpdateObject(2);
				UpdateHeldObject(&mLift, 2);
			} while (mLift.mState == 7);
		}
		result = -1;
		break;
	}
	case 104: {
		XObj* target      = FindObj(args[1]);
		mLift.mState      = 11;
		mLift.mPlugTarget = target;
		result            = -1;
		break;
	}
	case 105:
		if (mPlug._D3C) {
			PlayGameSound(&gGame.mSound, 0x37, 255);
			ClearPlugState(&mPlug, TRUE);
		}
		break;
	case 106: {
		XObj* target      = FindObj(args[1]);
		mLift.mState      = 8;
		mLift.mPlugTarget = target;
		result            = -1;
		break;
	}
	case 107:
		FindObj(args[1]);
		ClearPlugState(&mPlug, TRUE);
		result = -1;
		break;
	case 108: {
		XObj* target      = FindObj(args[1]);
		mLift.mState      = 9;
		mLift.mPlugTarget = target;
		result            = -1;
		break;
	}
	case 109:
		FindObj(args[1]);
		ClearPlugState(&mPlug, TRUE);
		result = -1;
		break;
	case 110: {
		mPlug._D3C = 0;
		s32 path[] = { 0 };
		CJObj joint(GetJointByPath(mPlug.mJObj, path, 1));
		joint.SetRotationX(0, 0);
		joint.SetRotationZ(0, 0);
		result = -1;
		break;
	}
	case 111:
		fn_800CE334(&mLift, args[1], TRUE);
		break;
	case 120:
		if (args[1])
			gGame.mStatus.mFlags |= 0x40000000000000ULL;
		else
			gGame.mStatus.mFlags &= ~0x40000000000000ULL;
		break;
	case 200:
		switch (args[1]) {
		case 205:
			TurnJoint(DEG2RAD * args[2], &mHeadTurn.mNeck);
			break;
		case 207: {
			Vec target = { 0, 0, 0 };
			target.x   = args[2] / 100.f;
			target.z   = args[3] / 100.f;
			TurnJoint((f32)atan2(target.x - mMoveTranslate.x, target.z - mMoveTranslate.z) - mMoveRotate.y, &mHeadTurn.mNeck);
			break;
		}
		case 208: {
			XObj* target = FindObj(args[2]);
			TurnJoint((f32)atan2(target->mMoveTranslate.x - mMoveTranslate.x, target->mMoveTranslate.z - mMoveTranslate.z) - mMoveRotate.y,
			          &mHeadTurn.mNeck);
			break;
		}
		case 220: {
			XObj* target            = FindObj(args[2]);
			mHeadTurn.mNeck.mMode   = 1;
			mHeadTurn.mNeck.mLookAt = target;
			break;
		}
		case 225: {
			CVec& eye = gGame.mMotionControlCamera.mCamera.mEye;
			TurnJoint((f32)atan2(eye.x - mMoveTranslate.x, eye.z - mMoveTranslate.z) - mMoveRotate.y, &mHeadTurn.mNeck);
			break;
		}
		}
		return 0;
	case 201:
		mHeadTurn.mNeck.mMode   = 2;
		mHeadTurn.mNeck.mTarget = 0;
		mHeadTurn.mNeck.mLookAt = NULL;
		return 0;
	case 202:
		return mHeadTurn.mNeck.mMode <= 0;
	case 300:
		mRope.SetState(4, FindObj(args[1]));
		result = -1;
		break;
	case 301:
		mLadder.SetState(4, FindObj(args[1]));
		result = -1;
		break;
	case 302:
		mJump.SetState(14, FindObj(args[1]));
		result = -1;
		break;
	case 303:
		mRadar.mState = args[1] ? 1 : 2;
		result        = -1;
		break;
	case 400:
		return mAttachmentRequest < 0;
	case 450:
		if ((u32)args[1] < 3)
			mSuits.mSpecial = args[1];
		break;
	case 451:
		return mSuits.mSpecial;
	case 503:
		if (args[1] == 16) {
			RequestScriptAttachment(this, 4);
			_95DC = 1;
		} else if (args[1] == 21) {
			RequestScriptAttachment(this, 5);
			_95DC = 1;
		} else if (args[1] == 22) {
			RequestScriptAttachment(this, 6);
			_95DC = 1;
		} else {
			mLift.mState = 24;
			mLift._48    = args[1];
		}
		result = -1;
		break;
	case 513:
		return ScriptLiftReady(this);
	case 500:
		if (FindOwnedInventoryItemIndex(mParams.mInventory.mItems, args[1]) >= 0) {
			if (args[1] == 16) {
				RequestScriptAttachment(this, 4);
			} else if (args[1] == 21) {
				RequestScriptAttachment(this, 5);
			} else if (args[1] == 22) {
				RequestScriptAttachment(this, 6);
			} else if (args[1] == 56) {
				RequestScriptAttachment(this, 7);
			} else {
				mLift.mState = 20;
				mLift._48    = args[1];
			}
		}
		result = -1;
		break;
	case 510:
		return ScriptLiftReady(this);
	case 501:
		if (mLift.mTarget) {
			if (IsAttachment(this, 4) || IsAttachment(this, 5) || IsAttachment(this, 6) || IsAttachment(this, 7) || IsAttachment(this, 8))
				RequestScriptAttachment(this, 0);
			else if (mLift.mTarget->GetCategory() == 8)
				mLift.mState = 19;
		}
		result = -1;
		break;
	case 514:
		return ScriptLiftReady(this);
	case 502:
		if (mLift.mTarget && mLift.mTarget->GetCategory() == 8)
			mLift.mState = 23;
		result = -1;
		break;
	case 550:
		gGame.GetPlayer()->mParams.mInventory.mCompact = TRUE;
		CompactInventory(&gGame.GetPlayer()->mParams.mInventory);
		break;
	case 551:
		gGame.GetPlayer()->mParams.mInventory.mCompact = FALSE;
		break;
	case 600:
		SetPlugSparkMode(&mPlug, args[1]);
		result = -1;
		break;
	case 601:
		DisablePlugSparkEffect(&mPlug);
		result = -1;
		break;
	case 700:
		_146D8 = 1;
		break;
	case 701:
		_146D8 = 0;
		break;
	case 702:
		return _146D8;
	case 800:
		_158A4 = 1;
		break;
	case 801:
		_158A4 = 0;
		break;
	case 802:
		return _158A4;
	case 850:
		mParams._1C[0] = 1;
		break;
	case 851:
		mParams._1C[0] = 0;
		break;
	case 852:
		return (s32)mParams._1C[0] > 0;
	case 900:
		_158A8        = args[1];
		mItemTintWord = ((u32)args[2] << 24) | ((u32)args[3] << 16) | ((u32)args[4] << 8) | 255;
		break;
	case 901:
		mCameraTargetMarginSetting = args[1] / 100.f;
		break;
	case 902:
		StopShots(this);
		break;
	case -1:
		ResetPlayerAttachment(this, 17);
		break;
	case -2: {
		s32 path[] = { 0 };
		SetPos(GetEventJointWorldPosition(this, 1, path), 0);
		s32 root[] = { 0 };
		SetObjectJointTranslationByPath(this, 1, root, 0, 0, 0);
		break;
	}
	}
	return result;
}

/**
 * @note Address: 0x8009F4F8
 * @note Size: 0x2D8
 */
extern "C" void SetObjectJointTranslationByPath(CJObj* object, s32 depth, s32* path, f32 x, f32 y, f32 z)
{
	CJObj joint(GetJointByPath(object->mJObj, path, depth));
	joint.CJObj::SetTranslate(x, y, z, TRUE);
}

// pool 9 objects decide for themselves whether chibi robo can reach them
/**
 * @note Address: 0x8009FC60
 * @note Size: 0x64
 */
extern "C" s32 GetCharacterObjectApproachResult(const XObjChr* chr, const XObj* target)
{
	if (target->GetCategory() == 9)
		return GetClearLetickerApproachSide((XObj*)target, &chr->mMoveTranslate);
	return 1;
}

// resets the plug, keeping its rotation while it's carried
/**
 * @note Address: 0x8009FCC4
 * @note Size: 0x120
 */
extern "C" void ResetPlayerPlug(XObjChr* chr)
{
	if (chr->mPlug.mStatus.mFlags & 0x0400000000000000ULL) {
		Vec rotation = (Vec&)chr->mPlug.mMoveRotate;
		chr->mPlug.mStatus.mFlags &= ~0x0400000000000000ULL;
		chr->mPlug.ResetObjectState();
		chr->mPlug.UpdateObject(10);
		XObj* plug = &chr->mPlug;
		plug->SetRot(rotation, 0);
		chr->mPlug.mStatus.mFlags |= 0x0400000000000000ULL;
	} else {
		chr->mPlug.ResetObjectState();
		chr->mPlug.UpdateObject(10);
	}
}

/**
 * @note Address: 0x8009FDE4
 * @note Size: 0xF8
 */
// NONMATCHING: regalloc
extern "C" BOOL IsPlayerBlockingEvents(const XObjChr* player)
{
	return IsEventBlocked(player);
}

// out of battery
/**
 * @note Address: 0x8009FEDC
 * @note Size: 0x64
 */
extern "C" BOOL IsPlayerStopped(const XObjChr* chr)
{
	return IsStopped(chr);
}

// XObjChr::OnHitFromPosition, hit from a position. flashes what he carries instead when he lifts something
/**
 * @note Address: 0x8009FF40
 * @note Size: 0xC0
 */
extern "C" void OnHitFromPosition__7XObjChrFRC3Vecf(XObjChr* chr, const Vec&, f32 strength)
{
	if (IsCarryingMug(&chr->mLift)) {
		chr->mLift.mTarget->vf12C(1, 150, 150, 0, 0.0f);
		chr->mLift.mTarget->vf12C(1, 0, 0, 0, 0.2f);
	} else if (chr->_A34 <= 0) {
		ApplyPlayerHitReaction(chr, TRUE, TRUE, strength);
	}
}

// still flashing from a hit
/**
 * @note Address: 0x800A0000
 * @note Size: 0x14
 */
extern "C" u8 IsPlayerFlashingFromHit(XObjChr* chr)
{
	return chr->_A34 > 0;
}

// hit, flinches when he's free to and starts the flash timer
/**
 * @note Address: 0x800A0014
 * @note Size: 0x220
 */
extern "C" void ApplyPlayerHitReaction(XObjChr* chr, BOOL start, BOOL cancel, f32 value)
{
	if (!(gGame.mStatus.mFlags & 0xC00ULL) && chr->mAttachmentRequest < 0 && gGame.mEvt.mData.mScript == NULL) {
		BOOL idle;
		if (start && ((idle = chr->mLift.mState == 0) || chr->mLift.mState == 4) && chr->mHang.mState == 0 && chr->mJump.mState == 0
		    && chr->mLadder.mState == 0 && chr->mRope.mState == 0 && chr->mPush.mState == 0 && chr->mTumble.mState == 0
		    && chr->mCopter.mState == 0 && (chr->mPopper.mState == 0 || chr->mPopper.mState == 4 || chr->mPopper.mState == 8)
		    && chr->mRadar.mState == 0) {
			if (idle)
				chr->SetAnim(0xD4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			else
				chr->SetAnim(0x94, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			chr->mTumble.mState = 8;
		} else if (cancel) {
			if (chr->mHang.mState == 5)
				chr->mHang.mState = 8;
			else if (chr->mLadder.mState == 5)
				chr->mLadder.mState = 8;
			else if (chr->mRope.mState == 5)
				chr->mRope.mState = 7;
		}
	}
	if (0.0f != value) {
#if defined(VERSION_PAL)
		chr->_A34 = gVideoFramesPerSecond;
#else
		chr->_A34 = 60;
#endif
		chr->_A38 = 0;
		chr->_A3C = value;
	}
}

// XObjChr::vf12C, the color goes to everything he wears and carries too. while a costume
// change is running, blend 2 turns into 1 and 1 is dropped
/**
 * @note Address: 0x800A0234
 * @note Size: 0x1E0
 */
extern "C" void vf12C__7XObjChrFlUcUcUcf(XObjChr* chr, s32 blend, u8 red, u8 green, u8 blue, f32 time)
{
	if (chr->mSuits.mChangeStep != 0) {
		if (blend == 2)
			blend = 1;
		else if (blend == 1)
			return;
	}
	vf12C__4XObjFlUcUcUcf(chr, blend, red, green, blue, time);
	chr->mPlug.vf12C(blend, red, green, blue, time);
	chr->mCopterObj.vf12C(blend, red, green, blue, time);
	chr->mPopperObj.vf12C(blend, red, green, blue, time);
	chr->mRadarObj.vf12C(blend, red, green, blue, time);
	chr->mOXObj.vf12C(blend, red, green, blue, time);
	for (s32 i = 0; i < 9; ++i)
		chr->mSuits.mObjects[i].vf12C(blend, red, green, blue, time);
	if (IsCarryingMug(&chr->mLift))
		chr->mLift.mTarget->vf12C(blend, red, green, blue, time);
}

static inline f32 PlayerCollisionRadius(XObjChr* chr)
{
	f32 size = 1.5f * chr->mMoveScale.x;
	// the plug anims pull it in and out
	if (IsCarryingMug(&chr->mLift) && !(chr->mAnimId == 11 && CurrentFrame(chr->mJObj) < 45.0f)
	    && !(chr->mAnimId == 10 && CurrentFrame(chr->mJObj) > 22.0f)) {
		CVec scale;
		s32 path[2] = { 0, 0 };
		HSD_JObjGetScale(FindJointByPath(chr->mJObj, path, 2), &scale);
		size += 4.0f * scale.x;
	}
	return size;
}

// his radius, bigger while he holds something up, by the scale of the held joint
/**
 * @note Address: 0x800A0414
 * @note Size: 0x280
 */
extern "C" f32 GetPlayerCollisionRadius(XObjChr* chr)
{
	return PlayerCollisionRadius(chr);
}

// XObjChr::SetMoveDir, stops whatever push is running and keeps the heading he walks towards
/**
 * @note Address: 0x800A0694
 * @note Size: 0x1BC
 */
extern "C" void SetMoveDir__7XObjChrFPC3Vecf(XObjChr* chr, const Vec* direction, f32 speed)
{
	StopMoving(chr);
	chr->CXObj::SetMoveDir(direction, speed);
	CVec horizontal = chr->mMoveTranslate.mSpeed;
	horizontal.y    = 0.0f;
	if (horizontal.Length() > 0.0f)
		chr->_12DF8 = WrapAngle(GetYAngle(chr->mMoveTranslate.mTarget - chr->mMoveTranslate));
}

/**
 * @note Address: 0x800A0850
 * @note Size: 0x44
 */
extern "C" CVec GetPlayerCarryJointLocalTranslation(CJObj* object)
{
	s32 path[2] = { 0, 0 };
	return CJObj_GetJointLocalTranslationByPath(object, 2, path);
}

// a joint's local translation
/**
 * @note Address: 0x800A0894
 * @note Size: 0xE4
 */
extern "C" CVec CJObj_GetJointLocalTranslationByPath(const CJObj* object, s32 depth, s32* path)
{
	Vec translate;
	HSD_JObj* jobj = GetJointByPath(object->mJObj, path, depth);
	HSD_JObjGetTranslation(jobj, &translate);
	return translate;
}

/**
 * @note Address: 0x800A0978
 * @note Size: 0x44
 */
extern "C" CVec GetPlayerCarryJointWorldPosition(CJObj* object)
{
	s32 path[2] = { 0, 0 };
	return CJObj_GetJointWorldPositionByPath(object, 2, path);
}

// halfway between two of his joints, pulled back a little along his heading
/**
 * @note Address: 0x800A09BC
 * @note Size: 0x21C
 */
extern "C" Vec GetPlayerFirstPersonEyePosition(XObjChr* chr)
{
	s32 left[8]  = { 0, 1, 0, 1, 2, 0, 0, 0 };
	CVec first   = CJObj_GetJointWorldPositionByPath(chr, 8, left);
	s32 right[8] = { 0, 1, 0, 1, 2, 0, 0, 2 };
	CVec second  = CJObj_GetJointWorldPositionByPath(chr, 8, right);
	Vec point    = first + (second - first) / 2.0f;
	point.x -= 1.5f * (f32)sin(chr->mMoveRotate.y);
	point.z -= 1.5f * (f32)cos(chr->mMoveRotate.y);
	return point;
}

extern "C" {
f32 GetAreaCameraFovRatio(CMotionControlCamera*);
BOOL IsObjectActive(XObj*);
BOOL IsCopterActionState(const XAction*, s32);
BOOL IsJumpActionState(const XAction*, s32);
BOOL IsLadderActionState(const XAction*, s32);
BOOL IsRopeActionState(const XAction*, s32);
s32 ClassifyFloatBits(f32);
f32 GetJointAnimationEndFrame(CJObj*);
f32 GetJointAnimationCurrentFrame(CJObj*);
}

/**
 * @note Address: 0x800A0BD8
 * @note Size: 0x1FF8 (US)
 */
Vec XObjChr::GetCameraTarget()
{
	if (IsObjectActive(this)) {
		s32 anim = mAnimId;
		Vec target;
		if (anim == 0xaf || anim == 0xea) {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			CVec start = mMoveTranslate;
			f32 end    = GetJointAnimationEndFrame(this);
			f32 t      = GetJointAnimationCurrentFrame(this) / end;
			target     = start + (joint - start) * t + (Vec) { 0, 10, 0 };
		} else if (anim == 0x17 || anim == 0x7d || anim == 0x1f || anim == 0x20) {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			CVec start = mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			f32 end    = GetJointAnimationEndFrame(this);
			f32 t      = GetJointAnimationCurrentFrame(this) / end;
			target     = start + (joint - start) * t + (Vec) { 0, 10, 0 };
		} else if (anim == 0x76) {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			CVec start = mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			f32 end    = GetJointAnimationEndFrame(this);
			f32 t      = GetJointAnimationCurrentFrame(this) / end;
			t *= 1.7f;
			if (t > 1.f)
				t = 1.f;
			target = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
		} else if (!IsRopeActionState(&mRope, 0)) {
			if (IsRopeActionState(&mRope, 2) && anim == 0x98) {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				if (GetJointAnimationCurrentFrame(this) > 80.f) {
					joint = mMoveTranslate;
					start.y -= mRope._10.y;
					f32 end = GetJointAnimationEndFrame(this) - 80.f;
					f32 t   = (GetJointAnimationCurrentFrame(this) - 80.f) / end;
					target  = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
				} else {
					start.x = joint.x;
					start.z = joint.z;
					target  = start + (Vec) { 0, 10, 0 };
				}
			} else if (IsRopeActionState(&mRope, 6) && anim == 0x24) {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetJointAnimationEndFrame(this);
				f32 t      = GetJointAnimationCurrentFrame(this) / end;
				target     = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			} else if (anim == 0x85) {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetJointAnimationEndFrame(this);
				f32 t      = GetJointAnimationCurrentFrame(this) / end;
				t *= 1.7f;
				if (t > 1.f)
					t = 1.f;
				target = joint + (start - joint) * t + (Vec) { 0, 10, 0 };
			} else {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				target     = start + (Vec) { 0, 10, 0 };
			}
		} else if (!IsLadderActionState(&mLadder, 0)) {
			if (anim == 0x87) {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetJointAnimationEndFrame(this);
				f32 t      = GetJointAnimationCurrentFrame(this) / end;
				t *= 1.7f;
				if (t > 1.f)
					t = 1.f;
				target = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			} else if (anim == 0x72 || anim == 0x73 || anim == 0x74) {
				s32 path[] = { 0 };
				target     = GetEventJointWorldPosition(this, 1, path) + (Vec) { 0, 10, 0 };
			} else {
				s32 path[] = { 0 };
				CVec joint = GetEventJointWorldPosition(this, 1, path);
				CVec start = mMoveTranslate;
				start.x    = joint.x;
				start.z    = joint.z;
				f32 end    = GetJointAnimationEndFrame(this);
				f32 t      = GetJointAnimationCurrentFrame(this) / end;
				target     = start + (joint - start) * t + (Vec) { 0, 10, 0 };
			}
		} else if (anim == 0x13 || anim == 0x14 || anim == 0x15 || anim == 0x16 || anim == 0x19 || anim == 0x1a || anim == 0x1b) {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			CVec start = mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			target     = start + (Vec) { 0, 10, 0 };
		} else if (anim == 0x18) {
			Vec offset = { mHang._10.x, -mHang._10.y, mHang._10.z };
			Mtx rot;
			PSMTXRotRad(rot, 'y', mMoveRotate.y);
			PSMTXMultVec(rot, &offset, &offset);
			s32 path[] = { 0 };
			CVec end   = GetEventJointWorldPosition(this, 1, path) + offset;
			CVec start = mMoveTranslate;

			start.x      = end.x;
			start.z      = end.z;
			f32 frame    = GetJointAnimationCurrentFrame(this);
			f32 duration = GetJointAnimationEndFrame(this);
			f32 t        = frame / duration;
			f32 x = start.x + t * (end.x - start.x), z = start.z + t * (end.z - start.z);
			f32 threshold = .91f * duration;
			f32 elapsed   = GetJointAnimationCurrentFrame(this) - threshold;
			f32 remaining = GetJointAnimationEndFrame(this) - threshold;
			f32 dy        = elapsed > 0 ? (end.y - start.y) * (elapsed / remaining) : 0.f;
			target        = (Vec) { x, 10.f + (start.y + dy), z };

		} else if (IsJumpActionState(&mJump, 9) || IsJumpActionState(&mJump, 12) || IsJumpActionState(&mJump, 15)
		           || IsJumpActionState(&mJump, 17)) {
			target   = mMoveTranslate + (Vec) { 0, 10, 0 };
			target.y = 10.f + mJump.mArcPosition.y;
		} else if (IsJumpActionState(&mJump, 10) || IsJumpActionState(&mJump, 13) || IsJumpActionState(&mJump, 16)) {
			CVec start = mMoveTranslate;
			CVec arc   = mJump.mArcPosition;
			f32 end    = GetJointAnimationEndFrame(this);
			f32 t      = GetJointAnimationCurrentFrame(this) / end;
			target     = arc + (start - arc) * t + (Vec) { 0, 10, 0 };
		} else if (IsCopterActionState(&mCopter, 7) || IsCopterActionState(&mCopter, 8) || IsCopterActionState(&mCopter, 9)) {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			CVec start = mMoveTranslate;
			start.x    = joint.x;
			start.z    = joint.z;
			target     = start + (Vec) { 0, 10, 0 };
		} else if (anim == 0xcea) {
			s32 path[]  = { 0 };
			CVec root   = GetEventJointWorldPosition(this, 1, path) + (Vec) { 0, 10, 0 };
			CVec middle = GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 })
			            + (GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 3, 0 })
			               - GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 }))
			                  / 2.f;
			f32 end = GetJointAnimationEndFrame(this);
			f32 t   = GetJointAnimationCurrentFrame(this) / end;
			target  = root + (middle - root) * t;
		} else if (anim == 0x43 || anim == 0x10e) {
			s32 path[]  = { 0 };
			CVec root   = GetEventJointWorldPosition(this, 1, path) + (Vec) { 0, 10, 0 };
			CVec middle = GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 })
			            + (GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 3, 0 })
			               - GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 }))
			                  / 2.f;
			f32 end = GetJointAnimationEndFrame(this);
			f32 t   = GetJointAnimationCurrentFrame(this) / end;
			target  = root + (middle - root) * t;
		} else if (anim == 0xed) {
			CVec middle = GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 })
			            + (GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 3, 0 })
			               - GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 }))
			                  / 2.f;
			target = middle;
		} else if (anim == 0xee) {
			s32 path[]  = { 0 };
			CVec root   = GetEventJointWorldPosition(this, 1, path) + (Vec) { 0, 10, 0 };
			CVec middle = GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 })
			            + (GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 3, 0 })
			               - GetEventJointWorldPosition(this, 9, (s32[]) { 0, 1, 0, 1, 2, 0, 0, 1, 0 }))
			                  / 2.f;
			f32 end = GetJointAnimationEndFrame(this);
			f32 t   = GetJointAnimationCurrentFrame(this) / end;
			target  = middle + (root - middle) * t;
		} else {
			s32 path[] = { 0 };
			CVec joint = GetEventJointWorldPosition(this, 1, path);
			target     = joint + (Vec) { 0, 10, 0 };
		}
		CMotionControlCamera& camera = gGame.mMotionControlCamera;
		if (camera.IsAction(1) || camera.IsAction(7)) {
			f32 rate = GetAreaCameraFovRatio(&camera);
			if (rate > 1.f)
				rate = 1.f;
			if (rate < .8f)
				rate = .8f;
			f32 leadX = mHeadOffset * (f32)sin(mMoveRotate.y);
			f32 leadZ = mHeadOffset * (f32)cos(mMoveRotate.y);
			target.x += leadX + (7.f * rate) * (mMoveTranslate.x - _264.x);
			target.z += leadZ + (7.f * rate) * (mMoveTranslate.z - _264.z);
		}
		f32 coord[] = { target.x, target.y, target.z };
		for (s32 i = 0; i < 3; ++i) {
			f32& old = mCameraTarget[i];
			if (ClassifyFloatBits(old) == FP_NAN) {
				old = coord[i];
				continue;
			}
			f32 margin = mCameraTargetMargin;
			if (old > coord[i] + margin)
				old += (coord[i] + margin) - old;
			else if (old < coord[i] - margin)
				old += (coord[i] - margin) - old;
			coord[i] = old;
		}
		return (Vec) { coord[0], coord[1], coord[2] };
	}
	return (Vec) { 0, 0, 0 };
}

// todo: weak copy
/**
 * @note Address: 0x800A2BD0
 * @note Size: 0x64
 */
extern "C" s32 ClassifyFloatBits(f32 value)
{
	s32 bits = *(s32*)&value;
	switch (bits & 0x7F800000) {
	case 0x7F800000:
		return (bits & 0x007FFFFF) ? FP_NAN : FP_INFINITE;
	case 0:
		return (bits & 0x007FFFFF) ? FP_SUBNORMAL : FP_ZERO;
	}
	return FP_NORMAL;
}

// todo: weak copy
/**
 * @note Address: 0x800A2C34
 * @note Size: 0x14
 */
extern "C" BOOL IsCopterActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

// todo: weak copy
/**
 * @note Address: 0x800A2C48
 * @note Size: 0x28
 */
extern "C" CVec __ct__4CVecFRC3Vec(const Vec& value)
{
	return CVec(value);
}

// todo: weak copy
/**
 * @note Address: 0x800A2C70
 * @note Size: 0x14
 */
extern "C" BOOL IsJumpActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

// todo: weak copy
/**
 * @note Address: 0x800A2C84
 * @note Size: 0x14
 */
extern "C" BOOL IsLadderActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

// todo: weak copy
/**
 * @note Address: 0x800A2C98
 * @note Size: 0x1C
 */
extern "C" void fn_800A2C98(Vec* dst, const Vec* src)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

// todo: weak copy
/**
 * @note Address: 0x800A2CB4
 * @note Size: 0x14
 */
extern "C" BOOL IsRopeActionState(const XAction* action, s32 state)
{
	return state == action->mState;
}

// todo: weak copy
/**
 * @note Address: 0x800A2CC8
 * @note Size: 0x1C
 */
extern "C" Vec& __as__3VecFRC3Vec(Vec* dst, const Vec* src)
{
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
	return *dst;
}

// todo: weak copy
/**
 * @note Address: 0x800A2CE4
 * @note Size: 0xA4
 */
extern "C" f32 GetJointAnimationEndFrame(CJObj* object)
{
	HSD_AObj* aobj = GetAObj(object->mJObj);
	return aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
}

// todo: weak copy
/**
 * @note Address: 0x800A2D88
 * @note Size: 0xA4
 */
extern "C" f32 GetJointAnimationCurrentFrame(CJObj* object)
{
	HSD_AObj* aobj = GetAObj(object->mJObj);
	return aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
}

// todo: weak copy
/**
 * @note Address: 0x800A2E2C
 * @note Size: 0x28
 */
extern "C" CVec __ct__4CVecFRC4CVec(const CVec& vector)
{
	return vector;
}

// todo: weak copy
/**
 * @note Address: 0x800A2E54
 * @note Size: 0x34
 */
extern "C" BOOL IsObjectActive(XObj* object)
{
	return (object->mStatus.mFlags & 1ULL) != 0;
}

// hides or shows one joint subtree of chibi robo's model
/**
 * @note Address: 0x800A2E88
 * @note Size: 0x264
 */
extern "C" void SetPlayerPopperMountHidden(XObj* chr, BOOL hidden)
{
	if (chr->mStatus.mFlags & 1ULL) {
		s32 path[5] = { 0, 1, 0, 1, 3 };
		CJObj joint(FindJointByPath(chr->mJObj, path, 5));
		if (hidden)
			HSD_JObjSetFlagsAll(joint.mJObj, 0x10);
		else
			HSD_JObjClearFlagsAll(joint.mJObj, 0x10);
	}
}

/**
 * @note Address: 0x800A30EC
 * @note Size: 0x5FC
 */
extern "C" void SetPlayerEyeCameraJointsHidden(XObj* player, BOOL hidden)
{
	if (player->mStatus.mFlags & 1ULL) {
		s32 pathA[] = { 0, 1, 0, 1, 2 };
		CJObj first(FindJointByPath(player->mJObj, pathA, 5));
		s32 pathB[] = { 0, 1, 0, 1, 0 };
		CJObj second(FindJointByPath(player->mJObj, pathB, 5));
		s32 pathC[] = { 0, 1, 0, 0, 0 };
		CJObj third(FindJointByPath(player->mJObj, pathC, 5));
		if (hidden) {
			HSD_JObjSetFlagsAll(first.mJObj, HIDDEN);
			HSD_JObjSetFlagsAll(second.mJObj, HIDDEN);
			HSD_JObjSetFlagsAll(third.mJObj, HIDDEN);
		} else {
			HSD_JObjClearFlagsAll(first.mJObj, HIDDEN);
			HSD_JObjClearFlagsAll(second.mJObj, HIDDEN);
			HSD_JObjClearFlagsAll(third.mJObj, HIDDEN);
		}
	}
}

// XObjChr::SetAnimRate, 1.135 times faster in lift state 4, and pushed onto the item's joints too
/**
 * @note Address: 0x800A36E8
 * @note Size: 0x1A4
 */
extern "C" void SetAnimRate__7XObjChrFfUl(XObjChr* chr, f32 rate, u32 mask)
{
	f32 multiplier;
	if (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift)) {
		multiplier = 1.135f;
	} else {
		multiplier = 1.0f;
	}
#if defined(VERSION_PAL)
	f32 frameScale = multiplier * (60.0f / (f32)gVideoFramesPerSecond);
#else
	f32 frameScale = multiplier;
#endif
	chr->CXObj::SetAnimRate(rate * frameScale, mask);
	for (s32 i = 0; gPlayerItemAnimationTable[i].mItem >= 0; i++) {
		XObjChrItemAnim* entry = &gPlayerItemAnimationTable[i];
		if (entry->mItem == chr->mSuits.mCurrent) {
			HSD_JObj* joint = GetJointByPath(chr->mJObj, entry->mPath, entry->mDepth);
			if (GetAObj(joint) != NULL)
				chr->Push(joint, 0x200, 1.0f);
		}
	}
}

// XObjChr::GetShapeAnimation, the shape anim for an id, 3000 up from the suit, 2000 up from the attachment,
// 1000 up from XObj's second archive
/**
 * @note Address: 0x800A388C
 * @note Size: 0x254
 */
extern "C" HSD_ShapeAnimJoint* GetShapeAnimation__7XObjChrFl(XObjChr* chr, s32 id)
{
	if (SuitAnimGroup(chr, id)) {
		HSD_ShapeAnimJoint** anims = chr->mSuitAnim.mShapeAnimJoint;
		return anims != NULL ? anims[(id % 3000) % 1000] : NULL;
	}
	if (AtcAnimGroup(chr, id)) {
		HSD_ShapeAnimJoint** anims = chr->mAttachmentAnim.mShapeAnimJoint;
		return anims != NULL ? anims[(id % 2000) % 1000] : NULL;
	}
	if (AnimGroup(chr, id)) {
		HSD_ShapeAnimJoint** anims = chr->mAnim.mShapeAnimJoint;
		return anims != NULL ? anims[(id % 1000) % 1000] : NULL;
	}
	HSD_ShapeAnimJoint** anims = chr->mArchive.mShapeAnimJoint;
	return anims != NULL ? anims[id % 1000] : NULL;
}

// XObjChr::GetMaterialAnimation, the material anim
/**
 * @note Address: 0x800A3AE0
 * @note Size: 0x254
 */
extern "C" HSD_MatAnimJoint* GetMaterialAnimation__7XObjChrFl(XObjChr* chr, s32 id)
{
	if (SuitAnimGroup(chr, id)) {
		HSD_MatAnimJoint** anims = chr->mSuitAnim.mMatAnimJoint;
		return anims != NULL ? anims[(id % 3000) % 1000] : NULL;
	}
	if (AtcAnimGroup(chr, id)) {
		HSD_MatAnimJoint** anims = chr->mAttachmentAnim.mMatAnimJoint;
		return anims != NULL ? anims[(id % 2000) % 1000] : NULL;
	}
	if (AnimGroup(chr, id)) {
		HSD_MatAnimJoint** anims = chr->mAnim.mMatAnimJoint;
		return anims != NULL ? anims[(id % 1000) % 1000] : NULL;
	}
	HSD_MatAnimJoint** anims = chr->mArchive.mMatAnimJoint;
	return anims != NULL ? anims[id % 1000] : NULL;
}

// XObjChr::GetJointAnimation, the joint anim
/**
 * @note Address: 0x800A3D34
 * @note Size: 0x254
 */
extern "C" HSD_AnimJoint* GetJointAnimation__7XObjChrFl(XObjChr* chr, s32 id)
{
	if (SuitAnimGroup(chr, id)) {
		HSD_AnimJoint** anims = chr->mSuitAnim.mAnimJoint;
		return anims != NULL ? anims[(id % 3000) % 1000] : NULL;
	}
	if (AtcAnimGroup(chr, id)) {
		HSD_AnimJoint** anims = chr->mAttachmentAnim.mAnimJoint;
		return anims != NULL ? anims[(id % 2000) % 1000] : NULL;
	}
	if (AnimGroup(chr, id)) {
		HSD_AnimJoint** anims = chr->mAnim.mAnimJoint;
		return anims != NULL ? anims[(id % 1000) % 1000] : NULL;
	}
	HSD_AnimJoint** anims = chr->mArchive.mAnimJoint;
	return anims != NULL ? anims[id % 1000] : NULL;
}

enum PlayerAnimVariant { CarryingAnim, LiftedAnim, AttachmentAnim };

static inline void FindPlayerAnimVariant(s32& id, PlayerAnimVariant variant)
{
	for (s32 i = 0; gPlayerAnimVariants[i].normal >= 0; ++i) {
		if (gPlayerAnimVariants[i].normal != id)
			continue;
		if (variant == CarryingAnim && gPlayerAnimVariants[i].carrying >= 0)
			id = gPlayerAnimVariants[i].carrying;
		else if (variant == AttachmentAnim) {
			if (gPlayerAnimVariants[i].attachment >= 0)
				id = gPlayerAnimVariants[i].attachment;
		} else if (gPlayerAnimVariants[i].lifted >= 0)
			id = gPlayerAnimVariants[i].lifted;
		break;
	}
}

// 1.135 times faster in lift state 4, like SetAnimRate
static inline f32 PlayerAnimRateMultiplier(XObjChr* chr)
{
	if (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift)) {
		return 1.135f;
	}
	return 1.0f;
}

// the anims not given come from the archive
static inline void ApplyPlayerAnimation(XObjChr* chr, CJAnim* archive, s32 localId, s32 id, BOOL force, void* jointAnim, void* matAnim,
                                        void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	void* joint = jointAnim != NULL ? jointAnim : archive->GetAnimJoint(localId);
	void* mat   = matAnim != NULL ? matAnim : archive->GetMatAnimJoint(localId);
	void* shape = shapeAnim != NULL ? shapeAnim : archive->GetShapeAnimJoint(localId);
	chr->XObj::SetAnim(id, force, joint, mat, shape, frame, rate * PlayerAnimRateMultiplier(chr), blend, blendRate);
}

/**
 * @note Address: 0x800A3F88
 * @note Size: 0x5EC
 */
void XObjChr::SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (IsCarryingToothbrush(&mLift) || IsCarryingSpoon(&mLift) || IsCarryingSquirter(&mLift))
		FindPlayerAnimVariant(id, CarryingAnim);
	else if (mLift.mState == 4)
		FindPlayerAnimVariant(id, LiftedAnim);
	else if (mLift.mState == 0 && (GetAttachment() == 1 || GetAttachment() == 3))
		FindPlayerAnimVariant(id, AttachmentAnim);
	if (SuitAnimGroup(this, id) == 1) {
		ApplyPlayerAnimation(this, &mSuitAnim, id % 3000, id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	} else if (AtcAnimGroup(this, id) == 1) {
		ApplyPlayerAnimation(this, &mAttachmentAnim, id % 2000, id, force, jointAnim, matAnim, shapeAnim, frame, rate, blend, blendRate);
	} else {
		XObj::SetAnim(id, force, jointAnim, matAnim, shapeAnim, frame, rate * PlayerAnimRateMultiplier(this), blend, blendRate);
	}
}

// the anim rate SetAnimRate would give him
/**
 * @note Address: 0x800A4574
 * @note Size: 0x44
 */
extern "C" f32 GetPlayerAnimationRateMultiplier(XObjChr* chr)
{
	f32 rate;
	if (chr->mLift.mState == 4 && IsLiftTargetPlug(&chr->mLift)) {
		rate = 1.135f;
	} else {
		rate = 1.0f;
	}
#if defined(VERSION_PAL)
	return rate * (60.0f / (f32)gVideoFramesPerSecond);
#else
	return rate;
#endif
}

// XObjChr::RegisterShadowObjects, his attachments cast shadows too, the plug only under game flag 0x0040000000000000
/**
 * @note Address: 0x800A45B8
 * @note Size: 0xC0
 */
extern "C" void RegisterShadowObjects__7XObjChrFP7CShadow(XObjChr* chr, CShadow* shadow)
{
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mJObj);
	if (gGame.mStatus.mFlags & 0x0040000000000000ULL) {
		HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mPlug.mJObj);
	}
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mCopterObj.mJObj);
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mPopperObj.mJObj);
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mRadarObj.mJObj);
	HSD_ShadowAddObject(shadow->mShadow, (HSD_Obj*)chr->mOXObj.mJObj);
}

// XObjChr::UpdateAnim
/**
 * @note Address: 0x800A54FC
 * @note Size: 0x20
 */
extern "C" void UpdateAnim__7XObjChrFUl(XObjChr* chr, u32 frames)
{
	chr->CXObjIntp::UpdateAnim(frames);
}

extern "C" void ClearActiveObjectFloors(XObj* object);
/**
 * @note Address: 0x800B27E0
 * @note Size: 0xBC
 */
extern "C" Vec GetFloorCenterOffset(XObjFloor* const* floors, s32 index)
{
	Vec value = gPlayerFloorCenterDefault;
	value.x   = (*floors)[index].mCenterOffset[0] / 100.0f;
	value.y   = (*floors)[index].mCenterOffset[1] / 100.0f;
	value.z   = (*floors)[index].mCenterOffset[2] / 100.0f;
	return value;
}

/**
 * @note Address: 0x800B289C
 * @note Size: 0xBC
 */
extern "C" Vec GetFloorExtent(XObjFloor* const* floors, s32 index)
{
	Vec value = gPlayerFloorExtentDefault;
	value.x   = (*floors)[index].mExtent[0] / 100.0f;
	value.y   = (*floors)[index].mExtent[1] / 100.0f;
	value.z   = (*floors)[index].mExtent[2] / 100.0f;
	return value;
}

static inline void SetPlayerCylinderDrawState(CGs* gs)
{
	SetGsChannelEnabled(gs, 0, 1);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 3, 1);
}

static inline void DrawPlayerLiftCylinder(const Vec& pos, const Vec& size, u32 color)
{
	Vec rotation = { HALF_PI, 0.0f, 0.0f };
	DrawGsCylinder(&gGame.mGs, color, 16, pos.x, pos.y, pos.z, rotation.x, rotation.y, rotation.z, size.x, size.y, size.z);
}

/**
 * @note Address: 0x800A4A60
 * @note Size: 0xA9C
 */
extern "C" void vfC4__7XObjChrFv(XObjChr* player)
{
	CGs* gs = &gGame.mGs;
	SetPlayerCylinderDrawState(gs);
	if (player->mLift.mState != 0) {
		if (IsCarryingToothbrush(&player->mLift)) {
			s32 path[] = { 0, 0, 0, 0, 0 };
			Vec pos    = CJObj_GetJointWorldPositionByPath(player->mLift.mTarget, 5, path);
			Vec size   = { 3.0f, 3.0f, 1.0f };
			DrawPlayerLiftCylinder(pos, size, 0x00FFFF80);
		}
		if (IsCarryingSpoon(&player->mLift)) {
			f32 yaw = player->mMoveRotate.y;
			Vec pos = player->mMoveTranslate;
			pos.x += 3.0f * (f32)sin(yaw);
			pos.z += 3.0f * (f32)cos(yaw);
			Vec size = { 1.0f, 1.0f, 1.0f };
			DrawPlayerLiftCylinder(pos, size, 0x00FFFF80);
		}
		if (IsCarryingSquirter(&player->mLift)) {
			f32 yaw = player->mMoveRotate.y;
			Vec pos = player->mMoveTranslate;
			pos.x += 2.0f * (f32)sin(yaw);
			pos.z += 2.0f * (f32)cos(yaw);
			Vec lowerSize = { 2.0f, 2.0f, 1.0f };
			DrawPlayerLiftCylinder(pos, lowerSize, 0x00FFFF80);
			pos.x += 2.0f * (f32)sin(yaw);
			pos.z += 2.0f * (f32)cos(yaw);
			pos.y += 7.0f;
			Vec upperSize = { 2.0f, 2.0f, 3.0f };
			DrawPlayerLiftCylinder(pos, upperSize, 0x00FFFF80);
			if (IsCarryingSquirter(&player->mLift) && player->mAnimId >= 0x8C2 && player->mAnimId <= 0x8C5) {
				XObj* object          = player->mLift.mTarget;
				s32 rootPath[]        = { 0, 0 };
				HSD_JObj* const joint = FindJointByPath(object->mJObj, rootPath, 2);
				Vec origin            = GetJointPosition(joint);
				s32 tipPath[]         = { 0, 3 };
				Vec delta             = CJObj_GetJointWorldPositionByPath(object, 2, tipPath) - origin;
				if (PSVECMag(&delta) > 0.1f) {
					Vec scale;
					HSD_MtxGetScale(HSD_JObjGetMtxPtr(joint), &scale);
					Vec size = { scale.x, scale.z, scale.y * 0.5f };
					DrawPlayerLiftCylinder(origin, size, player->mItemTintWord);
				}
			}
		}
	}
	if (player->mStatus.mFlags & 0x0200000000000000ULL) {
		SetPlayerCylinderDrawState(gs);
		CVec pos(player->mMoveTranslate);
		Vec offset   = player->mMoveScale * GetFloorCenterOffset(&player->mFloors, 0);
		Vec extent   = player->mMoveScale * GetFloorExtent(&player->mFloors, 0);
		f32 floorYaw = DEG2RAD * player->mFloors->mYawDegrees;
		f32 yaw      = player->mMoveRotate.y + floorYaw;
		pos.x        = player->mMoveTranslate.x + 0.5f * extent.x * (f32)sin(player->mMoveRotate.y);
		pos.z        = player->mMoveTranslate.z + 0.5f * extent.z * (f32)cos(player->mMoveRotate.y);

		Mtx translate, shift, turn, tilt, scale, model, view;
		PSMTXTrans(translate, pos.x, pos.y, pos.z);
		PSMTXTrans(shift, offset.x, offset.y, offset.z);
		PSMTXScale(scale, 0.9f * extent.x, extent.y, 0.9f * extent.z);
		PSMTXRotRad(turn, 'y', yaw);
		PSMTXRotRad(tilt, 'x', HALF_PI);
		PSMTXConcat(translate, turn, model);
		PSMTXConcat(model, shift, model);
		PSMTXConcat(model, scale, model);
		PSMTXConcat(model, tilt, model);
		PSMTXConcat(gs->mCamera->mViewMtx, model, view);
		DrawGsCylinderWithMatrix(gs, view, 0x9696FF60, 16);
		vfC4__4XObjFv(player);
		XObj* plug = &player->mPlug;
		if (gGame.mStatus.mFlags & 0x0040000000000000ULL)
			plug->vfC4();
		ClearActiveObjectFloors(plug);
	}
}

extern "C" void DrawPopperShotRings(XActPopper* act);
extern "C" void fn_800BD320(XActRadar* act);
extern "C" void DrawPointParticleDisplay(PointPtcl* points, f32 row);

static inline void DrawPlayerGauge(f32 x, const f32& y, f32 width, f32 fraction, u32 color)
{
	CGs* gs = &gGame.mGs;
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 7, 1);
	SetGsLineWidth(gs->mCurRenderMode, 30, 5);
	DrawGsCircle(gs, 16, color, color, x - 8.0f, y, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 0.0f);
	f32 filled = width * fraction;
	DrawGsLineSegment(gs, NULL, x - 2.0f, y, 0.0f, color, x + filled, y, 0.0f, color);
	DrawGsLineSegment(gs, NULL, (x - 2.0f) + filled, y, 0.0f, 0x64, x + width, y, 0.0f, 0x64);
	SetGsDepthMode(gs->mCurRenderMode, 1, 1, 0);
	SetGsLineWidth(gs->mCurRenderMode, 42, 5);
	DrawGsCircle(gs, 16, 0x96, 0x96, x - 8.0f, y, 0.0f, 18.4f, 18.4f, 0.0f, 0.0f, 0.0f);
	DrawGsLineSegment(gs, NULL, x - 2.0f, y, 0.0f, 0x96, 2.0f + (x + width), y, 0.0f, 0x96);
	SetGsBlendMode(gs->mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs->mCurRenderMode, 1, 3, 0);
	SetGsLineWidth(gs->mCurRenderMode, 12, 5);
}

/**
 * @note Address: 0x800A551C
 * @note Size: 0x73C
 */
extern "C" void vfC8__7XObjChrFv(XObjChr* player)
{
	vfC8__4XObjFv(player);
	DrawPopperShotRings(&player->mPopper);
	fn_800BD320(&player->mRadar);
	if (!(gGame.mStatus.mFlags & 0x100))
		DrawPlayerLookMarker(&player->mLook);

	CMotionControlCamera* camera = &gGame.mMotionControlCamera;
	if (!camera->IsAction(4) && player->_9AC4 > 0.0f) {
		Vec offset = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		player->ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
		x -= 31.0f;
		f32 duration = (f32)(s32)fn_8006E9AC(0) / 100.0f;
		DrawPlayerGauge(x, y, 62.0f, player->_9AC4 / (IDLE_SECOND * duration), 0x00FF0064);
	}
	if (!camera->IsAction(4) && (player->mAnimId == 0x7A || player->mAnimId == 0xCA)) {
		Vec offset = { 0.0f, 0.0f, 0.0f };
		f32 x, y;
		player->ProjectOffsetToScreen(&offset, &x, &y, NULL, NULL);
		x -= 36.0f;
		f32 frame    = GetAnimFrame(player->mJObj);
		f32 fraction = frame / GetAnimEndFrame(player->mJObj);
		u32 color;
		if (fraction <= 0.6f)
			color = 0xFFC80064;
		else
			color = 0xFF000064;
		DrawPlayerGauge(x, y, 72.0f, fraction, color);
	}
	s32 row = 0;
	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticleSlots[i] != NULL) {
			DrawPointParticleDisplay(gPointParticleSlots[i], (f32)row);
			if (gPointParticleSlots[i]->mType != 1)
				++row;
		}
	}
}

static inline s32 ActionTargetPlacement(const XAction& action)
{
	if (action.mTarget != NULL) {
		UnkStageObj* placement = action.mTarget->mCoord._00;
		return placement - GetStagePlacementByIndex(0);
	}
	return -1;
}

/**
 * @note Address: 0x800A4678
 * @note Size: 0x3E8
 */
void XObjChr::DrawDebugStatus()
{
	XObjChr* player = this;
	s32 nearIndex   = player->mFloorContacts[4].objectIndex;
	s32 hitIndex    = player->mHitObjectIndex;
	s32 rideIndex   = player->mFloorContacts[0].objectIndex;
	s32 eventIndex  = player->mFloorContacts[2].objectIndex;
	s32 lift        = ActionTargetPlacement(player->mLift);
	s32 hang        = ActionTargetPlacement(player->mHang);
	s32 jump        = ActionTargetPlacement(player->mJump);
	s32 ladder      = ActionTargetPlacement(player->mLadder);
	s32 rope        = ActionTargetPlacement(player->mRope);
	s32 push        = ActionTargetPlacement(player->mPush);
	f32 floorHeight = NAN;
	u32 flags       = 0;
	s32 floor       = -1;
	if (player->mFloorContacts[2].objectIndex >= 0) {
		flags = (gGame.GetObject(player->mFloorContacts[2].objectIndex)->mStatus.mFlags >> 32) & 0xFF;
	} else if (player->mHitObjectIndex >= 0) {
		XObj* object = gGame.GetObject(player->mHitObjectIndex);
		floorHeight  = object->mSelectedFloorHeight - player->mMoveTranslate.y;
		floor        = object->mSelectedFloorIndex;
		flags        = (object->mStatus.mFlags >> 32) & 0xFF;
	}
	s32 anim       = player->mAnimId;
	HSD_AObj* aobj = GetAObj(player->mJObj);
	f32 endFrame   = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	DrawGsStrokeTextFormat(&gGame.mGs, -56,
	                       "\\B00000080POS(%6.2f,%6.2f,%6.2f) %3d %6.3f %6.3f\n%3d(%6.2f/%6.2f), %d\n\n"
	                       "NEAR:%3d(%6.2f)\nHIT :%3d(%02X)\n    :%3d(%.2f)\nRIDE:%3d\nEVT :%3d\n\nACT :%2d\n"
	                       "LIFT:%2d(%d)\nHANG:%2d(%d)\nJUMP:%2d(%d)\nLADR:%2d(%d)\nROPE:%2d(%d)\nPUSH:%2d(%d)\n"
	                       "TMBL:%2d\nCOPT:%2d\nPOPP:%2d\nRADR:%2d\n",
	                       16.0f, 30.0f, 0.0f, 11.0f, 12.0f, player->mMoveTranslate.x, player->mMoveTranslate.y, player->mMoveTranslate.z,
	                       (s32)(RAD2DEG * player->mMoveRotate.y), player->_9B8, player->mMoveTranslate.mSpeed.y, anim,
	                       player->mFrameTime.mFrame, endFrame, player->_A2C, nearIndex, player->mNearDistance, hitIndex, flags, floor,
	                       floorHeight, rideIndex, eventIndex, player->mAttachment, player->mLift.mState, lift, player->mHang.mState, hang,
	                       player->mJump.mState, jump, player->mLadder.mState, ladder, player->mRope.mState, rope, player->mPush.mState,
	                       push, player->mTumble.mState, player->mCopter.mState, player->mPopper.mState, player->mRadar.mState);
}

extern "C" {
void DrawRadarTargetBeam(XActRadar*);
void DrawPopperShotParticles(XActPopper*);
void DrawTumbleRing(XActTumbleFade*, const Vec&, f32);
void PSMTXMultVecArray(const Mtx, const Vec*, Vec*, u32);
}

static inline u16 ShadeBatteryWarning(u32* colors, u32 color, f32 alpha, const XObjChr* player)
{
	u16 n = 0;
	for (int turn = 0; turn < 4; ++turn)
		for (int step = 0; step < 16; ++step) {
			colors[n]     = color | (u32)(alpha * ((1.f / 128) * n) * player->mBatteryWarningFade);
			colors[n + 1] = color;
			n += 2;
		}
	for (int turn = 0; turn < 4; ++turn)
		for (int step = 0; step < 16; ++step) {
			colors[n]     = color | (u32)(alpha * (1.f - (1.f / 128) * (turn * 32 + step)) * player->mBatteryWarningFade);
			colors[n + 1] = color;
			n += 2;
		}
	return n;
}
static inline Vec WarningJointPosition(HSD_JObj* root, s32* path, s32 depth)
{
	Vec position;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(FindJointByPath(root, path, depth)), &position);
	return position;
}

/**
 * @note Address: 0x800A5C58
 * @note Size: 0x16A0
 */
extern "C" void DrawObjectEffects__7XObjChrFv(XObjChr* player)
{
	if (gGame.mStatus.mFlags & 0x0040000000000000ULL)
		player->mPlug.DrawObjectEffects();
	player->mCopterObj.DrawObjectEffects();
	player->mPopperObj.DrawObjectEffects();
	player->mRadarObj.DrawObjectEffects();
	player->mOXObj.DrawObjectEffects();
	if (player->mBatteryWarningFade > 0) {
		f32 radius     = 1.2f * PlayerCollisionRadius(player);
		s32 fromPath[] = { 0, 1, 0, 0, 0 };
		CVec from(WarningJointPosition(player->mJObj, fromPath, 5));
		s32 toPath[] = { 0, 1, 0, 1, 2, 0, 0, 4 };
		CVec to(WarningJointPosition(player->mJObj, toPath, 8));
		if (player->mLift.mState && IsCarryingMug(&player->mLift)) {
			from.x = player->mLift.mTarget->mMoveTranslate.x;
			from.z = player->mLift.mTarget->mMoveTranslate.z;
			Vec up = { 0, 1, 0 };
			to     = from + up;
		}
		f32 yaw = atan2(to.x - from.x, to.z - from.z);
		f32 dz = to.z - from.z, dx = to.x - from.x, dy = to.y - from.y;
		f32 pitch = atan2(sqrtf(dx * dx + dz * dz), sqrtf(dy * dy));
		pitch     = dy < 0 ? HALF_PI - pitch : pitch - HALF_PI;
		Mtx trans, scale, rotY, rotX, model;
		PSMTXTrans(trans, from.x, from.y, from.z);
		PSMTXScale(scale, 1, 1, 1);
		PSMTXRotRad(rotY, 'y', yaw);
		PSMTXRotRad(rotX, 'x', pitch);
		PSMTXConcat(trans, rotY, model);
		PSMTXConcat(model, rotX, model);
		PSMTXConcat(model, scale, model);
		Vec vertices[256];
		u32 colors[256];
		f32 z = -3, phase = player->mBatteryWarningAngle;
		u16 n = 0;
		for (int turn = 0; turn < 8; ++turn)
			for (int step = 0; step < 16; ++step) {
				f32 a    = (TWO_PI / 16) * step;
				Vec& v   = vertices[n];
				v.x      = (2.f * (1.f - player->mBatteryWarningFade) + radius) * (f32)sin(a);
				v.y      = (2.f * (1.f - player->mBatteryWarningFade) + radius) * (f32)cos(a);
				v.z      = z;
				f32 wave = WrapAngle(phase);
				if (wave > PI)
					wave = 0;
				vertices[n + 1].x = v.x;
				vertices[n + 1].y = v.y;
				vertices[n + 1].z = v.z - 1.5f * (f32)sin(wave);
				z += .08f;
				phase += DEG2RAD * 30;
				n += 2;
			}
		PSMTXMultVecArray(model, vertices, vertices, n);
		CGs& gs = gGame.mGs;
		n       = ShadeBatteryWarning(colors, 0xff000000, 200, player);
		SetGsCullMode(gs.mCurRenderMode, 0);
		SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
		DrawGsColoredTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, colors, n);
		n = ShadeBatteryWarning(colors, 0xffffff00, 255, player);
		SetGsBlendMode(gs.mCurRenderMode, 1, 2, 4, 15);
		DrawGsColoredTrianglePrimitive(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, colors, n);
		SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
		SetGsCullMode(gs.mCurRenderMode, 2);
	}
	Vec ground = { player->mMoveTranslate.x, player->mFloorHit.mHeight, player->mMoveTranslate.z };
	DrawTumbleRing(&player->mTumble.mFade, ground, player->mMoveRotate.y);
	DrawRadarTargetBeam(&player->mRadar);
	DrawPopperShotParticles(&player->mPopper);
	DrawParticlePool(&player->mDust.mPtcl);
	DrawSpiralTrail(&gPlayerSuitChangeEffect, player->mMoveTranslate);
}

/**
 * @note Address: 0x800A72F8
 * @note Size: 0x4DC
 */
extern "C" void DrawGroundShadowAt__7XObjChrFPC3Vecf(XObjChr* player, const Vec*, f32 opacity)
{
	if (gGame.mMotionControlCamera.GetMapDistance() < 1.0f)
		return;
	if (player->mStatus.mFlags & 4) {
		if (gGame.mStatus.mFlags & 0x0040000000000000ULL)
			player->mPlug.DrawGroundShadow();
		return;
	}
	s32 rootPath[] = { 0 };
	Vec position   = GetEventJointWorldPosition(player, 1, rootPath);
	if (!player->mLift.mState || !IsCarryingMug(&player->mLift)) {
		DrawGroundShadowAt__4XObjFPC3Vecf(player, &position, opacity);
		return;
	}

	s32 path[] = { 0, 0 };
	CVec jointScale;
	HSD_JObjGetScale(FindJointByPath(player->mJObj, path, 2), &jointScale);
	Vec rotation   = { -HALF_PI, 0.0f, 0.0f };
	u32 shadowSize = player->mAttr._00 != NULL ? player->mAttr._00->_0C : 0;
	Vec size       = (player->mMoveScale * (f32)shadowSize) + ((jointScale * 2.5f) * 3.0f);
	CGs& gs        = gGame.mGs;
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	u32 edge   = (u32)(128.0f * opacity);
	u32 center = (u32)(160.0f * opacity);
	DrawGsCircle(&gs, 12, center, edge, position.x, position.y, position.z, size.x, size.z, rotation.x, rotation.y, rotation.z);
	PrimVertex vertices[26];
	f32 halfWidth = size.x * 0.5f;
	f32 halfDepth = size.z * 0.5f;
	for (s32 i = 0; i < 13; i++) {
		f32 angle     = (PI / 6.0f) * i;
		PrimVertex* v = &vertices[i * 2];
		v[0].pos.x    = position.x - halfWidth * (f32)sin(angle);
		v[0].pos.z    = position.z + halfDepth * (f32)cos(angle);
		v[0].pos.y    = position.y;
		v[0].color    = edge;
		v[1].pos.x    = position.x - (1.5f + halfWidth) * (f32)sin(angle);
		v[1].pos.z    = position.z + (1.5f + halfDepth) * (f32)cos(angle);
		v[1].pos.y    = position.y;
		v[1].color    = 0;
	}
	DrawGsTriangleVertices(&gs, GX_TRIANGLESTRIP, gs.mCamera->mViewMtx, vertices, 26);
}

// XObjChr::DrawGroundShadow, nothing up close to the camera
/**
 * @note Address: 0x800A77D4
 * @note Size: 0xC0
 */
extern "C" void DrawGroundShadow__7XObjChrFv(XObjChr* player)
{
	CMotionControlCamera* camera = &gGame.mMotionControlCamera;
	if (camera->GetMapDistance() < 1.0f)
		return;
	if (player->mStatus.mFlags & 4ULL) {
		if (gGame.mStatus.mFlags & 0x0040000000000000ULL) {
			XObj* plug = &player->mPlug;
			plug->DrawGroundShadow();
		}
	} else {
		DrawGroundShadow__4XObjFv(player);
	}
}

// XObjChr::DrawReflection
/**
 * @note Address: 0x800A7894
 * @note Size: 0x198
 */
extern "C" void DrawReflection__7XObjChrFPA4_f(XObjChr* player, Mtx view)
{
	CMotionControlCamera* camera = &gGame.mMotionControlCamera;
	if (camera->GetMapDistance() < 1.0f)
		return;
	if (gGame.mStatus.mFlags & 0x100ULL)
		SetPlayerEyeCameraJointsHidden(player, 0);
	player->DrawWithView(view);
	if (gGame.mStatus.mFlags & 0x100ULL)
		SetPlayerEyeCameraJointsHidden(player, 1);
	if (gGame.mStatus.mFlags & 0x0040000000000000ULL)
		player->mPlug.DrawReflection(view);
	player->mCopterObj.DrawReflection(view);
	player->mPopperObj.DrawReflection(view);
	player->mRadarObj.DrawReflection(view);
	player->mOXObj.DrawReflection(view);
}

// XObjChr::DrawWithView, himself then the attachments, the popper turned to face the camera on flag 0x100
/**
 * @note Address: 0x800A7A2C
 * @note Size: 0x218
 */
extern "C" void DrawWithView__7XObjChrFPA4_f(XObjChr* player, Mtx view)
{
	CMotionControlCamera* camera = &gGame.mMotionControlCamera;
	if (camera->GetMapDistance() < 1.0f)
		return;
	DrawWithView__4XObjFPA4_f(player, view);
	if ((gGame.mStatus.mFlags & 0x100ULL) && player->mLift.mState == 4 && IsLiftTargetPlug(&player->mLift)) {
		// no plug in this lift state
	} else if (gGame.mStatus.mFlags & 0x0040000000000000ULL) {
		player->mPlug.DrawWithView(view);
	}
	player->mCopterObj.DrawWithView(view);
	if (gGame.mStatus.mFlags & 0x100ULL) {
		Mtx rotation, combined;
		PSMTXRotRad(rotation, 'x', -camera->mCamera.mAngle.x);
		if (view != NULL) {
			PSMTXConcat(rotation, view, combined);
		} else {
			CCamera* current = ((CBase*)&gGame)->GetCamera();
			PSMTXConcat(rotation, current->mViewMtx, combined);
		}
		player->mPopperObj.DrawWithView(combined);
	} else {
		player->mPopperObj.DrawWithView(view);
	}
	player->mRadarObj.DrawWithView(view);
	player->mOXObj.DrawWithView(view);
}

// XObjAttachment::DrawWithView
/**
 * @note Address: 0x800A7C44
 * @note Size: 0x2C
 */
extern "C" void DrawWithView__14XObjAttachmentFPA4_f(XObjAttachment* object, Mtx view)
{
	if (object->mVisible != 0)
		DrawWithView__4XObjFPA4_f(object, view);
}

// steps the anim frame and fires the particles the frame tables ask for
/**
 * @note Address: 0x800A7C70
 * @note Size: 0x1AC
 */
extern "C" void UpdatePlayerFrameEffects(XObjChr* player)
{
	player->mFrameTime.mLastFrame = player->mFrameTime.mFrame;
	HSD_AObj* aobj                = GetAObj(player->mJObj);
	player->mFrameTime.mFrame     = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
	PlayerFrameParticle particle  = ProcessPlayerFrameEvents(player, &player->mFrameTime, gPlayerAnimationFrameEvents);
	if (particle.mCode >= 0 && !(gGame.mStatus.mFlags & 0x100ULL)) {
		(&gGame.mParticle)->CreateSRT((u32)particle.mCode | 0x10000, 1, particle.mTranslate, particle.mScale, particle.mRotate, FALSE);
	}
	if (player->mCopterObj.mVisible != 0)
		ProcessPlayerFrameEvents(&player->mCopterObj, NULL, gCopterAnimationFrameEvents);
}

/**
 * @note Address: 0x800A7E1C
 * @note Size: 0x638
 */
static inline void SetFrameParticleAtJoint(PlayerFrameParticle& particle, XObjChr* player, s32 code)
{
	particle.mCode      = code;
	s32 path[]          = { 0, 1, 0, 1, 2, 0, 1 };
	Vec offset          = { 0.0f, 2.0f, 0.0f };
	particle.mTranslate = GetEventJointWorldPosition(player, 7, path) + offset;
	particle.mScale     = player->mMoveScale;
	particle.mRotate    = player->mMoveRotate;
}

extern "C" PlayerFrameParticle ProcessPlayerFrameEvents(XObj* object, const FrameEventTime* frames, const FrameEvent* table)
{
	PlayerFrameParticle particle = { -1 };
	HSD_AObj* aobj               = GetAObj(object->mJObj);
	if (aobj == NULL)
		return particle;
	s32 anim = object->mAnimId;
	FrameEventTime time;
	if (frames != NULL) {
		time = *frames;
	} else {
		time.mFrame     = HSD_AObjGetCurrentFrame(aobj);
		time.mLastFrame = time.mFrame - 1.0f;
	}
	for (const FrameEvent* event = table; event->mAnim >= 0; ++event) {
		s32 result = 0;
		if (anim == event->mAnim && event->mWhenAnimMatches != NULL)
			result = event->mWhenAnimMatches(&aobj->flags, &time, event);
		else if (anim != event->mAnim && event->mWhenAnimDiffers != NULL)
			result = event->mWhenAnimDiffers(&aobj->flags, &time, event);
		if (result > 0) {
			XObjChr* player = gGame.GetPlayer();
			if (event->mId == 7 && anim == 7 && player->mLift.mTarget != NULL) {
				XObj* held = player->mLift.mTarget;
				if ((s16)held->GetPool() == 8) {
					s8 material = GetItemSoundIndex((s16)held->GetItem(), 0);
					if (material >= 0)
						PlayGameSound(&gGame.mSound, GetMaterialSoundId(material), event->mVolume);
				} else {
					PlayGameSound(&gGame.mSound, GetMaterialSoundId(held->mFloors->mType >> 8), event->mVolume);
				}
			} else {
				PlayGameSound(&gGame.mSound, event->mId, event->mVolume);
			}
			if (event->mId == 5) {
				SetFrameParticleAtJoint(particle, player, 0x18);
			} else if (event->mId == 6) {
				SetFrameParticleAtJoint(particle, player, 0x19);
			} else if (event->mId == 0x38 && anim == 0x9F) {
				const Vec& tint = player->mColorStep[0].mValue;
				u32 color       = ((u32)tint.x << 24) | ((u32)tint.y << 16) | ((u32)tint.z << 8);
				if (color < 0xFFFFFF00) {
					particle.mCode      = 0x55;
					s32 path[]          = { 0, 0 };
					particle.mTranslate = GetEventJointWorldPosition(player, 2, path);
					particle.mTranslate.x += 2.4f * (f32)sin(player->mMoveRotate.y);
					particle.mTranslate.z += 2.4f * (f32)cos(player->mMoveRotate.y);
					particle.mTranslate.y += 0.5f;
					particle.mScale  = player->mMoveScale;
					particle.mRotate = player->mMoveRotate;
				}
			}
		} else if (result < 0) {
			StopGameSfx(&gGame.mSound, event->mId);
		}
	}
	return particle;
}

// stop once the sound is playing
/**
 * @note Address: 0x800A8454
 * @note Size: 0x40
 */
extern "C" s32 FrameSoundStopIfPlaying(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	return IsGameSoundPlaying(&gGame.mSound, event->mId) ? -1 : 0;
}

// start while the sound isn't playing
/**
 * @note Address: 0x800A8494
 * @note Size: 0x3C
 */
extern "C" s32 FrameSoundStartIfSilent(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	return IsGameSoundPlaying(&gGame.mSound, event->mId) ? FALSE : TRUE;
}

// keep the sound playing inside the window only
/**
 * @note Address: 0x800A84D0
 * @note Size: 0x90
 */
extern "C" s32 FrameSoundInFrameWindow(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (time->mFrame >= event->mStart && time->mFrame < event->mEnd) {
		if (!IsGameSoundPlaying(&gGame.mSound, event->mId))
			return 1;
	} else {
		if (IsGameSoundPlaying(&gGame.mSound, event->mId))
			return -1;
	}
	return 0;
}

// keep the sound playing until the anim ends
/**
 * @note Address: 0x800A8560
 * @note Size: 0x7C
 */
extern "C" s32 FrameSoundUntilAnimEnd(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (*aobjFlags & 0x44000000) {
		if (IsGameSoundPlaying(&gGame.mSound, event->mId))
			return -1;
	} else {
		if (!IsGameSoundPlaying(&gGame.mSound, event->mId))
			return 1;
	}
	return 0;
}

// fires on the frame it crosses mStart or mEnd, a loop back counts as crossing
/**
 * @note Address: 0x800A85DC
 * @note Size: 0x64
 */
extern "C" s32 FrameSoundOnEitherBoundary(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if (((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
	    || ((time->mLastFrame < event->mEnd || time->mLastFrame > time->mFrame) && time->mFrame >= event->mEnd))
		return 1;
	return 0;
}

// fires on the frame it crosses mStart
/**
 * @note Address: 0x800A8640
 * @note Size: 0x3C
 */
extern "C" s32 FrameSoundOnStartFrame(const u32* aobjFlags, FrameEventTime* time, const FrameEvent* event)
{
	if ((time->mLastFrame < event->mStart || time->mLastFrame > time->mFrame) && time->mFrame >= event->mStart)
		return 1;
	return 0;
}

extern "C" {
void ResetPlayerAttachment(XObjChr*, s32);
void SetAreaCameraInterestTarget(CCameraActionArea*, s32, XObj*, f32);
void SelectCurrentCameraAreaByViewDirection(CCameraActionArea*, s32, f32);
}

static inline bool InAnimRange(s32 anim, s32 first, s32 last)
{
	return (u32)anim - (u32)first <= (u32)(last - first);
}

static inline void StopBatterySound(s32 id)
{
	if (IsGameSoundPlaying(&gGame.mSound, id))
		StopGameSfx(&gGame.mSound, id);
}

static inline void FlashBatteryWarning(XObjChr* p, u8 green, f32 duration)
{
	p->vf12C(1, 255, green, 0, 0.0f);
	p->vf12C(1, 0, 0, 0, duration);
	StartScreenFade(&gGame.mScrnTex.mColor, 255, green, 0, 0, 100, 100, 200, 0, duration);
}

/**
 * @note Address: 0x800A867C
 * @note Size: 0xF14
 */
extern "C" void UpdatePlayerBattery(XObjChr* p, u32 frames)
{
	s32 index = -1;
	s32 lift  = p->mLift.mState;
	if (lift && !p->mJump.mState) {
		switch (lift) {
		case 1:
		case 2:
		case 20:
			index = 7;
			break;
		case 6:
		case 19:
			index = 8;
			break;
		case 3:
		case 4:
			switch (p->mAnimId) {
			case 0x89:
			case 0xE0:
				index = 5;
				break;
			case 0x896:
			case 0xC:
				index = 2;
				break;
			case 0x897:
			case 0x88:
				index = 3;
				break;
			case 0x898:
			case 0xD:
				index = 4;
				break;
			case 0x895:
				index = 34;
				break;
			case 0x8B7:
			case 0x8AE:
				index = 35;
				break;
			case 0x8B1:
				index = 36;
				break;
			default:
				index = 0;
				break;
			}
			break;
		case 13:
			index = 9;
			break;
		case 27:
		case 28:
			if (p->mAnimId == 0x895)
				index = 34;
			break;
		case 29:
		case 30:
			if (p->mAnimId == 0x8B7)
				index = 35;
			break;
		case 31:
		case 32:
			if (p->mAnimId == 0x8B1)
				index = 36;
			break;
		case 35:
		case 36:
			switch (p->mAnimId) {
			case 0x8C0:
			case 0x8C1:
				index = 37;
				break;
			case 0x8C2:
			case 0x8C3:
			case 0x8C4:
			case 0x8C5:
				index = 38;
				break;
			}
			break;
		}
	} else if (p->mHang.mState) {
		switch (p->mHang.mState) {
		case 1:
		case 2:
			index = 10;
			break;
		case 4:
			index = 14;
			break;
		case 5:
			if (p->mHang._0C == -1 || p->mHang._0C == 1)
				index = 11;
			else
				index = 0;
			break;
		case 6:
		case 7:
			index = 12;
			break;
		case 9:
			index = 13;
			break;
		}
	} else if (p->mJump.mState) {
		switch (p->mJump.mState) {
		case 8:
		case 9:
		case 11:
		case 12:
		case 14:
		case 15:
			index = 15;
			break;
		case 6:
			index = 16;
			break;
		}
	} else if (p->mLadder.mState) {
		switch (p->mLadder.mState) {
		case 1:
		case 2:
			index = 17;
			break;
		case 5:
			if (p->mLadder._0C == 1)
				index = 18;
			else if (p->mLadder._0C == -1)
				index = 19;
			else
				index = 0;
			break;
		case 6:
			index = 20;
			break;
		case 7:
		case 9:
			index = 21;
			break;
		}
	} else if (p->mRope.mState) {
		switch (p->mRope.mState) {
		case 1:
		case 2:
			index = 22;
			break;
		case 5:
			if (p->mRope._0C == 1 || p->mRope._0C == 2)
				index = 23;
			else if (p->mRope._0C == -1)
				index = 24;
			else
				index = 0;
			break;
		case 6:
			index = 25;
			break;
		case 8:
			index = 26;
			break;
		}
	} else if (p->mPush.mState) {
		if (p->mPush.mState == 2) {
			if (p->mPush._0C == 1 || p->mPush._0C == -1)
				index = 27;
			else
				index = 0;
		}
	} else if (p->mCopter.mState) {
		switch (p->mCopter.mState) {
		case 1:
		case 2:
			index = 6;
			break;
		case 3:
			index = 28;
			break;
		case 4:
			index = 29;
			break;
		}
	} else {
		if (p->mPopper.mState != 0 && p->mPopper.mState != 4) {
			switch (p->mPopper.mState) {
			case 1:
			case 2:
			case 9:
			case 10:
				index = 6;
				break;
			case 3:
				index = 0;
				break;
			case 6:
			case 7:
			case 8:
				if (p->mPopper._2C != 0.0f)
					index = 30;
				break;
			default:
				break;
			}
		} else if (p->mRadar.mState != 0) {
			switch (p->mRadar.mState) {
			case 1:
			case 2:
				index = 6;
				break;
			case 3:
			case 4:
			case 5:
				index = 32;
				break;
			default:
				if (p->mRadar.mTarget)
					index = 33;
				break;
			}
		} else if (p->mRadar.mTarget) {
			index = 33;
		} else {
			switch (p->mAnimId) {
			case 0x79:
				index = 5;
				break;
			case 3:
			case 0xA3:
			case 0x818:
			case 0x874:
				index = 2;
				break;
			case 0x77:
			case 0x882:
				index = 3;
				break;
			case 4:
			case 0x819:
				index = 4;
				break;
			case 1:
			case 0x11:
			case 0x4A:
			case 0xA1:
			case 0x822:
				index = 0;
				break;
			case 2:
			case 0x12:
			case 0xCF7:
			case 0xCF8:
			case 0xCF9:
			case 0xCFA:
			case 0xCFB:
			case 0xCFC:
			case 0xCFD:
				index = 1;
				break;
			}
		}
	}
	f32 drain = 0.0f;
	if (index >= 0) {
		if (index == 9)
			drain = (f32)((u32)IDLE_SECOND / frames) * p->mLift.mBatteryCost;
		else
			drain = (f32)(s32)GetPlayerActionParameter(index) / 100.0f;
	}
	Params& stats = p->mParams;
	if ((s32)stats._1C[0] > 0) {
		drain = 0.0f;
		stats.SetBattery(stats.mMaxBattery);
	} else if (p->mPlug._D3C > 0 && (u8)p->mPlug._D3C) {
		drain = 0.0f;
		stats.SetBattery(stats.mMaxBattery);
	} else if (gGame.mEvt.mData.mScript || (gGame.mStatus.mFlags & 0xC00)) {
		drain = 0.0f;
	} else if ((gGame.mStatus.mFlags & 0x80000000) && !(gGame.mStatus.mFlags & 0x120) && p->mPad.mSubStickY != 0.0f) {
		drain = 0.0f;
	}
	if (drain > 0.0f)
		drain /= (f32)IDLE_SECOND;
	if (!gGame.mEvt.mData.mScript
	    && (p->mAttachmentRequest >= 0 || InAnimRange(p->mAnimId, 1, 2) || InAnimRange(p->mAnimId, 0x11, 0x12)
	        || InAnimRange(p->mAnimId, 0xCF7, 0xCFD) || InAnimRange(p->mAnimId, 5, 11) || InAnimRange(p->mAnimId, 0x894, 0x895)
	        || InAnimRange(p->mAnimId, 0x8AE, 0x8AF) || InAnimRange(p->mAnimId, 0x8B1, 0x8B3) || p->mAnimId == 0x14
	        || InAnimRange(p->mAnimId, 0x67, 0x68) || p->mAnimId == 0x8A || p->mAnimId == 0x8C || p->mAnimId == 0x3F || p->mAnimId == 0x4A
	        || p->mAnimId == 0xA1 || p->mAnimId == 0x822 || InAnimRange(p->mAnimId, 0x5D, 0x5E) || p->mAnimId == 0x5F)) {
		if (p->_A30 > 0.0f) {
			p->_A30 -= (f32)frames;
			if (p->_A30 < 0.0f)
				p->_A30 = 0.0f;
		}
	} else {
		p->_A30 = (f32)IDLE_SECOND;
	}
	if (p->_158A4) {
		f32 battery;
		if ((p->mTumble.mState == 8 || p->mTumble.mState == 9) && p->_A3C > 0.0f) {
			battery = stats.mBattery - p->_A3C;
			p->_A3C = 0.0f;
		} else {
			battery = stats.mBattery - drain * (f32)frames;
		}
		if (battery < 0.0f)
			battery = 0.0f;
		if (battery != stats.mBattery)
			stats.SetBattery(battery);
		if (stats.mBattery <= 0.0f && InfoScrn_IsBatteryGaugeEmpty(&gGame.mInfoScrn) && !gGame.mEvt.mData.mScript
		    && (gGame.mStatus.mFlags & 0x40000000) && GetStageBatteryDepletedScript() && p->mAttachmentRequest < 0
		    && (p->mLift.mState == 0 || p->mLift.mState == 4) && !p->mCopter.mState && p->mTumble.mState != 4) {
			ResetPlayerAttachment(p, -1);
			p->mTumble.mState = 4;
			FlashBatteryWarning(p, 0, 0.4f);
			ClearPlugState(&p->mPlug, TRUE);
			gGame.mMotionControlCamera.EndEye();
			CCameraActionArea* area = (CCameraActionArea*)gGame.mCameraActions.mTable[1];
			SetAreaCameraInterestTarget(area, 4, p, 0.5f);
			SelectCurrentCameraAreaByViewDirection((CCameraActionArea*)gGame.mCameraActions.mTable[1], 1, 1.0f);
			gGame.mSubwin.mEnabled = 0;
			gGame.mParticle.Kill(0x1D);
			StopBatterySound(8);
			StopBatterySound(0x186);
			StopBatterySound(0x18B);
			StopBatterySound(0x18C);
			StopBatterySound(0x18D);
			StopBatterySound(0x18E);
		}
	}
	if (stats.mBattery > 0.0f && stats.mBattery <= 30.0f) {
		p->mBatteryWarningFrames += frames;
		const Vec& color = p->mColorStep[1].mSpeed;
		if (color.x == 0.0f && color.y == 0.0f && color.z == 0.0f && p->mBatteryWarningFrames >= IDLE_SECOND && !gGame.mEvt.mData.mScript) {
			FlashBatteryWarning(p, 100, 1.0f);
			p->mBatteryWarningFrames = 0;
			p->mBatteryWarningFade   = 1.0f;
			PlayGameSound(&gGame.mSound, 0xB1, 255);
		}
	} else if (p->mBatteryWarningFrames > 0) {
		p->mBatteryWarningFrames = 0;
		p->mBatteryWarningFade   = 0.0f;
	}
	if (p->mBatteryWarningFade > 0.0f) {
		p->mBatteryWarningFade -= 0.02f * (f32)frames;
		if (p->mBatteryWarningFade < 0.0f)
			p->mBatteryWarningFade = 0.0f;
		p->mBatteryWarningAngle -= DEG2RAD * 5.0f * (f32)frames;
	}
}

static inline bool CanLookAtObjects(XObjChr* player)
{
	if (gGame.mEvt.mData.mScript != NULL || (gGame.mStatus.mFlags & 0x100) || player->mFloorContacts[4].objectIndex >= 0)
		return false;
	if (player->GetAttachment() == 1 || player->GetAttachment() == 2 || player->GetAttachment() == 3)
		return false;
	return (player->mLift.mState == 0 || player->mLift.mState == 4) && (player->mHang.mState == 0 || player->mHang.mState == 5)
	    && player->mJump.mState == 0 && (player->mLadder.mState == 0 || player->mLadder.mState == 5)
	    && (player->mRope.mState == 0 || player->mRope.mState == 5) && player->mPush.mState == 0 && player->mTumble.mState == 0
	    && player->mCopter.mState == 0 && player->mPopper.mState == 0 && player->mRadar.mState == 0;
}

static inline bool IsActiveActionTarget(const XAction& action, const XObj* object)
{
	return action.mState != 0 && action.mTarget == object;
}

/**
 * @note Address: 0x800A9590
 * @note Size: 0x82C
 */
extern "C" void UpdatePlayerLookAtObjects(XObjChr* player, u32 frames)
{
	bool enabled = CanLookAtObjects(player);
	CVec pos(player->mMoveTranslate);
	s32 path[] = { 0, 1, 0, 1, 2 };
	CVec head  = GetEventJointWorldPosition(player, 5, path);
	XObj* best = NULL;
	if (enabled) {
		f32 nearest      = NAN;
		s16 bestPriority = 0;
		for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
			XObj* obj = gGame.GetObject(i);
			if (!(obj->mStatus.mFlags & 1) || !(obj->mStatus.mFlags & 0x10000) || (obj->mStatus.mFlags & 0x4000000000000000ULL)
			    || obj->mId == player->mId || obj->GetKind() >= 10000)
				continue;
			if (IsActiveActionTarget(player->mLift, obj) || IsActiveActionTarget(player->mHang, obj)
			    || IsActiveActionTarget(player->mJump, obj) || IsActiveActionTarget(player->mLadder, obj)
			    || IsActiveActionTarget(player->mRope, obj) || IsActiveActionTarget(player->mPush, obj)
			    || IsActiveActionTarget(player->mTumble, obj) || IsActiveActionTarget(player->mCopter, obj)
			    || IsActiveActionTarget(player->mPopper, obj) || IsActiveActionTarget(player->mRadar, obj))
				continue;
			s16 priority = obj->mAttr._00 != NULL ? obj->mAttr._00->mLookPriority : 0;
			if (priority == 0)
				continue;
			Vec target = obj->mMoveTranslate;
			if (!(target.y > pos.y - 4.0f && target.y < 4.0f + head.y))
				continue;
			f32 dx       = pos.x - target.x;
			f32 dz       = pos.z - target.z;
			f32 distance = sqrtf(dx * dx + dz * dz);
			s16 range    = obj->mAttr._00 != NULL ? obj->mAttr._00->mLookRange : 0;
			if (!(distance < range))
				continue;
			f32 yaw = WrapAngle((f32)atan2(target.x - pos.x, target.z - pos.z) - player->mMoveRotate.y);
			if (yaw <= HALF_PI || yaw >= 3.0f * HALF_PI) {
				BOOL select = FALSE;
				if (best == NULL)
					select = TRUE;
				else if (priority > bestPriority)
					select = TRUE;
				else if (priority == bestPriority && distance < nearest)
					select = TRUE;
				if (select) {
					nearest      = distance;
					best         = obj;
					bestPriority = priority;
				}
			}
		}
	}
	if (best != NULL) {
		f32 yaw = atan2(best->mMoveTranslate.x - pos.x, best->mMoveTranslate.z - pos.z);
		TurnJoint(yaw - player->mMoveRotate.y, &player->mJointRot);
	} else if (player->mJointRot.mMode != 0 && player->mJointRot.mMode != 2) {
		player->mJointRot.mMode   = 2;
		player->mJointRot.mTarget = 0.0f;
		player->mJointRot.mLookAt = NULL;
	}
	player->mJointRot.Update(frames);
}

// loops the four idle joints' own anims, restarting them together after a random pause
/**
 * @note Address: 0x800A9DBC
 * @note Size: 0x2B4
 */
extern "C" void UpdatePlayerIdleJointAnimations(XObjChr* chr, u32 frames)
{
	XObjChrIdleJoint* joints = chr->_9500;
	if (joints[0].mJObj != NULL && joints[1].mJObj != NULL && joints[2].mJObj != NULL && joints[3].mJObj != NULL) {
		if (chr->_9530 > 0) {
			if ((chr->_9530 -= frames) < 0)
				chr->_9530 = 0;
		}
		u8* heads[4];
		for (int i = 0; i < 4; i++)
			heads[i] = AnimationHead(joints[i].mJObj);
		if (heads[0] == joints[0]._08 && heads[1] == joints[1]._08 && heads[2] == joints[2]._08 && heads[3] == joints[3]._08) {
			s32 started = 0;
			for (int i = 0; i < 4; i++) {
				if (chr->_9530 == 0) {
					if (joints[i].mJObj->aobj == NULL) {
						HSD_JObjAddAnim(joints[i].mJObj, joints[i].mAnimJoint, NULL, NULL);
						HSD_JObjReqAnim(joints[i].mJObj, 0.0f);
						started++;
						joints[i]._08 = joints[i].mJObj->aobj->fobj->ad_head;
					} else if (HSD_AObjGetFlags(joints[i].mJObj->aobj) & 0x44000000) {
						HSD_JObjReqAnim(joints[i].mJObj, 0.0f);
						started++;
					}
				}
				chr->Push(joints[i].mJObj, 0x200, 1.0f);
			}
			if (started > 0) {
				chr->_9530 = 0.5f * IDLE_SECOND + rand() % IDLE_SECOND;
			}
		} else {
			for (int i = 0; i < 4; i++)
				joints[i]._08 = NULL;
		}
	}
}

extern "C" {
extern s32 lbl_8065B268;
extern s32 gPlayerSuitIdleAnimations[9];
}

static inline f32 PlayerGaitRate(f32 speed, bool slow)
{
	f32 rate = slow ? speed : 0.5f * speed;
	if (rate > 1.0f)
		rate = 1.0f;
	f32 minimum = slow ? 0.5f : 0.7f;
	if (rate < minimum)
		rate = minimum;
	return rate;
}

static inline f32 PlayerGaitEndFrame(XObjChr* p)
{
	HSD_AObj* anim = GetAObj(p->mJObj);
	return anim ? HSD_AObjGetEndFrame(anim) : 0.0f;
}

static inline void ChangePlayerGait(XObjChr* p, s32 id, f32 rate)
{
	HSD_AObj* anim       = GetAObj(p->mJObj);
	p->mFrameTime.mFrame = 4.0f + (anim ? HSD_AObjGetCurrentFrame(anim) : 0.0f);
	if (p->mFrameTime.mFrame >= PlayerGaitEndFrame(p))
		p->mFrameTime.mFrame -= PlayerGaitEndFrame(p);
	p->SetAnim(id, FALSE, NULL, NULL, NULL, p->mFrameTime.mFrame, rate, 0.0f, 10.0f);
}

static inline void SetPlayerIdleAnim(XObjChr* p, s32 id)
{
	p->SetAnim(id, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
}

/**
 * @note Address: 0x800AA070
 * @note Size: 0x1670
 */
extern "C" void UpdatePlayerLocomotionAnimation(XObjChr* p, s32, HSD_AObj* anim, u32, f32, f32)
{
	f32 motion = 0.0f;
	if (!(gGame.mStatus.mFlags & 0x20) && !gGame.mEvt.mData.mScript)
		motion = sqrtf(p->mPad.mStickX * p->mPad.mStickX + p->mPad.mStickY * p->mPad.mStickY);
	if (motion == 0.0f && p->mFloorContacts[4].objectIndex >= 0)
		motion = 0.8f;
	if (motion > 0.0f && (gGame.mStatus.mFlags & 0x100) && !(gGame.mStatus.mFlags & 0x200))
		motion = 0.0f;
	if (__fpclassifyf(p->_12DF8) != FP_NAN)
		motion = PSVECMag(&p->mMoveTranslate.mSpeed);

	f32 speed;
	if (lbl_8065B268 >= 1) {
		f32 scale = 1.0f - 0.15f * (f32)lbl_8065B268;
		if (scale < 0.7f)
			scale = 0.7f;
		speed = p->_9B8 / scale;
	} else {
		speed = p->_9B8 / GetPlayerAnimationRateMultiplier(p);
	}
	if ((p->mLift.mState == 0 || p->mLift.mState == 4) && !p->mHang.mState && !p->mJump.mState && !p->mLadder.mState && !p->mRope.mState
	    && !p->mPush.mState && !p->mTumble.mState && !p->mCopter.mState
	    && (p->mPopper.mState == 0 || p->mPopper.mState == 8 || p->mPopper.mState == 4) && !p->mRadar.mState) {
		f32 distance = PSVECDistance(&p->_988, &p->_988.mTarget);
		if (distance / GetPlayerAnimationRateMultiplier(p) >= 3.0f) {
			p->SetAnim(0x79, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 30.0f);
		} else if (p->mAnimId == 0x79 || p->mAnimId == 0x89 || p->mAnimId == 0xE0) {
			SetPlayerIdleAnim(p, 0x11);
			p->_A2C = 3;
		}
	}
	if (!((p->mLift.mState == 0 || p->mLift.mState == 4) && !p->mHang.mState && !p->mJump.mState && !p->mLadder.mState && !p->mRope.mState
	      && !p->mPush.mState && !p->mTumble.mState && !p->mCopter.mState
	      && (p->mPopper.mState == 0 || p->mPopper.mState == 8 || p->mPopper.mState == 4) && !p->mRadar.mState))
		return;

	switch (p->mAnimId) {
	case 0:
	case 1:
	case 2:
	case 8:
	case 0x11:
	case 0x12:
	case 0x4A:
	case 0xA1:
	case 0xA3:
	case 0x822:
	case 0x874:
	case 0x894:
	case 0x8B2:
	case 0xCF7:
	case 0xCF8:
	case 0xCF9:
	case 0xCFA:
	case 0xCFB:
	case 0xCFC:
	case 0xCFD: {
		bool script = gGame.mEvt.mData.mScript != NULL;
		if (script && __fpclassifyf(p->_12DF8) == FP_NAN
		    && (p->mCopterObj.mVisible || p->mPopperObj.mVisible || p->mRadarObj.mVisible || p->mOXObj.mVisible))
			break;
		if (motion != 0.0f) {
			SetPlayerIdleAnim(p, 3);
			p->_A2C = 0;
			break;
		}
		if (p->mLift.mState != 0 || p->mFloorContacts[2].objectIndex >= 0 || p->mSuits.mChangeStep)
			break;
		if (p->GetAttachment() == 1 || p->GetAttachment() == 3) {
			SetPlayerIdleAnim(p, 0x4A);
		} else if (p->GetAttachment() == 2) {
			SetPlayerIdleAnim(p, 0x822);
		} else if ((gGame.mStatus.mFlags & 0x100) || script) {
			SetPlayerIdleAnim(p, 0x11);
			p->_A2C = 3;
		} else if (anim->flags & 0x44000000) {
			if (p->_A2C > 0)
				--p->_A2C;
			if (p->_A2C == 0 && (p->_980.mFlags & 4)) {
				s32 id = 0x11;
				if (!gGame.mEvt.mData.mScript && (p->mHitObjectIndex < 0 || p->mHitObjectIndex == p->mFloorContacts[0].objectIndex)) {
					s32 pick = rand();
					if (!(pick & 3)) {
						switch (pick & 0x30) {
						case 0x10:
							id = 1;
							break;
						case 0x20:
							id = 2;
							break;
						case 0x30:
							id = 0x12;
							break;
						default:
							id = gPlayerSuitIdleAnimations[p->mSuits.mCurrent];
							break;
						}
						p->_A2C = rand() % 10 + 5;
					}
				}
				SetPlayerIdleAnim(p, id);
			}
		}
		break;
	}
	case 3:
	case 0xC:
	case 0x818:
	case 0x896:
		if (motion == 0.0f) {
			if (p->GetAttachment() == 1 || p->GetAttachment() == 3)
				SetPlayerIdleAnim(p, 0x4A);
			else {
				SetPlayerIdleAnim(p, 0x11);
				p->_A2C = 3;
			}
		} else if (speed >= 1.25f) {
			ChangePlayerGait(p, 0x77, PlayerGaitRate(speed, false));
		} else {
			p->SetAnimRate(PlayerGaitRate(speed, true), 0x5A0);
		}
		break;
	case 0x77:
	case 0x88:
	case 0x882:
	case 0x897:
		if (speed < 1.25f)
			ChangePlayerGait(p, 3, PlayerGaitRate(speed, true));
		else if (speed >= 1.9f)
			ChangePlayerGait(p, 4, PlayerGaitRate(speed, false));
		else
			p->SetAnimRate(PlayerGaitRate(speed, false), 0x5A0);
		break;
	case 4:
	case 0xD:
	case 0x819:
	case 0x898:
		if (speed < 1.25f)
			ChangePlayerGait(p, 3, PlayerGaitRate(speed, true));
		else if (speed < 1.9f)
			ChangePlayerGait(p, 0x77, PlayerGaitRate(speed, true));
		else
			p->SetAnimRate(PlayerGaitRate(speed, false), 0x5A0);
		break;
	}
}

extern "C" {
void UpdatePlayerHangAction(XActHang*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePlayerJumpAction(XActJump*, s32, HSD_AObj*, u32, f32, f32);
void UpdateLadderActionAnimation(XActLadder*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePlayerRopeAction(XActRope*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePushActionAnimation(XActPush*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePlayerTumbleAnimation(XActTumble*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePlayerCopterFlight(XActCopter*, s32, HSD_AObj*, u32, f32, f32);
void UpdatePlayerPopperAction(XActPopper*, s32, HSD_AObj*, u32, f32, f32);
void UpdateRadarTargeting(XActRadar*, s32, HSD_AObj*, u32, f32, f32);
}

static inline void FinishPlayerActionAnimation(XObjChr* player, HSD_AObj* aobj)
{
	if (aobj->flags & 0x44000000) {
		player->SetAnim(17, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		player->_A2C = 3;
	}
}

/**
 * @note Address: 0x800AB6E0
 * @note Size: 0x718
 */
extern "C" void UpdatePlayerActionAnimations(XObjChr* player, u32 frames)
{
	if (!(player->_980.mFlags & 0x80000000)) {
		HSD_AObj* aobj = GetAObj(player->mJObj);
		if (aobj == NULL)
			return;
		s32 anim  = player->mAnimId;
		f32 frame = aobj != NULL ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
		f32 end   = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
		UpdateLiftActionAnimation(&player->mLift, anim, aobj, frames, frame, end);
		UpdatePlayerHangAction(&player->mHang, anim, aobj, frames, frame, end);
		UpdatePlayerJumpAction(&player->mJump, anim, aobj, frames, frame, end);
		UpdateLadderActionAnimation(&player->mLadder, anim, aobj, frames, frame, end);
		UpdatePlayerRopeAction(&player->mRope, anim, aobj, frames, frame, end);
		UpdatePushActionAnimation(&player->mPush, anim, aobj, frames, frame, end);
		UpdatePlayerTumbleAnimation(&player->mTumble, anim, aobj, frames, frame, end);
		UpdatePlayerCopterFlight(&player->mCopter, anim, aobj, frames, frame, end);
		UpdatePlayerPopperAction(&player->mPopper, anim, aobj, frames, frame, end);
		UpdateRadarTargeting(&player->mRadar, anim, aobj, frames, frame, end);
		if (player->mAttachmentRequest < 0 && !(gGame.mStatus.mFlags & 0x400))
			UpdatePlayerLocomotionAnimation(player, anim, aobj, frames, frame, end);

		switch (anim) {
		case 0x5D:
			if (aobj->flags & 0x44000000)
				player->SetAnim(0x5E, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
			break;
		case 0x5F:
			FinishPlayerActionAnimation(player, aobj);
			break;
		case 0x78:
		case 0x13B:
		case 0xEF:
			FinishPlayerActionAnimation(player, aobj);
			break;
		}
	}

	UpdatePlayerIdleJointAnimations(player, frames);
	if ((player->GetAttachment() == 1 || player->GetAttachment() == 3) && player->mAttachmentRequest < 0
	    && player->mAttachmentJoint->aobj == NULL) {
		HSD_JObjAddAnimAll(player->mAttachmentJoint, player->mAttachmentJointAnim, NULL, NULL);
		HSD_JObjReqAnimAll(player->mAttachmentJoint, 0.0f);
	}
	for (XObjChrItemAnim* entry = gPlayerItemAnimationTable; entry->mItem >= 0; ++entry) {
		if (entry->mItem == player->mSuits.mCurrent) {
			HSD_JObj* joint = FindJointByPath(player->mJObj, entry->mPath, entry->mDepth);
			if (GetAObj(joint) == NULL) {
				CJArchive& archive        = player->mSuits.mObjects[player->mSuits.mCurrent].mArchive;
				HSD_AnimJoint* anim       = archive.GetAnimJoint(0, entry->mDepth, entry->mPath);
				HSD_MatAnimJoint* mat     = archive.GetMatAnimJoint(0, entry->mDepth, entry->mPath);
				HSD_ShapeAnimJoint* shape = archive.GetShapeAnimJoint(0, entry->mDepth, entry->mPath);
				HSD_JObjAddAnim(joint, anim, mat, shape);
				HSD_JObjReqAnim(joint, entry->mFrame);
			}
		}
	}
	UpdatePlayerLookAtObjects(player, frames);
	player->mHeadTurn.mNeck.Update(frames);
	player->mHeadTurn.mHead.Update(frames);
	if (player->_A34 > 0) {
		if ((player->_A34 -= frames) < 0)
			player->_A34 = 0;
		++player->_A38;
		if (player->_A34 > 0) {
			f32 red = 128.0f * ((f32)player->_A34 / IDLE_SECOND);
			player->vf12C(1, (s32)red, 0, 0, 0.0f);
			StartScreenFade(&gGame.mScrnTex.mColor, (s32)red, 0, 0, 0, (s32)red, 0, 0, 0, 0.0f);
		} else {
			player->vf12C(1, 0, 0, 0, 0.0f);
			StartScreenFade(&gGame.mScrnTex.mColor, 0, 0, 0, 0, 100, 100, 200, 0, 0.0f);
		}
	}
}

extern "C" void SelectHighestActiveFloor(XObj* object);

static inline bool IsNearbyInteractionTarget(XObjChr* player, XObj* object, f32 minHeight)
{
	if (object->vf128() != NULL)
		return true;
	u64 flags     = object->mStatus.mFlags;
	bool eligible = (flags & 0x0000000100000000ULL) && object->mMaxFloorHeight > minHeight;
#if defined(VERSION_JP)
	if ((flags & 0x0000008000000000ULL) && (gGame.mStatus.mFlags & 0x0000080000000000ULL))
		eligible = true;
	if ((flags & 0x0000000200000000ULL) && (gGame.mStatus.mFlags & 0x0000080000000000ULL)) {
		f32 difference = Abs(object->mSelectedFloorHeight - player->mMoveTranslate.y);
		if (difference != 0.0f && difference <= 12.5f)
			eligible = true;
	} else
#endif
	    if (flags & 0x0000000800000000ULL) {
		if (object->mSelectedFloorIndex == 0 && object->mSelectedFloorHeight > player->mMaxFloorHeight) {
			f32 angle = WrapAngle(
			    (f32)atan2(object->mMoveTranslate.x - player->mMoveTranslate.x, object->mMoveTranslate.z - player->mMoveTranslate.z)
			    - object->mMoveRotate.y);
			if (angle >= DEG2RAD * 120.0f && angle <= DEG2RAD * 240.0f)
				eligible = true;
		}
	} else if ((flags & 0x0000001000000000ULL) && object->mSelectedFloorIndex == 0
	           && object->mSelectedFloorHeight > player->mMaxFloorHeight) {
		eligible = true;
	}
	return eligible;
}

/**
 * @note Address: 0x800ABDF8
 * @note Size: 0x868
 */
extern "C" s32 FindNearbyPlayerInteraction(XObjChr* player, f32 extraRadius)
{
	f32 radius = player->mMoveScale.x * GetFloorExtent(&player->mFloors, 0).x;
	f32 height = player->mMoveScale.x * (GetFloorCenterOffset(&player->mFloors, 0).y + GetFloorExtent(&player->mFloors, 0).y);
	// Retail rotates the local floor offset before testing contacts at the player position.
	Mtx rotation;
	PSMTXRotRad(rotation, 'y', player->mMoveRotate.y);
	Vec offset = player->mMoveScale * GetFloorCenterOffset(&player->mFloors, 0);
	PSMTXMultVec(rotation, &offset, &offset);

	const f32 x    = player->mMoveTranslate.x;
	const f32 y    = player->mMoveTranslate.y;
	const f32 z    = player->mMoveTranslate.z;
	f32 minHeight  = 1.0f + y;
	f32 bestHeight = NAN;
	s32 best       = -1;
	CVec push, contact;
	for (s32 i = 0; i < gGame.mXObjs.mObjectCount; ++i) {
		XObj* object = gGame.GetObject(i);
		u64 flags    = object->mStatus.mFlags;
		if (!(flags & 1) || !(flags & 0x10000) || (flags & 0x10000000) || object->mId == player->mId || object->GetKind() >= 10000
		    || (s16)object->GetPool() == 7)
			continue;
		if (player->mFloorContacts[0].objectIndex >= 0 && gGame.GetObject(player->mFloorContacts[0].objectIndex) == object)
			continue;
		if ((flags & 0x0000020000000000ULL) || (flags & 0x100000) || (flags & 0x0002000000000000ULL))
			continue;
		f32 dx = object->mMoveTranslate.x - x;
		f32 dz = object->mMoveTranslate.z - z;
		if (sqrtf(dx * dx + dz * dz) > 100.0f + (0.9f * radius + object->mFloorXZExtent))
			continue;
		u32 hit;
		{
			CVec from(x, y, z), to(x, y, z);
			hit = object->vfFC(&from, &to, radius + extraRadius, height, &push, &contact, 0, 0);
		}
		if ((hit & 3) != 3)
			continue;
		SelectHighestActiveFloor(object);
		if ((best < 0 || __fpclassifyf(bestHeight) == FP_NAN || object->mMaxFloorHeight > bestHeight)
		    && IsNearbyInteractionTarget(player, object, minHeight)) {
			bestHeight = object->mSelectedFloorHeight;
			best       = i;
		}
	}
	return best;
}

/**
 * @note Address: 0x800B1B0C
 * @note Size: 0x8
 */
extern "C" void fn_800B1B0C(XActPush* act, s32 value)
{
	act->_24 = value;
}

/**
 * @note Address: 0x800B1B14
 * @note Size: 0x8
 */
extern "C" void SetPushActionState(XActPush* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B1B1C
 * @note Size: 0x8
 */
extern "C" void SetHangActionState(XActHang* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B1B24
 * @note Size: 0x4C
 */
extern "C" BOOL fn_800B1B24(const XObjChr* chr)
{
	return chr->mLift.mState == 4 && IsLiftTargetPlug((XActLift*)&chr->mLift);
}

/**
 * @note Address: 0x800B1B70
 * @note Size: 0x14
 */
extern "C" void SetPlayerActionTarget(XAction* action, XObj* target)
{
	action->SetTarget(target);
}

/**
 * @note Address: 0x800B1B84
 * @note Size: 0x48
 */
extern "C" f32 fn_800B1B84()
{
	return (f32)(s32)fn_8006E9AC(2) / 100.0f;
}

/**
 * @note Address: 0x800B1BCC
 * @note Size: 0x48
 */
extern "C" f32 fn_800B1BCC()
{
	return (f32)(s32)fn_8006E9AC(1) / 100.0f;
}

/**
 * @note Address: 0x800B1C14
 * @note Size: 0x48
 */
extern "C" f32 fn_800B1C14()
{
	return (f32)(s32)fn_8006E9AC(0) / 100.0f;
}

/**
 * @note Address: 0x800B1C5C
 * @note Size: 0xF4
 */
extern "C" f32 GetControlStickMagnitude(const CPadOne* pad)
{
	f32 x = pad->mStickX;
	f32 y = pad->mStickY;
	return sqrtf(x * x + y * y);
}

/**
 * @note Address: 0x800B1D50
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B1D50(XObj* object)
{
	return (u8)((object->mStatus.mFlags & 0x0000000400000000ULL) != 0);
}

/**
 * @note Address: 0x800B1D84
 * @note Size: 0x8
 */
extern "C" CVec* GetCameraAngles(CCamera* camera)
{
	return &camera->mAngle;
}

/**
 * @note Address: 0x800B1D8C
 * @note Size: 0x50
 */
extern "C" void SetLookTargetPosition(XObjChrLook* look, f32 x, f32 y, f32 z)
{
	Vec target    = { 0.0f, 0.0f, 0.0f };
	target.x      = x;
	target.y      = y;
	target.z      = z;
	look->mTarget = target;
}

/**
 * @note Address: 0x800B1DDC
 * @note Size: 0x8
 */
extern "C" void SetPlayerLookMode(XObjChrLook* look, s32 mode)
{
	look->mMode = mode;
}

/**
 * @note Address: 0x800B1DE4
 * @note Size: 0xC
 */
extern "C" void EnablePlayerLook(XObjChrLook* look)
{
	look->mActive = TRUE;
}

/**
 * @note Address: 0x800B1DF0
 * @note Size: 0xC
 */
extern "C" u32 fn_800B1DF0(XObj* object, u32 mask)
{
	return object->_8B0 & mask;
}

/**
 * @note Address: 0x800B1DFC
 * @note Size: 0x8
 */
extern "C" u32 fn_800B1DFC(XObj* object)
{
	return object->_8B0;
}

/**
 * @note Address: 0x800B1E04
 * @note Size: 0x8
 */
extern "C" s32 fn_800B1E04(void*)
{
	return 0;
}

/**
 * @note Address: 0x800B1E0C
 * @note Size: 0xAC
 */
// is position behind the object, within 60 degrees either side
extern "C" BOOL IsPositionBehindObject(void*, const Vec* pos, CXObj* object)
{
	f32 angle = atan2(object->mMoveTranslate.x - pos->x, object->mMoveTranslate.z - pos->z);
	angle     = WrapAngle(angle - object->mMoveRotate.y);
	return angle >= 2.0f * PI / 3.0f && angle <= 4.0f * PI / 3.0f;
}

/**
 * @note Address: 0x800B1EB8
 * @note Size: 0x8
 */
extern "C" f32 GetObjectMaxFloorHeight(XObj* object)
{
	return object->mMaxFloorHeight;
}

/**
 * @note Address: 0x800B1EC0
 * @note Size: 0x144
 */
// NONMATCHING: retail issues the point loads in a different order.
extern "C" f32 GetObjectEdgeOutwardYaw(const XObj* object, Vec* point, s32 index)
{
	Vec first  = FirstEdgePoint(object, index, point->y);
	Vec second = SecondEdgePoint(object, index, point->y);
	CVec direction;
	GetOutwardPerpendicularToEdge(point, &first, &second, &direction);
	f32 angle = atan2(direction.x, direction.z);
	angle     = WrapAngle(angle);
	return angle;
}

/**
 * @note Address: 0x800B2004
 * @note Size: 0x8
 */
extern "C" s32 GetSelectedFloorIndex(const XObj* object)
{
	return object->mSelectedFloorIndex;
}

/**
 * @note Address: 0x800B200C
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B200C(const XObj*)
{
	return false;
}

/**
 * @note Address: 0x800B2014
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2014(const XObj* object)
{
	return TestStatus(object, 0x0000008000000000ULL);
}

/**
 * @note Address: 0x800B2048
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2048(const XObj* object)
{
	return TestStatus(object, 0x0000000100000000ULL);
}

/**
 * @note Address: 0x800B207C
 * @note Size: 0xC
 */
extern "C" void DisablePlayerLook(XObjChrLook* look)
{
	look->mActive = FALSE;
}

/**
 * @note Address: 0x800B2088
 * @note Size: 0x18
 */
extern "C" bool IsPlayerLookMarkerVisible(const XObjChrLook* look)
{
	return look->mFade > 0.0f;
}

/**
 * @note Address: 0x800B20A0
 * @note Size: 0x14
 */
extern "C" s32 GetObjectPoolUsedCount(const CGame* game, s32 index)
{
	return game->mXObjs.mPools[index].mUsed;
}

/**
 * @note Address: 0x800B20B4
 * @note Size: 0x10
 */
extern "C" UnkStageObj* GetIndexedCoordPlacement(const CoordInfo* info, s32 index)
{
	return info->_00 + index;
}

/**
 * @note Address: 0x800B20C4
 * @note Size: 0x8
 */
extern "C" CoordInfo* GetObjectCoordInfo(XObj* object)
{
	return &object->mCoord;
}

/**
 * @note Address: 0x800B20CC
 * @note Size: 0x3C
 */
extern "C" f32 GetHeadingBetweenPositions(const Vec* from, const Vec* to)
{
	return atan2(to->x - from->x, to->z - from->z);
}

/**
 * @note Address: 0x800B2108
 * @note Size: 0x20
 */
extern "C" void* GetObjectFromPool(const CGame* game, s32 pool, s32 index)
{
	return game->mXObjs.mPools[pool].mObjects + index * game->mXObjs.mPools[pool].mStride;
}

/**
 * @note Address: 0x800B2128
 * @note Size: 0xC
 */
extern "C" s32 GetStageObjectCount(const CGame* game)
{
	return game->mXObjs.mObjectCount;
}

/**
 * @note Address: 0x800B2134
 * @note Size: 0x8
 */
extern "C" f32 GetSelectedFloorHeight(const XObj* object)
{
	return object->mSelectedFloorHeight;
}

/**
 * @note Address: 0x800B213C
 * @note Size: 0x8
 */
extern "C" void SetPlayerActionState(XAction* action, s32 state)
{
	action->mState = state;
}

/**
 * @note Address: 0x800B2144
 * @note Size: 0x20
 */
extern "C" void SetPlayerRopeState(XObjChr* chr, s32 state, XObj* target)
{
	chr->mRope.SetState(state, target);
}

/**
 * @note Address: 0x800B2164
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2164(const XObj* object)
{
	return TestStatus(object, 0x0000001000000000ULL);
}

/**
 * @note Address: 0x800B2198
 * @note Size: 0x20
 */
extern "C" void SetPlayerLadderState(XObjChr* chr, s32 state, XObj* target)
{
	chr->mLadder.SetState(state, target);
}

/**
 * @note Address: 0x800B21B8
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B21B8(const XObj* object)
{
	return TestStatus(object, 0x0000000800000000ULL);
}

/**
 * @note Address: 0x800B21EC
 * @note Size: 0x44
 */
extern "C" void ReleasePlayerLiftTarget(XObjChr* chr)
{
	XObj* target = chr->mLift.mTarget;
	if (target != NULL) {
		target->mStatus.mFlags &= ~0x0400000000000000ULL;
		chr->mLift.mTarget = NULL;
	}
	chr->mLift.mState = 0;
}

/**
 * @note Address: 0x800B2230
 * @note Size: 0x20
 */
extern "C" void SetPlayerHangState(XObjChr* chr, s32 state, XObj* target)
{
	chr->mHang.SetState(state, target);
}

/**
 * @note Address: 0x800B2250
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2250(const XObj* object)
{
	return TestStatus(object, 0x0000000200000000ULL);
}

/**
 * @note Address: 0x800B2284
 * @note Size: 0x10
 */
extern "C" s32 fn_800B2284(const XObj* object, s32 index)
{
	const s32* entries = &object->_25C;
	return entries[index];
}

/**
 * @note Address: 0x800B2294
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2294(const XObj* object)
{
	return TestStatus(object, 0x0000002000000000ULL);
}

/**
 * @note Address: 0x800B22C8
 * @note Size: 0x8
 */
extern "C" void SetCopterActionState(XAction* action, s32 state)
{
	action->mState = state;
}

/**
 * @note Address: 0x800B22D0
 * @note Size: 0x28
 */
extern "C" BOOL IsPlayerRingMenuOpen(const XObjChrMenu* menu)
{
	return menu->IsOpen();
}

/**
 * @note Address: 0x800B22F8
 * @note Size: 0x48
 */
extern "C" f32 GetCopterDescentSpeedLimit()
{
	return (s32)GetCopterParameter(2) / 100.0f;
}

/**
 * @note Address: 0x800B2340
 * @note Size: 0x48
 */
extern "C" f32 GetCopterDescentAcceleration()
{
	return (s32)GetCopterParameter(1) / 100.0f;
}

/**
 * @note Address: 0x800B2388
 * @note Size: 0x8
 */
extern "C" void SetObjectTargetY(CObjVec* value, f32 y)
{
	value->mTarget.y = y;
}

/**
 * @note Address: 0x800B2390
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B2390(const XActPush*)
{
	return true;
}

/**
 * @note Address: 0x800B2398
 * @note Size: 0xE4
 */
extern "C" f32 SquareRootFloat(f32 value)
{
	return sqrtf(value);
}

/**
 * @note Address: 0x800B247C
 * @note Size: 0x40
 */
extern "C" f32 GetFloorYawRadians(XObjFloor* const* floors, s32 index)
{
	return DEG2RAD * (*floors)[index].mYawDegrees;
}

/**
 * @note Address: 0x800B24BC
 * @note Size: 0x8
 */
extern "C" s32 GetActionTargetFloor(const XAction* action)
{
	return action->mTargetFloor;
}

/**
 * @note Address: 0x800B24C4
 * @note Size: 0x8
 */
extern "C" XObjFloor** GetObjectFloorTableAddress(XObj* object)
{
	return &object->mFloors;
}

/**
 * @note Address: 0x800B24CC
 * @note Size: 0x8
 */
extern "C" CObjVec* GetObjectMoveScale(CXObj* object)
{
	return &object->mMoveScale;
}

/**
 * @note Address: 0x800B24D4
 * @note Size: 0x8
 */
extern "C" CObjVec* GetObjectMoveRotation(CXObj* object)
{
	return &object->mMoveRotate;
}

/**
 * @note Address: 0x800B24DC
 * @note Size: 0x28
 */
extern "C" s32 GetObjectPoolId(const XObj* object)
{
	return object->GetPool();
}

/**
 * @note Address: 0x800B2504
 * @note Size: 0x14
 */
extern "C" s32 GetDrawListCount(const CGame* game, s32 list)
{
	return game->mDrawLists[list].mCount;
}

/**
 * @note Address: 0x800B2518
 * @note Size: 0x24
 */
extern "C" void ClearObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags &= ~mask;
}

/**
 * @note Address: 0x800B253C
 * @note Size: 0xC
 */
extern "C" u32 fn_800B253C(const XObjChr* chr, u32 mask)
{
	return chr->_980.mFlags & mask;
}

/**
 * @note Address: 0x800B2548
 * @note Size: 0x1C
 */
extern "C" s32 GetDrawListIndex(const CGame* game, s32 list, s32 index)
{
	return game->mDrawLists[list].mIndices[index];
}

/**
 * @note Address: 0x800B2564
 * @note Size: 0x1C
 */
extern "C" void SetObjectStatusFlags(XObj* object, u64 mask)
{
	object->mStatus.mFlags |= mask;
}

/**
 * @note Address: 0x800B2580
 * @note Size: 0x14
 */
extern "C" f32 GetObjectHitFloorHeight(const XObj* object)
{
	return object->mFloorHeights[object->mHitFloorIndex];
}

/**
 * @note Address: 0x800B2594
 * @note Size: 0x8
 */
extern "C" f32 GetObjectFloorXZExtent(const XObj* object)
{
	return object->mFloorXZExtent;
}

/**
 * @note Address: 0x800B259C
 * @note Size: 0x104
 */
extern "C" f32 GetHorizontalDistance(const Vec* a, const Vec* b)
{
	f32 z = a->z - b->z;
	f32 x = a->x - b->x;
	return sqrtf(x * x + z * z);
}

/**
 * @note Address: 0x800B26A0
 * @note Size: 0x14
 */
extern "C" BOOL fn_800B26A0(const XAction* action, s32 wanted)
{
	return action->_0C == wanted;
}

/**
 * @note Address: 0x800B26B4
 * @note Size: 0x8
 */
extern "C" u32 GetObjectId(const CXObj* object)
{
	return object->mId;
}

/**
 * @note Address: 0x800B26BC
 * @note Size: 0x14
 */
extern "C" u64 GetMaskedObjectStatus(const XObj* object, u64 mask)
{
	return object->mStatus.mFlags & mask;
}

/**
 * @note Address: 0x800B26D0
 * @note Size: 0x1C
 */
extern "C" XObj* GetDrawListObject(const CGame* game, s32 list, s32 index)
{
	return game->mDrawLists[list].mObjects[index];
}

/**
 * @note Address: 0x800B26EC
 * @note Size: 0x78
 */
extern "C" void GetJointScale(CJObj*, HSD_JObj* jobj, Vec* scale)
{
	HSD_JObjGetScale(jobj, scale);
}

/**
 * @note Address: 0x800B2764
 * @note Size: 0x7C
 */
extern "C" HSD_JObj* GetObjectJointByPath(const CJObj* object, s32 depth, s32* path)
{
	return GetJointByPath(object->mJObj, path, depth);
}

/**
 * @note Address: 0x800B2958
 * @note Size: 0x1C
 */
extern "C" void SetObjectVectorTarget(CObjVec* value, const Vec& target)
{
	value->mTarget.Set(target);
}

/**
 * @note Address: 0x800B2974
 * @note Size: 0x10
 */
extern "C" void SetVectorComponents(CVec* value, f32 x, f32 y, f32 z)
{
	value->Set(x, y, z);
}

/**
 * @note Address: 0x800B2984
 * @note Size: 0x10
 */
extern "C" void SetObjectVelocity(CObjVec* value, f32 x, f32 y, f32 z)
{
	value->mSpeed.Set(x, y, z);
}

/**
 * @note Address: 0x800B2994
 * @note Size: 0x24
 */
extern "C" void ScaleVectorInPlace(Vec* value, f32 scale)
{
	PSVECScale(value, value, scale);
}

/**
 * @note Address: 0x800B29B8
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B29B8(const XActRadar*)
{
	return true;
}

/**
 * @note Address: 0x800B29C0
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B29C0(const XActPopper*)
{
	return true;
}

/**
 * @note Address: 0x800B29C8
 * @note Size: 0x2C
 */
extern "C" BOOL fn_800B29C8(const XActCopter* act)
{
	return act->mState == 0 || act->mState == 6 || act->mState == 9;
}

/**
 * @note Address: 0x800B29F4
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B29F4(const XActTumble*)
{
	return true;
}

/**
 * @note Address: 0x800B29FC
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B29FC(const XActPush*)
{
	return true;
}

/**
 * @note Address: 0x800B2A04
 * @note Size: 0x24
 */
extern "C" BOOL fn_800B2A04(const XActRope* act)
{
	return act->mState == 0 || act->mState == 10;
}

/**
 * @note Address: 0x800B2A28
 * @note Size: 0x24
 */
extern "C" BOOL fn_800B2A28(const XActLadder* act)
{
	return act->mState == 0 || act->mState == 11;
}

/**
 * @note Address: 0x800B2A4C
 * @note Size: 0x24
 */
extern "C" BOOL fn_800B2A4C(const XActJump* act)
{
	return act->mState == 0 || act->mState == 4;
}

/**
 * @note Address: 0x800B2A70
 * @note Size: 0x10
 */
extern "C" BOOL IsHangActionIdle(const XActHang* act)
{
	return (u8)(act->mState == 0);
}

/**
 * @note Address: 0x800B2A80
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B2A80(const XActLift*)
{
	return true;
}

/**
 * @note Address: 0x800B2A88
 * @note Size: 0x34
 */
extern "C" CVec* NormalizeVectorInPlace(CVec* value)
{
	PSVECNormalize(value, value);
	return value;
}

/**
 * @note Address: 0x800B2ABC
 * @note Size: 0xF4
 */
extern "C" BOOL fn_800B2ABC(const XObjMap* map, const Vec* pos, XObjMapHit* hit, Vec* tri)
{
	s32 start = FindFloorStart(map, 0.2f + pos->y);
	for (int i = 0; i < map->mTrigonCount; i++) {
		s32 index    = map->mTrigonOrder[(start + i) % map->mTrigonCount];
		int triangle = ((CTrigon*)&map->mTrigon[index])->Check((Vec*)pos, &hit->mHeight, tri);
		if (triangle >= 0) {
			hit->mTrigon   = index;
			hit->mTriangle = triangle;
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * @note Address: 0x800B2BB0
 * @note Size: 0x20
 */
extern "C" void SetTumbleStateAndFade(XObjChr* chr, s32 state, BOOL restart)
{
	chr->mTumble.mState = state;
	if (restart) {
		chr->mTumble.mFade.mActive = TRUE;
		chr->mTumble.mFade.mAmount = 0.0f;
	}
}

/**
 * @note Address: 0x800B2BD0
 * @note Size: 0x12C
 */
extern "C" void ResetPlayerPlugState(XObjPlug* plug, BOOL resetJoint)
{
	ClearPlugState(plug, resetJoint);
}

/**
 * @note Address: 0x800B2CFC
 * @note Size: 0xC
 */
extern "C" XSound* GetGameSound(CGame* game)
{
	return &game->mSound;
}

/**
 * @note Address: 0x800B2D08
 * @note Size: 0x24
 */
extern "C" u32 CheckCurrentStageFlags(const CGame* game, u32 mask)
{
	return CheckStageFlag(game->mStage, mask);
}

/**
 * @note Address: 0x800B2D2C
 * @note Size: 0x14
 */
extern "C" BOOL IsRadarActionState(const XActRadar* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B2D40
 * @note Size: 0x14
 */
extern "C" BOOL IsPopperActionState(const XActPopper* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B2D54
 * @note Size: 0x8
 */
extern "C" CVec* fn_800B2D54(XObjPlug* plug)
{
	return &plug->mCord._6C;
}

/**
 * @note Address: 0x800B2D5C
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B2D5C(const XActRadar*)
{
	return true;
}

/**
 * @note Address: 0x800B2D64
 * @note Size: 0x8
 */
extern "C" BOOL fn_800B2D64(const XActPopper*)
{
	return true;
}

/**
 * @note Address: 0x800B2D6C
 * @note Size: 0x54
 */
extern "C" BOOL fn_800B2D6C(const XActCopter* act)
{
	s32 state = act->mState;
	return (state == 0 || state == 3 || state == 4 || state == 7 || state == 8 || state == 5 || state == 6 || state == 9);
}

/**
 * @note Address: 0x800B2DC0
 * @note Size: 0x24
 */
extern "C" BOOL fn_800B2DC0(const XActTumble* act)
{
	return (act->mState != 8 && act->mState != 9);
}

/**
 * @note Address: 0x800B2DE4
 * @note Size: 0x34
 */
extern "C" BOOL fn_800B2DE4(const XActRope* act)
{
	s32 state = act->mState;
	return (state == 0 || state == 9 || state == 10 || state == 11);
}

/**
 * @note Address: 0x800B2E18
 * @note Size: 0x2C
 */
extern "C" BOOL fn_800B2E18(const XActLadder* act)
{
	s32 state = act->mState;
	return (state == 0 || state == 10 || state == 11);
}

/**
 * @note Address: 0x800B2E44
 * @note Size: 0x3C
 */
extern "C" BOOL fn_800B2E44(const XActJump* act)
{
	s32 state = act->mState;
	return (state == 0 || state == 2 || state == 3 || state == 4 || state == 17);
}

/**
 * @note Address: 0x800B2E80
 * @note Size: 0x24
 */
extern "C" BOOL fn_800B2E80(const XActHang* act)
{
	return (act->mState == 0 || act->mState == 10);
}

/**
 * @note Address: 0x800B2EA4
 * @note Size: 0xC
 */
extern "C" s32 IsGameMenuActive(const CGame* game)
{
	return game->mMenu.mActive;
}

/**
 * @note Address: 0x800B2EB0
 * @note Size: 0x8
 */
extern "C" u32 fn_800B2EB0(const XObjPlug* plug)
{
	return plug->_D3C;
}

/**
 * @note Address: 0x800B2EB8
 * @note Size: 0x18
 */
extern "C" void* GetMapPoolEntry(const CGame* game, s32 index)
{
	s32 stride  = game->mXObjs.mPools[3].mStride;
	u8* entries = game->mXObjs.mPools[3].mObjects;
	return entries + index * stride;
}

/**
 * @note Address: 0x800B2ED0
 * @note Size: 0x8
 */
extern "C" void SetObjectVerticalSpeed(CObjVec* value, f32 y)
{
	value->mSpeed.y = y;
}

/**
 * @note Address: 0x800B2ED8
 * @note Size: 0x28
 */
extern "C" void SetObjectVectorAndTargetFromVec(CObjVec* value, const Vec& v)
{
	f32 x = v.x, y = v.y, z = v.z;
	value->x         = x;
	value->y         = y;
	value->z         = z;
	value->mTarget.x = x;
	value->mTarget.y = y;
	value->mTarget.z = z;
}

/**
 * @note Address: 0x800B2F00
 * @note Size: 0xC
 */
extern "C" u32 GetHeldPadButtons(const CPadOne* pad, u32 mask)
{
	return pad->mButton & mask;
}

/**
 * @note Address: 0x800B2F0C
 * @note Size: 0x14
 */
extern "C" CPadOne* GetPadForPort(CGame* game, u8 port)
{
	CPadOne* pads = game->mPad.mPad;
	return &pads[port];
}

/**
 * @note Address: 0x800B2F20
 * @note Size: 0xC
 */
extern "C" void SetObjectZAndTarget(CObjVec* value, f32 z)
{
	value->z         = z;
	value->mTarget.z = z;
}

/**
 * @note Address: 0x800B2F2C
 * @note Size: 0xC
 */
extern "C" void SetObjectXAndTarget(CObjVec* value, f32 x)
{
	value->x         = x;
	value->mTarget.x = x;
}

/**
 * @note Address: 0x800B2F38
 * @note Size: 0x14
 */
extern "C" BOOL IsLiftActionState(const XActLift* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B2F4C
 * @note Size: 0x28
 */
extern "C" BOOL IsPlayerAttachmentEquipped(const XObjChr* chr, s32 wanted)
{
	return (u8)(wanted == chr->GetAttachment());
}

/**
 * @note Address: 0x800B2F74
 * @note Size: 0x18
 */
extern "C" BOOL IsEventScriptActive(const CGame* game)
{
	return game->mEvt.mData.mScript != NULL;
}

/**
 * @note Address: 0x800B2F8C
 * @note Size: 0x14
 */
extern "C" BOOL IsTumbleActionState(const XActTumble* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B2FA0
 * @note Size: 0x20
 */
extern "C" f32 GetVectorDistance(const Vec* a, const Vec* b)
{
	return PSVECDistance((Vec*)a, (Vec*)b);
}

/**
 * @note Address: 0x800B2FC0
 * @note Size: 0x4
 */
extern "C" Vec* fn_800B2FC0(CObjVec* value)
{
	return value;
}

/**
 * @note Address: 0x800B2FC4
 * @note Size: 0x14
 */
extern "C" u64 GetMaskedGameStatus(const CGame* game, u64 mask)
{
	return game->mStatus.mFlags & mask;
}

/**
 * @note Address: 0x800B2FD8
 * @note Size: 0xC
 */
extern "C" void SetObjectYAndTarget(CObjVec* value, f32 y)
{
	value->y         = y;
	value->mTarget.y = y;
}

/**
 * @note Address: 0x800B2FE4
 * @note Size: 0x8
 */
extern "C" s32 GetObjectAnimationId(const CXObj* object)
{
	return object->mAnimId;
}

/**
 * @note Address: 0x800B2FEC
 * @note Size: 0x1C
 */
extern "C" void SetObjectVectorAndTargetXYZ(CObjVec* value, f32 x, f32 y, f32 z)
{
	value->x         = x;
	value->y         = y;
	value->z         = z;
	value->mTarget.x = x;
	value->mTarget.y = y;
	value->mTarget.z = z;
}

/**
 * @note Address: 0x800B3008
 * @note Size: 0x8
 */
extern "C" void SetVectorZ(CVec* value, f32 z)
{
	value->z = z;
}

/**
 * @note Address: 0x800B3010
 * @note Size: 0x8
 */
extern "C" void SetVectorX(CVec* value, f32 x)
{
	value->x = x;
}

/**
 * @note Address: 0x800B3018
 * @note Size: 0x34
 */
extern "C" f32 WrapPlayerActionAngle(f32 angle)
{
	angle = WrapAngle(angle);
	return angle;
}

/**
 * @note Address: 0x800B304C
 * @note Size: 0x8
 */
extern "C" u32 fn_800B304C(const XAction* action)
{
	return action->_0C;
}

/**
 * @note Address: 0x800B3054
 * @note Size: 0x8
 */
extern "C" f32 fn_800B3054(const XActHang* act)
{
	return act->_48;
}

/**
 * @note Address: 0x800B305C
 * @note Size: 0x8
 */
extern "C" Vec* fn_800B305C(XActHang* act)
{
	return &act->_3C;
}

/**
 * @note Address: 0x800B3064
 * @note Size: 0x24
 */
extern "C" BOOL IsActionTargetFloorId(const XAction* action, s32 wanted)
{
	return (u8)(action->mTarget->mFloors[action->mTargetFloor].mId == wanted);
}

/**
 * @note Address: 0x800B3088
 * @note Size: 0x8
 */
extern "C" void SetObjectVectorSpeedZ(CObjVec* value, f32 z)
{
	value->mSpeed.z = z;
}

/**
 * @note Address: 0x800B3090
 * @note Size: 0x8
 */
extern "C" void SetObjectVectorSpeedX(CObjVec* value, f32 x)
{
	value->mSpeed.x = x;
}

/**
 * @note Address: 0x800B3098
 * @note Size: 0x8
 */
extern "C" CVec* GetObjectVectorSpeed(CObjVec* value)
{
	return &value->mSpeed;
}

/**
 * @note Address: 0x800B30A0
 * @note Size: 0x20
 */
extern "C" f32 GetVectorMagnitude(const Vec* value)
{
	return PSVECMag((Vec*)value);
}

/**
 * @note Address: 0x800B30C0
 * @note Size: 0x8
 */
extern "C" CVec* GetObjectVectorTarget(CObjVec* value)
{
	return &value->mTarget;
}

/**
 * @note Address: 0x800B30C8
 * @note Size: 0x8
 */
extern "C" void SetObjectVectorTargetZ(CObjVec* value, f32 z)
{
	value->mTarget.z = z;
}

/**
 * @note Address: 0x800B30D0
 * @note Size: 0x8
 */
extern "C" void SetObjectVectorTargetX(CObjVec* value, f32 x)
{
	value->mTarget.x = x;
}

/**
 * @note Address: 0x800B30D8
 * @note Size: 0x24
 */
extern "C" f32 PlayerActionCosine(f32 angle)
{
	return cos(angle);
}

/**
 * @note Address: 0x800B30FC
 * @note Size: 0x8
 */
extern "C" f32 fn_800B30FC(const XObjChr* chr)
{
	return chr->_A40;
}

/**
 * @note Address: 0x800B3104
 * @note Size: 0x24
 */
extern "C" f32 PlayerActionSine(f32 angle)
{
	return sin(angle);
}

/**
 * @note Address: 0x800B3128
 * @note Size: 0x8
 */
extern "C" XObj* GetPlayerActionTarget(const XAction* action)
{
	return action->mTarget;
}

/**
 * @note Address: 0x800B3130
 * @note Size: 0x14
 */
extern "C" BOOL IsHangActionState(const XActHang* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B3144
 * @note Size: 0x8
 */
extern "C" CVec* GetObjectPreviousMoveTranslation(XObj* object)
{
	return &object->_264;
}

/**
 * @note Address: 0x800B314C
 * @note Size: 0x8
 */
extern "C" CObjVec* GetObjectMoveTranslation(CXObj* object)
{
	return &object->mMoveTranslate;
}

/**
 * @note Address: 0x800B3154
 * @note Size: 0x14
 */
extern "C" XObj* GetGameObjectByIndex(const CGame* game, s32 index)
{
	return game->mXObjs.mObjects[index];
}

/**
 * @note Address: 0x800B3168
 * @note Size: 0x14
 */
extern "C" BOOL HasPlayerActionTarget(const XAction* action)
{
	return action->mTarget != NULL;
}

/**
 * @note Address: 0x800B317C
 * @note Size: 0x14
 */
extern "C" BOOL IsPushActionState(const XActPush* act, s32 wanted)
{
	return (u8)(wanted == act->mState);
}

/**
 * @note Address: 0x800B3190
 * @note Size: 0x1C
 */
extern "C" CVec __ct__4CVecFfff(f32 x, f32 y, f32 z)
{
	return CVec(x, y, z);
}

/**
 * @note Address: 0x800B31AC
 * @note Size: 0xC
 */
extern "C" u32 fn_800B31AC(const XObjChr* chr, u32 mask)
{
	return chr->_980.mFlags & mask;
}

// switches to the attachment he asked for, he stands still meanwhile
/**
 * @note Address: 0x800B31B8
 * @note Size: 0x414
 */
extern "C" void UpdatePlayerAttachmentRequest(XObjChr* chr, f32* speed)
{
	if (chr->mAttachmentRequest >= 0) {
		switch (chr->_95D8) {
		case 0:
			if (IsLiftDone(chr)) {
				if (chr->mLift.mTarget != NULL) {
					chr->mLift.mTarget->mStatus.mFlags &= ~0x0400000000000000ULL;
					chr->mLift.mTarget = NULL;
				}
				chr->mLift.mState = 0;
			}
			switch (chr->mAttachment) {
			case 1:
				chr->mCopter.mState = 2;
				break;
			case 2:
				chr->mPopper.mState = 9;
				ResetPopperCharge(&chr->mPopper.mCharge);
				break;
			case 3:
				chr->mRadar.mState = 2;
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				chr->mLift.mState = 19;
				break;
			default:
				if (IsCarryingToothbrush(&chr->mLift) || IsCarryingSpoon(&chr->mLift) || IsCarryingMug(&chr->mLift)
				    || IsCarryingSquirter(&chr->mLift))
					chr->mLift.mState = 19;
				break;
			}
			chr->_95D8 = 10;
		case 10:
			if (chr->mCopter.mState != 0 || chr->mPopper.mState != 0 || chr->mRadar.mState != 0 || chr->mLift.mState != 0)
				break;
			if (chr->mAttachmentAnim.mLoading == 0) {
				chr->mAttachment = chr->mAttachmentRequest;
				chr->_95D8       = 15;
				chr->mAttachmentAnim.Release();
			}
		case 15: {
			BOOL done;
			if (sAttachmentAnims[chr->mAttachment] != NULL) {
				if (LoadJointAnimationAsync(&chr->mAttachmentAnim, sAttachmentAnims[chr->mAttachment])) {
					chr->SetAnim(17, TRUE, NULL, NULL, NULL, 0.0f, 1.0f, 1.0f, 10.0f);
					done = TRUE;
				} else {
					done = FALSE;
				}
			} else {
				done = TRUE;
			}
			if (!done)
				break;
			chr->_95D8 = 20;
		}
		case 20:
			switch (chr->mAttachmentRequest) {
			case 1:
				chr->mCopter.mState = 1;
				break;
			case 2:
				chr->mPopper.mState = 1;
				break;
			case 3:
				chr->mRadar.mState = 1;
				break;
			case 4:
				TakeOutTool(chr, 0x10);
				break;
			case 5:
				TakeOutTool(chr, 0x15);
				break;
			case 6:
				TakeOutTool(chr, 0x16);
				break;
			case 7:
				TakeOutTool(chr, 0x38);
				break;
			case 8:
				chr->mAttachment = 0;
				break;
			}
			chr->_95D8 = 30;
			chr->_95DC = 0;
			break;
		case 30:
			if (chr->mCopter.mState == 0 && chr->mPopper.mState == 0 && chr->mRadar.mState == 0) {
				chr->mAttachmentRequest = -1;
				chr->_95D8              = 0;
				chr->_95DC              = 0;
				chr->_95E0              = chr->_95E4;
				chr->_95E4              = 0;
			}
			break;
		}
		*speed = 0.0f;
	}
}

/**
 * @note Address: 0x800B4F28
 * @note Size: 0x40
 */
extern "C" BOOL VectorsDiffer(const Vec* a, const Vec* b)
{
	BOOL different = FALSE;
	if (a->x != b->x || a->y != b->y || a->z != b->z)
		different = TRUE;
	return different;
}

/**
 * @note Address: 0x800B4F68
 * @note Size: 0xB4
 */
extern "C" Vec GetJointMatrixTranslation(void*, HSD_JObj* joint)
{
	Vec position;
	HSD_MtxGetTranslate(HSD_JObjGetMtxPtr(joint), &position);
	return position;
}

/**
 * @note Address: 0x800B501C
 * @note Size: 0x8
 */
extern "C" void SetRadarActionState(XActRadar* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B5024
 * @note Size: 0x8
 */
extern "C" void SetPopperActionState(XActPopper* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B502C
 * @note Size: 0x8
 */
extern "C" void SetRopeActionState(XActRope* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B5034
 * @note Size: 0x8
 */
extern "C" void SetLadderActionState(XActLadder* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B503C
 * @note Size: 0x8
 */
extern "C" void fn_800B503C(XActLift* act, s32 value)
{
	act->_24 = value;
}

/**
 * @note Address: 0x800B5044
 * @note Size: 0x8
 */
extern "C" void SetLiftActionState(XActLift* act, s32 state)
{
	act->mState = state;
}

/**
 * @note Address: 0x800B504C
 * @note Size: 0x130
 */
// NONMATCHING: regalloc
extern "C" void TryStartObjectEvent(CGame* game, const void* script, void* context, s16 first, s16 second)
{
	if (!IsEventBlocked(game->GetPlayer())) {
		StartOrQueueEventScript(&game->mEvt, (u8*)script, (u32)context, first, second);
	}
}

/**
 * @note Address: 0x800B517C
 * @note Size: 0x80
 */
extern "C" BOOL IsGameRunning(CGame* game)
{
	return game->IsRunning();
}

/**
 * @note Address: 0x800B51FC
 * @note Size: 0x1A4
 */
// opens the yes/no marks
extern "C" void OpenPlayerYesNoMenu(XObjChr* chr, s32 first, s32 second)
{
	if (!chr->mOXMenu.mStarted) {
		chr->mOXMenu.mStarted       = TRUE;
		chr->mOXMenu.mCrossMask     = first;
		chr->mOXMenu.mStartRot      = chr->mOXMenu.mChr->mMoveRotate.y;
		chr->mOXMenu.mSelectedIndex = second;
		SpinMarks(*chr);
		PlayGameSound(&gGame.mSound, 0x86, 255);
		chr->mOXMenu.mChr->SetAnim(0x5D, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		gGame.mStatus.mFlags |= 0x800ULL;
	}
}

/**
 * @note Address: 0x800B53A0
 * @note Size: 0x104
 */
extern "C" void ClosePlayerEquipmentMenu(XObjChr* chr, s32 dir)
{
	ClosePlayerEquipmentRings(&chr->mMenu, dir);
}

/**
 * @note Address: 0x800B54A4
 * @note Size: 0xC8
 */
extern "C" void SelectPlayerEquipmentRing(XObjChr* chr, s32 selection)
{
	if (selection >= 0) {
		chr->mMenu.mPrevSel = chr->mMenu.mSel;
		chr->mMenu.mSel     = selection;
	}
	if (IsMenuUp(chr)) {
		FocusEquipmentRing(&chr->mMenu);
	} else {
		OpenPlayerEquipmentRings(chr);
	}
}

/**
 * @note Address: 0x800B556C
 * @note Size: 0x20
 */
extern "C" void RequestPlayerUnequipAttachment(XObjChr* chr)
{
	if (chr->mAttachment != 0) {
		chr->mAttachmentRequest = 0;
		chr->_95D8              = 0;
	}
}

/**
 * @note Address: 0x800B558C
 * @note Size: 0x27C
 */
// stops him and brings up the attachment and costume rings
extern "C" void OpenPlayerEquipmentRings(XObjChr* chr)
{
	if (chr->mAttachmentRequest != -1)
		return;
	if (chr->mSuits.mChangeStep != 0)
		return;
	if (!chr->mMenu.IsOpen()) {
		StopMoving(chr);
		OpenMenus(chr);
	} else if (chr->mMenu.IsClosing()) {
		OpenMenus(chr);
	}
}

/**
 * @note Address: 0x800B5808
 * @note Size: 0x4
 */
extern "C" void vfE0__7XObjChrFf()
{
}

/**
 * @note Address: 0x800B580C
 * @note Size: 0x168
 */
extern "C" void vfDC__7XObjChrFiUc(XObjChr* chr, BOOL enabled, u8 value)
{
	vfDC__4XObjFiUc(chr, enabled, value);
	if (chr->mPlug.mJObj != NULL)
		chr->mPlug.vfDC(enabled, value);
	if (chr->mCopterObj.mJObj != NULL)
		chr->mCopterObj.vfDC(enabled, value);
	if (chr->mPopperObj.mJObj != NULL)
		chr->mPopperObj.vfDC(enabled, value);
	if (chr->mRadarObj.mJObj != NULL)
		chr->mRadarObj.vfDC(enabled, value);
	if (chr->mOXObj.mJObj != NULL)
		chr->mOXObj.vfDC(enabled, value);
	for (s32 i = 0; i < 9; ++i) {
		if (chr->mSuits.mObjects[i].mJObj != NULL)
			chr->mSuits.mObjects[i].vfDC(enabled, value);
	}
}

/**
 * @note Address: 0x800B5974
 * @note Size: 0x144
 */
extern "C" void vfD8__7XObjChrFPUl(XObjChr* chr, u32* flags)
{
	vfD8__4XObjFPUl(chr, flags);
	if (chr->mPlug.mJObj != NULL)
		chr->mPlug.vfD8(flags);
	if (chr->mCopterObj.mJObj != NULL)
		chr->mCopterObj.vfD8(flags);
	if (chr->mPopperObj.mJObj != NULL)
		chr->mPopperObj.vfD8(flags);
	if (chr->mRadarObj.mJObj != NULL)
		chr->mRadarObj.vfD8(flags);
	if (chr->mOXObj.mJObj != NULL)
		chr->mOXObj.vfD8(flags);
	for (s32 i = 0; i < 9; ++i) {
		if (chr->mSuits.mObjects[i].mJObj != NULL)
			chr->mSuits.mObjects[i].vfD8(flags);
	}
}

/**
 * @note Address: 0x800B5AB8
 * @note Size: 0xC4
 */
extern "C" void vf120__7XObjChrFl(XObjChr* chr, s32 mode)
{
	vf120__4XObjFl(chr, mode);
	chr->mPlug.vf120(mode);
	chr->mCopterObj.vf120(mode);
	chr->mPopperObj.vf120(mode);
	chr->mRadarObj.vf120(mode);
	chr->mOXObj.vf120(mode);
}

/**
 * @note Address: 0x800B5B7C
 * @note Size: 0xC0
 */
extern "C" void SpawnPlayerPointParticles(XObjChr*, s32 type, s32 value)
{
	for (s32 i = 0; i < 10; ++i) {
		if (gPointParticlePool[i].IsFree()) {
			for (s32 j = 0; j < 10; ++j) {
				if (gPointParticleSlots[j] == NULL) {
					gPointParticleSlots[j] = &gPointParticlePool[i];
					StartPointParticles(gPointParticleSlots[j], type, value);
					return;
				}
			}
			return;
		}
	}
}

/**
 * @note Address: 0x800B5C3C
 * @note Size: 0x1E0
 */
extern "C" void UpdatePlayerHeadOffset(XObjChr* chr, u32 frames)
{
	if (chr->mHeadOffsetReady == 0 && gGame.mEvt.mData.mScript == NULL)
		chr->mHeadOffsetReady = 1;
	f32 current, target, rate;
	if (chr->mPush.mState == 1 || chr->mPush.mState == 2) {
		if (chr->mPush._0C == 1)
			target = 8.0f;
		else if (chr->mPush._0C == -1)
			target = -5.0f;
		else
			target = 0.0f;
		rate = 0.4f;
	} else if (gGame.mEvt.mData.mScript != NULL && chr->mHeadOffsetReady != 0) {
		target = 0.0f;
		rate   = 0.2f;
	} else if (IsRopeState(chr, 1) || IsRopeState(chr, 2) || IsRopeState(chr, 3) || IsRopeState(chr, 4) || IsRopeState(chr, 5)
	           || IsRopeState(chr, 6) || IsRopeState(chr, 7) || IsRopeState(chr, 8)) {
		target = 0.0f;
		rate   = 0.2f;
	} else {
		target = 5.0f;
		rate   = 0.2f;
	}
	if (__fpclassifyf(target) != FP_NAN) {
		current = chr->mHeadOffset;
		if (current > target) {
			const f32 next   = current - rate * frames;
			chr->mHeadOffset = next;
			if (next < target)
				chr->mHeadOffset = target;
		} else if (current < target) {
			const f32 next   = current + rate * frames;
			chr->mHeadOffset = next;
			if (next > target)
				chr->mHeadOffset = target;
		}
	}
}

/**
 * @note Address: 0x800B5E1C
 * @note Size: 0x18
 */
extern "C" void ResetPlayerHeadOffset(XObjChr* chr)
{
	chr->mHeadOffset      = 5.0f;
	chr->mHeadOffsetReady = 0;
}

/**
 * @note Address: 0x8009F7D0
 * @note Size: 0x490
 */
extern "C" void SpawnPlayerDust(XObjChr* player, const Vec* origin)
{
	f32 angle = DEG2RAD * (rand() % 360);
	f32 vx    = (rand() % 100) * (f32)sin(angle) / 1000.0f;
	f32 vz    = (f32)cos(angle) * (rand() % 100) / 1000.0f;
	f32 vy    = (rand() % 100) / 800.0f;
	Vec position;
	position.x = origin->x + 16.0f * (vx * (rand() % 100) / 100.0f);
	position.z = origin->z + 16.0f * (vz * (rand() % 100) / 100.0f);
	position.y = origin->y;
	f32 width  = 1.0f + (rand() % 200) / 200.0f;
	Vec size;
	size.x         = width;
	size.y         = width;
	size.z         = 1.0f;
	Vec velocity   = { vx, vy, vz };
	CPtclOne* dust = SpawnPoolParticle(&player->mDust.mPtcl, &position, &size, &velocity, 0xFFFFFFA0);
	if (dust != NULL) {
		if (player->_158C0 == 10) {
			dust->mAngle      = DEG2RAD * (rand() % 360);
			dust->mAngleSpeed = DEG2RAD * (10 - rand() % 20);
			dust->mColor      = 0xFFFFFFC8;
			dust->mTex        = &player->mTex[1];
		} else {
			dust->mAngle      = DEG2RAD * (rand() % 360);
			dust->mAngleSpeed = DEG2RAD * (20 - rand() % 40);
			dust->mColor      = 0xFFFFFFA0;
			dust->mTex        = &player->mTex[0];
		}
	}
}
