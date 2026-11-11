#include "Version.h"
#include "game/hsd_jobj.h"
#include "game/ut.h"

extern "C" {
void PSMTXCopy(Mtx src, Mtx dst);
void PSMTXConcat(Mtx a, Mtx b, Mtx ab);
void PSMTXScale(Mtx m, f32 xS, f32 yS, f32 zS);
void PSMTXTrans(Mtx m, f32 xT, f32 yT, f32 zT);
void PSMTXMultVec(Mtx m, Vec* src, Vec* dst);
}

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

class CInterfaceCameraAction {
public:
	CVec _00;    // _00
	CVec _10;    // _10
	CVec _20;    // _20
	u8 _30[0xC]; // _30
	CInterfaceCameraAction() { }
	virtual ~CInterfaceCameraAction() { }
	virtual void vf0C();
	virtual void vf10() = 0;
	virtual void vf14();
	virtual void vf18();
	virtual void vf1C();
	virtual void vf20();
	virtual void vf24();
	virtual void vf28();
	virtual void vf2C(f32 x, f32 y, f32 z);
	virtual void vf30();
	virtual void vf34();
	virtual void vf38();
	virtual void vf3C(Vec* pos);
	virtual void vf40();
	virtual void vf44();
	virtual void vf48(f32 v);
};

namespace Car {

class CCameraActionCar : public CInterfaceCameraAction {
public:
	CCameraActionCar()
	{
		_40   = NULL;
		_44.x = _44.y = _44.z = 0.0f;
		_50.x = _50.y = _50.z = 0.0f;
	}
	virtual ~CCameraActionCar() { }
	virtual void vf10();

	void SetTarget(Vec pos) { _50 = pos; }
	void SetOffset(Vec pos) { _44 = pos; }

	MtxPtr _40;  // _40
	Vec _44;     // _44
	Vec _50;     // _50
	u8 _5C[0xC]; // _5C
};

class CarRoot {
public:
	virtual void Update(u32 arg);
	virtual void vf0C();
	virtual void vf10();

	void Register(XObj* a, XObj* b, XObj* c);

	XObj* _04; // _04
	XObj* _08; // _08
	XObj* _0C; // _0C
	f32 _10;   // _10
	f32 _14;   // _14
	f32 _18;   // _18
	Mtx _1C;   // _1C
	Mtx _4C;   // _4C
	u8 _7C;    // _7C
	u8 _7D;    // _7D
};

class PlayerCarRoot : public CarRoot {
public:
	virtual void Update(u32 arg);
	virtual void vf10();
	virtual void vf14();

	f32 _80; // _80
};

// prints "Car::Init"
void Init(void);

} // namespace Car

struct RoboView {
#if defined(VERSION_JP)
	u8 _00[0x970];    // _00
	u32 _980;         // _970
	u8 _984[0x12380]; // _974
	s32 _12DF4;       // _12CF4
#else
	u8 _00[0x980];    // _00
	u32 _980;         // _980
	u8 _984[0x12470]; // _984
	s32 _12DF4;       // _12DF4
#endif
};

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
#define GAME_OBJ()  (*(RoboView**)GAME_AT(GAME_OFS(0x96BC, 0x9674)))
#define SOUND()     GAME_AT(GAME_OFS(0xE7E24, 0xE713C))

static XObj* takiObj;
static Car::CCameraActionCar carCamera;

// see game/cmotioncontrolcamera.h
class CMotionControlCamera {
public:
	void ChangeAction(CInterfaceCameraAction* action);
};

// see game/ccameraaction.h
class CCameraActionScroll2 {
public:
	void SaveCamInfo(const void* info);
	void ClearHideList();
	BOOL AddHideObj(s32 id);
};

#define SCROLL2() (*(CCameraActionScroll2**)GAME_AT(GAME_OFS(0xE7DFC, 0xE7114)))
static Car::PlayerCarRoot playerCar;
static Car::CarRoot racerCar;
static u8 carsStarted;
static Vec carCameraTrail[32];
static u32 carCameraTrailIndex;
static u8* carParams;
static u8* carSoundCues;

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
void Update__Q23Car13PlayerCarRootFUl(Car::PlayerCarRoot* car, u32 arg);
void Start__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot* car);
void Clear__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot* car);
void Brake__Q23Car7CarRootFv(Car::CarRoot* car);
void Update__Q23Car7CarRootFUl(Car::CarRoot* car, u32 arg);
void Update__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam);
void Start__Q23Car7CarRootFv(Car::CarRoot* car);
s32 GetMode__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam);
s32 GetFlags__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam);
f32 GetEaseDivisor__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam);
}

inline XObj* ObjTable_Find(ObjTable* table, s16 id)
{
	return table->_00 != NULL ? (XObj*)ResolveEventObjectId(table, id) : NULL;
}

// little endian ints out of a loaded data file
inline s32 ReadS32LE(u8* p)
{
	if (p != NULL) {
		return p[0] + p[1] * 0x100 + p[2] * 0x10000 + p[3] * 0x1000000;
	}
	return 0;
}

// todo: copied inline, clean up
inline f32 HSD_AObjGetEndFrame(HSD_AObj* aobj)
{
	((aobj) ? ((void)0) : __assert("aobj.h", 171, "aobj"));
	return aobj->end_frame;
}

// clears the three user matrix bits copy_mtx sets, real name unknown
inline void JObjInline1199(HSD_JObj* jobj)
{
	JOBJ_H_ASSERT(1199, jobj);
	jobj->flags &= ~0x3800000;
	HSD_JObjSetMtxDirty(jobj);
}

inline void Car::CarRoot::Register(XObj* a, XObj* b, XObj* c)
{
#if !defined(VERSION_PAL)
	OSReport("Car::Register\n");
#endif
	_04 = a;
	_08 = b;
	_0C = c;
	a->_228 |= 0x10000;
	_08->_228 &= ~0x10000ULL;
	_10 = 0.0f;
	_14 = 0.0f;
	_18 = 1.0f;
	_7C = 0;
	_7D = 0;
}

// inline virtuals in the original, so direct calls get inlined
inline void CarRoot_Brake(Car::CarRoot* car)
{
	s32 i;
	XObj* obj;

	car->_7C = 1;
	car->_04->vf80(2, 0, 0.0f, 1.0f, 0.0f, 10.0f, 0, 0, 0);
	obj = car->_04;
	// takes the first free slot in the request ring
	for (i = 0; i < 16; i++) {
		s32 idx = (obj->_810 + i) % 16;
		if (obj->_814[idx].type == -1) {
			obj->_814[idx].type = 3;
			obj->_814[idx]._04  = 0;
			obj->_814[idx]._06  = 100;
			break;
		}
	}
}

inline void CarRoot_Start(Car::CarRoot* car)
{
	car->_7D = 1;
}

inline void PlayerCarRoot_Start(Car::PlayerCarRoot* car)
{
	car->_7D = 1;
	PlayGameSound(SOUND(), 0x2000D, 0xFF);
}

inline void RoboView_ResetFlags(RoboView* game)
{
	game->_980 = 0;
	game->_980 |= 0x20000000;
	game->_980 |= 7;
	game->_12DF4 = 1;
}

// body of Car::Init, inlined into Terminate
inline void Car_Init(void)
{
#if !defined(VERSION_PAL)
	OSReport("Car::Init\n");
#endif
	playerCar._08       = NULL;
	playerCar._04       = NULL;
	playerCar._80       = 0.0f;
	racerCar._08        = NULL;
	racerCar._04        = NULL;
	carsStarted         = 0;
	carCameraTrailIndex = 0;
	carParams           = NULL;
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
 * @note Address: stage02 .text 0x6170
 * @note Size: 0x30
 */
extern "C" s32 CmdGetTimer(s32 cmd, s32 argc, s32* argv)
{
	return InfoScrn_GetTimerHundredths(GAME_AT(GAME_OFS(0x8F38, 0x8EF0)));
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
 * @note Address: stage02 .text 0x62F0
 * @note Size: 0x24
 */
extern "C" s32 CmdTerminateCars(s32 cmd, s32 argc, s32* argv)
{
	Terminate();
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
 * @note Address: stage02 .text 0x6484
 * @note Size: 0x38
 */
extern "C" s32 CmdClearCamInfo(s32 cmd, s32 argc, s32* argv)
{
	SCROLL2()->SaveCamInfo(NULL);
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
 * @note Address: stage02 .text 0x6610
 * @note Size: 0x48
 */
// fn_5_6610

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
 * @note Address: stage02 .text 0x6888
 * @note Size: 0x20
 */
extern "C" s32 CmdGetFighterEvent(s32 cmd, s32 argc, s32* argv)
{
	return GetShotEvent();
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
 * @note Address: stage02 .text 0x69FC
 * @note Size: 0x24
 */
extern "C" s32 CmdEndFighter(s32 cmd, s32 argc, s32* argv)
{
	EndFighter();
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
 * @note Address: stage02 .text 0x6B9C
 * @note Size: 0x24
 */
extern "C" void ExitStage(void* arg)
{
	EndFighter();
	RestoreDefaultCamera();
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
 * @note Address: stage02 .text 0x6BF4
 * @note Size: 0x20
 */
extern "C" void DrawStageEffects(void* arg)
{
	DrawShots();
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
 * @note Address: stage02 .text 0x6E5C
 * @note Size: 0xCC
 */
extern "C" void RestoreDefaultCamera(void)
{
	JObjInline1199(((XObj*)GAME_OBJ())->jobj);
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))
	    ->ChangeAction(*(CInterfaceCameraAction**)GAME_AT(GAME_OFS(0xE5F0C, 0xE5224)));
}

/**
 * @note Address: stage02 .text 0x6F28
 * @note Size: 0x3DC
 */
extern "C" void Terminate(void)
{
#if !defined(VERSION_PAL)
	OSReport("Terminate\n");
#endif
	JObjInline1199(playerCar._04->jobj);
	if (playerCar._0C != NULL) {
		JObjInline1199(playerCar._0C->jobj);
	}
	playerCar._7D = 0;
	playerCar._7C = 0;
	JObjInline1199(racerCar._04->jobj);
	if (racerCar._0C != NULL) {
		JObjInline1199(racerCar._0C->jobj);
	}
	racerCar._7D = 0;
	racerCar._7C = 0;
	RoboView_ResetFlags(GAME_OBJ());
	JObjInline1199(((XObj*)GAME_OBJ())->jobj);
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))
	    ->ChangeAction(*(CInterfaceCameraAction**)GAME_AT(GAME_OFS(0xE5F0C, 0xE5224)));
	Car_Init();
}

// Terminate
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r3, lbl_5_rodata_244@ha
    stw      r0, 0x24(r1)
    addi     r3, r3, lbl_5_rodata_244@l
    stw      r31, 0x1c(r1)
    stw      r30, 0x18(r1)
    stw      r29, 0x14(r1)
    crclr    cr1eq
    bl       OSReport
    lis      r3, playerCar@ha
    addi     r31, r3, playerCar@l
    lwz      r3, 0x4(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00006F80
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00006F80:
    lwz      r0, 0x14(r30)
    cmplwi   r30, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r30)
    beq      .L_00006FDC
    bne      .L_00006FB0
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00006FB0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00006FCC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00006FCC
    li       r3, 0x1
.L_00006FCC:
    clrlwi.  r0, r3, 24
    bne      .L_00006FDC
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00006FDC:
    lis      r3, playerCar@ha
    addi     r3, r3, playerCar@l
    lwz      r3, 0xc(r3)
    cmplwi   r3, 0x0
    beq      .L_00007070
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00007014
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007014:
    lwz      r0, 0x14(r30)
    cmplwi   r30, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r30)
    beq      .L_00007070
    bne      .L_00007044
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007044:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00007060
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00007060
    li       r3, 0x1
.L_00007060:
    clrlwi.  r0, r3, 24
    bne      .L_00007070
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00007070:
    lis      r4, playerCar@ha
    lis      r3, racerCar@ha
    addi     r30, r3, racerCar@l
    li       r0, 0x0
    addi     r4, r4, playerCar@l
    lwz      r3, 0x4(r30)
    stb      r0, 0x7d(r4)
    stb      r0, 0x7c(r4)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_000070B4
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_000070B4:
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r29)
    beq      .L_00007110
    bne      .L_000070E4
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_000070E4:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00007100
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00007100
    li       r3, 0x1
.L_00007100:
    clrlwi.  r0, r3, 24
    bne      .L_00007110
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_00007110:
    lis      r3, racerCar@ha
    addi     r3, r3, racerCar@l
    lwz      r3, 0xc(r3)
    cmplwi   r3, 0x0
    beq      .L_000071A4
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00007148
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007148:
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r29)
    beq      .L_000071A4
    bne      .L_00007178
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007178:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00007194
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00007194
    li       r3, 0x1
.L_00007194:
    clrlwi.  r0, r3, 24
    bne      .L_000071A4
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_000071A4:
    lis      r3, gGame@ha
    lis      r4, racerCar@ha
    addi     r3, r3, gGame@l
    li       r5, 0x0
    addi     r4, r4, racerCar@l
    li       r0, 0x1
    addis    r7, r3, 0x1
    stb      r5, 0x7d(r4)
    lwz      r6, -0x6944(r7)
    stb      r5, 0x7c(r4)
    addis    r3, r6, 0x1
    stw      r5, 0x980(r6)
    lwz      r4, 0x980(r6)
    oris     r4, r4, 0x2000
    stw      r4, 0x980(r6)
    lwz      r4, 0x980(r6)
    ori      r4, r4, 0x7
    stw      r4, 0x980(r6)
    stw      r0, 0x2df4(r3)
    lwz      r3, -0x6944(r7)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00007218
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007218:
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r29)
    beq      .L_00007274
    bne      .L_00007248
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00007248:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00007264
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00007264
    li       r3, 0x1
.L_00007264:
    clrlwi.  r0, r3, 24
    bne      .L_00007274
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_00007274:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r4, 0x5f0c(r3)
    addi     r3, r3, 0x5d38
    bl       fn_801494A4
    lis      r3, lbl_5_rodata_250@ha
    addi     r3, r3, lbl_5_rodata_250@l
    crclr    cr1eq
    bl       OSReport
    lis      r3, lbl_5_rodata_25C@ha
    li       r0, 0x0
    addi     r7, r3, lbl_5_rodata_25C@l
    lis      r8, playerCar@ha
    lis      r6, racerCar@ha
    lfs      f0, 0x0(r7)
    addi     r8, r8, playerCar@l
    lis      r5, carsStarted@ha
    lis      r4, carCameraTrailIndex@ha
    lis      r3, carParams@ha
    addi     r6, r6, racerCar@l
    stw      r0, 0x8(r8)
    stw      r0, 0x4(r31)
    stfs     f0, 0x80(r8)
    stw      r0, 0x8(r6)
    stw      r0, 0x4(r30)
    stb      r0, carsStarted@l(r5)
    stw      r0, carCameraTrailIndex@l(r4)
    stw      r0, carParams@l(r3)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r0, 0x24(r1)
    lwz      r29, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x7304
 * @note Size: 0x30
 */
extern "C" s32 GetPlayerCarFrame(void)
{
	return 100.0f * playerCar._10;
}

/**
 * @note Address: stage02 .text 0x7334
 * @note Size: 0x144
 */
extern "C" s32 GetPlayerCarResult(void)
{
	CPad pad(0);
	HSD_AObj* aobj;
	f32 end;

	if (playerCar._7C && playerCar._18 <= 0.0f) {
		return 1;
	}
	aobj = GetAObj(playerCar._08->jobj);
	end  = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	if (end <= playerCar._10) {
		return 2;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x7478
 * @note Size: 0xC
 */
extern "C" void SetCarSoundCues(u8* data)
{
	carSoundCues = data;
}

/**
 * @note Address: stage02 .text 0x7484
 * @note Size: 0xC
 */
extern "C" void SetCarParams(u8* data)
{
	carParams = data;
}

/**
 * @note Address: stage02 .text 0x7490
 * @note Size: 0x4
 */
extern "C" void DrawCarHud(void* arg)
{
}

/**
 * @note Address: stage02 .text 0x7494
 * @note Size: 0x398
 */
extern "C" void UpdateCars(u32 arg)
{
	CPad pad(0);
	bool done;

	if (!carsStarted) {
		arg = 0;
	}
	if (playerCar._04 == NULL) {
		return;
	}
	if (racerCar._04 == NULL) {
		return;
	}
	Update__Q23Car13PlayerCarRootFUl(&playerCar, arg);
	Update__Q23Car7CarRootFUl(&racerCar, arg);
	if (!playerCar._7D) {
		done = false;
	} else {
		done = ReadS32LE(carParams + 4) <= playerCar._10;
	}
	if (done) {
		carCamera._40 = NULL;
	} else {
		Vec pos;
		s32 prev;
		pos.y = 0.0f;
		pos.z = 0.0f;
		pos.x = 0.0f;
		pos.y = 20.0f;
		PSMTXMultVec(playerCar._1C, &pos, &pos);
		carCameraTrail[carCameraTrailIndex] = pos;
		prev                                = carCameraTrailIndex - 10;
		if (prev < 0) {
			prev += 32;
		}
		CInterfaceCameraAction* cam = &carCamera;
		cam->vf2C(carCameraTrail[prev].x, carCameraTrail[prev].y, carCameraTrail[prev].z);
		if (++carCameraTrailIndex >= 32) {
			carCameraTrailIndex = 0;
		}
	}
	if (!racerCar._7C && ReadS32LE(carParams + 0xC) <= racerCar._10) {
		CarRoot_Brake(&racerCar);
	}
}

// UpdateCars
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    li       r4, 0x0
    stw      r0, 0x34(r1)
    stw      r31, 0x2c(r1)
    stw      r30, 0x28(r1)
    mr       r30, r3
    addi     r3, r1, 0x8
    bl       __ct__4CPadFUc
    lis      r3, carsStarted@ha
    lbz      r0, carsStarted@l(r3)
    cmplwi   r0, 0x0
    bne      .L_000074CC
    li       r30, 0x0
.L_000074CC:
    lis      r3, playerCar@ha
    addi     r3, r3, playerCar@l
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    bne      .L_000074F0
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
    b        .L_00007814
.L_000074F0:
    lis      r4, racerCar@ha
    addi     r31, r4, racerCar@l
    lwz      r0, 0x4(r31)
    cmplwi   r0, 0x0
    bne      .L_00007514
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
    b        .L_00007814
.L_00007514:
    mr       r4, r30
    bl       Update__Q23Car13PlayerCarRootFUl
    lis      r3, racerCar@ha
    mr       r4, r30
    addi     r3, r3, racerCar@l
    bl       Update__Q23Car7CarRootFUl
    lis      r3, playerCar@ha
    addi     r3, r3, playerCar@l
    lbz      r0, 0x7d(r3)
    cmplwi   r0, 0x0
    bne      .L_00007548
    li       r0, 0x0
    b        .L_000075C8
.L_00007548:
    lis      r3, carParams@ha
    addi     r3, r3, carParams@l
    lwz      r3, 0x0(r3)
    addic.   r4, r3, 0x4
    beq      .L_00007588
    lbz      r3, 0x1(r4)
    lbz      r0, 0x2(r4)
    lbz      r5, 0x3(r4)
    slwi     r3, r3, 8
    slwi     r0, r0, 16
    lbz      r4, 0x0(r4)
    slwi     r5, r5, 24
    add      r0, r3, r0
    add      r0, r0, r5
    add      r0, r4, r0
    b        .L_0000758C
.L_00007588:
    li       r0, 0x0
.L_0000758C:
    xoris    r3, r0, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lis      r4, lbl_5_rodata_280@ha
    lis      r3, playerCar@ha
    lfd      f2, lbl_5_rodata_280@l(r4)
    stw      r0, 0x20(r1)
    addi     r3, r3, playerCar@l
    lfs      f0, 0x10(r3)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    mfcr     r0
    extrwi   r0, r0, 1, 2
.L_000075C8:
    clrlwi.  r0, r0, 24
    beq      .L_000075E4
    lis      r3, carCamera@ha
    li       r0, 0x0
    addi     r3, r3, carCamera@l
    stw      r0, 0x40(r3)
    b        .L_000076BC
.L_000075E4:
    lis      r3, lbl_5_rodata_25C@ha
    lis      r4, lbl_5_rodata_274@ha
    addi     r5, r3, lbl_5_rodata_25C@l
    lis      r3, playerCar@ha
    lfs      f1, 0x0(r5)
    addi     r6, r4, lbl_5_rodata_274@l
    addi     r3, r3, playerCar@l
    addi     r4, r1, 0x10
    stfs     f1, 0x14(r1)
    mr       r5, r4
    lfs      f0, 0x0(r6)
    addi     r3, r3, 0x1c
    stfs     f1, 0x18(r1)
    stfs     f1, 0x10(r1)
    stfs     f0, 0x14(r1)
    bl       PSMTXMultVec
    lis      r4, carCameraTrailIndex@ha
    lis      r3, carCameraTrail@ha
    addi     r4, r4, carCameraTrailIndex@l
    lfs      f2, 0x10(r1)
    lwz      r4, 0x0(r4)
    addi     r0, r3, carCameraTrail@l
    lfs      f1, 0x14(r1)
    mulli    r3, r4, 0xc
    lfs      f0, 0x18(r1)
    subic.   r6, r4, 0xa
    add      r3, r0, r3
    stfs     f2, 0x0(r3)
    stfs     f1, 0x4(r3)
    stfs     f0, 0x8(r3)
    bge      .L_00007664
    addi     r6, r6, 0x20
.L_00007664:
    lis      r3, carCamera@ha
    lis      r4, carCameraTrail@ha
    addi     r3, r3, carCamera@l
    lwz      r12, 0x3c(r3)
    mulli    r0, r6, 0xc
    addi     r5, r4, carCameraTrail@l
    lwz      r12, 0x2c(r12)
    add      r4, r5, r0
    lfsx     f1, r5, r0
    lfs      f2, 0x4(r4)
    lfs      f3, 0x8(r4)
    mtctr    r12
    bctrl
    lis      r3, carCameraTrailIndex@ha
    addi     r4, r3, carCameraTrailIndex@l
    lwz      r3, 0x0(r4)
    addi     r0, r3, 0x1
    cmplwi   r0, 0x20
    stw      r0, 0x0(r4)
    blt      .L_000076BC
    li       r0, 0x0
    stw      r0, 0x0(r4)
.L_000076BC:
    lis      r3, racerCar@ha
    addi     r6, r3, racerCar@l
    lbz      r0, 0x7c(r6)
    cmplwi   r0, 0x0
    bne      .L_00007808
    lis      r3, carParams@ha
    addi     r3, r3, carParams@l
    lwz      r3, 0x0(r3)
    addic.   r4, r3, 0xc
    beq      .L_00007710
    lbz      r3, 0x1(r4)
    lbz      r0, 0x2(r4)
    lbz      r5, 0x3(r4)
    slwi     r3, r3, 8
    slwi     r0, r0, 16
    lbz      r4, 0x0(r4)
    slwi     r5, r5, 24
    add      r0, r3, r0
    add      r0, r0, r5
    add      r0, r4, r0
    b        .L_00007714
.L_00007710:
    li       r0, 0x0
.L_00007714:
    xoris    r3, r0, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lis      r4, lbl_5_rodata_280@ha
    lis      r3, racerCar@ha
    lfd      f2, lbl_5_rodata_280@l(r4)
    stw      r0, 0x20(r1)
    addi     r3, r3, racerCar@l
    lfs      f0, 0x10(r3)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    bne      .L_00007808
    li       r0, 0x1
    lis      r3, lbl_5_rodata_25C@ha
    addi     r4, r3, lbl_5_rodata_25C@l
    stb      r0, 0x7c(r6)
    lwz      r3, 0x4(r31)
    lis      r5, lbl_5_rodata_278@ha
    lfs      f1, 0x0(r4)
    lis      r4, lbl_5_rodata_27C@ha
    lwz      r12, 0x34(r3)
    addi     r6, r4, lbl_5_rodata_27C@l
    lfs      f2, lbl_5_rodata_278@l(r5)
    fmr      f3, f1
    lwz      r12, 0x80(r12)
    li       r4, 0x2
    lfs      f4, 0x0(r6)
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
    li       r0, 0x10
    lwz      r4, 0x4(r31)
    li       r5, 0x0
    mtctr    r0
.L_000077B0:
    lwz      r0, 0x810(r4)
    add      r3, r0, r5
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r3, r0, 3
    addi     r0, r3, 0x814
    lwzx     r0, r4, r0
    cmpwi    r0, -0x1
    bne      .L_00007800
    add      r4, r4, r3
    li       r0, 0x3
    stw      r0, 0x814(r4)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r4)
    sth      r0, 0x81a(r4)
    b        .L_00007808
.L_00007800:
    addi     r5, r5, 0x1
    bdnz     .L_000077B0
.L_00007808:
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
.L_00007814:
    lwz      r0, 0x34(r1)
    lwz      r31, 0x2c(r1)
    lwz      r30, 0x28(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/

/**
 * @note Address: stage02 .text 0x782C
 * @note Size: 0x64
 */
extern "C" void StartCars(void)
{
	carsStarted = 1;
	PlayerCarRoot_Start(&playerCar);
	CarRoot_Start(&racerCar);
}

/**
 * @note Address: stage02 .text 0x7890
 * @note Size: 0x10C
 */
extern "C" void StartCarCamera(void)
{
	RoboView* game = GAME_OBJ();
	Vec v;

	game->_980 = 0;
	game->_980 |= 0x40000000;
	game->_12DF4 = 0;
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))->ChangeAction(&carCamera);
	v.x = 0.0f;
	v.y = 30.0f;
	v.z = -120.0f;
	carCamera.SetTarget(v);
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 60.0f;
	carCamera.SetOffset(v);
	carCamera._40 = playerCar._1C;
}

/**
 * @note Address: stage02 .text 0x799C
 * @note Size: 0x19C
 */
extern "C" s32 RegisterCar(XObj* a, XObj* b, XObj* c)
{
	if (playerCar._04 == NULL) {
		playerCar.Register(a, b, c);
		return 1;
	}
	if (racerCar._04 == NULL) {
		racerCar.Register(a, b, c);
		return 2;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x7B38
 * @note Size: 0x78
 */
void Car::Init(void)
{
	Car_Init();
}

/**
 * @note Address: stage02 .text 0x7BB0
 * @note Size: 0x3A8
 */
extern "C" void Update__Q23Car13PlayerCarRootFUl(Car::PlayerCarRoot* car, u32 arg)
{
	s32 i;

	Update__Q23Car7CarRootFUl(car, arg);
	if (!car->_7D) {
		return;
	}
	CPad pad(0);
	void* sound;
	if (!car->_7C && (pad.mPad->mTrigger & 0x200)) {
		car->vf0C();
		car->_80 = car->_10;
		sound    = SOUND();
		StopGameSfx(sound, 0x2000D);
		if (car->_80 < ReadS32LE(carParams + 8)) {
			PlayGameSound(sound, 0x2000E, 0xFF);
		} else {
			PlayGameSound(sound, 0x20011, 0xFF);
		}
	}
	sound = SOUND();
	for (i = 0; ReadS32LE(carSoundCues + i * 8) > 0; i++) {
		s32 frame = ReadS32LE(carSoundCues + i * 8);
		s32 id    = ReadS32LE(carSoundCues + (i * 2 + 1) * 4);
		if (car->_14 < frame && frame <= car->_10) {
			PlayGameSound(sound, id, 0xFF);
			if (id == 0x2001F) {
				FadeOutGameSfx(sound, -0x10000, 20);
			} else if (id == 0x2000F) {
				StopGameSfx(sound, 0x2001F);
			}
		}
	}
	if (ReadS32LE(carParams + 4) <= car->_10) {
		car->_18 = 1.0f;
		if (car->_7C && car->_04->_158 != 4) {
			car->_04->vf80(4, 0, 0.0f, 1.0f, 0.0f, 10.0f, 0, 0, 0);
		}
	}
}

/**
 * @note Address: stage02 .text 0x7F58
 * @note Size: 0x44
 */
extern "C" void Start__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot* car)
{
	PlayerCarRoot_Start(car);
}

/**
 * @note Address: stage02 .text 0x7F9C
 * @note Size: 0x1C
 */
extern "C" void Clear__Q23Car13PlayerCarRootFv(Car::PlayerCarRoot* car)
{
	car->_08 = NULL;
	car->_04 = NULL;
	car->_80 = 0.0f;
}

/**
 * @note Address: stage02 .text 0x7FB8
 * @note Size: 0xE8
 */
extern "C" void Brake__Q23Car7CarRootFv(Car::CarRoot* car)
{
	CarRoot_Brake(car);
}

// Brake__Q23Car7CarRootFv
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r5, lbl_5_rodata_278@ha
    lis      r4, lbl_5_rodata_27C@ha
    stw      r0, 0x14(r1)
    li       r0, 0x1
    li       r7, 0x0
    li       r8, 0x0
    stw      r31, 0xc(r1)
    mr       r31, r3
    stb      r0, 0x7c(r3)
    lis      r3, lbl_5_rodata_25C@ha
    addi     r6, r3, lbl_5_rodata_25C@l
    lwz      r3, 0x4(r31)
    lfs      f1, 0x0(r6)
    addi     r6, r5, lbl_5_rodata_278@l
    lwz      r12, 0x34(r3)
    addi     r5, r4, lbl_5_rodata_27C@l
    lfs      f2, 0x0(r6)
    fmr      f3, f1
    lwz      r12, 0x80(r12)
    li       r4, 0x2
    lfs      f4, 0x0(r5)
    li       r5, 0x0
    li       r6, 0x0
    mtctr    r12
    bctrl
    li       r0, 0x10
    lwz      r4, 0x4(r31)
    li       r5, 0x0
    mtctr    r0
.L_00008034:
    lwz      r0, 0x810(r4)
    add      r3, r0, r5
    slwi     r0, r3, 28
    srwi     r3, r3, 31
    subf     r0, r3, r0
    rotlwi   r0, r0, 4
    add      r0, r0, r3
    slwi     r3, r0, 3
    addi     r0, r3, 0x814
    lwzx     r0, r4, r0
    cmpwi    r0, -0x1
    bne      .L_00008084
    add      r4, r4, r3
    li       r0, 0x3
    stw      r0, 0x814(r4)
    li       r3, 0x0
    li       r0, 0x64
    sth      r3, 0x818(r4)
    sth      r0, 0x81a(r4)
    b        .L_0000808C
.L_00008084:
    addi     r5, r5, 0x1
    bdnz     .L_00008034
.L_0000808C:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x80A0
 * @note Size: 0x490
 */
extern "C" void Update__Q23Car7CarRootFUl(Car::CarRoot* car, u32 arg)
{
	HSD_JObj* jobj;
	Mtx scale;
	Mtx m;

	if (car->_04 == NULL || car->_08 == NULL) {
		return;
	}
	s32 path[2] = { 0, 0 };
	jobj        = GetJointByPath(car->_08->jobj, path, 1);
	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, car->_1C);
	HSD_JObjCopyMtx(car->_04->jobj, car->_1C);
	if (car->_0C != NULL) {
		PSMTXTrans(m, 3.75f, 5.0f, -1.0f);
		PSMTXConcat(car->_1C, m, car->_4C);
		HSD_JObjCopyMtx(car->_0C->jobj, car->_4C);
	}
	if (car->_7D) {
		car->_14 = car->_10;
		car->_10 = car->_18 * arg + car->_10;
	}
	car->_08->vf80(0, 1, car->_10, 1.0f, 0.0f, 10.0f, 0, 0, 0);
	if (!car->_7C) {
		car->_04->vf80(1, 1, car->_10, 1.0f, 0.0f, 10.0f, 0, 0, 0);
		return;
	}
	if (0.0f < car->_18) {
		f32 rate = car->_18 - ReadS32LE(carParams) / 1000.0f;
		if (rate < 0.0f) {
			rate = 0.0f;
		}
		car->_18 = rate;
	}
}

// Update__Q23Car7CarRootFUl
/*
    stwu     r1, -0x90(r1)
    mflr     r0
    stw      r0, 0x94(r1)
    stw      r31, 0x8c(r1)
    mr       r31, r3
    stw      r30, 0x88(r1)
    mr       r30, r4
    stw      r29, 0x84(r1)
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_00008514
    lwz      r4, 0x8(r31)
    cmplwi   r4, 0x0
    bne      .L_000080DC
    b        .L_00008514
.L_000080DC:
    lis      r3, sCarJointPath@ha
    lwz      r29, 0x0(r4)
    addi     r4, r3, sCarJointPath@l
    lwz      r3, 0x0(r4)
    cmplwi   r29, 0x0
    lwz      r0, 0x4(r4)
    stw      r3, 0x8(r1)
    stw      r0, 0xc(r1)
    bne      .L_00008108
    li       r29, 0x0
    b        .L_00008154
.L_00008108:
    li       r0, 0x0
    bne      .L_00008118
    mr       r29, r0
    b        .L_0000811C
.L_00008118:
    lwz      r29, 0x10(r29)
.L_0000811C:
    lwz      r0, 0x8(r1)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_00008154
.L_0000812C:
    cmplwi   r29, 0x0
    bne      .L_0000813C
    li       r29, 0x0
    b        .L_00008140
.L_0000813C:
    lwz      r29, 0x8(r29)
.L_00008140:
    cmplwi   r29, 0x0
    bne      .L_00008150
    li       r29, 0x0
    b        .L_00008154
.L_00008150:
    bdnz     .L_0000812C
.L_00008154:
    lis      r4, lbl_5_rodata_2A4@ha
    addi     r3, r1, 0x40
    lfs      f1, lbl_5_rodata_2A4@l(r4)
    fmr      f2, f1
    fmr      f3, f1
    bl       PSMTXScale
    cmplwi   r29, 0x0
    bne      .L_0000818C
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x47c
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_0000818C:
    cmplwi   r29, 0x0
    beq      .L_000081DC
    bne      .L_000081B0
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_000081B0:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000081CC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000081CC
    li       r3, 0x1
.L_000081CC:
    clrlwi.  r0, r3, 24
    beq      .L_000081DC
    mr       r3, r29
    bl       HSD_JObjSetupMatrixSub
.L_000081DC:
    addi     r3, r29, 0x44
    addi     r4, r1, 0x40
    addi     r5, r31, 0x1c
    bl       PSMTXConcat
    lwz      r3, 0x4(r31)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00008214
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00008214:
    addic.   r0, r31, 0x1c
    bne      .L_00008234
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_2A8@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_2A8@l
    bl       __assert
.L_00008234:
    addi     r3, r31, 0x1c
    addi     r4, r29, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r29)
    beq      .L_0000829C
    bne      .L_00008270
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00008270:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000828C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000828C
    li       r3, 0x1
.L_0000828C:
    clrlwi.  r0, r3, 24
    bne      .L_0000829C
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_0000829C:
    lwz      r0, 0xc(r31)
    cmplwi   r0, 0x0
    beq      .L_00008388
    lis      r6, lbl_5_rodata_2AC@ha
    lis      r5, lbl_5_rodata_2B0@ha
    lis      r4, sCarAttachmentZ@ha
    lfs      f1, lbl_5_rodata_2AC@l(r6)
    lfs      f2, lbl_5_rodata_2B0@l(r5)
    addi     r3, r1, 0x10
    lfs      f3, sCarAttachmentZ@l(r4)
    bl       PSMTXTrans
    addi     r3, r31, 0x1c
    addi     r4, r1, 0x10
    addi     r5, r31, 0x4c
    bl       PSMTXConcat
    lwz      r3, 0xc(r31)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00008300
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_00008300:
    addic.   r0, r31, 0x4c
    bne      .L_00008320
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_2A8@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_2A8@l
    bl       __assert
.L_00008320:
    addi     r3, r31, 0x4c
    addi     r4, r29, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r29)
    beq      .L_00008388
    bne      .L_0000835C
    lis      r3, lbl_5_rodata_234@ha
    lis      r5, lbl_5_rodata_23C@ha
    addi     r3, r3, lbl_5_rodata_234@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_23C@l
    bl       __assert
.L_0000835C:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00008378
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00008378
    li       r3, 0x1
.L_00008378:
    clrlwi.  r0, r3, 24
    bne      .L_00008388
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_00008388:
    lbz      r0, 0x7d(r31)
    cmplwi   r0, 0x0
    beq      .L_000083C8
    lis      r0, 0x4330
    lfs      f0, 0x10(r31)
    stw      r30, 0x74(r1)
    lis      r3, sCarUnsignedConversionBias@ha
    lfd      f1, sCarUnsignedConversionBias@l(r3)
    stfs     f0, 0x14(r31)
    stw      r0, 0x70(r1)
    lfs      f2, 0x18(r31)
    lfd      f0, 0x70(r1)
    fsubs    f1, f0, f1
    lfs      f0, 0x10(r31)
    fmadds   f0, f2, f1, f0
    stfs     f0, 0x10(r31)
.L_000083C8:
    lwz      r3, 0x8(r31)
    lis      r5, lbl_5_rodata_25C@ha
    addi     r7, r5, lbl_5_rodata_25C@l
    lis      r4, lbl_5_rodata_27C@ha
    lwz      r12, 0x34(r3)
    addi     r8, r4, lbl_5_rodata_27C@l
    lis      r6, lbl_5_rodata_278@ha
    lfs      f3, 0x0(r7)
    lwz      r12, 0x80(r12)
    li       r4, 0x0
    lfs      f2, lbl_5_rodata_278@l(r6)
    li       r5, 0x1
    lfs      f4, 0x0(r8)
    li       r6, 0x0
    lfs      f1, 0x10(r31)
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
    lbz      r0, 0x7c(r31)
    cmplwi   r0, 0x0
    bne      .L_00008470
    lwz      r3, 0x4(r31)
    lis      r5, lbl_5_rodata_25C@ha
    addi     r7, r5, lbl_5_rodata_25C@l
    lis      r4, lbl_5_rodata_27C@ha
    lwz      r12, 0x34(r3)
    addi     r8, r4, lbl_5_rodata_27C@l
    lis      r6, lbl_5_rodata_278@ha
    lfs      f3, 0x0(r7)
    lwz      r12, 0x80(r12)
    li       r4, 0x1
    lfs      f2, lbl_5_rodata_278@l(r6)
    li       r5, 0x1
    lfs      f4, 0x0(r8)
    li       r6, 0x0
    lfs      f1, 0x10(r31)
    li       r7, 0x0
    li       r8, 0x0
    mtctr    r12
    bctrl
    b        .L_00008514
.L_00008470:
    lis      r3, lbl_5_rodata_25C@ha
    lfs      f3, 0x18(r31)
    lfs      f0, lbl_5_rodata_25C@l(r3)
    fcmpo    cr0, f0, f3
    bge      .L_00008514
    lis      r3, carParams@ha
    lwz      r4, carParams@l(r3)
    cmplwi   r4, 0x0
    beq      .L_000084C0
    lbz      r3, 0x1(r4)
    lbz      r0, 0x2(r4)
    lbz      r5, 0x3(r4)
    slwi     r3, r3, 8
    slwi     r0, r0, 16
    lbz      r4, 0x0(r4)
    slwi     r5, r5, 24
    add      r0, r3, r0
    add      r0, r0, r5
    add      r0, r4, r0
    b        .L_000084C4
.L_000084C0:
    li       r0, 0x0
.L_000084C4:
    xoris    r3, r0, 0x8000
    lis      r0, 0x4330
    lis      r4, lbl_5_rodata_280@ha
    stw      r3, 0x74(r1)
    addi     r5, r4, lbl_5_rodata_280@l
    lis      r3, lbl_5_rodata_2B8@ha
    stw      r0, 0x70(r1)
    addi     r4, r3, lbl_5_rodata_2B8@l
    lfd      f2, 0x0(r5)
    lis      r3, lbl_5_rodata_25C@ha
    lfd      f0, 0x70(r1)
    lfs      f1, 0x0(r4)
    fsubs    f2, f0, f2
    lfs      f0, lbl_5_rodata_25C@l(r3)
    fdivs    f1, f2, f1
    fsubs    f1, f3, f1
    fcmpo    cr0, f1, f0
    bge      .L_00008510
    fmr      f1, f0
.L_00008510:
    stfs     f1, 0x18(r31)
.L_00008514:
    lwz      r0, 0x94(r1)
    lwz      r31, 0x8c(r1)
    lwz      r30, 0x88(r1)
    lwz      r29, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x90
    blr
*/

/**
 * @note Address: stage02 .text 0x8530
 * @note Size: 0x74
 */
extern "C" void Update__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam)
{
	Vec pos;

	if (cam->_40 != NULL) {
		cam->vf48(40.0f);
		PSMTXMultVec(cam->_40, &cam->_44, &pos);
		cam->vf3C(&pos);
	}
}

/**
 * @note Address: stage02 .text 0x85A4
 * @note Size: 0xC
 */
extern "C" void Start__Q23Car7CarRootFv(Car::CarRoot* car)
{
	CarRoot_Start(car);
}

/**
 * @note Address: stage02 .text 0x85B0
 * @note Size: 0x98
 */
// fn_5_85B0

/**
 * @note Address: stage02 .text 0x8648
 * @note Size: 0x8
 */
extern "C" s32 GetMode__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam)
{
	return 0;
}

/**
 * @note Address: stage02 .text 0x8650
 * @note Size: 0x8
 */
extern "C" s32 GetFlags__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam)
{
	return 11;
}

/**
 * @note Address: stage02 .text 0x8658
 * @note Size: 0xC
 */
extern "C" f32 GetEaseDivisor__Q23Car16CCameraActionCarFv(Car::CCameraActionCar* cam)
{
	return 0.1f;
}

/**
 * @note Address: stage02 .text 0x8664
 * @note Size: 0xB4
 */
// fn_5_8664
