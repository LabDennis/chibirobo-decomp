#if defined(VERSION_R24J01)
#include "game/wii/cgame.h"
#include "rel/stage01/hamburg.h"

extern "C" s32 InitAnas(void);
extern "C" void UpdateAnas(u32 arg);
extern "C" void UpdateHamObjs(u32 arg);
extern "C" s32 PutHeldHamObjOnDish(s32 id);
extern "C" void DrawAnas(void);
extern "C" void ExitAnas(void);
extern "C" void ResetYakiRates(void);
extern "C" void InitHamburg(void);
extern "C" void ExitHamburg(void);
extern "C" HamObj* FindHamObj(s32 id);
extern "C" HamObj* RegisterHamObj(s32 id);
extern "C" HamObj* FindFacingHamburg(void);
extern "C" HamDish* RegisterHamDish(s32 id);
extern "C" HamDish* FindHamDish(s32 id);
extern "C" s32 CountHamDishObjs(HamDish* group);
extern "C" HamObj* FindFreeHamObj(void);
extern "C" HamObj* FindHamObjByActor(XObj* actor);
extern "C" void HamObj_Respawn(HamObj* slot);
extern "C" void HamObj_StartFlip(HamObj* slot, u32 frames);
extern "C" void HamObj_SetRot(HamObj* slot, f32 angle);
extern "C" void HamObj_StartSlide(HamObj* slot, f32 distance, f32 speed, f32 angle);
extern "C" void HamObj_StartPutDown(HamObj* slot);
extern "C" void AddTempActor(s32 id);
extern "C" void ClearTempActors(void);
extern "C" void fn_80170EE4(u8* data);
extern "C" void PlayYakiSound(void);
extern "C" s32 CheckYakiResult(void);
extern "C" f64 atan2(f64 y, f64 x);
extern "C" f32 GetSoundVolumeAtPosition(XSound* sound, const Vec* pos, s32 group);

extern "C" s32 InitStage(u32 arg)
{
	ResetYakiRates();
	InitAnas();
	return 1;
}

extern "C" s32 UpdateStage(u32 arg)
{
	Vec pos;
	UpdateHamObjs(arg);
	UpdateAnas(arg);
	pos.x = -92.52f;
	pos.y = 75.87f;
	pos.z = -165.98f;
	if (IsWiiSfxPlaying(&gGame.mSound, 0x28E)) {
		SetWiiSfxVolume(&gGame.mSound, 0x28E, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 3), 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x28F)) {
		SetWiiSfxVolume(&gGame.mSound, 0x28F, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 3), 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x290)) {
		SetWiiSfxVolume(&gGame.mSound, 0x290, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 3), 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x291)) {
		SetWiiSfxVolume(&gGame.mSound, 0x291, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 3), 0.0f);
	}
	pos.x = -281.1f;
	pos.y = 76.14f;
	pos.z = 110.23f;
	if (IsWiiSfxPlaying(&gGame.mSound, 0x286)) {
		SetWiiSfxVolume(&gGame.mSound, 0x286, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 2), 0.0f);
	}
	if (IsWiiSfxPlaying(&gGame.mSound, 0x288)) {
		SetWiiSfxVolume(&gGame.mSound, 0x288, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 2), 0.0f);
	}
	return 1;
}

extern "C" s32 DrawStageHud(u32 arg)
{
	return 1;
}

extern "C" s32 DrawStage(u32 arg)
{
	DrawAnas();
	return 1;
}

extern "C" s32 ExitStage(u32 arg)
{
	ExitAnas();
	return 1;
}

extern "C" s32 fn_80177480(s32 cmd, s32 count, s32* args)
{
	return 0;
}
extern "C" s32 fn_80177488(s32 cmd, s32 count, s32* args)
{
	return 0;
}
extern "C" s32 fn_80177490(s32 cmd, s32 count, s32* args)
{
	return 0;
}
extern "C" s32 fn_80177498(s32 cmd, s32 count, s32* args)
{
	return 0;
}
extern "C" s32 fn_801774A0(s32 cmd, s32 count, s32* args)
{
	return 0;
}
extern "C" s32 fn_801774A8(s32 cmd, s32 count, s32* args)
{
	return 0;
}

extern "C" s32 CmdInitHamburg(s32 cmd, s32 count, s32* args)
{
	InitHamburg();
	return 0;
}

extern "C" s32 CmdGetHamObjKind(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		return slot->mActor->mCoord.mPlacement->mObjectId;
	}
	return -1;
}

extern "C" s32 CmdRegisterHamDishes(s32 cmd, s32 count, s32* args)
{
	for (s32 i = 0; i < count; i++) {
		RegisterHamDish(args[i]);
	}
	return 1;
}

extern "C" s32 CmdRegisterHamObjs(s32 cmd, s32 count, s32* args)
{
	for (s32 i = 0; i < count; i++) {
		RegisterHamObj(args[i]);
	}
	return 1;
}

extern "C" s32 CmdGetFacingHamburgId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindFacingHamburg();
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

extern "C" s32 CmdFlipHamburg(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		HamObj_StartFlip(slot, args[1]);
	}
	return 1;
}

extern "C" s32 CmdPushHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return 0;
	}
	CVec* actorPos  = slot->mActor->GetPos();
	CVec* playerPos = gGame.GetPlayer()->GetPos();
	f32 angle       = atan2(actorPos->x - playerPos->x, actorPos->z - playerPos->z);
	HamObj_StartSlide(slot, args[1], 0.2f, angle);
	return 1;
}

extern "C" s32 CmdRespawnHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return 0;
	}
	HamObj_Respawn(slot);
	return 1;
}

extern "C" s32 CmdGetFreeHamObjId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindFreeHamObj();
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

extern "C" s32 CmdSetHamObjRot(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return -1;
	}
	HamObj_SetRot(slot, 0.017453292f * args[1]);
	return 0;
}

extern "C" s32 CmdPutDownHamObj(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot == NULL) {
		return -1;
	}
	ClearTempActors();
	for (s32 i = 1; i < count; i++) {
		AddTempActor(args[i]);
	}
	HamObj_StartPutDown(slot);
	return 0;
}

extern "C" s32 CmdPutHeldHamObjOnDish(s32 cmd, s32 count, s32* args)
{
	return PutHeldHamObjOnDish(args[0]);
}

extern "C" s32 CmdGetYakiTime(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObj(args[0]);
	if (slot != NULL) {
		return slot->mYakiTime[args[1]];
	}
	return -1;
}

extern "C" s32 CmdGetHeldHamObjId(s32 cmd, s32 count, s32* args)
{
	HamObj* slot = FindHamObjByActor(gGame.GetPlayer()->mLift.mTarget);
	if (slot != NULL) {
		return slot->mId;
	}
	return -1;
}

extern "C" s32 CmdExitHamburg(s32 cmd, s32 count, s32* args)
{
	ExitHamburg();
	return 1;
}

extern "C" s32 CmdGetHamDishCount(s32 cmd, s32 count, s32* args)
{
	HamDish* group = FindHamDish(args[0]);
	if (group != NULL) {
		return CountHamDishObjs(group);
	}
	return 0;
}

extern "C" s32 CmdSetYakiRates(s32 cmd, s32 count, s32* args)
{
	fn_80170EE4((u8*)args[0]);
	return 0;
}

extern "C" s32 CmdPlayYakiSound(s32 cmd, s32 count, s32* args)
{
	PlayYakiSound();
	return 0;
}

extern "C" s32 CmdCheckYakiResult(s32 cmd, s32 count, s32* args)
{
	return CheckYakiResult();
}

extern "C" s32 CmdPlaySoundAtActor(s32 cmd, s32 count, s32* args)
{
	XObj* actor = gGame.mXObjs.mObjects[args[1]];
	Vec pos     = *actor->GetPos();
	s32 id      = args[0];
	PlayWiiSoundEffect(&gGame.mSound, id, GetSoundVolumeAtPosition(&gGame.mSound, &pos, 0), 0.0f);
	return 0;
}
#endif
