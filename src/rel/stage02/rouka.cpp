#include "Version.h"

// the hallway stage: its entry points and its script commands. the cars are in car.cpp. the
// functions are in the order they were written, retail's code runs the other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/ut.h"

// CGame, accessed by offset
extern "C" u8 gGame[];
#define GAME_AT(off) (gGame + (off))
// an offset into CGame: japan lays it out differently from the others
#if defined(VERSION_JP)
#define GAME_OFS(other, japan) (japan)
#else
#define GAME_OFS(other, japan) (other)
#endif

// loaded model object, vtable at 0x34
class XObj {
public:
	HSD_JObj* jobj; // _00
	u8 _04[0x30];   // _04
	virtual void vf08();
	virtual void vf0C();
	virtual void vf10();
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C();
	virtual void vf20();
	virtual void vf24();
	virtual void vf28();
	virtual void vf2C();
	virtual void vf30();
	virtual void vf34();
	virtual void vf38();
	virtual void vf3C();
	virtual void vf40();
	virtual void vf44();
	virtual void vf48();
	virtual void vf4C();
	virtual void vf50();
	virtual void vf54();
	virtual void GetFollowDivisor();
	virtual void vf5C();
	virtual void vf60();
	virtual void vf64();
	virtual void vf68();
	virtual void vf6C();
	virtual void vf70();
	virtual void vf74();
	virtual void vf78();
	virtual void vf7C();
	virtual void vf80(s32 a, s32 b, f32 c, f32 d, f32 e, f32 f, s32 g, s32 h, s32 i);
#if defined(VERSION_JP)
	u8 _38[0x88]; // _38
#else
	u8 _38[0x90]; // _38
#endif
	u8 _C8[0x90];   // _C8 (JP: _C0)
	s32 _158;       // _158 (JP: _150)
	u8 _15C[0xCC];  // _15C (JP: _154)
	u64 _228;       // _228 (JP: _220)
	u8 _230[0x8];   // _230 (JP: _228)
	s32* _238;      // _238 (JP: _230)
	u8 _23C[0x5D4]; // _23C (JP: _234)
	s32 _810;       // _810 (JP: _808)
	struct {
		s32 type; // _00
		s16 _04;  // _04
		s16 _06;  // _06
	} _814[16];   // _814 (JP: _80C)
};

class CamInfo {
public:
	CamInfo(s32* values) { m_values = values; }
	virtual ~CamInfo() { }
	void Set(s32 idx, s32 value) { m_values[idx] = value; }

	s32* m_values; // _04
};

namespace Car {

// prints "Car::Init"
void Init(void);

} // namespace Car

struct Stage02Main {
#if defined(VERSION_JP)
	u8 _00[0x9818]; // _00
#else
	u8 _00[0x9860]; // _00
#endif
	XObj* mObjects[0x400]; // _9860, _9818 in japan
	s32 mObjectCount;      // _A860

	XObj* GetObject(s32 index) const { return mObjects[index]; }
};

#define MAIN() ((Stage02Main*)gGame)

struct ObjTable {
	void* _00; // _00
};

#define OBJ_TABLE() ((ObjTable*)GAME_AT(GAME_OFS(0x5D080, 0x5D038)))
#define SOUND()     GAME_AT(GAME_OFS(0xE7E24, 0xE713C))

static XObj* takiObj;

// see game/ccameraaction.h
class CCameraActionScroll2 {
public:
	void SaveCamInfo(const void* info);
	void ClearHideList();
	BOOL AddHideObj(s32 id);
};

#define SCROLL2() (*(CCameraActionScroll2**)GAME_AT(GAME_OFS(0xE7DFC, 0xE7114)))

extern "C" {
s32 InfoScrn_GetTimerHundredths(void* p);
void InfoScrn_SetTimerHundredths(void* p, s32 v);
void InfoScrn_HideTimer(void* p);
void InfoScrn_ShowTimer(void* p);
void InfoScrn_ApplyTimerHudCommand(void* p, s32 a, s32 b);
void* ResolveEventObjectId(void* table, s16 id);
void SetJunkUnkoFlag(s32 v);
s32 IsGameSoundPlaying(void* p, s32 id);
void PlayGameSound(void* p, s32 id, s32 v);
void HSD_AudioSFXSetPanByID(s32 id, s32 v);
s32 GetSoundVolumeAtPosition(void* p, void* q, s32 v);
void SetGameSfxVolume(void* p, s32 id, s32 v, s32 w);
void StopGameSfx(void* p, s32 id);
void FadeOutGameSfx(void* p, s32 a, s32 b);

// fighter.cpp
s32 ShowCenterPanel(s32 a, s32 b);
s32 ShowLowerPanel(s32 a);
void InitPilotNwing(void* a, void* b, void* c, void* d);
s32 GetShotEvent(void);
s32 EndFighter(void);
s32 TakeDestroyedMatoId(void);
s32 GetScore(void);
u32 GetTimeLeft(void);
f32 GetFighterProgress(void);
void SetSoundCueTable(s32 v);
void SetFighterParams(s32 v);
void SetSpeedRateTable(s32 v);
void AddMato(void* p, s32 id);
void StartFighter(void* a, void* b, void* c);
void DrawFighterHud(void* p);
s32 DrawShots(void);
s32 DrawFighterStage(void);
void UpdateFighter(void* p);
void ClearFighterObjects(void);

s32 CmdGetTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetTimerRunning(s32 cmd, s32 argc, s32* argv);
s32 CmdShowTimer(s32 cmd, s32 argc, s32* argv);
s32 CmdSetCarData(s32 cmd, s32 argc, s32* argv);
s32 CmdTerminateCars(s32 cmd, s32 argc, s32* argv);
s32 CmdGetCarState(s32 cmd, s32 argc, s32* argv);
s32 CmdStartCars(s32 cmd, s32 argc, s32* argv);
s32 CmdRegisterCar(s32 cmd, s32 argc, s32* argv);
s32 CmdClearCamInfo(s32 cmd, s32 argc, s32* argv);
s32 CmdSetCamInfo(s32 cmd, s32 argc, s32* argv);
s32 CmdClearObjFlagList(s32 cmd, s32 argc, s32* argv);
s32 CmdAddObjFlagList(s32 cmd, s32 argc, s32* argv);
s32 CmdSetJunkUnkoFlag(s32 cmd, s32 argc, s32* argv);
s32 CmdShowFighterCenterPanel(s32 cmd, s32 argc, s32* argv);
s32 CmdShowFighterLowerPanel(s32 cmd, s32 argc, s32* argv);
s32 CmdRegisterFighterLink(s32 cmd, s32 argc, s32* argv);
s32 CmdGetFighterEvent(s32 cmd, s32 argc, s32* argv);
s32 CmdQueryFighter(s32 cmd, s32 argc, s32* argv);
s32 CmdSetFighterData(s32 cmd, s32 argc, s32* argv);
s32 CmdEndFighter(s32 cmd, s32 argc, s32* argv);
s32 CmdAddMato(s32 cmd, s32 argc, s32* argv);
s32 CmdStartFighter(s32 cmd, s32 argc, s32* argv);
void ExitStage(void* arg);
void DrawStageHud(void* arg);
void DrawStageEffects(void* arg);
void DrawStage(void* arg);
void UpdateStage(void* arg);
void InitStage(void* arg);
void RestoreDefaultCamera(void);
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void DrawCarHud(void* arg);
void UpdateCars(u32 arg);
void StartCars(void);
void StartCarCamera(void);
s32 RegisterCar(XObj* a, XObj* b, XObj* c);
}

inline XObj* ObjTable_Find(ObjTable* table, s16 id)
{
	return table->_00 != NULL ? (XObj*)ResolveEventObjectId(table, id) : NULL;
}

typedef void (*RelSlotFunc)(void* arg);
typedef s32 (*RelCmdFunc)(s32 cmd, s32 argc, s32* argv);

extern "C" {
RelSlotFunc _prolog_slots[7] = {
	InitStage, UpdateStage, DrawStage, DrawStageEffects, DrawStageHud, NULL, ExitStage,
};
RelCmdFunc _prolog_cmds[] = {
	CmdStartFighter,
	CmdAddMato,
	CmdEndFighter,
	CmdSetFighterData,
	CmdQueryFighter,
	CmdAddObjFlagList,
	CmdClearObjFlagList,
	CmdSetCamInfo,
	CmdClearCamInfo,
	CmdGetFighterEvent,
	CmdRegisterFighterLink,
	CmdRegisterCar,
	CmdStartCars,
	CmdGetCarState,
	CmdTerminateCars,
	CmdShowTimer,
	CmdSetTimerRunning,
	CmdSetTimer,
	CmdGetTimer,
	CmdSetCarData,
	CmdShowFighterLowerPanel,
	CmdShowFighterCenterPanel,
	CmdSetJunkUnkoFlag,
};
}

/**
 * @note Address: stage02 .text 0x6DAC
 * @note Size: 0xB0
 */
extern "C" void InitStage(void* arg)
{
	XObj* obj;
	s32 count;
	s32 i;

	ClearFighterObjects();
	Car::Init();
	takiObj = NULL;
	count   = MAIN()->mObjectCount;
	for (i = 0; takiObj == NULL && i < count; i++) {
		obj = MAIN()->GetObject(i);
		if ((obj->_228 & 1) != 0 && obj->_238[0] == 0x254) {
			takiObj = obj;
		}
	}
}

// InitStage
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    stw      r0, 0x14(r1)
    bl       ClearFighterObjects
    bl       Init__3CarFv
    lis      r3, gGame@ha
    lis      r4, takiObj@ha
    addi     r8, r3, gGame@l
    li       r0, 0x0
    addi     r6, r4, takiObj@l
    li       r10, 0x0
    addis    r3, r8, 0x1
    stw      r0, 0x0(r6)
    lwz      r9, -0x57a0(r3)
    b        .L_00006E38
.L_00006DE8:
    addis    r4, r8, 0x1
    li       r3, 0x1
    lwz      r7, -0x67a0(r4)
    li       r4, 0x0
    lwz      r0, 0x228(r7)
    lwz      r5, 0x22c(r7)
    and      r0, r0, r4
    and      r3, r5, r3
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_00006E30
    lwz      r3, 0x238(r7)
    lwz      r0, 0x0(r3)
    cmpwi    r0, 0x254
    bne      .L_00006E30
    lis      r3, takiObj@ha
    stw      r7, takiObj@l(r3)
.L_00006E30:
    addi     r8, r8, 0x4
    addi     r10, r10, 0x1
.L_00006E38:
    lwz      r0, 0x0(r6)
    cmplwi   r0, 0x0
    bne      .L_00006E4C
    cmpw     r10, r9
    blt      .L_00006DE8
.L_00006E4C:
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x6C34
 * @note Size: 0x178
 */
extern "C" void UpdateStage(void* arg)
{
	UpdateFighter(arg);
	UpdateCars((u32)arg);
	if (takiObj != NULL) {
		if (takiObj->_158 == 0) {
			void* sound = SOUND();
			if (IsGameSoundPlaying(sound, 0x20019) == 0) {
				PlayGameSound(sound, 0x20019, 0xFF);
				HSD_AudioSFXSetPanByID(0x20019, 0);
			}
			if (IsGameSoundPlaying(sound, 0x2001A) == 0) {
				PlayGameSound(sound, 0x2001A, 0xFF);
				HSD_AudioSFXSetPanByID(0x2001A, 0xFE);
			}
			SetGameSfxVolume(sound, 0x20019, GetSoundVolumeAtPosition(sound, takiObj->_C8, 1), 0);
			SetGameSfxVolume(sound, 0x2001A, GetSoundVolumeAtPosition(sound, takiObj->_C8, 1), 0);
		} else {
			void* sound = SOUND();
			StopGameSfx(sound, 0x20019);
			StopGameSfx(sound, 0x2001A);
		}
	}
}

/**
 * @note Address: stage02 .text 0x6C14
 * @note Size: 0x20
 */
extern "C" void DrawStage(void* arg)
{
	DrawFighterStage();
}

/**
 * @note Address: stage02 .text 0x6BF4
 * @note Size: 0x20
 */
extern "C" void DrawStageEffects(void* arg)
{
	DrawShots();
}

/**
 * @note Address: stage02 .text 0x6BC0
 * @note Size: 0x34
 */
extern "C" void DrawStageHud(void* arg)
{
	DrawFighterHud(arg);
	DrawCarHud(arg);
}

/**
 * @note Address: stage02 .text 0x6B9C
 * @note Size: 0x24
 */
extern "C" void ExitStage(void* arg)
{
	EndFighter();
	RestoreDefaultCamera();
}

/**
 * @note Address: stage02 .text 0x6AA0
 * @note Size: 0xFC
 */
// NONMATCHING: europe's empty loop compares i with argc where this tests argc alone
extern "C" s32 CmdStartFighter(s32 cmd, s32 argc, s32* argv)
{
	XObj* objs[3];
	s32 i;
	u32 j;

// europe has the loop without its prints
#if !defined(VERSION_PAL)
	OSReport("args = ");
#endif
	for (i = 0; i < argc; i++) {
#if !defined(VERSION_PAL)
		OSReport("%d ", argv[i]);
#endif
	}
#if !defined(VERSION_PAL)
	OSReport("\n");
#endif
	for (j = 0; j < 3 && (s32)j - 1 < argc; j++) {
		objs[j] = ObjTable_Find(OBJ_TABLE(), argv[j]);
	}
	StartFighter(objs[0], objs[1], objs[2]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6A20
 * @note Size: 0x80
 */
extern "C" s32 CmdAddMato(s32 cmd, s32 argc, s32* argv)
{
	s32 i;
	for (i = 0; i < argc; i++) {
		AddMato(MAIN()->mObjects[argv[i]], argv[i]);
	}
	return 0;
}

// CmdAddMato
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r3, gGame@ha
    stw      r0, 0x24(r1)
    stw      r31, 0x1c(r1)
    addi     r31, r3, gGame@l
    stw      r30, 0x18(r1)
    mr       r30, r5
    stw      r29, 0x14(r1)
    li       r29, 0x0
    stw      r28, 0x10(r1)
    mr       r28, r4
    b        .L_00006A74
.L_00006A54:
    lwz      r4, 0x0(r30)
    slwi     r0, r4, 2
    add      r3, r31, r0
    addis    r3, r3, 0x1
    lwz      r3, -0x67a0(r3)
    bl       AddMato
    addi     r30, r30, 0x4
    addi     r29, r29, 0x1
.L_00006A74:
    cmpw     r29, r28
    blt      .L_00006A54
    lwz      r0, 0x24(r1)
    li       r3, 0x0
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r29, 0x14(r1)
    lwz      r28, 0x10(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x69FC
 * @note Size: 0x24
 */
extern "C" s32 CmdEndFighter(s32 cmd, s32 argc, s32* argv)
{
	EndFighter();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6994
 * @note Size: 0x68
 */
extern "C" s32 CmdSetFighterData(s32 cmd, s32 argc, s32* argv)
{
	switch (argv[0]) {
	case 0:
		SetSpeedRateTable(argv[1]);
		SetFighterParams(argv[2]);
		if (argc >= 4) {
			SetSoundCueTable(argv[3]);
		}
		break;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x68A8
 * @note Size: 0xEC
 */
extern "C" s32 CmdQueryFighter(s32 cmd, s32 argc, s32* argv)
{
	s32 percent = GetFighterProgress() * 100.0f;
	u32 flag    = GetTimeLeft();

	switch (argv[0]) {
	case 0:
		if (percent < 100 && flag != 0) {
			return 1;
		}
		return 0;
	case 1:
		return GetScore();
		break;
	case 2:
		return flag;
	case 3:
		return percent * 100;
	case 4:
		return TakeDestroyedMatoId();
	}
	if (argv[0] == 0) {
		return GetFighterProgress() * 100.0f;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6888
 * @note Size: 0x20
 */
extern "C" s32 CmdGetFighterEvent(s32 cmd, s32 argc, s32* argv)
{
	return GetShotEvent();
}

/**
 * @note Address: stage02 .text 0x67CC
 * @note Size: 0xBC
 */
extern "C" s32 CmdRegisterFighterLink(s32 cmd, s32 argc, s32* argv)
{
	XObj* objs[4];
	u32 i;

	for (i = 0; i < 4; i++) {
		objs[i] = NULL;
	}
	for (i = 0; i < 4 && (s32)i - 1 < argc; i++) {
		objs[i] = ObjTable_Find(OBJ_TABLE(), argv[i]);
	}
	InitPilotNwing(objs[0], objs[1], objs[2], objs[3]);
	return 0;
}

// CmdRegisterFighterLink
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    lis      r3, gGame@ha
    stw      r0, 0x34(r1)
    li       r0, 0x0
    addi     r3, r3, gGame@l
    stmw     r27, 0x1c(r1)
    addis    r30, r3, 0x6
    addi     r29, r1, 0x8
    mr       r27, r4
    stw      r0, 0x8(r1)
    mr       r31, r5
    li       r28, 0x0
    subi     r30, r30, 0x2f80
    stw      r0, 0xc(r1)
    stw      r0, 0x10(r1)
    stw      r0, 0x14(r1)
    b        .L_00006848
.L_00006814:
    lwz      r0, 0x0(r30)
    lwz      r3, 0x0(r31)
    cmplwi   r0, 0x0
    extsh    r4, r3
    beq      .L_00006834
    mr       r3, r30
    bl       ResolveEventObjectId
    b        .L_00006838
.L_00006834:
    li       r3, 0x0
.L_00006838:
    stw      r3, 0x0(r29)
    addi     r31, r31, 0x4
    addi     r29, r29, 0x4
    addi     r28, r28, 0x1
.L_00006848:
    cmplwi   r28, 0x4
    bge      .L_0000685C
    subi     r0, r28, 0x1
    cmpw     r0, r27
    blt      .L_00006814
.L_0000685C:
    lwz      r3, 0x8(r1)
    lwz      r4, 0xc(r1)
    lwz      r5, 0x10(r1)
    lwz      r6, 0x14(r1)
    bl       InitPilotNwing
    lmw      r27, 0x1c(r1)
    li       r3, 0x0
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage02 .text 0x678C
 * @note Size: 0x40
 */
extern "C" s32 CmdShowFighterLowerPanel(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		ShowLowerPanel(1);
	} else {
		ShowLowerPanel(0);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6744
 * @note Size: 0x48
 */
extern "C" s32 CmdShowFighterCenterPanel(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		ShowCenterPanel(1, argv[1]);
	} else {
		ShowCenterPanel(0, 0);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6704
 * @note Size: 0x40
 */
extern "C" s32 CmdSetJunkUnkoFlag(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		SetJunkUnkoFlag(1);
	} else {
		SetJunkUnkoFlag(0);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x668C
 * @note Size: 0x78
 */
extern "C" s32 CmdAddObjFlagList(s32 cmd, s32 argc, s32* argv)
{
	s32 i;
	for (i = 0; i < argc; i++) {
		SCROLL2()->AddHideObj(argv[i]);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6658
 * @note Size: 0x34
 */
extern "C" s32 CmdClearObjFlagList(s32 cmd, s32 argc, s32* argv)
{
	SCROLL2()->ClearHideList();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6610
 * @note Size: 0x48
 */
// fn_5_6610

/**
 * @note Address: stage02 .text 0x64BC
 * @note Size: 0x154
 */
extern "C" s32 CmdSetCamInfo(s32 cmd, s32 argc, s32* argv)
{
	s32 info[8];
	{
		CamInfo c(info);
		c.Set(1, 0);
	}
	{
		CamInfo c(info);
		c.Set(2, 79300);
	}
	{
		CamInfo c(info);
		c.Set(3, 0);
	}
	{
		CamInfo c(info);
		c.Set(4, 1);
	}
	{
		CamInfo c(info);
		c.Set(5, 10000);
	}
	{
		CamInfo c(info);
		c.Set(6, 0);
	}
	{
		CamInfo c(info);
		c.Set(7, 6000);
	}
	SCROLL2()->SaveCamInfo(info);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6484
 * @note Size: 0x38
 */
extern "C" s32 CmdClearCamInfo(s32 cmd, s32 argc, s32* argv)
{
	SCROLL2()->SaveCamInfo(NULL);
	return 0;
}

/**
 * @note Address: stage02 .text 0x63A4
 * @note Size: 0xE0
 */
extern "C" s32 CmdRegisterCar(s32 cmd, s32 argc, s32* argv)
{
	XObj* c = NULL;
	if (argc >= 3) {
		c = ObjTable_Find(OBJ_TABLE(), argv[2]);
	}
	return RegisterCar(ObjTable_Find(OBJ_TABLE(), argv[0]), ObjTable_Find(OBJ_TABLE(), argv[1]), c);
}

/**
 * @note Address: stage02 .text 0x635C
 * @note Size: 0x48
 */
extern "C" s32 CmdStartCars(s32 cmd, s32 argc, s32* argv)
{
	if (argc == 0) {
		StartCars();
		return 0;
	}
	switch (argv[0]) {
	case 1:
		StartCarCamera();
		break;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x6314
 * @note Size: 0x48
 */
extern "C" s32 CmdGetCarState(s32 cmd, s32 argc, s32* argv)
{
	if (argc == 0) {
		return GetPlayerCarResult();
	}
	switch (argv[0]) {
	case 1:
		return GetPlayerCarFrame();
	}
	return -1;
}

/**
 * @note Address: stage02 .text 0x62F0
 * @note Size: 0x24
 */
extern "C" s32 CmdTerminateCars(s32 cmd, s32 argc, s32* argv)
{
	Terminate();
	return 0;
}

/**
 * @note Address: stage02 .text 0x6298
 * @note Size: 0x58
 */
extern "C" s32 CmdSetCarData(s32 cmd, s32 argc, s32* argv)
{
	SetCarParams((u8*)argv[0]);
	if (argc < 2) {
		return 0;
	}
	SetCarSoundCues((u8*)argv[1]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6230
 * @note Size: 0x68
 */
extern "C" s32 CmdShowTimer(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		InfoScrn_ApplyTimerHudCommand(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)), 1, 1);
	} else {
		InfoScrn_ApplyTimerHudCommand(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)), 0, 1);
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x61D8
 * @note Size: 0x58
 */
extern "C" s32 CmdSetTimerRunning(s32 cmd, s32 argc, s32* argv)
{
	if (argv[0] != 0) {
		InfoScrn_ShowTimer(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)));
	} else {
		InfoScrn_HideTimer(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)));
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x61A0
 * @note Size: 0x38
 */
extern "C" s32 CmdSetTimer(s32 cmd, s32 argc, s32* argv)
{
	InfoScrn_SetTimerHundredths(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)), argv[0]);
	return 0;
}

/**
 * @note Address: stage02 .text 0x6170
 * @note Size: 0x30
 */
extern "C" s32 CmdGetTimer(s32 cmd, s32 argc, s32* argv)
{
	return InfoScrn_GetTimerHundredths(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)));
}
