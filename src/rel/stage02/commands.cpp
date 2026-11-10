#include "rel/stage02/main.h"
#include "types.h"

class XObj;
extern "C" u8 gGame[];
#define GAME_AT(off) (gGame + (off))

class CamInfo {
public:
	CamInfo(s32* values) { m_values = values; }
	virtual ~CamInfo();
	void Set(s32 idx, s32 value) { m_values[idx] = value; }

	s32* m_values; // _04
};

#define MAIN() ((Stage02Main*)gGame)

struct ObjTable {
	void* _00; // _00
};

#if defined(VERSION_JP)
#define OBJ_TABLE() ((ObjTable*)GAME_AT(0x5D038))
#else
#define OBJ_TABLE() ((ObjTable*)GAME_AT(0x5D080))
#endif
// see game/ccameraaction.h
class CCameraActionScroll2 {
public:
	void SaveCamInfo(const void* info);
	void ClearHideList();
	BOOL AddHideObj(s32 id);
};

#if defined(VERSION_JP)
#define SCROLL2() (*(CCameraActionScroll2**)GAME_AT(0xE7114))
#else
#define SCROLL2() (*(CCameraActionScroll2**)GAME_AT(0xE7DFC))
#endif

extern "C" {
void OSReport(const char* format, ...);
void* ResolveEventObjectId(void* table, s16 id);
void SetJunkUnkoFlag(u8 on);
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
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void StartCars(void);
void StartCarCamera(void);
s32 RegisterCar(XObj* a, XObj* b, XObj* c);
}

inline XObj* ObjTable_Find(ObjTable* table, s16 id)
{
	return table->_00 != NULL ? (XObj*)ResolveEventObjectId(table, id) : NULL;
}

/**
 * @note Address: stage02 .text 0x6610
 * @note Size: 0x48
 */
inline CamInfo::~CamInfo()
{
}

/**
 * @note Address: stage02 .text 0x6AA0
 * @note Size: 0xFC
 */
extern "C" s32 CmdStartFighter(s32 cmd, s32 argc, s32* argv)
{
	XObj* objs[3];
	int i;
	u32 j;

#if !defined(VERSION_PAL)
	OSReport("args = ");
#endif
	// PAL removes the reports but retains the loop.
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
