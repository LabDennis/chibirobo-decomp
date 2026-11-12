#include "game/hsd_jobj.h"
#include "game/ut.h"
#include "game/gmath.h"

extern "C" {
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/random.h>
}

// dol functions
extern "C" u8 gGame[];
extern "C" s32 IsGameSoundPlaying(void* se, s32 id);
extern "C" void PlayGameSound(void* se, s32 id, s32 vol);
extern "C" void SetGsBlendMode(void* gx, s32 a, s32 b, s32 c, s32 d);
extern "C" void DrawGsCircle(void* game, s32 kind, s32 color, s32 color2, f32 x, f32 y, f32 z, f32 w, f32 h, f32 vx, f32 vy, f32 vz);

// CGame, with the fields past the symbol's 0xA6C30 size
#define GAME_FIELD(type, offset) (*(type*)(gGame + (offset)))
// an offset into CGame: japan lays it out differently from the others
#if defined(VERSION_JP)
#define GAME_OFS(other, japan) (japan)
#else
#define GAME_OFS(other, japan) (other)
#endif
// CGame::mStage
#if defined(VERSION_JP)
#define GAME_STAGE 0x342C
#elif defined(VERSION_PAL)
#define GAME_STAGE 0x344C
#else
#define GAME_STAGE 0x3444
#endif

struct _GXTexObj {
	u32 dummy[8]; // _00
};

// copied as one block, so it's its own struct
struct UnkTexMtxs {
	Mtx _00;      // _00
	Mtx _30;      // _30
	u8 _60[0x60]; // _60
};

struct TPLDescriptor;

class CTexObj : public _GXTexObj {
public:
	u32 _20;        // _20
	u32 _24;        // _24
	u32 _28;        // _28
	u32 _2C;        // _2C
	u32 _30;        // _30
	u32 _34;        // _34
	s32 _38;        // _38
	UnkTexMtxs _3C; // _3C

	CTexObj() { }
	virtual ~CTexObj() { }

	CTexObj& Init(TPLDescriptor* desc, u32 tlutName);
};

// xobj style actor from the dol
struct UnkDolObjVtbl {
	u8 _00[0x80];                                                                                       // _00
	void (*_80)(void* obj, f32 frame, f32 rate, f32 start, f32 end, s32 a, s32 b, s32 c, s32 d, s32 e); // _80
	u8 _84[0x88];                                                                                       // _84
	void (*_10C)(void* obj, Vec* pos, f32* x, f32* y, f32* z, s32 flag);                                // _10C
	f32 (*_110)(void* obj);                                                                             // _110
	f32 (*_114)(void* obj);                                                                             // _114
	BOOL (*_118)(void* obj, f32 a, f32 b, f32* outA, f32* outB, s32 flag);                              // _118
};

struct UnkDolObj {
	HSD_JObj* _00;       // _00
	u8 _04[0x30];        // _04
	UnkDolObjVtbl* vtbl; // _34
#if defined(VERSION_JP)
	u8 _38[0x88]; // _38
#else
	u8 _38[0x90]; // _38
#endif
	CVec _C8;       // _C8, japan has it and what follows 8 bytes earlier
	u8 _D8[0x150];  // _D8
	u64 _228;       // _228
	u8 _230[0x14];  // _230
	s32* _244;      // _244
	u8 _248[0x804]; // _248
	u32* _A4C;      // _A4C
	u8 _A50[0x1C];  // _A50
	u8 _A6C[0x100]; // _A6C
	s32 _B6C;       // _B6C
};

class CCamera;

class CInterfaceCameraAction {
public:
	CVec _00; // _00
	CVec _10; // _10
	CVec _20; // _20
	f32 _30;  // _30
	f32 _34;  // _34
	s32 _38;  // _38

	virtual ~CInterfaceCameraAction() { }       // _08
	virtual void PreviousUpdate() { }           // _0C
	virtual void Update() = 0;                  // _10
	virtual void Reset() { }                    // _14
	virtual void SetCamera(CCamera* camera) { } // _18
	virtual void Init() { }                     // _1C
	virtual void vf20() { }                     // _20
	virtual void vf24() { }                     // _24
	virtual void vf28() { }                     // _28
	virtual void SetEye(f32 x, f32 y, f32 z)    // _2C
	{
		_00.x = x;
		_00.y = y;
		_00.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z) // _30
	{
		_10.x = x;
		_10.y = y;
		_10.z = z;
	}
	virtual void SetAngle(f32 x, f32 y, f32 z) // _34
	{
		_20.x = x;
		_20.y = y;
		_20.z = z;
	}
	virtual void SetEye(const Vec& v) { SetEye(v.x, v.y, v.z); }           // _38
	virtual void SetInterest(const Vec& v) { SetInterest(v.x, v.y, v.z); } // _3C
	virtual void SetAngle(const Vec& v) { SetAngle(v.x, v.y, v.z); }       // _40
	virtual void SetDistance(f32 v) { _30 = v; }                           // _44
	virtual void SetFov(f32 v) { _34 = v; }                                // _48
	virtual void SetAngleZ(f32 v) { _20.z = v; }                           // _4C
	virtual s32 GetMode() { return 0; }                                    // _50
	virtual s32 GetFlags() { return _38; }                                 // _54
	virtual f32 GetEaseDivisor() { return 12.0f; }                         // _58
	virtual s32 vf5C() { return 0; }                                       // _5C
	virtual s32 GetTarget() { return 0; }                                  // _60
};

inline Vec MakeVec(f32 x, f32 y, f32 z)
{
	Vec v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

namespace fighter {

class CCameraActionFighter : public CInterfaceCameraAction {
public:
	CCameraActionFighter()
	{
		_40   = NULL;
		_44.x = _44.y = _44.z = 0.0f;
		_50.x = _50.y = _50.z = 0.0f;
		_68                   = true;
	}
	virtual ~CCameraActionFighter() { }

	void SetView(Vec at, Vec eye, MtxPtr mtx)
	{
		_68 = false;
		_50 = at;
		_44 = eye;
		_40 = mtx;
	}

	virtual void Update();
	virtual void Init() { _68 = false; }
	virtual s32 GetMode() { return 0; }
	virtual s32 GetFlags() { return 11; }
	virtual f32 GetEaseDivisor() { return 0.1f; }

	MtxPtr _40;  // _40
	Vec _44;     // _44
	Vec _50;     // _50
	u8 _5C[0xC]; // _5C
	bool _68;    // _68
};

} // namespace fighter

// one of the things dropped into the stage
struct Mato {
	Mato()
	{
		_04 = NULL;
		_00 = -1;
		_14 = 0xFFFFFFC8;
	}

	CVec GetPos() const { return _04->_C8; }

	s32 _00;              // _00
	UnkDolObj* _04;       // _04
	f32 _08;              // _08
	f32 _0C;              // _0C
	f32 _10;              // _10
	u32 _14;              // _14
	u8 _18;               // _18
	u8 _19;               // _19
	u8 _1A[0x2];          // _1A
	s32 _1C;              // _1C
	f32 _20;              // _20
	s32 _24;              // _24
	s32 _28;              // _28
	s32 _2C;              // _2C
	struct MatoList* _30; // _30
};

struct MatoList {
	s32 _00;        // _00
	Vec _04;        // _04
	s32 _10;        // _10
	s32 _14;        // _14
	f32 _18;        // _18
	u32 _1C;        // _1C
	s32 _20[2];     // _20
	Mato* _28;      // _28
	Mato _2C[0x50]; // _2C
};

struct Shot {
	Shot() { }
	~Shot() { }

	void Start(Vec from, Vec to, Mato* item)
	{
		u32 i;
		_00   = true;
		_04   = from;
		_10   = to;
		_1C   = from;
		_B0   = item;
		_34   = 60;
		_28.x = (_10.x - _04.x) / _34;
		_28.y = (_10.y - _04.y) / _34;
		_28.z = (_10.z - _04.z) / _34;
		for (i = 0; i < 10; i++) {
			_38[i] = _1C;
		}
	}

	u8 _00;      // _00
	u8 _01[0x3]; // _01
	Vec _04;     // _04
	Vec _10;     // _10
	Vec _1C;     // _1C
	Vec _28;     // _28
	s32 _34;     // _34
	Vec _38[10]; // _38
	Mato* _B0;   // _B0
	CStatus _B4; // _B4
};

// where the player is aiming and what's under it
struct FighterAim {
	FighterAim() { _18 = NULL; }

	Vec _00;   // _00
	f32 _0C;   // _0C
	f32 _10;   // _10
	f32 _14;   // _14
	Mato* _18; // _18
};

struct ShotList {
	void Add(Vec from, Vec to, Mato* item)
	{
		u32 i;
		for (i = 0; i < 5; i++) {
			if (!_00[i]._00) {
				break;
			}
		}
		if (i < 5) {
			_00[i].Start(from, to, item);
			PlayGameSound(gGame + GAME_OFS(0xE7E24, 0xE713C), 11, 0xFF);
		}
	}

	Shot _00[5]; // _00
	s32 _3AC;    // _3AC
	s32 _3B0;    // _3B0
};

struct Fighter {
	~Fighter() { }

	UnkDolObj* _00; // _00
	UnkDolObj* _04; // _04
	UnkDolObj* _08; // _08
	Mtx _0C;        // _0C
	Mtx _3C;        // _3C
	Mtx _6C;        // _6C
	f32 _9C;        // _9C
	f32 _A0;        // _A0
	f32 _A4;        // _A4
	f32 _A8;        // _A8
	f32 _AC;        // _AC
	u8* _B0;        // _B0
	u8* _B4;        // _B4
	u32 _B8;        // _B8
	u32 _BC;        // _BC
	u8 _C0[0x8];    // _C0
	Vec _C8;        // _C8
	u8 _D4[0xC];    // _D4
	f32 _E0;        // _E0
	f32 _E4;        // _E4
	f32 _E8;        // _E8
	f32 _EC;        // _EC
	s32 _F0;        // _F0
	FighterAim _F4; // _F4
	Mato* _110;     // _110
	ShotList _114;  // _114
};

struct PilotNwing {
	UnkDolObj* _00; // _00
	UnkDolObj* _04; // _04
	UnkDolObj* _08; // _08
	UnkDolObj* _0C; // _0C
	Mtx _10;        // _10
	Mtx _40;        // _40
	Mtx _70;        // _70
	f32 _A0;        // _A0
	f32 _A4;        // _A4
};

struct FighterPanels {
	s32 _00; // _00
	s32 _04; // _04
	f32 _08; // _08
	f32 _0C; // _0C
	s32 _10; // _10
	s32 _14; // _14
	s32 _18; // _18
	s32 _1C; // _1C
	s32 _20; // _20
};

// little endian words out of a loaded file
#define READ_LE32(p) (((u8*)(p))[0] + (((u8*)(p))[1] * 0x100) + (((u8*)(p))[2] * 0x10000) + (((u8*)(p))[3] * 0x1000000))
#define GET_LE32(p)  ((p) != NULL ? READ_LE32(p) : 0)

// todo: copied inline, clean up
inline f32 HSD_AObjGetEndFrame(void* aobj)
{
	((aobj) ? ((void)0) : __assert("aobj.h", 171, "aobj"));
	return ((f32*)aobj)[3];
}

extern "C" void GXProject(f32 x, f32 y, f32 z, Mtx mtx, f32* pm, f32* vp, f32* sx, f32* sy, f32* sz);
extern "C" u16 GXGetTexObjWidth(_GXTexObj* obj);
extern "C" u16 GXGetTexObjHeight(_GXTexObj* obj);
extern "C" void SetGsLineWidth(void* gx, s32 a, s32 b);
extern "C" void SetGsTevMode(void* gx, s32 mode);
extern "C" void DrawGsLineStrip(void* game, Vec* pts, s32 color, s32 n);
extern "C" void DrawGsLineSegment(void* game, s32 a, s32 color, s32 color2, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1);
extern "C" void DrawGsTexturedQuad(void* game, CTexObj* tex, s32 c0, s32 c1, s32 c2, s32 c3, f32 x, f32 y, f32 z, f32 w, f32 h, f32 u,
                                   f32 v, f32 rot);
extern "C" s32 FindStageIndexByName(const char* name);
extern "C" const char* GetLocalizedInterfaceText(s32 id);
extern "C" f32 MeasureGameTextWidth(void* game, const char* text, s32 a, f32 size);
extern "C" void DrawGsQuad(void* game, s32 r, s32 g, s32 b, s32 a, f32 x, f32 y, f32 z, f32 w, f32 h, f32 u, f32 v, f32 rot);
class CGs;
extern "C" void DrawGsProportionalTextFormat(CGs* game, s32 color, const char* text, f32 x, f32 y, f32 z, f32 sx, f32 sy, ...);
extern "C" f64 sin(f64 x);
extern "C" f64 cos(f64 x);

extern "C" void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
extern "C" void PSMTXRotRad(Mtx m, char axis, f32 rad);
extern "C" f64 atan2(f64 y, f64 x);

// hangs off CGame, stage model jobj at _00
struct UnkGame_96BC {
	HSD_JObj* _00; // _00
#if defined(VERSION_JP)
	u8 _04[0x96C]; // _04
#else
	u8 _04[0x97C]; // _04
#endif
	u32 _980;        // _980, _970 in japan
	u8 _984[0x8CD4]; // _984
	f32 _9658;       // _9658
	f32 _965C;       // _965C
#if defined(VERSION_JP)
	u8 _9660[0x96A4]; // _9660
#else
	u8 _9660[0x9794]; // _9660
#endif
	s32 _12DF4; // _12DF4, _12CF4 in japan
};

inline void ResetModelFlags(UnkGame_96BC* model)
{
	model->_980 = 0;
	model->_980 |= 0x20000000;
	model->_980 |= 7;
	model->_12DF4 = 1;
}

// shadow offsets for the hud captions
f32 captionShadowX[4] = { -2.0f, 2.0f, 2.0f, -2.0f };
f32 captionShadowY[4] = { -2.0f, -2.0f, 2.0f, 2.0f };

u8 lbl_5_bss_0;
CStatus shotEvents;
f32 reticleSpin[10];
extern fighter::CCameraActionFighter fighterCamera;
extern MatoList matoList;
extern Fighter fighterWork;
extern PilotNwing pilotNwing;
extern FighterPanels panels;
extern s32 fireHoldFrames;
extern u8* soundCueTable[2];

extern "C" void DrawScoreHud(u32 score, u32 time, u32 gauge);
extern "C" void Fighter_Fire(Fighter* hole, s32 n);
extern "C" void Fighter_Update(Fighter* hole, u32 dt);
extern "C" void PilotNwing_Update(PilotNwing* link, u32 dt, f32 frame);
extern "C" void Fighter_Init(Fighter* hole, UnkDolObj* a, UnkDolObj* b, void* c);
// see game/cmotioncontrolcamera.h
class CMotionControlCamera {
public:
	void ChangeAction(CInterfaceCameraAction* action);
};
extern "C" void StopGameSfx(void* se, s32 id);
extern "C" void FighterPanels_Draw(FighterPanels* fade);
extern "C" void FighterPanels_Update(FighterPanels* fade, u32 dt);
// game camera
struct UnkGame_E5D38 {
	u8 _00[0xE8];  // _00
	Mtx _E8;       // _E8
	u8 _118[0x44]; // _118
	f32 _15C[7];   // _15C
	f32 _178[6];   // _178
};

extern "C" void FighterAim_Draw(FighterAim* aim, UnkGame_E5D38* cam, Mtx m);
extern "C" void ShotList_Update(ShotList* list, u32 dt);

/**
 * @note Address: stage02 .text 0x0
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0xC
 * @note Size: 0x98
 */
// generated dtor

/**
 * @note Address: stage02 .text 0xA4
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0xAC
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0xB4
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0xC0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xC4
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xC8
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xCC
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD4
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0xD8
 * @note Size: 0x38
 */
// SetEye
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lfs      f1, 0x0(r4)
    stw      r0, 0x14(r1)
    lfs      f2, 0x4(r4)
    lwz      r12, 0x3c(r3)
    lfs      f3, 0x8(r4)
    lwz      r12, 0x2c(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x110
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x120
 * @note Size: 0x38
 */
// SetInterest
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lfs      f1, 0x0(r4)
    stw      r0, 0x14(r1)
    lfs      f2, 0x4(r4)
    lwz      r12, 0x3c(r3)
    lfs      f3, 0x8(r4)
    lwz      r12, 0x30(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x158
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x168
 * @note Size: 0x38
 */
// CInterfaceCameraAction::SetAngle(const Vec&)
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lfs      f1, 0x0(r4)
    stw      r0, 0x14(r1)
    lfs      f2, 0x4(r4)
    lwz      r12, 0x3c(r3)
    lfs      f3, 0x8(r4)
    lwz      r12, 0x34(r12)
    mtctr    r12
    bctrl
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x1A0
 * @note Size: 0x10
 */
// inline

/**
 * @note Address: stage02 .text 0x1B0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1B8
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1C0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1C8
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1D0
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x1D8
 * @note Size: 0x16D4
 */
// draws a caption with a black shadow on every side
inline void DrawCaption(s32 id, f32 x, f32 y)
{
	s32 i;
	f32 cx;
	f32 cy;

	DrawGsQuad(gGame, 0xFF, 0xFF, 0xFF, 0xFF, x, y, 0.0f,
	           1.0f * (MeasureGameTextWidth(gGame, GetLocalizedInterfaceText(id), 1, 20.0f) - 4.0f), 12.0f, 0.0f, 0.0f, 0.0f);
	cx = x - MeasureGameTextWidth(gGame, GetLocalizedInterfaceText(id), 1, 20.0f) * 0.5f;
	cy = y - 13.0f;
	for (i = 0; i < 4; i++) {
		DrawGsProportionalTextFormat((CGs*)gGame, 0xFF, GetLocalizedInterfaceText(id), cx + captionShadowX[i], cy + captionShadowY[i], 0.0f,
		                             20.0f, 20.0f);
	}
	DrawGsProportionalTextFormat((CGs*)gGame, -1, GetLocalizedInterfaceText(id), cx, cy, 0.0f, 20.0f, 20.0f);
}

extern "C" void DrawScoreHud(u32 score, u32 time, u32 gauge)
{
	CTexObj tex;
	s32 i;
	s32 d;
	f32 x;
	f32 left;
	f32 digW;
	f32 digH;
	f32 step;
	f32 w;
	u32 base;
	s32 hi;
	s32 lo;

	if (time >= 100) {
		time = 99;
	}
	if (FindStageIndexByName("stage02") == GAME_FIELD(s32, GAME_STAGE)) {
		for (i = 0; i < 48; i++) {
			UnkDolObj* obj = (UnkDolObj*)(GAME_FIELD(u8*, GAME_OFS(0x96F8, 0x96B0)) + i * GAME_FIELD(s32, GAME_OFS(0x9708, 0x96C0)));
			s32 kind       = obj->_244 != NULL ? *obj->_244 : -1;
			if (kind != 0x149) {
				continue;
			}
			if (obj->_B6C == 0) {
				break;
			}

			// score
			tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x18), 0);
			w   = 1.0f * GXGetTexObjWidth(&tex);
			x   = 640.0f - w * 0.5f - 40.0f;
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, x, 40.0f, 0.0f, w, 1.0f * GXGetTexObjHeight(&tex), 0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
#if !defined(VERSION_JP)
			{
				CTexObj unused;
				DrawCaption(0x5A, x, 40.0f);
			}
#endif
			tex  = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 8), 0);
			left = 20.0f + (x - 12.0f - 88.0f);
			digW = 0.8f * (1.0f * (GXGetTexObjWidth(&tex) / 11));
			digH = 0.8f * (1.0f * GXGetTexObjHeight(&tex));
			PSMTXScale(tex._3C._30, 0.09090909f, 1.0f, 1.0f);
			tex._38 = 1;
			base    = 255.0f;
			hi      = base | 0xFAE0CF00;
			lo      = base | 0xE9813A00;
			step    = 7.0f * digW;
			for (d = 0; d < 4; d++) {
				f32 div = 1.0f;
				s32 k;
				s32 digit;
				for (k = 0; k < d; k++) {
					div *= 10.0f;
				}
				digit = score / div;
				if (d == 0 || digit != 0) {
					PSMTXTrans(tex._3C._00, 0.09090909f * (digit % 10), 0.0f, 0.0f);
					tex._38 = 1;
					SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
					DrawGsTexturedQuad(gGame, &tex, hi, hi, lo, lo, step + (left - d * digW) - 32.0f, 68.0f, 0.0f, digW, digH, 0.0f, 0.0f,
					                   0.0f);
					SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				}
			}

			// time
			tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x20), 0);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 320.0f, 40.0f, 0.0f, 1.0f * GXGetTexObjWidth(&tex),
			                   1.0f * GXGetTexObjHeight(&tex), 0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
#if !defined(VERSION_JP)
			{
				CTexObj unused;
				DrawCaption(0x5B, 320.0f, 40.0f);
			}
#endif
			tex  = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 8), 0);
			digW = 1.4f * (1.0f * (GXGetTexObjWidth(&tex) / 11));
			digH = 1.4f * (1.0f * GXGetTexObjHeight(&tex));
			left = 110.0f + 11.0f;
			PSMTXScale(tex._3C._30, 0.09090909f, 1.0f, 1.0f);
			hi      = base | 0x1F33FF00;
			lo      = base | 0xFFFFFF00;
			step    = 7.0f * digW;
			tex._38 = 1;
			for (d = 0; d < 2; d++) {
				f32 div = 1.0f;
				s32 k;
				s32 digit;
				for (k = 0; k < d; k++) {
					div *= 10.0f;
				}
				digit = time / div;
				if (d == 0 || digit != 0) {
					PSMTXTrans(tex._3C._00, 0.09090909f * (digit % 10), 0.0f, 0.0f);
					tex._38 = 1;
					SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
					DrawGsTexturedQuad(gGame, &tex, lo, lo, hi, hi, step + (left - d * (digW - 8.0f)) - 32.0f, 76.0f, 0.0f, digW, digH,
					                   0.0f, 0.0f, 0.0f);
					SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				}
			}
			digW = 1.4f * (1.0f * (GXGetTexObjWidth(&tex) / 11));
			digH = 1.4f * (1.0f * GXGetTexObjHeight(&tex));
			left = 110.0f + 11.0f;
			PSMTXTrans(tex._3C._00, 0.90909094f, 0.0f, 0.0f);
			tex._38 = 1;
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			s32 lead = 2;
			if (time < 10)
				lead = 1;
			DrawGsTexturedQuad(gGame, &tex, lo, lo, hi, hi, 7.0f * digW + (left - lead * (digW - 8.0f)) - 32.0f, 76.0f, 0.0f, digW, digH,
			                   0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);

			// gauge
			tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x28), 0);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 44.0f, 0.0f, 1.0f * GXGetTexObjWidth(&tex),
			                   1.0f * GXGetTexObjHeight(&tex), 0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
#if !defined(VERSION_JP)
			{
				CTexObj unused;
				DrawCaption(0x5C, 100.0f, 44.0f);
			}
#endif
			tex  = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x30), 0);
			w    = 1.0f * GXGetTexObjWidth(&tex);
			digH = 1.0f * GXGetTexObjHeight(&tex);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 72.0f, 0.0f, w, digH, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
			tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x38), 0);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f - w * 0.5f + gauge * w / 100.0f, 72.0f, 0.0f,
			                   1.0f * GXGetTexObjWidth(&tex), 1.0f * GXGetTexObjHeight(&tex), 0.0f, 0.0f, 0.0f);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
			break;
		}
	}
}

// DrawScoreHud
/*
    stwu     r1, -0x4c0(r1)
    mflr     r0
    stw      r0, 0x4c4(r1)
    stfd     f31, 0x4b0(r1)
    psq_st   f31, 0x4b8(r1), 0, qr0
    stfd     f30, 0x4a0(r1)
    psq_st   f30, 0x4a8(r1), 0, qr0
    stfd     f29, 0x490(r1)
    psq_st   f29, 0x498(r1), 0, qr0
    stfd     f28, 0x480(r1)
    psq_st   f28, 0x488(r1), 0, qr0
    stfd     f27, 0x470(r1)
    psq_st   f27, 0x478(r1), 0, qr0
    stmw     r17, 0x434(r1)
    mr       r26, r4
    lis      r4, __vt__7CTexObj@ha
    addi     r0, r4, __vt__7CTexObj@l
    mr       r27, r3
    cmplwi   r26, 0x64
    stw      r0, 0x404(r1)
    mr       r23, r5
    blt      .L_00000234
    li       r26, 0x63
.L_00000234:
    lis      r4, gGame@ha
    lis      r3, lbl_5_rodata_8C@ha
    addi     r4, r4, gGame@l
    lwz      r17, 0x3444(r4)
    addi     r3, r3, lbl_5_rodata_8C@l
    bl       FindStageIndexByName
    cmpw     r3, r17
    bne      .L_00001864
    lis      r3, gGame@ha
    li       r0, 0x30
    addi     r3, r3, gGame@l
    li       r4, 0x0
    addis    r3, r3, 0x1
    lwz      r6, -0x68f8(r3)
    lwz      r3, -0x6908(r3)
    mtctr    r0
.L_00000274:
    add      r28, r3, r4
    lwz      r5, 0x244(r28)
    cmplwi   r5, 0x0
    beq      .L_0000028C
    lwz      r0, 0x0(r5)
    b        .L_00000290
.L_0000028C:
    li       r0, -0x1
.L_00000290:
    cmpwi    r0, 0x149
    bne      .L_0000185C
    lwz      r0, 0xb6c(r28)
    cmpwi    r0, 0x0
    beq      .L_00001864
    lwz      r4, 0xa4c(r28)
    addi     r3, r28, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x18
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    addi     r25, r1, 0x344
    lwz      r6, 0x4(r3)
    li       r0, 0x18
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_00000348:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00000348
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x40c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x408(r1)
    addi     r3, r1, 0x308
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x408(r1)
    fsubs    f0, f0, f1
    fmuls    f27, f2, f0
    bl       GXGetTexObjHeight
    lis      r4, lbl_5_rodata_9C@ha
    lis      r5, lbl_5_rodata_98@ha
    addi     r6, r4, lbl_5_rodata_9C@l
    lfs      f1, lbl_5_rodata_98@l(r5)
    lfs      f2, 0x0(r6)
    lis      r4, lbl_5_rodata_A0@ha
    addi     r5, r4, lbl_5_rodata_A0@l
    mr       r17, r3
    fnmsubs  f1, f27, f2, f1
    lfs      f0, 0x0(r5)
    lis      r4, gGame@ha
    addi     r31, r4, gGame@l
    fsubs    f29, f1, f0
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    clrlwi   r4, r17, 16
    lis      r0, 0x4330
    stw      r4, 0x414(r1)
    lis      r3, lbl_5_rodata_108@ha
    lis      r5, lbl_5_rodata_A4@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    stw      r0, 0x410(r1)
    lis      r4, gFighterHudTextureScale@ha
    lfs      f3, lbl_5_rodata_A4@l(r5)
    addi     r3, r4, gFighterHudTextureScale@l
    lfd      f0, 0x410(r1)
    lis      r4, lbl_5_rodata_A0@ha
    lfs      f5, 0x0(r3)
    lis      r3, gGame@ha
    fsubs    f0, f0, f1
    lfs      f2, lbl_5_rodata_A0@l(r4)
    fmr      f1, f29
    addi     r3, r3, gGame@l
    fmr      f4, f27
    addi     r4, r1, 0x308
    fmr      f6, f3
    li       r5, -0x1
    fmr      f7, f3
    li       r6, -0x1
    fmr      f8, f3
    li       r7, -0x1
    fmuls    f5, f5, f0
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r4, __vt__7CTexObj@ha
    li       r3, 0x5a
    addi     r0, r4, __vt__7CTexObj@l
    stw      r0, 0x304(r1)
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r3, lbl_5_rodata_AC@ha
    lis      r4, gFighterHudTextureScale@ha
    addi     r5, r3, lbl_5_rodata_AC@l
    lis      r6, gGame@ha
    lfs      f0, 0x0(r5)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    addi     r5, r4, gFighterHudTextureScale@l
    fsubs    f1, f1, f0
    lfs      f0, 0x0(r5)
    lis      r4, lbl_5_rodata_A0@ha
    lis      r3, lbl_5_rodata_B0@ha
    addi     r5, r4, lbl_5_rodata_A0@l
    fmr      f6, f3
    addi     r4, r3, lbl_5_rodata_B0@l
    fmuls    f4, f0, f1
    lfs      f2, 0x0(r5)
    addi     r3, r6, gGame@l
    lfs      f5, 0x0(r4)
    fmr      f1, f29
    fmr      f7, f3
    fmr      f8, f3
    li       r4, 0xff
    li       r5, 0xff
    li       r6, 0xff
    li       r7, 0xff
    bl       DrawGsQuad
    li       r3, 0x5a
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r4, lbl_5_rodata_9C@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_9C@l
    lis      r3, lbl_5_rodata_B4@ha
    lfs      f0, 0x0(r5)
    addi     r5, r3, lbl_5_rodata_B4@l
    lis      r4, captionShadowY@ha
    lis      r3, captionShadowX@ha
    fnmsubs  f28, f1, f0, f29
    addi     r29, r4, captionShadowY@l
    addi     r30, r3, captionShadowX@l
    lis      r4, lbl_5_rodata_A4@ha
    lis      r3, lbl_5_rodata_A8@ha
    lfs      f27, 0x0(r5)
    mr       r19, r29
    mr       r20, r30
    addi     r21, r6, gGame@l
    addi     r18, r4, lbl_5_rodata_A4@l
    addi     r17, r3, lbl_5_rodata_A8@l
    li       r22, 0x0
.L_00000570:
    li       r3, 0x5a
    bl       GetLocalizedInterfaceText
    lfs      f1, 0x0(r20)
    mr       r5, r3
    lfs      f0, 0x0(r19)
    mr       r3, r21
    lfs      f4, 0x0(r17)
    fadds    f1, f28, f1
    fadds    f2, f27, f0
    lfs      f3, 0x0(r18)
    fmr      f5, f4
    li       r4, 0xff
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    addi     r22, r22, 0x1
    addi     r20, r20, 0x4
    cmpwi    r22, 0x4
    addi     r19, r19, 0x4
    blt      .L_00000570
    li       r3, 0x5a
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, lbl_5_rodata_B4@ha
    lfs      f4, lbl_5_rodata_A8@l(r4)
    lis      r5, lbl_5_rodata_A4@ha
    addi     r4, r5, lbl_5_rodata_A4@l
    fmr      f1, f28
    fmr      f5, f4
    lfs      f3, 0x0(r4)
    lis      r7, gGame@ha
    lfs      f2, lbl_5_rodata_B4@l(r6)
    mr       r5, r3
    addi     r3, r7, gGame@l
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    lwz      r4, 0xa4c(r28)
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    addi     r3, r28, 0xa6c
    lwz      r4, 0x8(r4)
    li       r5, 0x0
    stw      r0, 0x304(r1)
    addi     r4, r4, 0x8
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_000006AC:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_000006AC
    lis      r4, lbl_5_rodata_B0@ha
    lis      r3, lbl_5_rodata_B8@ha
    lfs      f1, lbl_5_rodata_B0@l(r4)
    lis      r5, lbl_5_rodata_A8@ha
    lfs      f0, lbl_5_rodata_B8@l(r3)
    addi     r3, r1, 0x308
    fsubs    f1, f29, f1
    lfs      f2, lbl_5_rodata_A8@l(r5)
    fsubs    f0, f1, f0
    fadds    f27, f2, f0
    bl       GXGetTexObjWidth
    lis      r4, 0x2e8c
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    lis      r5, gFighterHudTextureScale@ha
    subi     r4, r4, 0x5d17
    lis      r6, gFighterHudFullAlpha@ha
    mulhw    r4, r4, r3
    stw      r0, 0x410(r1)
    lis      r3, lbl_5_rodata_110@ha
    lfs      f2, gFighterHudTextureScale@l(r5)
    lfd      f1, lbl_5_rodata_110@l(r3)
    lfs      f3, gFighterHudFullAlpha@l(r6)
    srawi    r0, r4, 1
    addi     r3, r1, 0x308
    srwi     r4, r0, 31
    add      r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x414(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fmuls    f29, f3, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x40c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x408(r1)
    lis      r3, gFighterHudFullAlpha@ha
    addi     r5, r3, gFighterHudFullAlpha@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x408(r1)
    lis      r3, lbl_5_rodata_C0@ha
    lfs      f4, 0x0(r5)
    fmr      f3, f2
    fsubs    f0, f0, f1
    addi     r22, r1, 0x374
    lfs      f1, lbl_5_rodata_C0@l(r3)
    mr       r3, r22
    fmuls    f0, f2, f0
    fmuls    f28, f4, f0
    bl       PSMTXScale
    lis      r3, gFighterHudFullAlpha@ha
    li       r0, 0x1
    stw      r0, 0x340(r1)
    li       r21, 0x0
    lfs      f1, gFighterHudFullAlpha@l(r3)
    bl       __cvt_fp2unsigned
    lis      r4, lbl_5_rodata_C8@ha
    mr       r24, r3
    lfs      f0, lbl_5_rodata_C8@l(r4)
    lis      r3, lbl_5_rodata_108@ha
    oris     r18, r24, 0xfae0
    oris     r17, r24, 0xe981
    fmuls    f30, f0, f29
    lis      r4, gFighterHudTextureScale@ha
    lfd      f31, lbl_5_rodata_108@l(r3)
    ori      r18, r18, 0xcf00
    ori      r17, r17, 0x3a00
    addi     r19, r4, gFighterHudTextureScale@l
    lis      r20, 0x4330
.L_000007E8:
    cmpwi    r21, 0x0
    lfs      f1, 0x0(r19)
    ble      .L_00000870
    li       r5, 0x0
    ble      .L_00000870
    cmpwi    r21, 0x8
    subi     r4, r21, 0x8
    ble      .L_0000084C
    addi     r0, r4, 0x7
    lis      r3, lbl_5_rodata_CC@ha
    srwi     r0, r0, 3
    lfs      f0, lbl_5_rodata_CC@l(r3)
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_0000084C
.L_00000824:
    fmuls    f1, f1, f0
    addi     r5, r5, 0x8
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    bdnz     .L_00000824
.L_0000084C:
    lis      r3, lbl_5_rodata_CC@ha
    subf     r0, r5, r21
    lfs      f0, lbl_5_rodata_CC@l(r3)
    mtctr    r0
    cmpw     r5, r21
    bge      .L_00000870
.L_00000864:
    fmuls    f1, f1, f0
    addi     r5, r5, 0x1
    bdnz     .L_00000864
.L_00000870:
    stw      r27, 0x414(r1)
    cmpwi    r21, 0x0
    stw      r20, 0x410(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f31
    fdivs    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x408(r1)
    lwz      r0, 0x40c(r1)
    beq      .L_000008A0
    cmpwi    r0, 0x0
    beq      .L_000009A8
.L_000008A0:
    lis      r4, 0x6666
    lis      r3, 0x4330
    addi     r5, r4, 0x6667
    stw      r3, 0x410(r1)
    mulhw    r6, r5, r0
    lis      r4, lbl_5_rodata_110@ha
    lis      r3, lbl_5_rodata_A4@ha
    lfd      f1, lbl_5_rodata_110@l(r4)
    lfs      f2, lbl_5_rodata_A4@l(r3)
    lis      r5, lbl_5_rodata_C0@ha
    addi     r4, r5, lbl_5_rodata_C0@l
    srawi    r6, r6, 2
    srwi     r5, r6, 31
    lfs      f4, 0x0(r4)
    add      r5, r6, r5
    fmr      f3, f2
    mulli    r5, r5, 0xa
    mr       r3, r25
    subf     r0, r5, r0
    xoris    r0, r0, 0x8000
    stw      r0, 0x414(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f4, f0
    bl       PSMTXTrans
    li       r0, 0x1
    lwz      r3, 0x294(r31)
    stw      r0, 0x340(r1)
    li       r4, 0x0
    bl       SetGsTevMode
    xoris    r3, r21, 0x8000
    lis      r0, 0x4330
    lis      r4, lbl_5_rodata_110@ha
    stw      r3, 0x40c(r1)
    addi     r5, r4, lbl_5_rodata_110@l
    lis      r3, lbl_5_rodata_A4@ha
    stw      r0, 0x408(r1)
    lis      r4, gFighterPanelYOffset@ha
    lfd      f1, 0x0(r5)
    addi     r5, r4, gFighterPanelYOffset@l
    lfd      f0, 0x408(r1)
    fmr      f4, f29
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r3, lbl_5_rodata_D4@ha
    fsubs    f1, f0, f1
    addi     r4, r3, lbl_5_rodata_D4@l
    lfs      f0, 0x0(r5)
    lis      r3, gGame@ha
    lfs      f2, 0x0(r4)
    fnmsubs  f1, f1, f29, f27
    fmr      f5, f28
    addi     r3, r3, gGame@l
    fmr      f6, f3
    mr       r5, r18
    fadds    f1, f30, f1
    fmr      f7, f3
    fmr      f8, f3
    mr       r6, r18
    fsubs    f1, f1, f0
    mr       r7, r17
    mr       r8, r17
    addi     r4, r1, 0x308
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
.L_000009A8:
    addi     r21, r21, 0x1
    cmpwi    r21, 0x4
    blt      .L_000007E8
    lwz      r4, 0xa4c(r28)
    addi     r3, r28, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x20
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_00000A54:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00000A54
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    mr       r18, r3
    addi     r3, r1, 0x308
    bl       GXGetTexObjHeight
    mr       r17, r3
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    lis      r4, 0x4330
    clrlwi   r5, r18, 16
    stw      r5, 0x414(r1)
    clrlwi   r0, r17, 16
    lis      r3, lbl_5_rodata_108@ha
    lis      r5, lbl_5_rodata_A4@ha
    stw      r4, 0x410(r1)
    lis      r6, lbl_5_rodata_D8@ha
    lfd      f2, lbl_5_rodata_108@l(r3)
    addi     r7, r6, lbl_5_rodata_D8@l
    lfd      f0, 0x410(r1)
    lis      r3, gFighterHudTextureScale@ha
    stw      r4, 0x408(r1)
    lis      r8, gGame@ha
    lfs      f3, lbl_5_rodata_A4@l(r5)
    addi     r5, r3, gFighterHudTextureScale@l
    lfs      f5, 0x0(r5)
    fsubs    f4, f0, f2
    stw      r0, 0x40c(r1)
    lis      r3, lbl_5_rodata_A0@ha
    addi     r6, r3, lbl_5_rodata_A0@l
    fmr      f6, f3
    lfd      f0, 0x408(r1)
    lfs      f1, 0x0(r7)
    addi     r3, r8, gGame@l
    fsubs    f0, f0, f2
    lfs      f2, 0x0(r6)
    fmuls    f4, f5, f4
    addi     r4, r1, 0x308
    fmr      f7, f3
    li       r5, -0x1
    fmr      f8, f3
    li       r6, -0x1
    fmuls    f5, f5, f0
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r4, __vt__7CTexObj@ha
    li       r3, 0x5b
    addi     r0, r4, __vt__7CTexObj@l
    stw      r0, 0x204(r1)
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r3, lbl_5_rodata_AC@ha
    lis      r4, gFighterHudTextureScale@ha
    addi     r5, r3, lbl_5_rodata_AC@l
    li       r7, 0xff
    lfs      f0, 0x0(r5)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, lbl_5_rodata_D8@ha
    fsubs    f1, f1, f0
    lfs      f0, gFighterHudTextureScale@l(r4)
    lis      r4, lbl_5_rodata_A0@ha
    addi     r6, r5, lbl_5_rodata_D8@l
    lis      r3, lbl_5_rodata_B0@ha
    fmr      f6, f3
    addi     r5, r4, lbl_5_rodata_A0@l
    addi     r4, r3, lbl_5_rodata_B0@l
    fmuls    f4, f0, f1
    lis      r3, gGame@ha
    lfs      f1, 0x0(r6)
    fmr      f7, f3
    lfs      f2, 0x0(r5)
    fmr      f8, f3
    lfs      f5, 0x0(r4)
    addi     r3, r3, gGame@l
    li       r4, 0xff
    li       r5, 0xff
    li       r6, 0xff
    bl       DrawGsQuad
    li       r3, 0x5b
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r3, lbl_5_rodata_9C@ha
    lis      r4, lbl_5_rodata_D8@ha
    addi     r5, r3, lbl_5_rodata_9C@l
    lfs      f0, lbl_5_rodata_D8@l(r4)
    lfs      f2, 0x0(r5)
    lis      r3, lbl_5_rodata_B4@ha
    addi     r5, r3, lbl_5_rodata_B4@l
    lis      r6, gGame@ha
    fnmsubs  f28, f1, f2, f0
    lis      r4, lbl_5_rodata_A4@ha
    lis      r3, lbl_5_rodata_A8@ha
    lfs      f27, 0x0(r5)
    mr       r17, r29
    mr       r18, r30
    addi     r21, r6, gGame@l
    addi     r20, r4, lbl_5_rodata_A4@l
    addi     r19, r3, lbl_5_rodata_A8@l
    li       r27, 0x0
.L_00000C40:
    li       r3, 0x5b
    bl       GetLocalizedInterfaceText
    lfs      f1, 0x0(r18)
    mr       r5, r3
    lfs      f0, 0x0(r17)
    mr       r3, r21
    lfs      f4, 0x0(r19)
    fadds    f1, f28, f1
    fadds    f2, f27, f0
    lfs      f3, 0x0(r20)
    fmr      f5, f4
    li       r4, 0xff
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    addi     r27, r27, 0x1
    addi     r18, r18, 0x4
    cmpwi    r27, 0x4
    addi     r17, r17, 0x4
    blt      .L_00000C40
    li       r3, 0x5b
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, lbl_5_rodata_B4@ha
    lfs      f4, lbl_5_rodata_A8@l(r4)
    lis      r5, lbl_5_rodata_A4@ha
    addi     r4, r5, lbl_5_rodata_A4@l
    fmr      f1, f28
    fmr      f5, f4
    lfs      f3, 0x0(r4)
    lis      r7, gGame@ha
    lfs      f2, lbl_5_rodata_B4@l(r6)
    mr       r5, r3
    addi     r3, r7, gGame@l
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    lwz      r4, 0xa4c(r28)
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    addi     r3, r28, 0xa6c
    lwz      r4, 0x8(r4)
    li       r5, 0x0
    stw      r0, 0x204(r1)
    addi     r4, r4, 0x8
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_00000D7C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00000D7C
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    lis      r4, 0x2e8c
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    lis      r5, gFighterHudTextureScale@ha
    subi     r4, r4, 0x5d17
    lis      r6, lbl_5_rodata_DC@ha
    mulhw    r4, r4, r3
    stw      r0, 0x410(r1)
    lis      r3, lbl_5_rodata_110@ha
    lfs      f2, gFighterHudTextureScale@l(r5)
    lfd      f1, lbl_5_rodata_110@l(r3)
    lfs      f3, lbl_5_rodata_DC@l(r6)
    srawi    r0, r4, 1
    addi     r3, r1, 0x308
    srwi     r4, r0, 31
    add      r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x414(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fmuls    f29, f3, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x40c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x408(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    lis      r4, lbl_5_rodata_E0@ha
    lfd      f0, 0x408(r1)
    lis      r6, lbl_5_rodata_DC@ha
    addi     r5, r4, lbl_5_rodata_E0@l
    lis      r3, gFighterHudTimerXOffset@ha
    fsubs    f0, f0, f1
    addi     r4, r3, gFighterHudTimerXOffset@l
    lfs      f4, lbl_5_rodata_DC@l(r6)
    lis      r3, lbl_5_rodata_C0@ha
    lfs      f27, 0x0(r5)
    fmr      f3, f2
    fmuls    f1, f2, f0
    lfs      f0, 0x0(r4)
    fadds    f27, f27, f0
    fmuls    f28, f4, f1
    lfs      f1, lbl_5_rodata_C0@l(r3)
    mr       r3, r22
    bl       PSMTXScale
    lis      r4, lbl_5_rodata_C8@ha
    lis      r3, lbl_5_rodata_108@ha
    lfs      f0, lbl_5_rodata_C8@l(r4)
    li       r5, 0x1
    oris     r18, r24, 0x1f33
    li       r0, -0x100
    fmuls    f30, f0, f29
    lis      r4, gFighterHudTextureScale@ha
    stw      r5, 0x340(r1)
    or       r17, r24, r0
    lfd      f31, lbl_5_rodata_108@l(r3)
    ori      r18, r18, 0xff00
    addi     r22, r4, gFighterHudTextureScale@l
    li       r19, 0x0
    lis      r21, 0x4330
.L_00000E9C:
    cmpwi    r19, 0x0
    lfs      f1, 0x0(r22)
    ble      .L_00000F24
    li       r5, 0x0
    ble      .L_00000F24
    cmpwi    r19, 0x8
    subi     r4, r19, 0x8
    ble      .L_00000F00
    addi     r0, r4, 0x7
    lis      r3, lbl_5_rodata_CC@ha
    srwi     r0, r0, 3
    lfs      f0, lbl_5_rodata_CC@l(r3)
    mtctr    r0
    cmpwi    r4, 0x0
    ble      .L_00000F00
.L_00000ED8:
    fmuls    f1, f1, f0
    addi     r5, r5, 0x8
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    fmuls    f1, f1, f0
    bdnz     .L_00000ED8
.L_00000F00:
    lis      r3, lbl_5_rodata_CC@ha
    subf     r0, r5, r19
    lfs      f0, lbl_5_rodata_CC@l(r3)
    mtctr    r0
    cmpw     r5, r19
    bge      .L_00000F24
.L_00000F18:
    fmuls    f1, f1, f0
    addi     r5, r5, 0x1
    bdnz     .L_00000F18
.L_00000F24:
    stw      r26, 0x414(r1)
    cmpwi    r19, 0x0
    stw      r21, 0x410(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f31
    fdivs    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x408(r1)
    lwz      r0, 0x40c(r1)
    beq      .L_00000F54
    cmpwi    r0, 0x0
    beq      .L_00001064
.L_00000F54:
    lis      r4, 0x6666
    lis      r3, 0x4330
    addi     r5, r4, 0x6667
    stw      r3, 0x410(r1)
    mulhw    r6, r5, r0
    lis      r4, lbl_5_rodata_110@ha
    lis      r3, lbl_5_rodata_A4@ha
    lfd      f1, lbl_5_rodata_110@l(r4)
    lfs      f2, lbl_5_rodata_A4@l(r3)
    lis      r5, lbl_5_rodata_C0@ha
    addi     r4, r5, lbl_5_rodata_C0@l
    srawi    r6, r6, 2
    srwi     r5, r6, 31
    lfs      f4, 0x0(r4)
    add      r5, r6, r5
    fmr      f3, f2
    mulli    r5, r5, 0xa
    mr       r3, r25
    subf     r0, r5, r0
    xoris    r0, r0, 0x8000
    stw      r0, 0x414(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f4, f0
    bl       PSMTXTrans
    li       r0, 0x1
    lwz      r3, 0x294(r31)
    stw      r0, 0x340(r1)
    li       r4, 0x0
    bl       SetGsTevMode
    xoris    r4, r19, 0x8000
    lis      r0, 0x4330
    stw      r4, 0x40c(r1)
    lis      r5, lbl_5_rodata_110@ha
    lis      r3, lbl_5_rodata_E8@ha
    lfd      f2, lbl_5_rodata_110@l(r5)
    lfs      f0, lbl_5_rodata_E8@l(r3)
    lis      r4, gFighterPanelYOffset@ha
    stw      r0, 0x408(r1)
    lis      r3, lbl_5_rodata_A4@ha
    fsubs    f0, f29, f0
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lfd      f1, 0x408(r1)
    addi     r5, r4, gFighterPanelYOffset@l
    lis      r3, lbl_5_rodata_EC@ha
    fmr      f4, f29
    fsubs    f1, f1, f2
    addi     r4, r3, lbl_5_rodata_EC@l
    lis      r6, gGame@ha
    lfs      f2, 0x0(r4)
    addi     r3, r6, gGame@l
    fmr      f5, f28
    fnmsubs  f1, f1, f0, f27
    lfs      f0, 0x0(r5)
    fmr      f6, f3
    mr       r5, r17
    fmr      f7, f3
    mr       r6, r17
    fadds    f1, f30, f1
    mr       r7, r18
    fmr      f8, f3
    mr       r8, r18
    addi     r4, r1, 0x308
    fsubs    f1, f1, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
.L_00001064:
    addi     r19, r19, 0x1
    cmpwi    r19, 0x2
    blt      .L_00000E9C
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    lis      r4, 0x2e8c
    lis      r0, 0x4330
    clrlwi   r3, r3, 16
    lis      r5, gFighterHudTextureScale@ha
    subi     r4, r4, 0x5d17
    lis      r6, lbl_5_rodata_DC@ha
    mulhw    r4, r4, r3
    stw      r0, 0x410(r1)
    lis      r3, lbl_5_rodata_110@ha
    lfs      f2, gFighterHudTextureScale@l(r5)
    lfd      f1, lbl_5_rodata_110@l(r3)
    lfs      f3, lbl_5_rodata_DC@l(r6)
    srawi    r0, r4, 1
    addi     r3, r1, 0x308
    srwi     r4, r0, 31
    add      r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x414(r1)
    lfd      f0, 0x410(r1)
    fsubs    f0, f0, f1
    fmuls    f0, f2, f0
    fmuls    f27, f3, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x40c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x408(r1)
    lis      r6, lbl_5_rodata_DC@ha
    lfs      f3, gFighterHudTextureScale@l(r4)
    lis      r3, lbl_5_rodata_A4@ha
    lfd      f0, 0x408(r1)
    addi     r7, r6, lbl_5_rodata_DC@l
    lis      r5, lbl_5_rodata_E0@ha
    lfs      f2, lbl_5_rodata_A4@l(r3)
    fsubs    f0, f0, f1
    lis      r4, gFighterHudTimerXOffset@ha
    addi     r6, r5, lbl_5_rodata_E0@l
    lfs      f4, 0x0(r7)
    addi     r5, r4, gFighterHudTimerXOffset@l
    lfs      f29, 0x0(r6)
    fmuls    f1, f3, f0
    lfs      f0, 0x0(r5)
    lis      r3, lbl_5_rodata_F0@ha
    fmr      f3, f2
    addi     r4, r3, lbl_5_rodata_F0@l
    fadds    f29, f29, f0
    fmuls    f28, f4, f1
    lfs      f1, 0x0(r4)
    mr       r3, r25
    bl       PSMTXTrans
    li       r0, 0x1
    lwz      r3, 0x294(r31)
    stw      r0, 0x340(r1)
    li       r4, 0x0
    bl       SetGsTevMode
    cmplwi   r26, 0xa
    li       r0, 0x2
    bge      .L_00001170
    li       r0, 0x1
.L_00001170:
    xoris    r4, r0, 0x8000
    lis      r0, 0x4330
    stw      r4, 0x41c(r1)
    lis      r5, lbl_5_rodata_110@ha
    lis      r3, lbl_5_rodata_E8@ha
    lfd      f2, lbl_5_rodata_110@l(r5)
    lfs      f0, lbl_5_rodata_E8@l(r3)
    lis      r4, lbl_5_rodata_C8@ha
    stw      r0, 0x418(r1)
    addi     r5, r4, lbl_5_rodata_C8@l
    fsubs    f0, f27, f0
    lis      r3, lbl_5_rodata_A4@ha
    lfd      f1, 0x418(r1)
    lis      r4, gFighterPanelYOffset@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r3, lbl_5_rodata_EC@ha
    fsubs    f1, f1, f2
    lfs      f2, 0x0(r5)
    fmr      f4, f27
    mr       r5, r17
    fmr      f5, f28
    mr       r6, r17
    fnmsubs  f1, f1, f0, f29
    lfs      f0, gFighterPanelYOffset@l(r4)
    addi     r4, r3, lbl_5_rodata_EC@l
    fmr      f6, f3
    lis      r3, gGame@ha
    fmr      f7, f3
    fmadds   f1, f2, f27, f1
    lfs      f2, 0x0(r4)
    fmr      f8, f3
    addi     r3, r3, gGame@l
    mr       r7, r18
    mr       r8, r18
    fsubs    f1, f1, f0
    addi     r4, r1, 0x308
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0xa4c(r28)
    addi     r3, r28, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x28
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_000012B0:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_000012B0
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    mr       r18, r3
    addi     r3, r1, 0x308
    bl       GXGetTexObjHeight
    mr       r17, r3
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    lis      r4, 0x4330
    clrlwi   r5, r18, 16
    stw      r5, 0x424(r1)
    clrlwi   r0, r17, 16
    lis      r3, lbl_5_rodata_108@ha
    lis      r5, lbl_5_rodata_A4@ha
    stw      r4, 0x420(r1)
    lis      r6, lbl_5_rodata_F4@ha
    lfd      f2, lbl_5_rodata_108@l(r3)
    addi     r7, r6, lbl_5_rodata_F4@l
    lfd      f0, 0x420(r1)
    lis      r3, gFighterHudTextureScale@ha
    stw      r4, 0x428(r1)
    lis      r8, gGame@ha
    lfs      f3, lbl_5_rodata_A4@l(r5)
    addi     r5, r3, gFighterHudTextureScale@l
    lfs      f5, 0x0(r5)
    fsubs    f4, f0, f2
    stw      r0, 0x42c(r1)
    lis      r3, lbl_5_rodata_F8@ha
    addi     r6, r3, lbl_5_rodata_F8@l
    fmr      f6, f3
    lfd      f0, 0x428(r1)
    lfs      f1, 0x0(r7)
    addi     r3, r8, gGame@l
    fsubs    f0, f0, f2
    lfs      f2, 0x0(r6)
    fmuls    f4, f5, f4
    addi     r4, r1, 0x308
    fmr      f7, f3
    li       r5, -0x1
    fmr      f8, f3
    li       r6, -0x1
    fmuls    f5, f5, f0
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lis      r4, __vt__7CTexObj@ha
    li       r3, 0x5c
    addi     r0, r4, __vt__7CTexObj@l
    stw      r0, 0x104(r1)
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r3, lbl_5_rodata_AC@ha
    lis      r4, gFighterHudTextureScale@ha
    addi     r5, r3, lbl_5_rodata_AC@l
    li       r7, 0xff
    lfs      f0, 0x0(r5)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, lbl_5_rodata_F4@ha
    fsubs    f1, f1, f0
    lfs      f0, gFighterHudTextureScale@l(r4)
    lis      r4, lbl_5_rodata_F8@ha
    addi     r6, r5, lbl_5_rodata_F4@l
    lis      r3, lbl_5_rodata_B0@ha
    fmr      f6, f3
    addi     r5, r4, lbl_5_rodata_F8@l
    addi     r4, r3, lbl_5_rodata_B0@l
    fmuls    f4, f0, f1
    lis      r3, gGame@ha
    lfs      f1, 0x0(r6)
    fmr      f7, f3
    lfs      f2, 0x0(r5)
    fmr      f8, f3
    lfs      f5, 0x0(r4)
    addi     r3, r3, gGame@l
    li       r4, 0xff
    li       r5, 0xff
    li       r6, 0xff
    bl       DrawGsQuad
    li       r3, 0x5c
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, gGame@ha
    addi     r5, r4, lbl_5_rodata_A8@l
    lfs      f1, 0x0(r5)
    mr       r4, r3
    addi     r3, r6, gGame@l
    li       r5, 0x1
    bl       MeasureGameTextWidth
    lis      r3, lbl_5_rodata_9C@ha
    lis      r4, lbl_5_rodata_F4@ha
    addi     r5, r3, lbl_5_rodata_9C@l
    lfs      f0, lbl_5_rodata_F4@l(r4)
    lfs      f2, 0x0(r5)
    lis      r3, lbl_5_rodata_FC@ha
    addi     r5, r3, lbl_5_rodata_FC@l
    lis      r6, gGame@ha
    fnmsubs  f28, f1, f2, f0
    lis      r4, lbl_5_rodata_A4@ha
    lis      r3, lbl_5_rodata_A8@ha
    lfs      f27, 0x0(r5)
    addi     r19, r6, gGame@l
    addi     r18, r4, lbl_5_rodata_A4@l
    addi     r17, r3, lbl_5_rodata_A8@l
    li       r20, 0x0
.L_00001494:
    li       r3, 0x5c
    bl       GetLocalizedInterfaceText
    lfs      f1, 0x0(r30)
    mr       r5, r3
    lfs      f0, 0x0(r29)
    mr       r3, r19
    lfs      f4, 0x0(r17)
    fadds    f1, f28, f1
    fadds    f2, f27, f0
    lfs      f3, 0x0(r18)
    fmr      f5, f4
    li       r4, 0xff
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    addi     r20, r20, 0x1
    addi     r30, r30, 0x4
    cmpwi    r20, 0x4
    addi     r29, r29, 0x4
    blt      .L_00001494
    li       r3, 0x5c
    bl       GetLocalizedInterfaceText
    lis      r4, lbl_5_rodata_A8@ha
    lis      r6, lbl_5_rodata_FC@ha
    lfs      f4, lbl_5_rodata_A8@l(r4)
    lis      r5, lbl_5_rodata_A4@ha
    addi     r4, r5, lbl_5_rodata_A4@l
    fmr      f1, f28
    fmr      f5, f4
    lfs      f3, 0x0(r4)
    lis      r7, gGame@ha
    lfs      f2, lbl_5_rodata_FC@l(r6)
    mr       r5, r3
    addi     r3, r7, gGame@l
    li       r4, -0x1
    crset    cr1eq
    bl       DrawGsProportionalTextFormat
    lwz      r4, 0xa4c(r28)
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    addi     r3, r28, 0xa6c
    lwz      r4, 0x8(r4)
    li       r5, 0x0
    stw      r0, 0x104(r1)
    addi     r4, r4, 0x30
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_000015D0:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_000015D0
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x42c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x428(r1)
    addi     r3, r1, 0x308
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x428(r1)
    fsubs    f0, f0, f1
    fmuls    f28, f2, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x424(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x420(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x420(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    fmuls    f27, f2, f0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r4, lbl_5_rodata_100@ha
    lis      r3, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r5)
    fmr      f4, f28
    lfs      f2, lbl_5_rodata_100@l(r4)
    fmr      f5, f27
    addi     r3, r3, gGame@l
    fmr      f6, f3
    addi     r4, r1, 0x308
    fmr      f7, f3
    li       r5, -0x1
    fmr      f8, f3
    li       r6, -0x1
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0xa4c(r28)
    addi     r3, r28, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x38
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    subi     r5, r25, 0x4
    addi     r4, r3, 0x38
    stw      r7, 0x308(r1)
    stw      r6, 0x30c(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x310(r1)
    stw      r6, 0x314(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x318(r1)
    stw      r6, 0x31c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x320(r1)
    stw      r6, 0x324(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x328(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x32c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x330(r1)
    stw      r6, 0x334(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x338(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x33c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x340(r1)
    mtctr    r0
.L_00001754:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00001754
    addi     r3, r1, 0x308
    bl       GXGetTexObjWidth
    mr       r18, r3
    addi     r3, r1, 0x308
    bl       GXGetTexObjHeight
    mr       r17, r3
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    lis      r7, 0x4330
    lis      r3, lbl_5_rodata_108@ha
    stw      r23, 0x41c(r1)
    addi     r4, r3, lbl_5_rodata_108@l
    lfd      f9, 0x0(r4)
    lis      r5, lbl_5_rodata_F4@ha
    stw      r7, 0x418(r1)
    addi     r8, r5, lbl_5_rodata_F4@l
    lfs      f2, 0x0(r8)
    clrlwi   r3, r18, 16
    lfd      f0, 0x418(r1)
    lis      r4, lbl_5_rodata_9C@ha
    addi     r9, r4, lbl_5_rodata_9C@l
    clrlwi   r0, r17, 16
    fsubs    f0, f0, f9
    lfs      f4, 0x0(r9)
    lis      r5, lbl_5_rodata_A4@ha
    lis      r4, gFighterHudTextureScale@ha
    lfs      f3, lbl_5_rodata_A4@l(r5)
    lis      r6, lbl_5_rodata_100@ha
    fmuls    f0, f0, f28
    lfs      f5, gFighterHudTextureScale@l(r4)
    addi     r5, r6, lbl_5_rodata_100@l
    stw      r3, 0x414(r1)
    lis      r6, gGame@ha
    fnmsubs  f10, f28, f4, f2
    fdivs    f1, f0, f2
    stw      r7, 0x410(r1)
    lfs      f2, 0x0(r5)
    addi     r3, r6, gGame@l
    lfd      f0, 0x410(r1)
    addi     r4, r1, 0x308
    stw      r7, 0x408(r1)
    fsubs    f4, f0, f9
    fmr      f6, f3
    li       r5, -0x1
    stw      r0, 0x40c(r1)
    fmr      f7, f3
    fmr      f8, f3
    lfd      f0, 0x408(r1)
    fadds    f1, f10, f1
    li       r6, -0x1
    fsubs    f0, f0, f9
    li       r7, -0x1
    fmuls    f4, f5, f4
    li       r8, -0x1
    fmuls    f5, f5, f0
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    b        .L_00001864
.L_0000185C:
    add      r4, r4, r6
    bdnz     .L_00000274
.L_00001864:
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x404(r1)
    psq_l    f31, 0x4b8(r1), 0, qr0
    lfd      f31, 0x4b0(r1)
    psq_l    f30, 0x4a8(r1), 0, qr0
    lfd      f30, 0x4a0(r1)
    psq_l    f29, 0x498(r1), 0, qr0
    lfd      f29, 0x490(r1)
    psq_l    f28, 0x488(r1), 0, qr0
    lfd      f28, 0x480(r1)
    psq_l    f27, 0x478(r1), 0, qr0
    lfd      f27, 0x470(r1)
    lmw      r17, 0x434(r1)
    lwz      r0, 0x4c4(r1)
    mtlr     r0
    addi     r1, r1, 0x4c0
    blr
*/

/**
 * @note Address: stage02 .text 0x18AC
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x18F4
 * @note Size: 0x244
 */
extern "C" void DrawFighterHud(void)
{
	MatoList* list;
	s32 i;
	f32 b;
	f32 a;

	if (fighterWork._00 == NULL) {
		return;
	}
	list        = &matoList;
	list->_04.x = fighterWork._F4._00.x;
	list->_04.y = fighterWork._F4._00.y;
	list->_04.z = fighterWork._F4._00.z;
	for (i = 0; i < list->_00; i++) {
		Mato* item     = &list->_2C[i];
		UnkDolObj* obj = item->_04;
		f32 w;
		f32 h;
		if (obj == NULL || !(obj->_228 & 1) || item->_19) {
			continue;
		}
		w = 0.5f * obj->vtbl->_110(obj);
		if (w < 1.0f) {
			w = 1.0f;
		}
		h = item->_04->vtbl->_114(item->_04);
		if (h < 1.0f) {
			h = 1.0f;
		}
		if (item->_04->vtbl->_118(item->_04, w, h, &a, &b, 0)) {
			Vec zero  = { 0.0f, 0.0f, 0.0f };
			item->_18 = true;
			if (a > b) {
				a = b;
			}
			item->_04->vtbl->_10C(item->_04, &zero, &item->_08, &item->_0C, &item->_10, 0);
			item->_0C -= a * 0.5f;
		} else {
			item->_18 = false;
		}
	}
	DrawScoreHud(matoList._10, matoList._1C, 100.0f * matoList._18);
	FighterAim_Draw(&fighterWork._F4, (UnkGame_E5D38*)(gGame + GAME_OFS(0xE5D38, 0xE5050)), fighterWork._0C);
	FighterPanels_Draw(&panels);
}

// DrawFighterHud
/*
    stwu     r1, -0x50(r1)
    mflr     r0
    stw      r0, 0x54(r1)
    stfd     f31, 0x40(r1)
    psq_st   f31, 0x48(r1), 0, qr0
    stmw     r27, 0x2c(r1)
    lis      r3, fighterWork@ha
    addi     r4, r3, fighterWork@l
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_00001B1C
    addi     r27, r4, 0xf4
    lis      r3, matoList@ha
    lfs      f2, 0xf4(r4)
    addi     r31, r3, matoList@l
    lfs      f1, 0xf8(r4)
    mr       r28, r31
    lfs      f0, 0xfc(r4)
    li       r30, 0x0
    stfs     f2, 0x4(r31)
    stfs     f1, 0x8(r31)
    stfs     f0, 0xc(r31)
    b        .L_00001AAC
.L_00001950:
    addi     r29, r28, 0x2c
    lwz      r3, 0x30(r28)
    cmplwi   r3, 0x0
    beq      .L_00001AA4
    lwz      r0, 0x228(r3)
    li       r5, 0x0
    lwz      r6, 0x22c(r3)
    li       r4, 0x1
    and      r0, r0, r5
    and      r4, r6, r4
    xor      r4, r4, r5
    xor      r0, r0, r5
    or.      r0, r4, r0
    beq      .L_00001AA4
    lbz      r0, 0x19(r29)
    cmplwi   r0, 0x0
    bne      .L_00001AA4
    lwz      r12, 0x34(r3)
    lwz      r12, 0x110(r12)
    mtctr    r12
    bctrl
    lis      r4, lbl_5_rodata_9C@ha
    lis      r3, gFighterHudTextureScale@ha
    lfs      f2, lbl_5_rodata_9C@l(r4)
    lfsu     f0, gFighterHudTextureScale@l(r3)
    fmuls    f31, f1, f2
    fcmpo    cr0, f31, f0
    bge      .L_000019C4
    lfs      f31, 0x0(r3)
.L_000019C4:
    lwz      r3, 0x4(r29)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x114(r12)
    mtctr    r12
    bctrl
    fmr      f2, f1
    lis      r3, gFighterHudTextureScale@ha
    lfsu     f0, gFighterHudTextureScale@l(r3)
    fcmpo    cr0, f2, f0
    bge      .L_000019F0
    lfs      f2, 0x0(r3)
.L_000019F0:
    lwz      r3, 0x4(r29)
    fmr      f1, f31
    addi     r4, r1, 0x8
    addi     r5, r1, 0xc
    lwz      r12, 0x34(r3)
    li       r6, 0x0
    lwz      r12, 0x118(r12)
    mtctr    r12
    bctrl
    cmpwi    r3, 0x0
    beq      .L_00001A9C
    lfs      f0, 0x8(r1)
    li       r0, 0x1
    lfs      f1, 0xc(r1)
    stb      r0, 0x18(r29)
    fcmpo    cr0, f0, f1
    ble      .L_00001A38
    stfs     f1, 0x8(r1)
.L_00001A38:
    lis      r4, lbl_5_rodata_0@ha
    lwz      r3, 0x4(r29)
    addi     r7, r4, lbl_5_rodata_0@l
    addi     r4, r1, 0x10
    lwz      r8, 0x0(r7)
    addi     r5, r29, 0x8
    lwz      r9, 0x4(r7)
    addi     r6, r29, 0xc
    lwz      r0, 0x8(r7)
    addi     r7, r29, 0x10
    stw      r8, 0x10(r1)
    li       r8, 0x0
    stw      r9, 0x14(r1)
    stw      r0, 0x18(r1)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x10c(r12)
    mtctr    r12
    bctrl
    lis      r3, lbl_5_rodata_9C@ha
    lfs      f2, 0x8(r1)
    lfs      f0, 0xc(r29)
    lfs      f1, lbl_5_rodata_9C@l(r3)
    fnmsubs  f0, f2, f1, f0
    stfs     f0, 0xc(r29)
    b        .L_00001AA4
.L_00001A9C:
    li       r0, 0x0
    stb      r0, 0x18(r29)
.L_00001AA4:
    addi     r28, r28, 0x34
    addi     r30, r30, 0x1
.L_00001AAC:
    lwz      r0, 0x0(r31)
    cmpw     r30, r0
    blt      .L_00001950
    lis      r3, matoList@ha
    lis      r4, lbl_5_rodata_F4@ha
    addi     r3, r3, matoList@l
    lfs      f1, lbl_5_rodata_F4@l(r4)
    lfs      f0, 0x18(r3)
    fmuls    f1, f1, f0
    bl       __cvt_fp2unsigned
    lis      r4, matoList@ha
    mr       r5, r3
    addi     r4, r4, matoList@l
    lwz      r3, 0x10(r4)
    lwz      r4, 0x1c(r4)
    bl       DrawScoreHud
    lis      r3, gGame@ha
    lis      r5, fighterWork@ha
    addi     r4, r3, gGame@l
    mr       r3, r27
    addis    r4, r4, 0xe
    addi     r5, r5, fighterWork@l
    addi     r5, r5, 0xc
    addi     r4, r4, 0x5d38
    bl       FighterAim_Draw
    lis      r3, panels@ha
    addi     r3, r3, panels@l
    bl       FighterPanels_Draw
.L_00001B1C:
    psq_l    f31, 0x48(r1), 0, qr0
    lfd      f31, 0x40(r1)
    lmw      r27, 0x2c(r1)
    lwz      r0, 0x54(r1)
    mtlr     r0
    addi     r1, r1, 0x50
    blr
*/

/**
 * @note Address: stage02 .text 0x1B38
 * @note Size: 0x2FC
 */
extern "C" void DrawShots(void)
{
	u32 i;
	s32 j;

	if (fighterWork._00 == NULL) {
		return;
	}
	Shot* entry = fighterWork._114._00;
	for (i = 0; i < 5; i++, entry++) {
		if (!entry->_00) {
			continue;
		}
		GAME_FIELD(s32, 0x29C) = 1;
		SetGsBlendMode(GAME_FIELD(void*, 0x290), 1, 4, 5, 15);
		for (j = 0; j < 10; j++) {
			Vec* pos  = &entry->_38[j];
			Vec up    = { 0.0f, 1.0f, 0.0f };
			u32 alpha = 96.0f * (1.0f - 0.1f * j);
			DrawGsCircle(gGame, 16, alpha | 0xFFFFFF00, 0xFFFFFF00, pos->x, pos->y, pos->z, 4.0f, 4.0f, up.x, up.y, up.z);
		}
		Vec* pos = &entry->_1C;
		SetGsBlendMode(GAME_FIELD(void*, 0x290), 1, 2, 4, 15);
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(gGame, 16, 0xFFFFFF80, 0xFFFFFF00, pos->x, pos->y, pos->z, 4.0f, 4.0f, up.x, up.y, up.z);
		}
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(gGame, 16, 0xFFFFFF80, 0xFFFFFF00, pos->x, pos->y, pos->z, 5.0f, 5.0f, up.x, up.y, up.z);
		}
		SetGsBlendMode(GAME_FIELD(void*, 0x290), 1, 4, 5, 15);
		{
			Vec up = { 0.0f, 1.0f, 0.0f };
			DrawGsCircle(gGame, 16, 0x64FF64C8, 0x64FF6400, pos->x, pos->y, pos->z, 5.0f, 5.0f, up.x, up.y, up.z);
		}
		GAME_FIELD(s32, 0x29C) = 0;
	}
}

// DrawShots
/*
    stwu     r1, -0xb0(r1)
    mflr     r0
    stw      r0, 0xb4(r1)
    stfd     f31, 0xa0(r1)
    psq_st   f31, 0xa8(r1), 0, qr0
    stfd     f30, 0x90(r1)
    psq_st   f30, 0x98(r1), 0, qr0
    stfd     f29, 0x80(r1)
    psq_st   f29, 0x88(r1), 0, qr0
    stfd     f28, 0x70(r1)
    psq_st   f28, 0x78(r1), 0, qr0
    stmw     r22, 0x48(r1)
    lis      r3, fighterWork@ha
    addi     r4, r3, fighterWork@l
    lwz      r0, 0x0(r4)
    cmplwi   r0, 0x0
    beq      .L_00001E00
    lis      r3, gGame@ha
    addi     r28, r4, 0x114
    addi     r31, r3, gGame@l
    li       r30, 0x0
.L_00001B8C:
    lbz      r0, 0x0(r28)
    cmplwi   r0, 0x0
    beq      .L_00001DF0
    li       r0, 0x1
    lwz      r3, 0x290(r31)
    stw      r0, 0x29c(r31)
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x5
    li       r7, 0xf
    bl       SetGsBlendMode
    lis      r6, lbl_5_rodata_134@ha
    lis      r5, lbl_5_rodata_88@ha
    lis      r4, lbl_5_rodata_110@ha
    lis      r3, gFighterHudTextureScale@ha
    addi     r8, r6, lbl_5_rodata_134@l
    addi     r7, r5, lbl_5_rodata_88@l
    addi     r6, r4, lbl_5_rodata_110@l
    addi     r5, r3, gFighterHudTextureScale@l
    lis      r9, lbl_5_rodata_C@ha
    lis      r4, gGame@ha
    lis      r3, lbl_5_rodata_AC@ha
    lfs      f28, 0x0(r8)
    lfs      f29, 0x0(r7)
    mr       r29, r28
    lfd      f30, 0x0(r6)
    addi     r24, r9, lbl_5_rodata_C@l
    lfs      f31, 0x0(r5)
    addi     r26, r4, gGame@l
    addi     r27, r3, lbl_5_rodata_AC@l
    li       r23, 0x0
    lis      r25, 0x4330
.L_00001C0C:
    xoris    r0, r23, 0x8000
    lwz      r4, 0x0(r24)
    stw      r0, 0x3c(r1)
    addi     r22, r29, 0x38
    lwz      r3, 0x4(r24)
    stw      r25, 0x38(r1)
    lwz      r0, 0x8(r24)
    lfd      f0, 0x38(r1)
    stw      r4, 0x8(r1)
    fsubs    f0, f0, f30
    stw      r3, 0xc(r1)
    fnmsubs  f0, f29, f0, f31
    stw      r0, 0x10(r1)
    fmuls    f1, f28, f0
    bl       __cvt_fp2unsigned
    lfs      f4, 0x0(r27)
    li       r0, -0x100
    or       r5, r3, r0
    lfs      f1, 0x0(r22)
    fmr      f5, f4
    lfs      f2, 0x4(r22)
    lfs      f3, 0x8(r22)
    mr       r3, r26
    lfs      f6, 0x8(r1)
    li       r4, 0x10
    lfs      f7, 0xc(r1)
    li       r6, -0x100
    lfs      f8, 0x10(r1)
    bl       DrawGsCircle
    addi     r23, r23, 0x1
    addi     r29, r29, 0xc
    cmpwi    r23, 0xa
    blt      .L_00001C0C
    lwz      r3, 0x290(r31)
    addi     r29, r28, 0x1c
    li       r4, 0x1
    li       r5, 0x2
    li       r6, 0x4
    li       r7, 0xf
    bl       SetGsBlendMode
    lis      r4, lbl_5_rodata_18@ha
    lis      r3, lbl_5_rodata_AC@ha
    addi     r8, r4, lbl_5_rodata_18@l
    lis      r5, gGame@ha
    addi     r4, r3, lbl_5_rodata_AC@l
    lwz      r7, 0x0(r8)
    lfs      f4, 0x0(r4)
    addi     r3, r5, gGame@l
    lwz      r6, 0x4(r8)
    li       r4, 0x10
    lwz      r0, 0x8(r8)
    fmr      f5, f4
    stw      r6, 0x18(r1)
    li       r5, -0x80
    lfs      f1, 0x0(r29)
    li       r6, -0x100
    stw      r7, 0x14(r1)
    lfs      f2, 0x4(r29)
    stw      r0, 0x1c(r1)
    lfs      f3, 0x8(r29)
    lfs      f6, 0x14(r1)
    lfs      f7, 0x18(r1)
    lfs      f8, 0x1c(r1)
    bl       DrawGsCircle
    lis      r4, lbl_5_rodata_24@ha
    lis      r3, lbl_5_rodata_138@ha
    addi     r8, r4, lbl_5_rodata_24@l
    lis      r5, gGame@ha
    addi     r4, r3, lbl_5_rodata_138@l
    lwz      r7, 0x0(r8)
    lfs      f4, 0x0(r4)
    addi     r3, r5, gGame@l
    lwz      r6, 0x4(r8)
    li       r4, 0x10
    lwz      r0, 0x8(r8)
    fmr      f5, f4
    stw      r6, 0x24(r1)
    li       r5, -0x80
    lfs      f1, 0x0(r29)
    li       r6, -0x100
    stw      r7, 0x20(r1)
    lfs      f2, 0x4(r29)
    stw      r0, 0x28(r1)
    lfs      f3, 0x8(r29)
    lfs      f6, 0x20(r1)
    lfs      f7, 0x24(r1)
    lfs      f8, 0x28(r1)
    bl       DrawGsCircle
    lwz      r3, 0x290(r31)
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x5
    li       r7, 0xf
    bl       SetGsBlendMode
    lis      r3, lbl_5_rodata_30@ha
    lis      r4, 0x64ff
    addi     r8, r3, lbl_5_rodata_30@l
    lfs      f1, 0x0(r29)
    lis      r3, lbl_5_rodata_138@ha
    lwz      r7, 0x0(r8)
    addi     r5, r3, lbl_5_rodata_138@l
    lwz      r6, 0x4(r8)
    lfs      f4, 0x0(r5)
    lis      r3, gGame@ha
    stw      r6, 0x30(r1)
    addi     r3, r3, gGame@l
    lwz      r0, 0x8(r8)
    fmr      f5, f4
    stw      r7, 0x2c(r1)
    addi     r5, r4, 0x64c8
    addi     r6, r4, 0x6400
    lfs      f2, 0x4(r29)
    stw      r0, 0x34(r1)
    lfs      f3, 0x8(r29)
    li       r4, 0x10
    lfs      f6, 0x2c(r1)
    lfs      f7, 0x30(r1)
    lfs      f8, 0x34(r1)
    bl       DrawGsCircle
    li       r0, 0x0
    stw      r0, 0x29c(r31)
.L_00001DF0:
    addi     r30, r30, 0x1
    addi     r28, r28, 0xbc
    cmplwi   r30, 0x5
    blt      .L_00001B8C
.L_00001E00:
    psq_l    f31, 0xa8(r1), 0, qr0
    lfd      f31, 0xa0(r1)
    psq_l    f30, 0x98(r1), 0, qr0
    lfd      f30, 0x90(r1)
    psq_l    f29, 0x88(r1), 0, qr0
    lfd      f29, 0x80(r1)
    psq_l    f28, 0x78(r1), 0, qr0
    lfd      f28, 0x70(r1)
    lmw      r22, 0x48(r1)
    lwz      r0, 0xb4(r1)
    mtlr     r0
    addi     r1, r1, 0xb0
    blr
*/

/**
 * @note Address: stage02 .text 0x1E34
 * @note Size: 0x10
 */
extern "C" void DrawFighterStage(void)
{
	if (fighterWork._00 == NULL) {
		return;
	}
}

/**
 * @note Address: stage02 .text 0x1E44
 * @note Size: 0x514
 */
extern "C" void UpdateFighter(u32 arg)
{
	Mato* best;
	f32 bestDist;
	s32 i;
	void* se;

	if (fighterWork._00 == NULL) {
		return;
	}
	shotEvents.mFlags = 0;
	CPad pad(0);
	if (pad.mPad->mButton & 0x100) {
		if (pad.mPad->mTrigger & 0x100) {
			Fighter_Fire(&fighterWork, 1);
		}
		fireHoldFrames++;
	} else {
		if (fireHoldFrames >= 20) {
			Fighter_Fire(&fighterWork, 10);
		}
		fireHoldFrames = 0;
	}
	fighterWork._9C = -1.3962634f * pad.mPad->mStickX;
	Fighter_Update(&fighterWork, arg);
	PilotNwing_Update(&pilotNwing, arg, fighterWork._A0);
	matoList._14++;
	matoList._18 = fighterWork._A0 / fighterWork._A8;
	matoList._1C = fighterWork._BC / 60;
	for (i = 0; i < matoList._00; i++) {
		Mato* item = &matoList._2C[i];
		if (!item->_18) {
			item->_1C = 0;
		} else {
			if (item->_1C >= 15) {
				item->_14 = 0xFF0000C8;
			} else {
				item->_14 = 0xFFFFFF50;
			}
			item->_20 += 1.0f;
		}
	}
	se = gGame + GAME_OFS(0xE7E24, 0xE713C);
	if (IsGameSoundPlaying(se, 0x20006) == 0 && 0.01f < fighterWork._A0 / fighterWork._A8) {
		Vec pos;
		best                = NULL;
		fighterWork._F4._18 = NULL;
		pos                 = fighterWork._F4._00;
		PSMTXMultVec(fighterWork._0C, &pos, &pos);
		for (i = 0; i < matoList._00; i++) {
			Mato* item = &matoList._2C[i];
			f32 dist;
			f32 range;
			if (!(item->_04->_228 & 0x10000) || item->_24 <= 0) {
				continue;
			}
			dist = PSVECDistance(&item->GetPos(), &pos);
			if (fighterWork._B4 == NULL) {
				range = 75.0f;
			} else {
				range = (s32)READ_LE32(fighterWork._B4 + 0xC) / 100.0f;
			}
			if (range >= dist && (best == NULL || dist < bestDist)) {
				best     = item;
				bestDist = dist;
			}
		}
		if (best != NULL && !best->_19) {
			if (best != NULL && fighterWork._110 != best) {
				PlayGameSound(se, 0x20006, 0xFF);
			}
			fighterWork._F4._18 = best;
		}
	}
	FighterPanels_Update(&panels, arg);
	for (i = 0; i < GET_LE32(soundCueTable[0] + i * 8); i++) {
		s32 frame = GET_LE32(soundCueTable[0] + i * 8);
		s32 id    = GET_LE32(soundCueTable[0] + i * 8 + 4);
		if (fighterWork._A4 < (f32)frame && (f32)frame <= fighterWork._A0) {
			PlayGameSound(se, id, 0xFF);
		}
	}
}

// UpdateFighter
/*
    stwu     r1, -0x80(r1)
    mflr     r0
    stw      r0, 0x84(r1)
    stfd     f31, 0x70(r1)
    psq_st   f31, 0x78(r1), 0, qr0
    stmw     r21, 0x44(r1)
    lis      r4, fighterWork@ha
    mr       r22, r3
    lwz      r0, fighterWork@l(r4)
    cmplwi   r0, 0x0
    beq      .L_0000233C
    lis      r3, shotEvents@ha
    li       r0, 0x0
    addi     r4, r3, shotEvents@l
    addi     r3, r1, 0x8
    stw      r0, 0x0(r4)
    li       r4, 0x0
    bl       __ct__4CPadFUc
    lwz      r3, 0x8(r1)
    lwz      r0, 0x4(r3)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_00001ED0
    lwz      r0, 0x14(r3)
    rlwinm.  r0, r0, 0, 23, 23
    beq      .L_00001EB8
    lis      r3, fighterWork@ha
    li       r4, 0x1
    addi     r3, r3, fighterWork@l
    bl       Fighter_Fire
.L_00001EB8:
    lis      r3, fireHoldFrames@ha
    addi     r4, r3, fireHoldFrames@l
    lwz      r3, 0x0(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x0(r4)
    b        .L_00001EFC
.L_00001ED0:
    lis      r3, fireHoldFrames@ha
    lwz      r0, fireHoldFrames@l(r3)
    cmpwi    r0, 0x14
    blt      .L_00001EF0
    lis      r3, fighterWork@ha
    li       r4, 0xa
    addi     r3, r3, fighterWork@l
    bl       Fighter_Fire
.L_00001EF0:
    lis      r3, fireHoldFrames@ha
    li       r0, 0x0
    stw      r0, fireHoldFrames@l(r3)
.L_00001EFC:
    lwz      r4, 0x8(r1)
    lis      r3, lbl_5_rodata_13C@ha
    addi     r5, r3, lbl_5_rodata_13C@l
    lfs      f0, 0x34(r4)
    lis      r3, fighterWork@ha
    lfs      f1, 0x0(r5)
    addi     r3, r3, fighterWork@l
    mr       r4, r22
    fmuls    f0, f1, f0
    stfs     f0, 0x9c(r3)
    bl       Fighter_Update
    lis      r3, fighterWork@ha
    lis      r5, pilotNwing@ha
    addi     r28, r3, fighterWork@l
    mr       r4, r22
    lfs      f1, 0xa0(r28)
    addi     r3, r5, pilotNwing@l
    bl       PilotNwing_Update
    lis      r4, fighterWork@ha
    lfs      f1, 0xa0(r28)
    addi     r29, r4, fighterWork@l
    lis      r3, 0x8889
    lfs      f0, 0xa8(r29)
    lis      r4, matoList@ha
    addi     r26, r4, matoList@l
    lwz      r0, 0xbc(r29)
    fdivs    f0, f1, f0
    subi     r4, r3, 0x7777
    mulhwu   r4, r4, r0
    lwz      r3, 0x14(r26)
    mr       r6, r26
    addi     r0, r3, 0x1
    stw      r0, 0x14(r26)
    srwi     r0, r4, 5
    li       r4, 0x0
    stfs     f0, 0x18(r26)
    stw      r0, 0x1c(r26)
    b        .L_00001FF0
.L_00001F94:
    addi     r5, r6, 0x2c
    lbz      r0, 0x44(r6)
    cmplwi   r0, 0x0
    bne      .L_00001FB0
    li       r0, 0x0
    stw      r0, 0x1c(r5)
    b        .L_00001FE8
.L_00001FB0:
    lwz      r0, 0x1c(r5)
    cmpwi    r0, 0xf
    blt      .L_00001FCC
    lis      r3, 0xff00
    addi     r0, r3, 0xc8
    stw      r0, 0x14(r5)
    b        .L_00001FD4
.L_00001FCC:
    li       r0, -0xb0
    stw      r0, 0x14(r5)
.L_00001FD4:
    lis      r3, gFighterHudTextureScale@ha
    lfs      f1, 0x20(r5)
    lfs      f0, gFighterHudTextureScale@l(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x20(r5)
.L_00001FE8:
    addi     r6, r6, 0x34
    addi     r4, r4, 0x1
.L_00001FF0:
    lwz      r0, 0x0(r26)
    cmpw     r4, r0
    blt      .L_00001F94
    lis      r4, gGame@ha
    lis      r3, 0x2
    addi     r4, r4, gGame@l
    addis    r25, r4, 0xe
    addi     r4, r3, 0x6
    addi     r25, r25, 0x7e24
    mr       r3, r25
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_000021D8
    lfs      f1, 0xa0(r28)
    lis      r3, lbl_5_rodata_140@ha
    lfs      f0, 0xa8(r29)
    lfs      f2, lbl_5_rodata_140@l(r3)
    fdivs    f0, f1, f0
    fcmpo    cr0, f2, f0
    bge      .L_000021D8
    lis      r3, fighterWork@ha
    addi     r4, r1, 0x20
    addi     r31, r3, fighterWork@l
    li       r0, 0x0
    lfs      f2, 0xf4(r31)
    mr       r5, r4
    lfs      f1, 0xf8(r31)
    addi     r3, r31, 0xc
    lfs      f0, 0xfc(r31)
    li       r24, 0x0
    stw      r0, 0x10c(r31)
    stfs     f2, 0x20(r1)
    stfs     f1, 0x24(r1)
    stfs     f0, 0x28(r1)
    bl       PSMTXMultVec
    lis      r4, fighterWork@ha
    lis      r3, matoList@ha
    li       r23, 0x0
    lis      r29, 0x1
    addi     r30, r4, fighterWork@l
    addi     r21, r3, matoList@l
    b        .L_0000218C
.L_00002098:
    lwz      r3, 0x30(r26)
    li       r4, 0x0
    addi     r27, r26, 0x2c
    lwz      r0, 0x228(r3)
    lwz      r3, 0x22c(r3)
    and      r0, r0, r4
    and      r3, r3, r29
    xor      r3, r3, r4
    xor      r0, r0, r4
    or.      r0, r3, r0
    beq      .L_00002184
    lwz      r0, 0x24(r27)
    cmpwi    r0, 0x0
    ble      .L_00002184
    mr       r4, r27
    addi     r3, r1, 0x10
    bl       GetPos
    addi     r3, r1, 0x10
    addi     r4, r1, 0x20
    bl       PSVECDistance
    lwz      r7, 0xb4(r30)
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    cmplwi   r7, 0x0
    stw      r0, 0x1c(r1)
    bne      .L_0000210C
    lis      r3, lbl_5_rodata_144@ha
    lfs      f0, lbl_5_rodata_144@l(r3)
    b        .L_00002164
.L_0000210C:
    lbz      r5, 0xd(r7)
    lis      r0, 0x4330
    lbz      r8, 0xf(r7)
    lis      r3, lbl_5_rodata_110@ha
    lbz      r4, 0xe(r7)
    slwi     r6, r5, 8
    lbz      r7, 0xc(r7)
    slwi     r8, r8, 24
    slwi     r5, r4, 16
    addi     r4, r3, lbl_5_rodata_110@l
    add      r5, r6, r5
    stw      r0, 0x30(r1)
    add      r5, r5, r8
    lis      r3, lbl_5_rodata_F4@ha
    add      r0, r7, r5
    lfd      f3, 0x0(r4)
    xoris    r0, r0, 0x8000
    lfs      f0, lbl_5_rodata_F4@l(r3)
    stw      r0, 0x34(r1)
    lfd      f2, 0x30(r1)
    fsubs    f2, f2, f3
    fdivs    f0, f2, f0
.L_00002164:
    fcmpo    cr0, f0, f1
    blt      .L_00002184
    cmplwi   r24, 0x0
    beq      .L_0000217C
    fcmpo    cr0, f1, f31
    bge      .L_00002184
.L_0000217C:
    mr       r24, r27
    fmr      f31, f1
.L_00002184:
    addi     r26, r26, 0x34
    addi     r23, r23, 0x1
.L_0000218C:
    lwz      r0, 0x0(r21)
    cmpw     r23, r0
    blt      .L_00002098
    cmplwi   r24, 0x0
    beq      .L_000021D8
    lbz      r0, 0x19(r24)
    cmplwi   r0, 0x0
    bne      .L_000021D8
    cmplwi   r24, 0x0
    beq      .L_000021D4
    lwz      r0, 0x110(r31)
    cmplw    r0, r24
    beq      .L_000021D4
    lis      r4, 0x2
    mr       r3, r25
    addi     r4, r4, 0x6
    li       r5, 0xff
    bl       PlayGameSound
.L_000021D4:
    stw      r24, 0x10c(r31)
.L_000021D8:
    lis      r3, panels@ha
    mr       r4, r22
    addi     r3, r3, panels@l
    bl       FighterPanels_Update
    lis      r3, lbl_5_rodata_110@ha
    lis      r5, fighterWork@ha
    addi     r4, r3, lbl_5_rodata_110@l
    li       r23, 0x0
    lis      r3, soundCueTable@ha
    lfd      f31, 0x0(r4)
    addi     r29, r5, fighterWork@l
    li       r27, 0x0
    addi     r31, r3, soundCueTable@l
    li       r22, 0x4
    lis      r30, 0x4330
    b        .L_000022EC
.L_00002218:
    cmplwi   r7, 0x0
    beq      .L_0000224C
    lbz      r4, 0x1(r7)
    lbz      r3, 0x2(r7)
    lbz      r6, 0x3(r7)
    slwi     r4, r4, 8
    slwi     r3, r3, 16
    lbz      r5, 0x0(r7)
    slwi     r6, r6, 24
    add      r3, r4, r3
    add      r6, r3, r6
    add      r6, r5, r6
    b        .L_00002250
.L_0000224C:
    li       r6, 0x0
.L_00002250:
    add.     r5, r0, r22
    beq      .L_00002284
    lbz      r3, 0x1(r5)
    lbz      r0, 0x2(r5)
    lbz      r4, 0x3(r5)
    slwi     r3, r3, 8
    slwi     r0, r0, 16
    lbz      r5, 0x0(r5)
    slwi     r4, r4, 24
    add      r0, r3, r0
    add      r4, r0, r4
    add      r4, r5, r4
    b        .L_00002288
.L_00002284:
    li       r4, 0x0
.L_00002288:
    xoris    r5, r6, 0x8000
    stw      r30, 0x30(r1)
    lfs      f1, 0xa4(r29)
    stw      r5, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f31
    fcmpo    cr0, f1, f0
    bge      .L_000022E0
    lis      r0, 0x4330
    lis      r3, lbl_5_rodata_110@ha
    stw      r5, 0x34(r1)
    lfd      f2, lbl_5_rodata_110@l(r3)
    stw      r0, 0x30(r1)
    lfs      f0, 0xa0(r28)
    lfd      f1, 0x30(r1)
    fsubs    f1, f1, f2
    fcmpo    cr0, f1, f0
    cror     eq, lt, eq
    bne      .L_000022E0
    mr       r3, r25
    li       r5, 0xff
    bl       PlayGameSound
.L_000022E0:
    addi     r27, r27, 0x8
    addi     r22, r22, 0x8
    addi     r23, r23, 0x1
.L_000022EC:
    lwz      r0, 0x0(r31)
    add.     r7, r0, r27
    beq      .L_00002324
    lbz      r4, 0x1(r7)
    lbz      r3, 0x2(r7)
    lbz      r6, 0x3(r7)
    slwi     r4, r4, 8
    slwi     r3, r3, 16
    lbz      r5, 0x0(r7)
    slwi     r6, r6, 24
    add      r3, r4, r3
    add      r3, r3, r6
    add      r3, r5, r3
    b        .L_00002328
.L_00002324:
    li       r3, 0x0
.L_00002328:
    cmpw     r23, r3
    blt      .L_00002218
    lis      r3, __vt__4CPad@ha
    addi     r0, r3, __vt__4CPad@l
    stw      r0, 0xc(r1)
.L_0000233C:
    psq_l    f31, 0x78(r1), 0, qr0
    lfd      f31, 0x70(r1)
    lmw      r21, 0x44(r1)
    lwz      r0, 0x84(r1)
    mtlr     r0
    addi     r1, r1, 0x80
    blr
*/

/**
 * @note Address: stage02 .text 0x2358
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x23A0
 * @note Size: 0x2C
 */
// inline

/**
 * @note Address: stage02 .text 0x23CC
 * @note Size: 0x48
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x2414
 * @note Size: 0x3C
 */
extern "C" void ShowCenterPanel(bool on, s32 arg)
{
	if (on) {
		panels._04 = 1;
		panels._14 = 0;
		panels._20 = arg;
	} else {
		panels._04 = 2;
	}
}

/**
 * @note Address: stage02 .text 0x2450
 * @note Size: 0x30
 */
extern "C" void ShowLowerPanel(bool on)
{
	if (on) {
		panels._00 = 1;
		panels._10 = 0;
	} else {
		panels._00 = 2;
	}
}

/**
 * @note Address: stage02 .text 0x2480
 * @note Size: 0xA0
 */
extern "C" void InitPilotNwing(UnkDolObj* a, UnkDolObj* b, UnkDolObj* c, UnkDolObj* d)
{
	if (a == NULL || b == NULL) {
		return;
	}
	pilotNwing._00 = a;
	pilotNwing._04 = b;
	pilotNwing._08 = c;
	pilotNwing._0C = d;
	a->_228 |= 0x10000;
	pilotNwing._04->_228 &= ~0x10000ULL;
	pilotNwing._08->_228 |= 0x10000;
	pilotNwing._0C->_228 |= 0x10000;
	pilotNwing._A0 = 0.0f;
	pilotNwing._A4 = 1.0f;
}

/**
 * @note Address: stage02 .text 0x2520
 * @note Size: 0x50
 */
extern "C" s32 GetShotEvent(void)
{
	s32 ret   = 0;
	u32 flags = shotEvents.mFlags;
	if (flags & 4) {
		ret = 3;
	} else if (flags & 8) {
		ret = 4;
	} else if (flags & 1) {
		ret = 1;
	} else if (flags & 2) {
		ret = 2;
	}
	return ret;
}

/**
 * @note Address: stage02 .text 0x2570
 * @note Size: 0x448
 */
extern "C" void EndFighter(void)
{
	if (fighterWork._00 == NULL) {
		return;
	}
	ResetModelFlags(GAME_FIELD(UnkGame_96BC*, GAME_OFS(0x96BC, 0x9674)));
	HSD_JObjClearMtxFlags(GAME_FIELD(UnkGame_96BC*, GAME_OFS(0x96BC, 0x9674))->_00);
	((CMotionControlCamera*)(gGame + GAME_OFS(0xE5D38, 0xE5050)))
	    ->ChangeAction(GAME_FIELD(CInterfaceCameraAction*, GAME_OFS(0xE5F0C, 0xE5224)));
	if (IsGameSoundPlaying(gGame + GAME_OFS(0xE7E24, 0xE713C), 0x20004)) {
		StopGameSfx(gGame + GAME_OFS(0xE7E24, 0xE713C), 0x20004);
	}
	if (fighterWork._00 != NULL) {
		HSD_JObjClearMtxFlags(fighterWork._00->_00);
		if (fighterWork._08 != NULL) {
			HSD_JObjClearMtxFlags(fighterWork._08->_00);
		}
	}
	HSD_JObjClearMtxFlags(pilotNwing._00->_00);
	if (pilotNwing._08 != NULL) {
		HSD_JObjClearMtxFlags(pilotNwing._08->_00);
	}
	if (pilotNwing._0C != NULL) {
		HSD_JObjClearMtxFlags(pilotNwing._0C->_00);
	}
	fighterWork._00 = fighterWork._04 = fighterWork._08 = NULL;
	pilotNwing._00 = pilotNwing._04 = NULL;
}

// EndFighter
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r3, fighterWork@ha
    stw      r0, 0x14(r1)
    stw      r31, 0xc(r1)
    lwz      r0, fighterWork@l(r3)
    cmplwi   r0, 0x0
    beq      .L_000029A4
    lis      r4, gGame@ha
    li       r3, 0x0
    addi     r4, r4, gGame@l
    li       r0, 0x1
    addis    r6, r4, 0x1
    lwz      r5, -0x6944(r6)
    stw      r3, 0x980(r5)
    addis    r3, r5, 0x1
    lwz      r4, 0x980(r5)
    oris     r4, r4, 0x2000
    stw      r4, 0x980(r5)
    lwz      r4, 0x980(r5)
    ori      r4, r4, 0x7
    stw      r4, 0x980(r5)
    stw      r0, 0x2df4(r3)
    lwz      r3, -0x6944(r6)
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_000025F4
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000025F4:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_00002650
    bne      .L_00002624
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002624:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00002640
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00002640
    li       r3, 0x1
.L_00002640:
    clrlwi.  r0, r3, 24
    bne      .L_00002650
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00002650:
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0xe
    lwz      r4, 0x5f0c(r3)
    addi     r3, r3, 0x5d38
    bl       fn_801494A4
    lis      r4, gGame@ha
    lis      r3, 0x2
    addi     r4, r4, gGame@l
    addis    r31, r4, 0xe
    addi     r4, r3, 0x4
    addi     r31, r31, 0x7e24
    mr       r3, r31
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    beq      .L_000026A0
    lis      r4, 0x2
    mr       r3, r31
    addi     r4, r4, 0x4
    bl       StopGameSfx
.L_000026A0:
    lis      r3, fighterWork@ha
    addi     r3, r3, fighterWork@l
    lwz      r3, 0x0(r3)
    cmplwi   r3, 0x0
    beq      .L_000027C8
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_000026D8
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000026D8:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_00002734
    bne      .L_00002708
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002708:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00002724
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00002724
    li       r3, 0x1
.L_00002724:
    clrlwi.  r0, r3, 24
    bne      .L_00002734
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00002734:
    lis      r3, fighterWork@ha
    addi     r3, r3, fighterWork@l
    lwz      r3, 0x8(r3)
    cmplwi   r3, 0x0
    beq      .L_000027C8
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_0000276C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_0000276C:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_000027C8
    bne      .L_0000279C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_0000279C:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000027B8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000027B8
    li       r3, 0x1
.L_000027B8:
    clrlwi.  r0, r3, 24
    bne      .L_000027C8
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_000027C8:
    lis      r3, pilotNwing@ha
    addi     r3, r3, pilotNwing@l
    lwz      r3, 0x0(r3)
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_000027F8
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000027F8:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_00002854
    bne      .L_00002828
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002828:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00002844
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00002844
    li       r3, 0x1
.L_00002844:
    clrlwi.  r0, r3, 24
    bne      .L_00002854
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_00002854:
    lis      r3, pilotNwing@ha
    addi     r3, r3, pilotNwing@l
    lwz      r3, 0x8(r3)
    cmplwi   r3, 0x0
    beq      .L_000028E8
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_0000288C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_0000288C:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_000028E8
    bne      .L_000028BC
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000028BC:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000028D8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000028D8
    li       r3, 0x1
.L_000028D8:
    clrlwi.  r0, r3, 24
    bne      .L_000028E8
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_000028E8:
    lis      r3, pilotNwing@ha
    addi     r3, r3, pilotNwing@l
    lwz      r3, 0xc(r3)
    cmplwi   r3, 0x0
    beq      .L_0000297C
    lwz      r31, 0x0(r3)
    cmplwi   r31, 0x0
    bne      .L_00002920
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x4af
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002920:
    lwz      r0, 0x14(r31)
    cmplwi   r31, 0x0
    rlwinm   r0, r0, 0, 9, 5
    stw      r0, 0x14(r31)
    beq      .L_0000297C
    bne      .L_00002950
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002950:
    lwz      r4, 0x14(r31)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000296C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000296C
    li       r3, 0x1
.L_0000296C:
    clrlwi.  r0, r3, 24
    bne      .L_0000297C
    mr       r3, r31
    bl       HSD_JObjSetMtxDirtySub
.L_0000297C:
    lis      r4, fighterWork@ha
    lis      r3, pilotNwing@ha
    addi     r4, r4, fighterWork@l
    li       r0, 0x0
    addi     r3, r3, pilotNwing@l
    stw      r0, 0x8(r4)
    stw      r0, 0x4(r4)
    stw      r0, 0x0(r4)
    stw      r0, 0x4(r3)
    stw      r0, 0x0(r3)
.L_000029A4:
    lwz      r0, 0x14(r1)
    lwz      r31, 0xc(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x29B8
 * @note Size: 0x2C
 */
extern "C" s32 TakeDestroyedMatoId(void)
{
	s32 ret      = matoList._28 != NULL ? matoList._28->_00 : -1;
	matoList._28 = NULL;
	return ret;
}

/**
 * @note Address: stage02 .text 0x29E4
 * @note Size: 0x10
 */
extern "C" s32 GetScore(void)
{
	return matoList._10;
}

/**
 * @note Address: stage02 .text 0x29F4
 * @note Size: 0x10
 */
extern "C" u32 GetTimeLeft(void)
{
	return matoList._1C;
}

/**
 * @note Address: stage02 .text 0x2A04
 * @note Size: 0x10
 */
extern "C" f32 GetFighterProgress(void)
{
	return matoList._18;
}

/**
 * @note Address: stage02 .text 0x2A14
 * @note Size: 0xC
 */
extern "C" void SetSoundCueTable(u8* data)
{
	soundCueTable[0] = data;
}

/**
 * @note Address: stage02 .text 0x2A20
 * @note Size: 0x1D4
 */
extern "C" void SetFighterParams(u8* data)
{
	fighterWork._B4 = data;
	fighterWork._E0 = (s32)READ_LE32(data + 0x10) / 100.0f;
	fighterWork._E4 = (s32)READ_LE32(data + 0x14) / 100.0f;
	fighterWork._E8 = (s32)READ_LE32(data + 0x18) / 100.0f;
	fighterWork._EC = (s32)READ_LE32(data + 0x1C) / 100.0f;
	fighterWork._F0 = READ_LE32(data + 0x20) * 60 + 59;
	matoList._20[0] = READ_LE32(data + 0x24);
	matoList._20[1] = READ_LE32(data + 0x28);
}

/**
 * @note Address: stage02 .text 0x2BF4
 * @note Size: 0x10
 */
extern "C" void SetSpeedRateTable(u8* rates)
{
	fighterWork._B0 = rates;
}

/**
 * @note Address: stage02 .text 0x2C04
 * @note Size: 0x108
 */
extern "C" void AddMato(UnkDolObj* obj, s32 id)
{
	Mato* item;
	s32 kind;

	if (matoList._00 >= ARRAY_SIZE(matoList._2C)) {
		return;
	}
	item      = &matoList._2C[matoList._00++];
	item->_04 = obj;
	item->_00 = id;
	item->_1C = 0;
	item->_19 = false;
	item->_20 = HSD_Rand();
	item->_04->_228 |= 0x10000;
	kind = item->_04->_244 != NULL ? *item->_04->_244 : -1;
	switch (kind) {
	case 0x14D:
		item->_24 = 1;
		item->_28 = 1;
		item->_2C = 0;
		break;
	case 0x2EA:
		item->_24 = 3;
		item->_28 = 3;
		item->_2C = 1;
		break;
	}
	item->_30 = &matoList;
}

// AddMato
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r5, matoList@ha
    stw      r0, 0x24(r1)
    addi     r7, r5, matoList@l
    stw      r31, 0x1c(r1)
    lwz      r6, 0x0(r7)
    cmplwi   r6, 0x50
    bge      .L_00002CF8
    mulli    r5, r6, 0x34
    addi     r0, r6, 0x1
    stw      r0, 0x0(r7)
    li       r0, 0x0
    add      r5, r7, r5
    addi     r31, r5, 0x2c
    stw      r3, 0x30(r5)
    stw      r4, 0x2c(r5)
    stw      r0, 0x48(r5)
    stb      r0, 0x45(r5)
    bl       HSD_Rand
    xoris    r0, r3, 0x8000
    lis      r3, 0x4330
    stw      r0, 0xc(r1)
    lis      r4, lbl_5_rodata_110@ha
    lfd      f1, lbl_5_rodata_110@l(r4)
    lis      r0, 0x1
    stw      r3, 0x8(r1)
    lfd      f0, 0x8(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x20(r31)
    lwz      r5, 0x4(r31)
    lwz      r4, 0x22c(r5)
    or       r0, r4, r0
    stw      r0, 0x22c(r5)
    lwz      r3, 0x4(r31)
    lwz      r3, 0x244(r3)
    cmplwi   r3, 0x0
    beq      .L_00002CA4
    lwz      r0, 0x0(r3)
    b        .L_00002CA8
.L_00002CA4:
    li       r0, -0x1
.L_00002CA8:
    cmpwi    r0, 0x2ea
    beq      .L_00002CD8
    bge      .L_00002CEC
    cmpwi    r0, 0x14d
    beq      .L_00002CC0
    b        .L_00002CEC
.L_00002CC0:
    li       r3, 0x1
    li       r0, 0x0
    stw      r3, 0x24(r31)
    stw      r3, 0x28(r31)
    stw      r0, 0x2c(r31)
    b        .L_00002CEC
.L_00002CD8:
    li       r3, 0x3
    li       r0, 0x1
    stw      r3, 0x24(r31)
    stw      r3, 0x28(r31)
    stw      r0, 0x2c(r31)
.L_00002CEC:
    lis      r3, matoList@ha
    addi     r0, r3, matoList@l
    stw      r0, 0x30(r31)
.L_00002CF8:
    lwz      r0, 0x24(r1)
    lwz      r31, 0x1c(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x2D0C
 * @note Size: 0x1BC
 */
extern "C" void StartFighter(UnkDolObj* a, UnkDolObj* b, void* c)
{
	UnkGame_96BC* model;

	if (a == NULL || b == NULL) {
		return;
	}
	shotEvents.mFlags = 0;
	Fighter_Init(&fighterWork, a, b, c);
	matoList._00 = 0;
	matoList._10 = 0;
	matoList._14 = 0;
	matoList._18 = 0.0f;
	matoList._1C = 6000;
	matoList._28 = NULL;
	model        = GAME_FIELD(UnkGame_96BC*, GAME_OFS(0x96BC, 0x9674));
	model->_980  = 0;
	model->_980 |= 0x40000000;
	model->_12DF4 = 0;
	((CMotionControlCamera*)(gGame + GAME_OFS(0xE5D38, 0xE5050)))->ChangeAction(&fighterCamera);
	{
		fighterCamera.SetView(MakeVec(0.0f, 40.0f, -180.0f), MakeVec(0.0f, 0.0f, 120.0f), fighterWork._0C);
	}
	fireHoldFrames = 0;
	lbl_5_bss_0    = 0;
	panels._00     = 0;
	panels._04     = 0;
	panels._08     = 0.0f;
	panels._0C     = 0.0f;
	panels._10     = 0;
	panels._14     = 0;
	panels._18     = 6;
	panels._1C     = 6;
	panels._20     = 0;
}

// StartFighter
/*
    stwu     r1, -0x40(r1)
    mflr     r0
    mr.      r8, r3
    mr       r7, r4
    stw      r0, 0x44(r1)
    mr       r6, r5
    stw      r31, 0x3c(r1)
    beq      .L_00002EB4
    cmplwi   r7, 0x0
    bne      .L_00002D38
    b        .L_00002EB4
.L_00002D38:
    lis      r4, shotEvents@ha
    li       r0, 0x0
    lis      r3, fighterWork@ha
    stw      r0, shotEvents@l(r4)
    addi     r3, r3, fighterWork@l
    mr       r4, r8
    mr       r5, r7
    bl       Fighter_Init
    lis      r4, lbl_5_rodata_A4@ha
    lis      r3, gGame@ha
    addi     r5, r3, gGame@l
    lfs      f0, lbl_5_rodata_A4@l(r4)
    addis    r3, r5, 0x1
    li       r7, 0x0
    lis      r6, matoList@ha
    stwu     r7, matoList@l(r6)
    li       r0, 0x1770
    lwz      r8, -0x6944(r3)
    stw      r7, 0x10(r6)
    addis    r3, r5, 0xe
    lis      r4, fighterCamera@ha
    addis    r5, r8, 0x1
    stw      r7, 0x14(r6)
    addi     r4, r4, fighterCamera@l
    addi     r3, r3, 0x5d38
    stfs     f0, 0x18(r6)
    stw      r0, 0x1c(r6)
    stw      r7, 0x28(r6)
    stw      r7, 0x980(r8)
    lwz      r0, 0x980(r8)
    oris     r0, r0, 0x4000
    stw      r0, 0x980(r8)
    stw      r7, 0x2df4(r5)
    bl       fn_801494A4
    lis      r4, lbl_5_rodata_A0@ha
    lis      r3, lbl_5_rodata_158@ha
    lfs      f0, lbl_5_rodata_158@l(r3)
    lis      r5, lbl_5_rodata_A4@ha
    lfs      f1, lbl_5_rodata_A0@l(r4)
    lis      r3, lbl_5_rodata_15C@ha
    stfs     f0, 0x28(r1)
    addi     r4, r3, lbl_5_rodata_15C@l
    lfs      f6, lbl_5_rodata_A4@l(r5)
    lis      r7, fighterCamera@ha
    stfs     f1, 0x24(r1)
    lis      r3, fighterWork@ha
    lfs      f0, 0x0(r4)
    addi     r6, r3, fighterWork@l
    lwz      r10, 0x24(r1)
    addi     r12, r7, fighterCamera@l
    lwz      r9, 0x28(r1)
    li       r31, 0x0
    stfs     f6, 0x20(r1)
    lis      r3, panels@ha
    lis      r5, fireHoldFrames@ha
    lis      r4, lbl_5_bss_0@ha
    lwz      r11, 0x20(r1)
    addi     r6, r6, 0xc
    stfs     f6, 0x24(r1)
    li       r0, 0x6
    stwu     r31, panels@l(r3)
    lwz      r8, 0x24(r1)
    stfs     f0, 0x28(r1)
    lwz      r7, 0x28(r1)
    stw      r11, 0x14(r1)
    stw      r10, 0x18(r1)
    lfs      f5, 0x14(r1)
    stw      r9, 0x1c(r1)
    lfs      f4, 0x18(r1)
    stw      r11, 0x8(r1)
    lfs      f3, 0x1c(r1)
    stw      r8, 0xc(r1)
    lfs      f2, 0x8(r1)
    stw      r7, 0x10(r1)
    lfs      f1, 0xc(r1)
    lfs      f0, 0x10(r1)
    stb      r31, 0x68(r12)
    stfs     f5, 0x50(r12)
    stfs     f4, 0x54(r12)
    stfs     f3, 0x58(r12)
    stfs     f6, 0x20(r1)
    stfs     f2, 0x44(r12)
    stfs     f1, 0x48(r12)
    stfs     f0, 0x4c(r12)
    stw      r6, 0x40(r12)
    stw      r31, fireHoldFrames@l(r5)
    stb      r31, lbl_5_bss_0@l(r4)
    stw      r31, 0x4(r3)
    stfs     f6, 0x8(r3)
    stfs     f6, 0xc(r3)
    stw      r31, 0x10(r3)
    stw      r31, 0x14(r3)
    stw      r0, 0x18(r3)
    stw      r0, 0x1c(r3)
    stw      r31, 0x20(r3)
.L_00002EB4:
    lwz      r0, 0x44(r1)
    lwz      r31, 0x3c(r1)
    mtlr     r0
    addi     r1, r1, 0x40
    blr
*/

/**
 * @note Address: stage02 .text 0x2EC8
 * @note Size: 0x2C
 */
extern "C" void ClearFighterObjects(void)
{
	fighterWork._00 = fighterWork._04 = fighterWork._08 = NULL;
	pilotNwing._00 = pilotNwing._04 = NULL;
}

/**
 * @note Address: stage02 .text 0x2EF4
 * @note Size: 0x448
 */
extern "C" void PilotNwing_Update(PilotNwing* link, u32 dt, f32 frame)
{
	Mtx scale;
	Mtx m;
	HSD_JObj* jobj;

	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	s32 path[2] = { 0, 0 };
	jobj        = GetJointByPath(link->_04->_00, path, 2);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, link->_10);
	HSD_JObjCopyMtx(link->_00->_00, link->_10);
	link->_A0 = frame;
	link->_04->vtbl->_80(link->_04, link->_A0, 1.0f, 0.0f, 10.0f, 0, 1, 0, 0, 0);
	if (link->_08 != NULL) {
		PSMTXTrans(m, 0.0f, -1.89f, -1.0f);
		PSMTXConcat(link->_10, m, link->_40);
		HSD_JObjCopyMtx(link->_08->_00, link->_40);
	}
	if (link->_0C != NULL) {
		PSMTXTrans(m, 0.0f, -1.89f, -7.16f);
		PSMTXConcat(link->_10, m, link->_70);
		HSD_JObjCopyMtx(link->_0C->_00, link->_70);
	}
}

// PilotNwing_Update
/*
    stwu     r1, -0x90(r1)
    mflr     r0
    stw      r0, 0x94(r1)
    stfd     f31, 0x80(r1)
    psq_st   f31, 0x88(r1), 0, qr0
    stw      r31, 0x7c(r1)
    stw      r30, 0x78(r1)
    lis      r4, lbl_5_rodata_9C@ha
    fmr      f31, f1
    lfs      f1, lbl_5_rodata_9C@l(r4)
    mr       r31, r3
    addi     r3, r1, 0x40
    fmr      f2, f1
    fmr      f3, f1
    bl       PSMTXScale
    lwz      r3, 0x4(r31)
    lis      r4, lbl_5_rodata_48@ha
    addi     r5, r1, 0x8
    lwz      r30, 0x0(r3)
    lwzu     r3, lbl_5_rodata_48@l(r4)
    cmplwi   r30, 0x0
    lwz      r0, 0x4(r4)
    stw      r3, 0x8(r1)
    stw      r0, 0xc(r1)
    bne      .L_00002F60
    li       r30, 0x0
    b        .L_00002FC0
.L_00002F60:
    li       r3, 0x0
.L_00002F64:
    cmplwi   r30, 0x0
    bne      .L_00002F74
    li       r30, 0x0
    b        .L_00002F78
.L_00002F74:
    lwz      r30, 0x10(r30)
.L_00002F78:
    lwz      r0, 0x0(r5)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_00002FB0
.L_00002F88:
    cmplwi   r30, 0x0
    bne      .L_00002F98
    li       r30, 0x0
    b        .L_00002F9C
.L_00002F98:
    lwz      r30, 0x8(r30)
.L_00002F9C:
    cmplwi   r30, 0x0
    bne      .L_00002FAC
    li       r30, 0x0
    b        .L_00002FC0
.L_00002FAC:
    bdnz     .L_00002F88
.L_00002FB0:
    addi     r3, r3, 0x1
    addi     r5, r5, 0x4
    cmpwi    r3, 0x2
    blt      .L_00002F64
.L_00002FC0:
    cmplwi   r30, 0x0
    bne      .L_00002FE0
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x47c
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00002FE0:
    cmplwi   r30, 0x0
    beq      .L_00003030
    bne      .L_00003004
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003004:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003020
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003020
    li       r3, 0x1
.L_00003020:
    clrlwi.  r0, r3, 24
    beq      .L_00003030
    mr       r3, r30
    bl       HSD_JObjSetupMatrixSub
.L_00003030:
    addi     r3, r30, 0x44
    addi     r4, r1, 0x40
    addi     r5, r31, 0x10
    bl       PSMTXConcat
    lwz      r3, 0x0(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00003068
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003068:
    addic.   r0, r31, 0x10
    bne      .L_00003088
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_00003088:
    addi     r3, r31, 0x10
    addi     r4, r30, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r30)
    cmplwi   r30, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r30)
    beq      .L_000030F0
    bne      .L_000030C4
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000030C4:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000030E0
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000030E0
    li       r3, 0x1
.L_000030E0:
    clrlwi.  r0, r3, 24
    bne      .L_000030F0
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_000030F0:
    stfs     f31, 0xa0(r31)
    lis      r6, gFighterHudTextureScale@ha
    lis      r5, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_CC@ha
    lwz      r3, 0x4(r31)
    addi     r7, r6, gFighterHudTextureScale@l
    addi     r8, r5, lbl_5_rodata_A4@l
    addi     r9, r4, lbl_5_rodata_CC@l
    lwz      r12, 0x34(r3)
    li       r4, 0x0
    lfs      f1, 0xa0(r31)
    li       r5, 0x1
    lwz      r12, 0x80(r12)
    li       r6, 0x0
    lfs      f2, 0x0(r7)
    li       r7, 0x0
    lfs      f3, 0x0(r8)
    li       r8, 0x0
    lfs      f4, 0x0(r9)
    mtctr    r12
    bctrl
    lwz      r0, 0x8(r31)
    cmplwi   r0, 0x0
    beq      .L_00003230
    lis      r6, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_164@ha
    lis      r4, lbl_5_rodata_168@ha
    lfs      f1, lbl_5_rodata_A4@l(r6)
    lfs      f2, lbl_5_rodata_164@l(r5)
    addi     r3, r1, 0x10
    lfs      f3, lbl_5_rodata_168@l(r4)
    bl       PSMTXTrans
    addi     r3, r31, 0x10
    addi     r4, r1, 0x10
    addi     r5, r31, 0x40
    bl       PSMTXConcat
    lwz      r3, 0x8(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_000031A8
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000031A8:
    addic.   r0, r31, 0x40
    bne      .L_000031C8
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_000031C8:
    addi     r3, r31, 0x40
    addi     r4, r30, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r30)
    cmplwi   r30, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r30)
    beq      .L_00003230
    bne      .L_00003204
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003204:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003220
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003220
    li       r3, 0x1
.L_00003220:
    clrlwi.  r0, r3, 24
    bne      .L_00003230
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_00003230:
    lwz      r0, 0xc(r31)
    cmplwi   r0, 0x0
    beq      .L_0000331C
    lis      r6, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_164@ha
    lis      r4, lbl_5_rodata_16C@ha
    lfs      f1, lbl_5_rodata_A4@l(r6)
    lfs      f2, lbl_5_rodata_164@l(r5)
    addi     r3, r1, 0x10
    lfs      f3, lbl_5_rodata_16C@l(r4)
    bl       PSMTXTrans
    addi     r3, r31, 0x10
    addi     r4, r1, 0x10
    addi     r5, r31, 0x70
    bl       PSMTXConcat
    lwz      r3, 0xc(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    bne      .L_00003294
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003294:
    addic.   r0, r31, 0x70
    bne      .L_000032B4
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_000032B4:
    addi     r3, r31, 0x70
    addi     r4, r30, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r30)
    cmplwi   r30, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r30)
    beq      .L_0000331C
    bne      .L_000032F0
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_000032F0:
    lwz      r4, 0x14(r30)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_0000330C
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_0000330C
    li       r3, 0x1
.L_0000330C:
    clrlwi.  r0, r3, 24
    bne      .L_0000331C
    mr       r3, r30
    bl       HSD_JObjSetMtxDirtySub
.L_0000331C:
    psq_l    f31, 0x88(r1), 0, qr0
    lwz      r0, 0x94(r1)
    lfd      f31, 0x80(r1)
    lwz      r31, 0x7c(r1)
    lwz      r30, 0x78(r1)
    mtlr     r0
    addi     r1, r1, 0x90
    blr
*/

/**
 * @note Address: stage02 .text 0x333C
 * @note Size: 0x874
 */
extern "C" void Fighter_Fire(Fighter* hole, s32 n)
{
	Mato* item = hole->_F4._18;
	Vec pos;
	Vec v;
	CVec dir;
	UnkGame_96BC* model;
	f32 dist;

	v.x = v.z = v.y = 0.0f;
	v.y             = 10.0f;
	PSMTXMultVec(hole->_0C, &v, &dir);
	if (item == NULL) {
		v = hole->_F4._00;
		PSMTXMultVec(hole->_0C, &v, &pos);
		hole->_114.Add(pos, dir, item);
	} else {
		item->_24--;
		if (item->_24 <= 0) {
			item->_19 = true;
		}
		hole->_110 = NULL;
		hole->_114.Add(item->GetPos(), dir, item);
	}
	model        = GAME_FIELD(UnkGame_96BC*, GAME_OFS(0x96BC, 0x9674));
	model->_9658 = model->_9658 - n > 0.0f ? model->_9658 - n : 0.0f;
	if (model->_9658 < 0.0f) {
		model->_9658 = 0.0f;
	}
	if (model->_9658 > model->_965C) {
		model->_9658 = model->_965C;
	}
	{
		f32 dx = pos.x - dir.x;
		f32 dz = pos.z - dir.z;
		dist   = sqrtf(dx * dx + dz * dz);
	}
	if (dist != 0.0f) {
		f32 angle = atan2(dir.x - pos.x, dir.z - pos.z);
		while (angle < 0.0f) {
			angle += TWO_PI;
		}
		while (TWO_PI <= angle) {
			angle -= TWO_PI;
		}
	}
	hole->_AC -= hole->_EC;
	if (hole->_AC < hole->_E0) {
		hole->_AC = hole->_E0;
	}
}

// Fighter_Fire
/*
    stwu     r1, -0xd0(r1)
    mflr     r0
    lis      r5, lbl_5_rodata_A4@ha
    stw      r0, 0xd4(r1)
    lfs      f1, lbl_5_rodata_A4@l(r5)
    lis      r5, __vt__4CVec@ha
    stw      r31, 0xcc(r1)
    addi     r0, r5, __vt__4CVec@l
    addi     r5, r1, 0x94
    stw      r30, 0xc8(r1)
    mr       r30, r4
    addi     r4, r1, 0x88
    stw      r29, 0xc4(r1)
    mr       r29, r3
    lis      r3, lbl_5_rodata_CC@ha
    lwz      r31, 0x10c(r29)
    lfs      f0, lbl_5_rodata_CC@l(r3)
    addi     r3, r29, 0xc
    stfs     f1, 0x8c(r1)
    stw      r0, 0xa0(r1)
    stfs     f1, 0x90(r1)
    stfs     f1, 0x88(r1)
    stfs     f0, 0x8c(r1)
    bl       PSMTXMultVec
    cmplwi   r31, 0x0
    bne      .L_00003680
    lfs      f0, 0xf4(r29)
    addi     r3, r29, 0xc
    addi     r4, r1, 0x88
    addi     r5, r1, 0x7c
    stfs     f0, 0x88(r1)
    lfs      f0, 0xf8(r29)
    stfs     f0, 0x8c(r1)
    lfs      f0, 0xfc(r29)
    stfs     f0, 0x90(r1)
    bl       PSMTXMultVec
    lbz      r0, 0x114(r29)
    li       r8, 0x0
    lwz      r7, 0x7c(r1)
    lwz      r6, 0x80(r1)
    cmplwi   r0, 0x0
    lwz      r5, 0x84(r1)
    lwz      r4, 0x94(r1)
    lwz      r3, 0x98(r1)
    lwz      r0, 0x9c(r1)
    stw      r7, 0x64(r1)
    stw      r6, 0x68(r1)
    stw      r5, 0x6c(r1)
    stw      r4, 0x70(r1)
    stw      r3, 0x74(r1)
    stw      r0, 0x78(r1)
    beq      .L_00003450
    lbz      r0, 0x1d0(r29)
    li       r8, 0x1
    cmplwi   r0, 0x0
    beq      .L_00003450
    lbz      r0, 0x28c(r29)
    li       r8, 0x2
    cmplwi   r0, 0x0
    beq      .L_00003450
    lbz      r0, 0x348(r29)
    li       r8, 0x3
    cmplwi   r0, 0x0
    beq      .L_00003450
    lbz      r0, 0x404(r29)
    li       r8, 0x4
    cmplwi   r0, 0x0
    beq      .L_00003450
    li       r8, 0x5
.L_00003450:
    cmplwi   r8, 0x5
    bge      .L_00003970
    mulli    r4, r8, 0xbc
    lwz      r3, 0x70(r1)
    lwz      r5, 0x74(r1)
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lis      r3, lbl_5_rodata_110@ha
    stw      r5, 0x28(r1)
    add      r8, r29, r4
    li       r5, 0x1
    lwz      r4, 0x78(r1)
    lwz      r6, 0x64(r1)
    stw      r4, 0x2c(r1)
    li       r4, 0x3c
    lwz      r7, 0x68(r1)
    stw      r6, 0x30(r1)
    lwz      r6, 0x6c(r1)
    stw      r7, 0x34(r1)
    lfs      f3, 0x24(r1)
    stb      r5, 0x114(r8)
    lfs      f4, 0x28(r1)
    stfs     f3, 0x118(r8)
    lfs      f5, 0x2c(r1)
    stfs     f4, 0x11c(r8)
    lfs      f0, 0x30(r1)
    stfs     f5, 0x120(r8)
    lfs      f1, 0x34(r1)
    stfs     f0, 0x124(r8)
    lfd      f2, lbl_5_rodata_110@l(r3)
    stw      r6, 0x38(r1)
    lfs      f0, 0x38(r1)
    stfs     f1, 0x128(r8)
    stfs     f0, 0x12c(r8)
    stfs     f3, 0x130(r8)
    stfs     f4, 0x134(r8)
    stfs     f5, 0x138(r8)
    stw      r31, 0x1c4(r8)
    stw      r4, 0x148(r8)
    lwz      r3, 0x148(r8)
    stw      r0, 0xa8(r1)
    xoris    r3, r3, 0x8000
    lfs      f3, 0x124(r8)
    stw      r3, 0xac(r1)
    lfs      f1, 0x118(r8)
    lfd      f0, 0xa8(r1)
    fsubs    f1, f3, f1
    stw      r0, 0xb0(r1)
    fsubs    f0, f0, f2
    stw      r0, 0xb8(r1)
    fdivs    f0, f1, f0
    stfs     f0, 0x13c(r8)
    lwz      r0, 0x148(r8)
    lfs      f1, 0x128(r8)
    xoris    r0, r0, 0x8000
    lfs      f0, 0x11c(r8)
    stw      r0, 0xb4(r1)
    fsubs    f1, f1, f0
    lfd      f0, 0xb0(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x140(r8)
    lwz      r0, 0x148(r8)
    lfs      f1, 0x12c(r8)
    xoris    r0, r0, 0x8000
    lfs      f0, 0x120(r8)
    stw      r0, 0xbc(r1)
    fsubs    f1, f1, f0
    lfd      f0, 0xb8(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x144(r8)
    lfs      f0, 0x130(r8)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r4, 0xb
    stfs     f0, 0x14c(r8)
    addis    r3, r3, 0xe
    li       r5, 0xff
    lfs      f0, 0x134(r8)
    addi     r3, r3, 0x7e24
    stfs     f0, 0x150(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x154(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x158(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x15c(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x160(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x164(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x168(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x16c(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x170(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x174(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x178(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x17c(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x180(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x184(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x188(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x18c(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x190(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x194(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x198(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x19c(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1a0(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1a4(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1a8(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1ac(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1b0(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1b4(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1b8(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1bc(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1c0(r8)
    bl       PlayGameSound
    b        .L_00003970
.L_00003680:
    lwz      r3, 0x24(r31)
    subi     r0, r3, 0x1
    stw      r0, 0x24(r31)
    lwz      r0, 0x24(r31)
    cmpwi    r0, 0x0
    bgt      .L_000036A0
    li       r0, 0x1
    stb      r0, 0x19(r31)
.L_000036A0:
    li       r0, 0x0
    mr       r4, r31
    stw      r0, 0x110(r29)
    addi     r3, r1, 0x3c
    bl       GetPos
    lbz      r0, 0x114(r29)
    li       r8, 0x0
    lwz      r7, 0x3c(r1)
    lwz      r6, 0x40(r1)
    cmplwi   r0, 0x0
    lwz      r5, 0x44(r1)
    lwz      r4, 0x94(r1)
    lwz      r3, 0x98(r1)
    lwz      r0, 0x9c(r1)
    stw      r7, 0x4c(r1)
    stw      r6, 0x50(r1)
    stw      r5, 0x54(r1)
    stw      r4, 0x58(r1)
    stw      r3, 0x5c(r1)
    stw      r0, 0x60(r1)
    beq      .L_00003738
    lbz      r0, 0x1d0(r29)
    li       r8, 0x1
    cmplwi   r0, 0x0
    beq      .L_00003738
    lbz      r0, 0x28c(r29)
    li       r8, 0x2
    cmplwi   r0, 0x0
    beq      .L_00003738
    lbz      r0, 0x348(r29)
    li       r8, 0x3
    cmplwi   r0, 0x0
    beq      .L_00003738
    lbz      r0, 0x404(r29)
    li       r8, 0x4
    cmplwi   r0, 0x0
    beq      .L_00003738
    li       r8, 0x5
.L_00003738:
    cmplwi   r8, 0x5
    bge      .L_00003964
    mulli    r4, r8, 0xbc
    lwz      r3, 0x58(r1)
    lwz      r5, 0x5c(r1)
    lis      r0, 0x4330
    stw      r3, 0xc(r1)
    lis      r3, lbl_5_rodata_110@ha
    stw      r5, 0x10(r1)
    add      r8, r29, r4
    li       r5, 0x1
    lwz      r4, 0x60(r1)
    lwz      r6, 0x4c(r1)
    stw      r4, 0x14(r1)
    li       r4, 0x3c
    lwz      r7, 0x50(r1)
    stw      r6, 0x18(r1)
    lwz      r6, 0x54(r1)
    stw      r7, 0x1c(r1)
    lfs      f3, 0xc(r1)
    stb      r5, 0x114(r8)
    lfs      f4, 0x10(r1)
    stfs     f3, 0x118(r8)
    lfs      f5, 0x14(r1)
    stfs     f4, 0x11c(r8)
    lfs      f0, 0x18(r1)
    stfs     f5, 0x120(r8)
    lfs      f1, 0x1c(r1)
    stfs     f0, 0x124(r8)
    lfd      f2, lbl_5_rodata_110@l(r3)
    stw      r6, 0x20(r1)
    lfs      f0, 0x20(r1)
    stfs     f1, 0x128(r8)
    stfs     f0, 0x12c(r8)
    stfs     f3, 0x130(r8)
    stfs     f4, 0x134(r8)
    stfs     f5, 0x138(r8)
    stw      r31, 0x1c4(r8)
    stw      r4, 0x148(r8)
    lwz      r3, 0x148(r8)
    stw      r0, 0xb8(r1)
    xoris    r3, r3, 0x8000
    lfs      f3, 0x124(r8)
    stw      r3, 0xbc(r1)
    lfs      f1, 0x118(r8)
    lfd      f0, 0xb8(r1)
    fsubs    f1, f3, f1
    stw      r0, 0xb0(r1)
    fsubs    f0, f0, f2
    stw      r0, 0xa8(r1)
    fdivs    f0, f1, f0
    stfs     f0, 0x13c(r8)
    lwz      r0, 0x148(r8)
    lfs      f1, 0x128(r8)
    xoris    r0, r0, 0x8000
    lfs      f0, 0x11c(r8)
    stw      r0, 0xb4(r1)
    fsubs    f1, f1, f0
    lfd      f0, 0xb0(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x140(r8)
    lwz      r0, 0x148(r8)
    lfs      f1, 0x12c(r8)
    xoris    r0, r0, 0x8000
    lfs      f0, 0x120(r8)
    stw      r0, 0xac(r1)
    fsubs    f1, f1, f0
    lfd      f0, 0xa8(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f1, f0
    stfs     f0, 0x144(r8)
    lfs      f0, 0x130(r8)
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    li       r4, 0xb
    stfs     f0, 0x14c(r8)
    addis    r3, r3, 0xe
    li       r5, 0xff
    lfs      f0, 0x134(r8)
    addi     r3, r3, 0x7e24
    stfs     f0, 0x150(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x154(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x158(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x15c(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x160(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x164(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x168(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x16c(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x170(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x174(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x178(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x17c(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x180(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x184(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x188(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x18c(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x190(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x194(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x198(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x19c(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1a0(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1a4(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1a8(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1ac(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1b0(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1b4(r8)
    lfs      f0, 0x130(r8)
    stfs     f0, 0x1b8(r8)
    lfs      f0, 0x134(r8)
    stfs     f0, 0x1bc(r8)
    lfs      f0, 0x138(r8)
    stfs     f0, 0x1c0(r8)
    bl       PlayGameSound
.L_00003964:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0x48(r1)
.L_00003970:
    xoris    r3, r30, 0x8000
    lis      r5, gGame@ha
    lis      r0, 0x4330
    stw      r3, 0xbc(r1)
    addi     r5, r5, gGame@l
    lis      r4, lbl_5_rodata_110@ha
    stw      r0, 0xb8(r1)
    addis    r5, r5, 0x1
    lwz      r5, -0x6944(r5)
    lis      r3, lbl_5_rodata_A4@ha
    lfd      f1, lbl_5_rodata_110@l(r4)
    lfd      f0, 0xb8(r1)
    addis    r4, r5, 0x1
    lfs      f2, -0x69a8(r4)
    fsubs    f1, f0, f1
    lfs      f0, lbl_5_rodata_A4@l(r3)
    fsubs    f1, f2, f1
    fcmpo    cr0, f1, f0
    ble      .L_000039C0
    fmr      f0, f1
.L_000039C0:
    stfs     f0, -0x69a8(r4)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
    lfs      f1, -0x69a8(r4)
    fcmpo    cr0, f1, f0
    bge      .L_000039DC
    stfs     f0, -0x69a8(r4)
.L_000039DC:
    lfs      f0, -0x69a8(r4)
    lfs      f1, -0x69a4(r4)
    fcmpo    cr0, f0, f1
    ble      .L_000039F0
    stfs     f1, -0x69a8(r4)
.L_000039F0:
    lfs      f1, 0x84(r1)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, 0x9c(r1)
    lfs      f2, 0x7c(r1)
    fsubs    f3, f1, f0
    lfs      f1, 0x94(r1)
    lfs      f0, lbl_5_rodata_A4@l(r3)
    fsubs    f2, f2, f1
    fmuls    f1, f3, f3
    fmadds   f4, f2, f2, f1
    fcmpo    cr0, f4, f0
    ble      .L_00003A70
    frsqrte  f1, f4
    lis      r4, lbl_5_rodata_118@ha
    lis      r3, lbl_5_rodata_120@ha
    lfd      f3, lbl_5_rodata_118@l(r4)
    lfd      f2, lbl_5_rodata_120@l(r3)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f0, f1, f0
    fmul     f4, f4, f0
    frsp     f4, f4
    b        .L_00003AF8
.L_00003A70:
    lis      r3, lbl_5_rodata_128@ha
    lfd      f0, lbl_5_rodata_128@l(r3)
    fcmpo    cr0, f4, f0
    bge      .L_00003A8C
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
    b        .L_00003AF8
.L_00003A8C:
    stfs     f4, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_00003AB4
    bge      .L_00003AE4
    cmpwi    r3, 0x0
    beq      .L_00003ACC
    b        .L_00003AE4
.L_00003AB4:
    clrlwi.  r0, r4, 9
    beq      .L_00003AC4
    li       r0, 0x1
    b        .L_00003AE8
.L_00003AC4:
    li       r0, 0x2
    b        .L_00003AE8
.L_00003ACC:
    clrlwi.  r0, r4, 9
    beq      .L_00003ADC
    li       r0, 0x5
    b        .L_00003AE8
.L_00003ADC:
    li       r0, 0x3
    b        .L_00003AE8
.L_00003AE4:
    li       r0, 0x4
.L_00003AE8:
    cmpwi    r0, 0x1
    bne      .L_00003AF8
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
.L_00003AF8:
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
    fcmpu    cr0, f0, f4
    beq      .L_00003B64
    lfs      f3, 0x94(r1)
    lfs      f1, 0x7c(r1)
    lfs      f2, 0x9c(r1)
    lfs      f0, 0x84(r1)
    fsubs    f1, f3, f1
    fsubs    f2, f2, f0
    bl       atan2
    lis      r3, lbl_5_rodata_A4@ha
    frsp     f2, f1
    lis      r4, lbl_5_rodata_130@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
    lfs      f1, lbl_5_rodata_130@l(r4)
    b        .L_00003B40
.L_00003B3C:
    fadds    f2, f2, f1
.L_00003B40:
    fcmpo    cr0, f2, f0
    blt      .L_00003B3C
    lis      r3, lbl_5_rodata_130@ha
    lfs      f0, lbl_5_rodata_130@l(r3)
    b        .L_00003B58
.L_00003B54:
    fsubs    f2, f2, f0
.L_00003B58:
    fcmpo    cr0, f0, f2
    cror     eq, lt, eq
    beq      .L_00003B54
.L_00003B64:
    lfs      f1, 0xac(r29)
    lfs      f0, 0xec(r29)
    fsubs    f0, f1, f0
    stfs     f0, 0xac(r29)
    lfs      f0, 0xac(r29)
    lfs      f1, 0xe0(r29)
    fcmpo    cr0, f0, f1
    bge      .L_00003B88
    stfs     f1, 0xac(r29)
.L_00003B88:
    lis      r3, __vt__4CVec@ha
    addi     r0, r3, __vt__4CVec@l
    stw      r0, 0xa0(r1)
    lwz      r31, 0xcc(r1)
    lwz      r30, 0xc8(r1)
    lwz      r29, 0xc4(r1)
    lwz      r0, 0xd4(r1)
    mtlr     r0
    addi     r1, r1, 0xd0
    blr
*/

/**
 * @note Address: stage02 .text 0x3BB0
 * @note Size: 0x664
 */
extern "C" void Fighter_Update(Fighter* hole, u32 dt)
{
	Mtx scale;
	Mtx m;
	Mtx rot;
	Vec zero;
	HSD_JObj* jobj;
	f32 rate;

	hole->_BC -= dt;
	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	s32 path[2]  = { 0, 0 };
	s32 path2[1] = { 0 };
	jobj         = GetJointByPath(hole->_04->_00, path, 2);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, hole->_0C);
	HSD_JObjCopyMtx(hole->_00->_00, hole->_0C);
	zero.x = zero.y = zero.z = 0.0f;
	PSMTXMultVec(hole->_0C, &zero, &hole->_C8);
	PSMTXRotRad(rot, 'Y', hole->_9C);
	PSMTXTrans(m, 0.0f, -1.89f, -7.16f);
	PSMTXConcat(m, rot, m);
	jobj = GetJointByPath(hole->_00->_00, path2, 1);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), m, hole->_3C);
	HSD_JObjCopyMtx(GAME_FIELD(UnkGame_96BC*, GAME_OFS(0x96BC, 0x9674))->_00, hole->_3C);
	if (hole->_08 != NULL) {
		PSMTXTrans(m, 0.0f, 4.11f, -1.0f);
		PSMTXConcat(hole->_0C, m, hole->_6C);
		HSD_JObjCopyMtx(hole->_08->_00, hole->_6C);
	}
	hole->_AC += dt * hole->_E8 / 60.0f;
	if (hole->_E4 < hole->_AC) {
		hole->_AC = hole->_E4;
	}
	hole->_A4 = hole->_A0;
	if (hole->_B0 == NULL) {
		rate = 0.0f;
	} else {
		rate = hole->_B0[(s32)hole->_A0] / 100.0f;
		if (rate <= 0.0f) {
			rate = 1.0f;
		}
		if (2.5f < rate) {
			rate = 2.5f;
		}
	}
	hole->_A0 = hole->_AC * rate + hole->_A0;
	hole->_04->vtbl->_80(hole->_04, hole->_A0, 1.0f, 0.0f, 10.0f, 0, 1, 0, 0, 0);
	ShotList_Update(&hole->_114, dt);
}

// Fighter_Update
/*
    stwu     r1, -0xd0(r1)
    mflr     r0
    lis      r5, lbl_5_rodata_9C@ha
    stw      r0, 0xd4(r1)
    lfs      f1, lbl_5_rodata_9C@l(r5)
    stw      r31, 0xcc(r1)
    mr       r31, r3
    fmr      f2, f1
    stw      r30, 0xc8(r1)
    mr       r30, r4
    fmr      f3, f1
    stw      r29, 0xc4(r1)
    lwz      r0, 0xbc(r3)
    addi     r3, r1, 0x50
    subf     r0, r30, r0
    stw      r0, 0xbc(r31)
    bl       PSMTXScale
    lwz      r3, 0x4(r31)
    lis      r4, lbl_5_rodata_3C@ha
    addi     r5, r1, 0xc
    lwz      r29, 0x0(r3)
    lwzu     r3, lbl_5_rodata_3C@l(r4)
    cmplwi   r29, 0x0
    lwz      r0, 0x4(r4)
    stw      r3, 0xc(r1)
    stw      r0, 0x10(r1)
    bne      .L_00003C24
    li       r29, 0x0
    b        .L_00003C84
.L_00003C24:
    li       r3, 0x0
.L_00003C28:
    cmplwi   r29, 0x0
    bne      .L_00003C38
    li       r29, 0x0
    b        .L_00003C3C
.L_00003C38:
    lwz      r29, 0x10(r29)
.L_00003C3C:
    lwz      r0, 0x0(r5)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_00003C74
.L_00003C4C:
    cmplwi   r29, 0x0
    bne      .L_00003C5C
    li       r29, 0x0
    b        .L_00003C60
.L_00003C5C:
    lwz      r29, 0x8(r29)
.L_00003C60:
    cmplwi   r29, 0x0
    bne      .L_00003C70
    li       r29, 0x0
    b        .L_00003C84
.L_00003C70:
    bdnz     .L_00003C4C
.L_00003C74:
    addi     r3, r3, 0x1
    addi     r5, r5, 0x4
    cmpwi    r3, 0x2
    blt      .L_00003C28
.L_00003C84:
    cmplwi   r29, 0x0
    bne      .L_00003CA4
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x47c
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003CA4:
    cmplwi   r29, 0x0
    beq      .L_00003CF4
    bne      .L_00003CC8
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003CC8:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003CE4
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003CE4
    li       r3, 0x1
.L_00003CE4:
    clrlwi.  r0, r3, 24
    beq      .L_00003CF4
    mr       r3, r29
    bl       HSD_JObjSetupMatrixSub
.L_00003CF4:
    addi     r3, r29, 0x44
    addi     r4, r1, 0x50
    addi     r5, r31, 0xc
    bl       PSMTXConcat
    lwz      r3, 0x0(r31)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00003D2C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003D2C:
    addic.   r0, r31, 0xc
    bne      .L_00003D4C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_00003D4C:
    addi     r3, r31, 0xc
    addi     r4, r29, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r29)
    beq      .L_00003DB4
    bne      .L_00003D88
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003D88:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003DA4
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003DA4
    li       r3, 0x1
.L_00003DA4:
    clrlwi.  r0, r3, 24
    bne      .L_00003DB4
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_00003DB4:
    lis      r4, lbl_5_rodata_A4@ha
    addi     r3, r31, 0xc
    addi     r5, r4, lbl_5_rodata_A4@l
    addi     r4, r1, 0x14
    lfs      f0, 0x0(r5)
    addi     r5, r31, 0xc8
    stfs     f0, 0x1c(r1)
    stfs     f0, 0x18(r1)
    stfs     f0, 0x14(r1)
    bl       PSMTXMultVec
    lfs      f1, 0x9c(r31)
    addi     r3, r1, 0x80
    li       r4, 0x59
    bl       PSMTXRotRad
    lis      r6, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_164@ha
    lis      r4, lbl_5_rodata_16C@ha
    lfs      f1, lbl_5_rodata_A4@l(r6)
    lfs      f2, lbl_5_rodata_164@l(r5)
    addi     r3, r1, 0x20
    lfs      f3, lbl_5_rodata_16C@l(r4)
    bl       PSMTXTrans
    addi     r3, r1, 0x20
    addi     r4, r1, 0x80
    mr       r5, r3
    bl       PSMTXConcat
    lis      r3, lbl_5_rodata_44@ha
    lwz      r0, lbl_5_rodata_44@l(r3)
    stw      r0, 0x8(r1)
    lwz      r3, 0x0(r31)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00003E40
    li       r29, 0x0
    b        .L_00003E8C
.L_00003E40:
    li       r0, 0x0
    bne      .L_00003E50
    mr       r29, r0
    b        .L_00003E54
.L_00003E50:
    lwz      r29, 0x10(r29)
.L_00003E54:
    lwz      r0, 0x8(r1)
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_00003E8C
.L_00003E64:
    cmplwi   r29, 0x0
    bne      .L_00003E74
    li       r29, 0x0
    b        .L_00003E78
.L_00003E74:
    lwz      r29, 0x8(r29)
.L_00003E78:
    cmplwi   r29, 0x0
    bne      .L_00003E88
    li       r29, 0x0
    b        .L_00003E8C
.L_00003E88:
    bdnz     .L_00003E64
.L_00003E8C:
    cmplwi   r29, 0x0
    bne      .L_00003EAC
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x47c
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003EAC:
    cmplwi   r29, 0x0
    beq      .L_00003EFC
    bne      .L_00003ED0
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003ED0:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003EEC
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003EEC
    li       r3, 0x1
.L_00003EEC:
    clrlwi.  r0, r3, 24
    beq      .L_00003EFC
    mr       r3, r29
    bl       HSD_JObjSetupMatrixSub
.L_00003EFC:
    addi     r3, r29, 0x44
    addi     r4, r1, 0x20
    addi     r5, r31, 0x3c
    bl       PSMTXConcat
    lis      r3, gGame@ha
    addi     r3, r3, gGame@l
    addis    r3, r3, 0x1
    lwz      r3, -0x6944(r3)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_00003F40
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003F40:
    addic.   r0, r31, 0x3c
    bne      .L_00003F60
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_00003F60:
    addi     r3, r31, 0x3c
    addi     r4, r29, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r29)
    beq      .L_00003FC8
    bne      .L_00003F9C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00003F9C:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_00003FB8
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_00003FB8
    li       r3, 0x1
.L_00003FB8:
    clrlwi.  r0, r3, 24
    bne      .L_00003FC8
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_00003FC8:
    lwz      r0, 0x8(r31)
    cmplwi   r0, 0x0
    beq      .L_000040B4
    lis      r6, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_178@ha
    lis      r4, lbl_5_rodata_168@ha
    lfs      f1, lbl_5_rodata_A4@l(r6)
    lfs      f2, lbl_5_rodata_178@l(r5)
    addi     r3, r1, 0x20
    lfs      f3, lbl_5_rodata_168@l(r4)
    bl       PSMTXTrans
    addi     r3, r31, 0xc
    addi     r4, r1, 0x20
    addi     r5, r31, 0x6c
    bl       PSMTXConcat
    lwz      r3, 0x8(r31)
    lwz      r29, 0x0(r3)
    cmplwi   r29, 0x0
    bne      .L_0000402C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x495
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_0000402C:
    addic.   r0, r31, 0x6c
    bne      .L_0000404C
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_160@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x496
    addi     r5, r5, lbl_5_rodata_160@l
    bl       __assert
.L_0000404C:
    addi     r3, r31, 0x6c
    addi     r4, r29, 0x44
    bl       PSMTXCopy
    lwz      r0, 0x14(r29)
    cmplwi   r29, 0x0
    oris     r0, r0, 0x380
    stw      r0, 0x14(r29)
    beq      .L_000040B4
    bne      .L_00004088
    lis      r3, sStage02PathJobjH@ha
    lis      r5, lbl_5_rodata_150@ha
    addi     r3, r3, sStage02PathJobjH@l
    li       r4, 0x25d
    addi     r5, r5, lbl_5_rodata_150@l
    bl       __assert
.L_00004088:
    lwz      r4, 0x14(r29)
    li       r3, 0x0
    rlwinm.  r0, r4, 0, 8, 8
    bne      .L_000040A4
    rlwinm.  r0, r4, 0, 25, 25
    beq      .L_000040A4
    li       r3, 0x1
.L_000040A4:
    clrlwi.  r0, r3, 24
    bne      .L_000040B4
    mr       r3, r29
    bl       HSD_JObjSetMtxDirtySub
.L_000040B4:
    lis      r0, 0x4330
    lis      r3, lbl_5_rodata_108@ha
    stw      r30, 0xb4(r1)
    addi     r4, r3, lbl_5_rodata_108@l
    lis      r3, lbl_5_rodata_17C@ha
    lfd      f3, 0x0(r4)
    stw      r0, 0xb0(r1)
    lfs      f1, 0xe8(r31)
    lfd      f2, 0xb0(r1)
    lfs      f0, lbl_5_rodata_17C@l(r3)
    fsubs    f2, f2, f3
    lfs      f3, 0xac(r31)
    fmuls    f1, f2, f1
    fdivs    f0, f1, f0
    fadds    f0, f3, f0
    stfs     f0, 0xac(r31)
    lfs      f1, 0xe4(r31)
    lfs      f0, 0xac(r31)
    fcmpo    cr0, f1, f0
    bge      .L_00004108
    stfs     f1, 0xac(r31)
.L_00004108:
    lfs      f0, 0xa0(r31)
    stfs     f0, 0xa4(r31)
    lwz      r6, 0xb0(r31)
    lfs      f0, 0xa0(r31)
    cmplwi   r6, 0x0
    bne      .L_0000412C
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f2, lbl_5_rodata_A4@l(r3)
    b        .L_00004198
.L_0000412C:
    fctiwz   f0, f0
    lis      r3, lbl_5_rodata_108@ha
    addi     r5, r3, lbl_5_rodata_108@l
    lis      r0, 0x4330
    lis      r4, lbl_5_rodata_F4@ha
    lfd      f3, 0x0(r5)
    stfd     f0, 0xb0(r1)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    lwz      r5, 0xb4(r1)
    stw      r0, 0xb8(r1)
    lbzx     r0, r6, r5
    lfs      f0, lbl_5_rodata_A4@l(r3)
    stw      r0, 0xbc(r1)
    lfd      f2, 0xb8(r1)
    fsubs    f2, f2, f3
    fdivs    f2, f2, f1
    fcmpo    cr0, f2, f0
    cror     eq, lt, eq
    bne      .L_00004184
    lis      r3, gFighterHudTextureScale@ha
    lfs      f2, gFighterHudTextureScale@l(r3)
.L_00004184:
    lis      r3, lbl_5_rodata_174@ha
    lfs      f0, lbl_5_rodata_174@l(r3)
    fcmpo    cr0, f0, f2
    bge      .L_00004198
    fmr      f2, f0
.L_00004198:
    lfs      f1, 0xac(r31)
    lis      r3, lbl_5_rodata_CC@ha
    lfs      f0, 0xa0(r31)
    lis      r4, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r4)
    lis      r5, gFighterHudTextureScale@ha
    fmadds   f0, f1, f2, f0
    lfs      f2, gFighterHudTextureScale@l(r5)
    lfs      f4, lbl_5_rodata_CC@l(r3)
    li       r4, 0x0
    li       r5, 0x1
    li       r6, 0x0
    stfs     f0, 0xa0(r31)
    li       r7, 0x0
    li       r8, 0x0
    lwz      r3, 0x4(r31)
    lfs      f1, 0xa0(r31)
    lwz      r12, 0x34(r3)
    lwz      r12, 0x80(r12)
    mtctr    r12
    bctrl
    mr       r4, r30
    addi     r3, r31, 0x114
    bl       ShotList_Update
    lwz      r0, 0xd4(r1)
    lwz      r31, 0xcc(r1)
    lwz      r30, 0xc8(r1)
    lwz      r29, 0xc4(r1)
    mtlr     r0
    addi     r1, r1, 0xd0
    blr
*/

/**
 * @note Address: stage02 .text 0x4214
 * @note Size: 0x2DC
 */
extern "C" void Fighter_Init(Fighter* hole, UnkDolObj* a, UnkDolObj* b, void* c)
{
	HSD_AObj* aobj;
	f32 x;
	f32 y;
	f32 z;
	u32 i;

	hole->_00 = a;
	hole->_04 = b;
	hole->_08 = (UnkDolObj*)c;
	hole->_00->_228 |= 0x10000;
	hole->_00->_228 |= 0x40000;
	hole->_04->_228 &= ~0x10000ULL;
	hole->_A0       = 0.0f;
	aobj            = GetAObj(hole->_04->_00);
	hole->_A8       = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	hole->_AC       = 1.0f;
	hole->_B8       = 0;
	hole->_BC       = hole->_F0;
	z               = hole->_B4 == NULL ? 170.0f : (s32)READ_LE32(hole->_B4 + 8) / 100.0f;
	y               = hole->_B4 == NULL ? 20.0f : (s32)READ_LE32(hole->_B4 + 4) / 100.0f;
	x               = hole->_B4 == NULL ? 0.0f : (s32)READ_LE32(hole->_B4) / 100.0f;
	hole->_F4._00.x = x;
	hole->_F4._00.y = y;
	hole->_F4._00.z = z;
	hole->_F4._18   = NULL;
	hole->_110      = NULL;
	for (i = 0; i < 5; i++) {
		hole->_114._00[i]._00        = false;
		hole->_114._00[i]._B4.mFlags = 0;
	}
	hole->_114._3AC = hole->_114._3B0 = 0;
}

// Fighter_Init
/*
    stwu     r1, -0x20(r1)
    mflr     r0
    lis      r7, 0xffff
    lis      r8, 0x1
    stw      r0, 0x24(r1)
    subi     r0, r7, 0x1
    stw      r31, 0x1c(r1)
    mr       r31, r3
    stw      r30, 0x18(r1)
    stw      r4, 0x0(r3)
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
    li       r4, -0x1
    stw      r5, 0x4(r31)
    lis      r5, 0x4
    stw      r6, 0x8(r31)
    lwz      r7, 0x0(r31)
    lwz      r3, 0x22c(r7)
    lwz      r6, 0x228(r7)
    or       r3, r3, r8
    stw      r3, 0x22c(r7)
    lwz      r7, 0x0(r31)
    lwz      r3, 0x22c(r7)
    lwz      r6, 0x228(r7)
    or       r3, r3, r5
    stw      r3, 0x22c(r7)
    lwz      r6, 0x4(r31)
    lwz      r5, 0x22c(r6)
    lwz      r3, 0x228(r6)
    and      r0, r5, r0
    stw      r0, 0x22c(r6)
    and      r0, r3, r4
    stw      r0, 0x228(r6)
    stfs     f0, 0xa0(r31)
    lwz      r3, 0x4(r31)
    lwz      r30, 0x0(r3)
    cmplwi   r30, 0x0
    beq      .L_000042F4
    mr       r3, r30
    bl       FindJointAnimationInTree
    cmplwi   r3, 0x0
    beq      .L_000042C4
    mr       r30, r3
    b        .L_000042F8
.L_000042C4:
    mr       r3, r30
    bl       FindMaterialOrTextureAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_000042DC
    mr       r30, r3
    b        .L_000042F8
.L_000042DC:
    mr       r3, r30
    bl       FindPolygonAnimationInJointTree
    cmplwi   r3, 0x0
    beq      .L_000042F4
    mr       r30, r3
    b        .L_000042F8
.L_000042F4:
    li       r30, 0x0
.L_000042F8:
    cmplwi   r30, 0x0
    beq      .L_00004324
    bne      .L_0000431C
    lis      r3, sStage02PathAobjH@ha
    lis      r5, lbl_5_rodata_188@ha
    addi     r3, r3, sStage02PathAobjH@l
    li       r4, 0xab
    addi     r5, r5, lbl_5_rodata_188@l
    bl       __assert
.L_0000431C:
    lfs      f0, 0xc(r30)
    b        .L_0000432C
.L_00004324:
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
.L_0000432C:
    lis      r3, gFighterHudTextureScale@ha
    stfs     f0, 0xa8(r31)
    lfs      f0, gFighterHudTextureScale@l(r3)
    li       r0, 0x0
    stfs     f0, 0xac(r31)
    stw      r0, 0xb8(r31)
    lwz      r0, 0xf0(r31)
    stw      r0, 0xbc(r31)
    lwz      r3, 0xb4(r31)
    cmplwi   r3, 0x0
    bne      .L_00004364
    lis      r4, lbl_5_rodata_170@ha
    lfs      f3, lbl_5_rodata_170@l(r4)
    b        .L_000043BC
.L_00004364:
    lbz      r6, 0x9(r3)
    lis      r0, 0x4330
    lbz      r9, 0xb(r3)
    lis      r4, lbl_5_rodata_110@ha
    lbz      r5, 0xa(r3)
    slwi     r7, r6, 8
    lbz      r8, 0x8(r3)
    slwi     r9, r9, 24
    slwi     r6, r5, 16
    addi     r5, r4, lbl_5_rodata_110@l
    add      r6, r7, r6
    stw      r0, 0x8(r1)
    add      r6, r6, r9
    lis      r4, lbl_5_rodata_F4@ha
    add      r0, r8, r6
    lfd      f2, 0x0(r5)
    xoris    r0, r0, 0x8000
    lfs      f0, lbl_5_rodata_F4@l(r4)
    stw      r0, 0xc(r1)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f3, f1, f0
.L_000043BC:
    cmplwi   r3, 0x0
    bne      .L_000043D0
    lis      r4, lbl_5_rodata_A8@ha
    lfs      f4, lbl_5_rodata_A8@l(r4)
    b        .L_00004428
.L_000043D0:
    lbz      r6, 0x5(r3)
    lis      r0, 0x4330
    lbz      r9, 0x7(r3)
    lis      r4, lbl_5_rodata_110@ha
    lbz      r5, 0x6(r3)
    slwi     r7, r6, 8
    lbz      r8, 0x4(r3)
    slwi     r9, r9, 24
    slwi     r6, r5, 16
    addi     r5, r4, lbl_5_rodata_110@l
    add      r6, r7, r6
    stw      r0, 0x8(r1)
    add      r6, r6, r9
    lis      r4, lbl_5_rodata_F4@ha
    add      r0, r8, r6
    lfd      f2, 0x0(r5)
    xoris    r0, r0, 0x8000
    lfs      f0, lbl_5_rodata_F4@l(r4)
    stw      r0, 0xc(r1)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f4, f1, f0
.L_00004428:
    cmplwi   r3, 0x0
    bne      .L_0000443C
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f0, lbl_5_rodata_A4@l(r3)
    b        .L_00004490
.L_0000443C:
    lbz      r6, 0x1(r3)
    lis      r0, 0x4330
    lbz      r5, 0x2(r3)
    lis      r4, lbl_5_rodata_110@ha
    lbz      r8, 0x3(r3)
    slwi     r6, r6, 8
    slwi     r5, r5, 16
    lbz      r7, 0x0(r3)
    slwi     r8, r8, 24
    lis      r3, lbl_5_rodata_F4@ha
    add      r5, r6, r5
    stw      r0, 0x8(r1)
    add      r5, r5, r8
    lfd      f2, lbl_5_rodata_110@l(r4)
    add      r0, r7, r5
    lfs      f0, lbl_5_rodata_F4@l(r3)
    xoris    r0, r0, 0x8000
    stw      r0, 0xc(r1)
    lfd      f1, 0x8(r1)
    fsubs    f1, f1, f2
    fdivs    f0, f1, f0
.L_00004490:
    stfs     f0, 0xf4(r31)
    li       r0, 0x0
    stfs     f4, 0xf8(r31)
    stfs     f3, 0xfc(r31)
    stw      r0, 0x10c(r31)
    stw      r0, 0x110(r31)
    stb      r0, 0x114(r31)
    stw      r0, 0x1c8(r31)
    stb      r0, 0x1d0(r31)
    stw      r0, 0x284(r31)
    stb      r0, 0x28c(r31)
    stw      r0, 0x340(r31)
    stb      r0, 0x348(r31)
    stw      r0, 0x3fc(r31)
    stb      r0, 0x404(r31)
    stw      r0, 0x4b8(r31)
    stw      r0, 0x4c4(r31)
    stw      r0, 0x4c0(r31)
    lwz      r31, 0x1c(r1)
    lwz      r30, 0x18(r1)
    lwz      r0, 0x24(r1)
    mtlr     r0
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x44F0
 * @note Size: 0xCB4
 */
// NONMATCHING: retail reads the 1.0f-scale operands and the 32.0f panel offset as named .rodata
// objects (gFighterHudTextureScale, gFighterPanelYOffset), so it keeps a real lfs+fmuls where
// literals fold away. No source form reproduces both the loads and their placement.
extern "C" void FighterPanels_Draw(FighterPanels* fade)
{
	CTexObj tex;
	s32 i;

	if (FindStageIndexByName("stage02") == GAME_FIELD(s32, GAME_STAGE)) {
		for (i = 0; i < 48; i++) {
			UnkDolObj* obj = (UnkDolObj*)(GAME_FIELD(u8*, GAME_OFS(0x96F8, 0x96B0)) + i * GAME_FIELD(s32, GAME_OFS(0x9708, 0x96C0)));
			s32 kind       = obj->_244 != NULL ? *obj->_244 : -1;
			f32 y;
			f32 half;
			f32 w;
			f32 h;
			if (kind != 0x149 || obj->_B6C == 0) {
				continue;
			}
			if (fade->_08 > 0.0f) {
				tex  = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x50), 0);
				w    = 1.1f * (1.0f * GXGetTexObjWidth(&tex));
				y    = 320.0f - 32.0f;
				half = fade->_08 * (1.0f * GXGetTexObjHeight(&tex));
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, y, 0.0f, w, half, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + (fade->_10 + 25) * 8), 0);
				w   = 1.1f * (1.0f * GXGetTexObjWidth(&tex));
				h   = fade->_08 * (1.0f * GXGetTexObjHeight(&tex));
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x58), 0);
				half *= 0.5f;
				w = 1.1f * (1.0f * GXGetTexObjWidth(&tex));
				h = 1.0f * GXGetTexObjHeight(&tex);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, y - half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, y + half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
			}
			if (fade->_0C > 0.0f) {
				u16 texW;
				tex  = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x40), 0);
				texW = GXGetTexObjWidth(&tex);
				half = fade->_0C * (1.0f * GXGetTexObjHeight(&tex));
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 180.0f, 0.0f, 1.1f * (1.0f * texW), half, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				if (fade->_20 == 0) {
					tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + (fade->_14 + 12) * 8), 0);
				} else {
					tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + (fade->_14 + 18) * 8), 0);
				}
				w = 1.1f * (1.0f * GXGetTexObjWidth(&tex));
				h = fade->_0C * (1.0f * GXGetTexObjHeight(&tex));
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 180.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				tex = ((CTexObj*)(obj->_A6C))->Init((TPLDescriptor*)(obj->_A4C[2] + 0x48), 0);
				half *= 0.5f;
				w = 1.1f * (1.0f * GXGetTexObjWidth(&tex));
				h = 1.0f * GXGetTexObjHeight(&tex);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 180.0f - half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
				DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, 100.0f, 180.0f + half, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
				SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
			}
		}
	}
}

// FighterPanels_Draw
/*
    stwu     r1, -0x1a0(r1)
    mflr     r0
    stw      r0, 0x1a4(r1)
    stfd     f31, 0x190(r1)
    psq_st   f31, 0x198(r1), 0, qr0
    stfd     f30, 0x180(r1)
    psq_st   f30, 0x188(r1), 0, qr0
    stfd     f29, 0x170(r1)
    psq_st   f29, 0x178(r1), 0, qr0
    stfd     f28, 0x160(r1)
    psq_st   f28, 0x168(r1), 0, qr0
    stfd     f27, 0x150(r1)
    psq_st   f27, 0x158(r1), 0, qr0
    stmw     r26, 0x138(r1)
    lis      r4, __vt__7CTexObj@ha
    lis      r5, gGame@ha
    addi     r0, r4, __vt__7CTexObj@l
    lis      r4, lbl_5_rodata_8C@ha
    addi     r5, r5, gGame@l
    stw      r0, 0x104(r1)
    addi     r0, r4, lbl_5_rodata_8C@l
    mr       r27, r3
    lwz      r26, 0x3444(r5)
    mr       r3, r0
    bl       FindStageIndexByName
    cmpw     r3, r26
    bne      .L_0000515C
    lis      r3, gGame@ha
    li       r28, 0x0
    addi     r31, r3, gGame@l
    addis    r29, r31, 0x1
.L_0000456C:
    lwz      r0, -0x68f8(r29)
    lwz      r3, -0x6908(r29)
    mullw    r0, r28, r0
    add      r30, r3, r0
    lwz      r3, 0x244(r30)
    cmplwi   r3, 0x0
    beq      .L_00004590
    lwz      r0, 0x0(r3)
    b        .L_00004594
.L_00004590:
    li       r0, -0x1
.L_00004594:
    cmpwi    r0, 0x149
    bne      .L_00005150
    lwz      r0, 0xb6c(r30)
    cmpwi    r0, 0x0
    beq      .L_00005150
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f1, 0x8(r27)
    lfs      f0, lbl_5_rodata_A4@l(r3)
    fcmpo    cr0, f1, f0
    ble      .L_00004B10
    lwz      r4, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x50
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_0000465C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_0000465C
    addi     r3, r1, 0x8
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x10c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x108(r1)
    lis      r3, lbl_5_rodata_190@ha
    addi     r5, r3, lbl_5_rodata_190@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x108(r1)
    addi     r3, r1, 0x8
    fsubs    f0, f0, f1
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    fmuls    f27, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x114(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x110(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    lis      r4, lbl_5_rodata_D8@ha
    lfd      f0, 0x110(r1)
    addi     r5, r4, lbl_5_rodata_D8@l
    lis      r3, gFighterPanelYOffset@ha
    lfs      f3, 0x8(r27)
    fsubs    f0, f0, f1
    lfs      f31, 0x0(r5)
    li       r4, 0x0
    fmuls    f1, f2, f0
    lfs      f0, gFighterPanelYOffset@l(r3)
    lwz      r3, 0x294(r31)
    fsubs    f31, f31, f0
    fmuls    f30, f3, f1
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    addi     r3, r5, gGame@l
    fmr      f2, f31
    addi     r4, r1, 0x8
    fmr      f4, f27
    li       r5, -0x1
    fmr      f5, f30
    li       r6, -0x1
    fmr      f6, f3
    li       r7, -0x1
    fmr      f7, f3
    li       r8, -0x1
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r6, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    lwz      r4, 0x10(r27)
    li       r5, 0x0
    lwz      r6, 0x8(r6)
    addi     r0, r4, 0x19
    slwi     r0, r0, 3
    add      r4, r6, r0
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_00004818:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00004818
    addi     r3, r1, 0x8
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x11c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x118(r1)
    lis      r3, lbl_5_rodata_190@ha
    addi     r5, r3, lbl_5_rodata_190@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x118(r1)
    addi     r3, r1, 0x8
    fsubs    f0, f0, f1
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    fmuls    f29, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x124(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x120(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x120(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    lfs      f1, 0x8(r27)
    fmuls    f0, f2, f0
    fmuls    f27, f1, f0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    addi     r3, r5, gGame@l
    fmr      f2, f31
    addi     r4, r1, 0x8
    fmr      f4, f29
    li       r5, -0x1
    fmr      f5, f27
    li       r6, -0x1
    fmr      f6, f3
    li       r7, -0x1
    fmr      f7, f3
    li       r8, -0x1
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x58
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_000049B0:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_000049B0
    lis      r4, lbl_5_rodata_9C@ha
    addi     r3, r1, 0x8
    lfs      f0, lbl_5_rodata_9C@l(r4)
    fmuls    f30, f30, f0
    fsubs    f27, f31, f30
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x12c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x128(r1)
    lis      r3, lbl_5_rodata_190@ha
    addi     r5, r3, lbl_5_rodata_190@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x128(r1)
    addi     r3, r1, 0x8
    fsubs    f0, f0, f1
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    fmuls    f29, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x134(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x130(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x130(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    fmuls    f28, f2, f0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    addi     r3, r5, gGame@l
    fmr      f2, f27
    addi     r4, r1, 0x8
    fmr      f4, f29
    li       r5, -0x1
    fmr      f5, f28
    li       r6, -0x1
    fmr      f6, f3
    li       r7, -0x1
    fmr      f7, f3
    li       r8, -0x1
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    addi     r3, r5, gGame@l
    fmr      f4, f29
    addi     r4, r1, 0x8
    fmr      f5, f28
    li       r5, -0x1
    fmr      f6, f3
    li       r6, -0x1
    fmr      f7, f3
    li       r7, -0x1
    fmr      f8, f3
    li       r8, -0x1
    fadds    f2, f31, f30
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
.L_00004B10:
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f1, 0xc(r27)
    lfs      f0, lbl_5_rodata_A4@l(r3)
    fcmpo    cr0, f1, f0
    ble      .L_00005150
    lwz      r4, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x40
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_00004BC4:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00004BC4
    addi     r3, r1, 0x8
    bl       GXGetTexObjWidth
    mr       r26, r3
    addi     r3, r1, 0x8
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x134(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x130(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x130(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    lfs      f1, 0xc(r27)
    fmuls    f0, f2, f0
    fmuls    f31, f1, f0
    bl       SetGsTevMode
    clrlwi   r4, r26, 16
    lis      r0, 0x4330
    stw      r4, 0x12c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x128(r1)
    lis      r5, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r5)
    lis      r3, lbl_5_rodata_190@ha
    lfd      f0, 0x128(r1)
    fmr      f5, f31
    lfs      f2, gFighterHudTextureScale@l(r4)
    lis      r4, lbl_5_rodata_F4@ha
    fsubs    f0, f0, f1
    lfs      f4, lbl_5_rodata_190@l(r3)
    addi     r5, r4, lbl_5_rodata_F4@l
    lis      r3, lbl_5_rodata_194@ha
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    addi     r4, r3, lbl_5_rodata_194@l
    fmr      f6, f3
    lis      r3, gGame@ha
    lfs      f2, 0x0(r4)
    fmr      f7, f3
    fmr      f8, f3
    addi     r3, r3, gGame@l
    fmuls    f4, f4, f0
    addi     r4, r1, 0x8
    li       r5, -0x1
    li       r6, -0x1
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r0, 0x20(r27)
    cmpwi    r0, 0x0
    bne      .L_00004D94
    lwz      r6, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    lwz      r4, 0x14(r27)
    li       r5, 0x0
    lwz      r6, 0x8(r6)
    addi     r0, r4, 0xc
    slwi     r0, r0, 3
    add      r4, r6, r0
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_00004D7C:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00004D7C
    b        .L_00004E54
.L_00004D94:
    lwz      r6, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    lwz      r4, 0x14(r27)
    li       r5, 0x0
    lwz      r6, 0x8(r6)
    addi     r0, r4, 0x12
    slwi     r0, r0, 3
    add      r4, r6, r0
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_00004E40:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00004E40
.L_00004E54:
    addi     r3, r1, 0x8
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x134(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x130(r1)
    lis      r3, lbl_5_rodata_190@ha
    addi     r5, r3, lbl_5_rodata_190@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x130(r1)
    addi     r3, r1, 0x8
    fsubs    f0, f0, f1
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    fmuls    f30, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x12c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x128(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x128(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    lfs      f1, 0xc(r27)
    fmuls    f0, f2, f0
    fmuls    f29, f1, f0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r5, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r4, lbl_5_rodata_194@ha
    lis      r3, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r5)
    fmr      f4, f30
    lfs      f2, lbl_5_rodata_194@l(r4)
    fmr      f5, f29
    addi     r3, r3, gGame@l
    fmr      f6, f3
    addi     r4, r1, 0x8
    fmr      f7, f3
    li       r5, -0x1
    fmr      f8, f3
    li       r6, -0x1
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r4, 0xa4c(r30)
    addi     r3, r30, 0xa6c
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x48
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x40
    addi     r4, r3, 0x38
    stw      r7, 0x8(r1)
    stw      r6, 0xc(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x10(r1)
    stw      r6, 0x14(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x18(r1)
    stw      r6, 0x1c(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x20(r1)
    stw      r6, 0x24(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x28(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x2c(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x30(r1)
    stw      r6, 0x34(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x38(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x3c(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x40(r1)
    mtctr    r0
.L_00004FDC:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00004FDC
    lis      r3, lbl_5_rodata_9C@ha
    lis      r5, lbl_5_rodata_194@ha
    addi     r4, r3, lbl_5_rodata_9C@l
    lfs      f1, lbl_5_rodata_194@l(r5)
    lfs      f0, 0x0(r4)
    addi     r3, r1, 0x8
    fmuls    f30, f31, f0
    fsubs    f28, f1, f30
    bl       GXGetTexObjWidth
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x124(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x120(r1)
    lis      r3, lbl_5_rodata_190@ha
    addi     r5, r3, lbl_5_rodata_190@l
    lfs      f2, gFighterHudTextureScale@l(r4)
    lfd      f0, 0x120(r1)
    addi     r3, r1, 0x8
    fsubs    f0, f0, f1
    lfs      f1, 0x0(r5)
    fmuls    f0, f2, f0
    fmuls    f31, f1, f0
    bl       GXGetTexObjHeight
    clrlwi   r4, r3, 16
    lis      r0, 0x4330
    stw      r4, 0x11c(r1)
    lis      r3, lbl_5_rodata_108@ha
    lfd      f1, lbl_5_rodata_108@l(r3)
    lis      r4, gFighterHudTextureScale@ha
    stw      r0, 0x118(r1)
    addi     r5, r4, gFighterHudTextureScale@l
    lfs      f2, 0x0(r5)
    li       r4, 0x0
    lfd      f0, 0x118(r1)
    lwz      r3, 0x294(r31)
    fsubs    f0, f0, f1
    fmuls    f29, f2, f0
    bl       SetGsTevMode
    lis      r3, lbl_5_rodata_A4@ha
    lis      r4, lbl_5_rodata_F4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, gGame@ha
    lfs      f1, lbl_5_rodata_F4@l(r4)
    addi     r3, r5, gGame@l
    fmr      f2, f28
    addi     r4, r1, 0x8
    fmr      f4, f31
    li       r5, -0x1
    fmr      f5, f29
    li       r6, -0x1
    fmr      f6, f3
    li       r7, -0x1
    fmr      f7, f3
    li       r8, -0x1
    fmr      f8, f3
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
    lwz      r3, 0x294(r31)
    li       r4, 0x0
    bl       SetGsTevMode
    lis      r4, lbl_5_rodata_194@ha
    lis      r3, lbl_5_rodata_A4@ha
    lfs      f3, lbl_5_rodata_A4@l(r3)
    lis      r5, lbl_5_rodata_F4@ha
    lfs      f0, lbl_5_rodata_194@l(r4)
    lis      r3, gGame@ha
    fmr      f4, f31
    lfs      f1, lbl_5_rodata_F4@l(r5)
    fmr      f5, f29
    addi     r3, r3, gGame@l
    fmr      f6, f3
    addi     r4, r1, 0x8
    fmr      f7, f3
    li       r5, -0x1
    fmr      f8, f3
    li       r6, -0x1
    fadds    f2, f0, f30
    li       r7, -0x1
    li       r8, -0x1
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r31)
    li       r4, 0x4
    bl       SetGsTevMode
.L_00005150:
    addi     r28, r28, 0x1
    cmpwi    r28, 0x30
    blt      .L_0000456C
.L_0000515C:
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x104(r1)
    psq_l    f31, 0x198(r1), 0, qr0
    lfd      f31, 0x190(r1)
    psq_l    f30, 0x188(r1), 0, qr0
    lfd      f30, 0x180(r1)
    psq_l    f29, 0x178(r1), 0, qr0
    lfd      f29, 0x170(r1)
    psq_l    f28, 0x168(r1), 0, qr0
    lfd      f28, 0x160(r1)
    psq_l    f27, 0x158(r1), 0, qr0
    lfd      f27, 0x150(r1)
    lmw      r26, 0x138(r1)
    lwz      r0, 0x1a4(r1)
    mtlr     r0
    addi     r1, r1, 0x1a0
    blr
*/

/**
 * @note Address: stage02 .text 0x51A4
 * @note Size: 0x240
 */
extern "C" void FighterPanels_Update(FighterPanels* fade, u32 dt)
{
	switch (fade->_00) {
	case 0:
		break;
	case 1:
		fade->_08 += 0.06f * dt;
		if (fade->_08 > 1.0f) {
			fade->_08 = 1.0f;
			fade->_00 = 0;
		}
		break;
	case 2:
		fade->_08 -= 0.06f * dt;
		if (fade->_08 < 0.0f) {
			fade->_08 = 0.0f;
			fade->_00 = 0;
		}
		break;
	}
	fade->_18 -= dt;
	if (--fade->_18 < 0) {
		fade->_10++;
		if (fade->_10 > 8) {
			fade->_10 = 0;
		}
		fade->_18 = 6;
	}
	switch (fade->_04) {
	case 0:
		break;
	case 1:
		fade->_0C += 0.06f * dt;
		if (fade->_0C > 1.0f) {
			fade->_0C = 1.0f;
			fade->_04 = 0;
		}
		break;
	case 2:
		fade->_0C -= 0.06f * dt;
		if (fade->_0C < 0.0f) {
			fade->_0C = 0.0f;
			fade->_04 = 0;
		}
		break;
	}
	fade->_1C -= dt;
	if (--fade->_1C < 0) {
		fade->_14++;
		if (fade->_14 >= (fade->_20 == 0 ? 6 : 7)) {
			fade->_14 = 0;
		}
		fade->_1C = 6;
	}
}

/**
 * @note Address: stage02 .text 0x53E4
 * @note Size: 0x664
 */
extern "C" void FighterAim_Draw(FighterAim* aim, UnkGame_E5D38* cam, Mtx m)
{
	Vec pos;
	Vec ring[65];
	f32 cx;
	f32 cy;
	s32 k;
	s32 i;
	s32 j;

	if (IsGameSoundPlaying(gGame + GAME_OFS(0xE7E24, 0xE713C), 0x20006) == 0 && aim->_18 == NULL) {
		return;
	}
	PSMTXMultVec(m, &aim->_00, &pos);
	GXProject(pos.x, pos.y, pos.z, cam->_E8, cam->_15C, cam->_178, &aim->_0C, &aim->_10, &aim->_14);
	if (aim->_18 == NULL) {
		cx = aim->_0C;
		cy = aim->_10;
	} else {
		cx = aim->_18->_08;
		cy = aim->_18->_0C;
	}
	SetGsBlendMode(GAME_FIELD(void*, 0x290), 1, 4, 5, 15);
	SetGsLineWidth(GAME_FIELD(void*, 0x290), 12, 5);
	for (i = 9; i > 0; i--) {
		reticleSpin[i] = reticleSpin[i - 1];
	}
	reticleSpin[0] = 0.0f;
	for (k = 0; k < 2; k++) {
		f32 radius = k == 0 ? 10.0f : 30.000002f;
		f32 inner;
		s32 color;
		for (i = 0; i < 65; i++) {
			f32 angle = 0.09817477f * i;
			f32 s     = sin(angle);
			f32 c     = cos(angle);
			ring[i].x = s * radius + cx;
			ring[i].y = c * radius + cy;
			ring[i].z = 0.0f;
		}
		DrawGsLineStrip(gGame, ring, -1, 65);
		inner = radius - 10.0f;
		color = -1;
		for (j = 0; j < 10; j++) {
			for (i = 0; i < 5; i++) {
				f32 angle = HALF_PI * i;
				f32 s     = sin(angle + reticleSpin[j]);
				f32 c     = cos(angle + reticleSpin[j]);
				DrawGsLineSegment(gGame, 0, color, color, s * radius + cx, c * radius + cy, 0.0f, s * inner + cx, c * inner + cy, 0.0f);
			}
			color = 0xFFFFFF00 | (u8)(s32)(0.5f * (255.0f * (10 - j) / 10.0f));
		}
	}
	{
		CTexObj tex;
		static s32 frame = 0;
		frame++;
		for (i = 0; i < 4; i++) {
			f32 angle = HALF_PI * i;
			f32 ds    = sin(DEG2RAD * ((frame * 24) % 360));
			f32 dist  = ds * 3.0f;
			dist += 30.000002f;
			f32 s = sin(angle);
			f32 c = cos(angle);
			u16 w;
			u16 h;
			SetGsTevMode(GAME_FIELD(void*, 0x294), 0);
			tex = ((CTexObj*)(gGame + GAME_OFS(0x3D6C, 0x3D4C)))
			          ->Init((TPLDescriptor*)(GAME_FIELD(u32*, GAME_OFS(0x3D4C, 0x3D2C))[2] + 0x400), 0);
			w = GXGetTexObjWidth(&tex);
			h = GXGetTexObjHeight(&tex);
			DrawGsTexturedQuad(gGame, &tex, -1, -1, -1, -1, dist * s + cx, dist * c + cy, 0.0f, 1.1f * w, h, 0.0f, 0.0f, angle);
			SetGsTevMode(GAME_FIELD(void*, 0x294), 4);
		}
	}
}

// FighterAim_Draw
/*
    stwu     r1, -0x550(r1)
    mflr     r0
    stw      r0, 0x554(r1)
    stfd     f31, 0x540(r1)
    psq_st   f31, 0x548(r1), 0, qr0
    stfd     f30, 0x530(r1)
    psq_st   f30, 0x538(r1), 0, qr0
    stfd     f29, 0x520(r1)
    psq_st   f29, 0x528(r1), 0, qr0
    stfd     f28, 0x510(r1)
    psq_st   f28, 0x518(r1), 0, qr0
    stfd     f27, 0x500(r1)
    psq_st   f27, 0x508(r1), 0, qr0
    stfd     f26, 0x4f0(r1)
    psq_st   f26, 0x4f8(r1), 0, qr0
    stfd     f25, 0x4e0(r1)
    psq_st   f25, 0x4e8(r1), 0, qr0
    stfd     f24, 0x4d0(r1)
    psq_st   f24, 0x4d8(r1), 0, qr0
    stfd     f23, 0x4c0(r1)
    psq_st   f23, 0x4c8(r1), 0, qr0
    stfd     f22, 0x4b0(r1)
    psq_st   f22, 0x4b8(r1), 0, qr0
    stfd     f21, 0x4a0(r1)
    psq_st   f21, 0x4a8(r1), 0, qr0
    stfd     f20, 0x490(r1)
    psq_st   f20, 0x498(r1), 0, qr0
    stfd     f19, 0x480(r1)
    psq_st   f19, 0x488(r1), 0, qr0
    stfd     f18, 0x470(r1)
    psq_st   f18, 0x478(r1), 0, qr0
    stmw     r23, 0x44c(r1)
    lis      r6, gGame@ha
    mr       r25, r3
    addi     r3, r6, gGame@l
    lis      r6, 0x2
    addis    r3, r3, 0xe
    mr       r24, r4
    mr       r23, r5
    addi     r4, r6, 0x6
    addi     r3, r3, 0x7e24
    bl       IsGameSoundPlaying
    cmpwi    r3, 0x0
    bne      .L_000054A0
    lwz      r0, 0x18(r25)
    cmplwi   r0, 0x0
    beq      .L_000059C4
.L_000054A0:
    mr       r3, r23
    mr       r4, r25
    addi     r5, r1, 0x8
    bl       PSMTXMultVec
    lfs      f1, 0x8(r1)
    addi     r3, r24, 0xe8
    lfs      f2, 0xc(r1)
    addi     r4, r24, 0x15c
    lfs      f3, 0x10(r1)
    addi     r5, r24, 0x178
    addi     r6, r25, 0xc
    addi     r7, r25, 0x10
    addi     r8, r25, 0x14
    bl       GXProject
    lwz      r3, 0x18(r25)
    cmplwi   r3, 0x0
    bne      .L_000054F0
    lfs      f22, 0xc(r25)
    lfs      f21, 0x10(r25)
    b        .L_000054F8
.L_000054F0:
    lfs      f22, 0x8(r3)
    lfs      f21, 0xc(r3)
.L_000054F8:
    lis      r3, gGame@ha
    li       r4, 0x1
    addi     r24, r3, gGame@l
    li       r5, 0x4
    lwz      r3, 0x290(r24)
    li       r6, 0x5
    li       r7, 0xf
    bl       SetGsBlendMode
    lwz      r3, 0x290(r24)
    li       r4, 0xc
    li       r5, 0x5
    bl       SetGsLineWidth
    lis      r4, reticleSpin@ha
    lis      r3, lbl_5_rodata_A4@ha
    addi     r29, r4, reticleSpin@l
    lis      r7, lbl_5_rodata_1A4@ha
    lfs      f0, 0x20(r29)
    addi     r28, r3, lbl_5_rodata_A4@l
    lis      r5, lbl_5_rodata_CC@ha
    lis      r4, lbl_5_rodata_9C@ha
    stfs     f0, 0x24(r29)
    lis      r3, gFighterHudFullAlpha@ha
    lis      r9, lbl_5_rodata_1A0@ha
    addi     r8, r7, lbl_5_rodata_1A4@l
    lfs      f0, 0x1c(r29)
    lis      r6, lbl_5_rodata_110@ha
    addi     r7, r6, lbl_5_rodata_110@l
    lfs      f23, 0x0(r28)
    stfs     f0, 0x20(r29)
    lis      r6, gGame@ha
    lfs      f25, 0x0(r8)
    addi     r25, r6, gGame@l
    lfs      f0, 0x18(r29)
    li       r27, 0x0
    lfd      f26, 0x0(r7)
    lis      r24, 0x4330
    stfs     f0, 0x1c(r29)
    lfs      f27, lbl_5_rodata_CC@l(r5)
    lfs      f0, 0x14(r29)
    lfs      f30, lbl_5_rodata_9C@l(r4)
    stfs     f0, 0x18(r29)
    lfs      f31, gFighterHudFullAlpha@l(r3)
    lfs      f0, 0x10(r29)
    lfs      f24, lbl_5_rodata_1A0@l(r9)
    stfs     f0, 0x14(r29)
    lfs      f0, 0xc(r29)
    stfs     f0, 0x10(r29)
    lfs      f0, 0x8(r29)
    stfs     f0, 0xc(r29)
    lfs      f0, 0x4(r29)
    stfs     f0, 0x8(r29)
    lfs      f0, 0x0(r29)
    stfs     f0, 0x4(r29)
    stfs     f23, 0x0(r29)
.L_000055D0:
    cmpwi    r27, 0x0
    bne      .L_000055E4
    lis      r3, lbl_5_rodata_CC@ha
    lfs      f20, lbl_5_rodata_CC@l(r3)
    b        .L_000055EC
.L_000055E4:
    lis      r3, lbl_5_rodata_19C@ha
    lfs      f20, lbl_5_rodata_19C@l(r3)
.L_000055EC:
    li       r30, 0x0
    addi     r26, r1, 0x114
.L_000055F4:
    xoris    r0, r30, 0x8000
    stw      r24, 0x420(r1)
    stw      r0, 0x424(r1)
    lfd      f0, 0x420(r1)
    fsubs    f0, f0, f26
    fmuls    f19, f24, f0
    fmr      f1, f19
    bl       sin
    frsp     f28, f1
    fmr      f1, f19
    bl       cos
    frsp     f0, f1
    addi     r30, r30, 0x1
    fmadds   f1, f28, f20, f22
    cmpwi    r30, 0x41
    fmadds   f0, f0, f20, f21
    stfs     f1, 0x0(r26)
    stfs     f0, 0x4(r26)
    stfs     f23, 0x8(r26)
    addi     r26, r26, 0xc
    blt      .L_000055F4
    mr       r3, r25
    addi     r4, r1, 0x114
    li       r5, -0x1
    li       r6, 0x41
    bl       DrawGsLineStrip
    fsubs    f28, f20, f27
    mr       r26, r29
    li       r30, -0x1
    li       r31, 0x0
    fmr      f29, f28
.L_00005670:
    li       r23, 0x0
.L_00005674:
    xoris    r0, r23, 0x8000
    stw      r24, 0x420(r1)
    lfs      f0, 0x0(r26)
    stw      r0, 0x424(r1)
    lfd      f1, 0x420(r1)
    fsubs    f1, f1, f26
    fmuls    f18, f25, f1
    fadds    f1, f18, f0
    bl       sin
    lfs      f0, 0x0(r26)
    frsp     f19, f1
    fadds    f1, f18, f0
    bl       cos
    frsp     f0, f1
    lfs      f3, 0x0(r28)
    fmadds   f1, f19, f20, f22
    mr       r3, r25
    fmr      f6, f3
    mr       r5, r30
    fmadds   f2, f0, f20, f21
    mr       r6, r30
    fmadds   f4, f19, f28, f22
    li       r4, 0x0
    fmadds   f5, f0, f29, f21
    bl       DrawGsLineSegment
    addi     r23, r23, 0x1
    cmpwi    r23, 0x5
    blt      .L_00005674
    subfic   r0, r31, 0xa
    addi     r31, r31, 0x1
    xoris    r0, r0, 0x8000
    stw      r24, 0x420(r1)
    cmpwi    r31, 0xa
    li       r30, -0x100
    stw      r0, 0x424(r1)
    addi     r26, r26, 0x4
    lfd      f0, 0x420(r1)
    fsubs    f0, f0, f26
    fmuls    f0, f31, f0
    fdivs    f0, f0, f27
    fmuls    f0, f30, f0
    fctiwz   f0, f0
    stfd     f0, 0x428(r1)
    lwz      r0, 0x42c(r1)
    rlwimi   r30, r0, 0, 24, 31
    blt      .L_00005670
    addi     r27, r27, 0x1
    cmpwi    r27, 0x2
    blt      .L_000055D0
    lis      r3, sReticleFrameInitialized@ha
    lis      r4, __vt__7CTexObj@ha
    addi     r5, r3, sReticleFrameInitialized@l
    lbz      r0, 0x0(r5)
    addi     r3, r4, __vt__7CTexObj@l
    stw      r3, 0x110(r1)
    extsb.   r0, r0
    bne      .L_0000576C
    li       r0, 0x1
    lis      r3, sReticleFrame@ha
    li       r4, 0x0
    stb      r0, 0x0(r5)
    stw      r4, sReticleFrame@l(r3)
.L_0000576C:
    lis      r3, sReticleFrame@ha
    lis      r9, lbl_5_rodata_1A4@ha
    addi     r28, r3, sReticleFrame@l
    lis      r4, lbl_5_rodata_190@ha
    lwz      r10, 0x0(r28)
    lis      r3, lbl_5_rodata_108@ha
    lis      r8, lbl_5_rodata_110@ha
    lis      r7, lbl_5_rodata_1A8@ha
    addi     r0, r10, 0x1
    addi     r11, r9, lbl_5_rodata_1A4@l
    addi     r10, r8, lbl_5_rodata_110@l
    addi     r9, r7, lbl_5_rodata_1A8@l
    lis      r6, lbl_5_rodata_19C@ha
    lis      r8, 0xb60b
    addi     r7, r6, lbl_5_rodata_19C@l
    lis      r5, lbl_5_rodata_1AC@ha
    addi     r6, r5, lbl_5_rodata_1AC@l
    stw      r0, 0x0(r28)
    lis      r5, gGame@ha
    lfs      f28, 0x0(r11)
    addi     r29, r5, gGame@l
    lfd      f27, 0x0(r10)
    lis      r5, lbl_5_rodata_A4@ha
    lfs      f26, 0x0(r9)
    lfs      f25, 0x0(r7)
    addi     r27, r29, 0x3d6c
    lfs      f24, 0x0(r6)
    addi     r31, r8, 0x60b7
    lfs      f23, lbl_5_rodata_190@l(r4)
    addi     r25, r5, lbl_5_rodata_A4@l
    lfd      f20, lbl_5_rodata_108@l(r3)
    li       r26, 0x0
    lis      r30, 0x4330
.L_000057F0:
    lwz      r0, 0x0(r28)
    xoris    r3, r26, 0x8000
    stw      r3, 0x42c(r1)
    mulli    r4, r0, 0x18
    stw      r30, 0x428(r1)
    mulhw    r0, r31, r4
    lfd      f0, 0x428(r1)
    stw      r30, 0x420(r1)
    fsubs    f0, f0, f27
    add      r0, r0, r4
    fmuls    f18, f28, f0
    srawi    r0, r0, 8
    srwi     r3, r0, 31
    add      r0, r0, r3
    mulli    r0, r0, 0x168
    subf     r0, r0, r4
    xoris    r0, r0, 0x8000
    stw      r0, 0x424(r1)
    lfd      f0, 0x420(r1)
    fsubs    f0, f0, f27
    fmuls    f1, f26, f0
    bl       sin
    frsp     f0, f1
    fmr      f1, f18
    fmuls    f0, f24, f0
    fadds    f30, f25, f0
    bl       sin
    frsp     f19, f1
    fmr      f1, f18
    bl       cos
    frsp     f29, f1
    lwz      r3, 0x294(r29)
    li       r4, 0x0
    bl       SetGsTevMode
    lwz      r4, 0x3d4c(r29)
    mr       r3, r27
    li       r5, 0x0
    lwz      r4, 0x8(r4)
    addi     r4, r4, 0x400
    bl       fn_801A0554
    lwz      r7, 0x0(r3)
    li       r0, 0x18
    lwz      r6, 0x4(r3)
    addi     r5, r1, 0x4c
    addi     r4, r3, 0x38
    stw      r7, 0x14(r1)
    stw      r6, 0x18(r1)
    lwz      r7, 0x8(r3)
    lwz      r6, 0xc(r3)
    stw      r7, 0x1c(r1)
    stw      r6, 0x20(r1)
    lwz      r7, 0x10(r3)
    lwz      r6, 0x14(r3)
    stw      r7, 0x24(r1)
    stw      r6, 0x28(r1)
    lwz      r7, 0x18(r3)
    lwz      r6, 0x1c(r3)
    stw      r7, 0x2c(r1)
    stw      r6, 0x30(r1)
    lwz      r6, 0x20(r3)
    stw      r6, 0x34(r1)
    lwz      r6, 0x24(r3)
    stw      r6, 0x38(r1)
    lwz      r7, 0x28(r3)
    lwz      r6, 0x2c(r3)
    stw      r7, 0x3c(r1)
    stw      r6, 0x40(r1)
    lwz      r6, 0x30(r3)
    stw      r6, 0x44(r1)
    lwz      r6, 0x34(r3)
    stw      r6, 0x48(r1)
    lwz      r3, 0x38(r3)
    stw      r3, 0x4c(r1)
    mtctr    r0
.L_00005918:
    lwz      r3, 0x4(r4)
    lwzu     r0, 0x8(r4)
    stw      r3, 0x4(r5)
    stwu     r0, 0x8(r5)
    bdnz     .L_00005918
    addi     r3, r1, 0x14
    bl       GXGetTexObjWidth
    mr       r24, r3
    addi     r3, r1, 0x14
    bl       GXGetTexObjHeight
    clrlwi   r0, r24, 16
    lfs      f3, 0x0(r25)
    stw      r0, 0x434(r1)
    clrlwi   r0, r3, 16
    fmr      f8, f18
    mr       r3, r29
    stw      r30, 0x430(r1)
    fmadds   f1, f30, f19, f22
    fmr      f6, f3
    addi     r4, r1, 0x14
    lfd      f0, 0x430(r1)
    fmr      f7, f3
    stw      r0, 0x43c(r1)
    fmadds   f2, f30, f29, f21
    fsubs    f4, f0, f20
    li       r5, -0x1
    stw      r30, 0x438(r1)
    li       r6, -0x1
    li       r7, -0x1
    lfd      f0, 0x438(r1)
    fmuls    f4, f23, f4
    li       r8, -0x1
    fsubs    f5, f0, f20
    bl       DrawGsTexturedQuad
    lwz      r3, 0x294(r29)
    li       r4, 0x4
    bl       SetGsTevMode
    addi     r26, r26, 0x1
    cmpwi    r26, 0x4
    blt      .L_000057F0
    lis      r3, __vt__7CTexObj@ha
    addi     r0, r3, __vt__7CTexObj@l
    stw      r0, 0x110(r1)
.L_000059C4:
    psq_l    f31, 0x548(r1), 0, qr0
    lfd      f31, 0x540(r1)
    psq_l    f30, 0x538(r1), 0, qr0
    lfd      f30, 0x530(r1)
    psq_l    f29, 0x528(r1), 0, qr0
    lfd      f29, 0x520(r1)
    psq_l    f28, 0x518(r1), 0, qr0
    lfd      f28, 0x510(r1)
    psq_l    f27, 0x508(r1), 0, qr0
    lfd      f27, 0x500(r1)
    psq_l    f26, 0x4f8(r1), 0, qr0
    lfd      f26, 0x4f0(r1)
    psq_l    f25, 0x4e8(r1), 0, qr0
    lfd      f25, 0x4e0(r1)
    psq_l    f24, 0x4d8(r1), 0, qr0
    lfd      f24, 0x4d0(r1)
    psq_l    f23, 0x4c8(r1), 0, qr0
    lfd      f23, 0x4c0(r1)
    psq_l    f22, 0x4b8(r1), 0, qr0
    lfd      f22, 0x4b0(r1)
    psq_l    f21, 0x4a8(r1), 0, qr0
    lfd      f21, 0x4a0(r1)
    psq_l    f20, 0x498(r1), 0, qr0
    lfd      f20, 0x490(r1)
    psq_l    f19, 0x488(r1), 0, qr0
    lfd      f19, 0x480(r1)
    psq_l    f18, 0x478(r1), 0, qr0
    lfd      f18, 0x470(r1)
    lmw      r23, 0x44c(r1)
    lwz      r0, 0x554(r1)
    mtlr     r0
    addi     r1, r1, 0x550
    blr
*/

// after FighterAim_Draw so its static frame counter lands between reticleSpin and these in .bss
fighter::CCameraActionFighter fighterCamera;
MatoList matoList;
Fighter fighterWork;
PilotNwing pilotNwing;
FighterPanels panels;
s32 fireHoldFrames;
u8* soundCueTable[2];

/**
 * @note Address: stage02 .text 0x5A48
 * @note Size: 0x2A0
 */
extern "C" void ShotList_Update(ShotList* list, u32 dt)
{
	u32 i;
	s32 j;

	for (i = 0; i < 5; i++) {
		Shot* entry = &list->_00[i];
		if (!entry->_00) {
			continue;
		}
		if (entry->_00) {
			entry->_34 -= dt;
			if (entry->_34 <= 0) {
				Mato* item;
				entry->_00 = false;
				item       = entry->_B0;
				if (item != NULL) {
					if (--item->_28 > 0) {
						item->_19 = false;
					} else if (item->_04 != NULL && (item->_04->_228 & 0x10000)) {
						item->_30->_28 = item;
						item->_30->_10 += item->_30->_20[item->_2C];
						shotEvents.mFlags |= 1;
					}
					entry->_B4.mFlags |= 1;
				} else {
					entry->_B4.mFlags |= 2;
				}
			} else {
				entry->_1C.x = entry->_28.x * dt + entry->_1C.x;
				entry->_1C.y = entry->_28.y * dt + entry->_1C.y;
				entry->_1C.z = entry->_28.z * dt + entry->_1C.z;
				for (j = 9; j > 0; j--) {
					entry->_38[j] = entry->_38[j - 1];
				}
				entry->_38[0] = entry->_1C;
			}
		}
		if (entry->_B4.mFlags & 1) {
			list->_3AC++;
			list->_3B0 = 0;
			if (list->_3AC >= 5) {
				shotEvents.mFlags |= 4;
				list->_3AC = 0;
			}
		} else if (entry->_B4.mFlags & 2) {
			list->_3AC = 0;
			list->_3B0++;
			if (list->_3B0 >= 5) {
				shotEvents.mFlags |= 8;
				list->_3B0 = 0;
			}
		}
		entry->_B4.mFlags = 0;
	}
}

// ShotList_Update
/*
    stwu     r1, -0x20(r1)
    li       r6, 0x0
    mr       r5, r3
.L_00005A54:
    lbz      r0, 0x0(r5)
    cmplwi   r0, 0x0
    beq      .L_00005CD0
    beq      .L_00005C48
    lwz      r0, 0x34(r5)
    subf     r0, r4, r0
    stw      r0, 0x34(r5)
    lwz      r0, 0x34(r5)
    cmpwi    r0, 0x0
    bgt      .L_00005B30
    li       r9, 0x0
    stb      r9, 0x0(r5)
    lwz      r10, 0xb0(r5)
    cmplwi   r10, 0x0
    beq      .L_00005B20
    lwz      r7, 0x28(r10)
    subic.   r0, r7, 0x1
    stw      r0, 0x28(r10)
    ble      .L_00005AA8
    stb      r9, 0x19(r10)
    b        .L_00005B10
.L_00005AA8:
    lwz      r8, 0x4(r10)
    cmplwi   r8, 0x0
    beq      .L_00005B10
    lwz      r0, 0x228(r8)
    lis      r7, 0x1
    lwz      r8, 0x22c(r8)
    and      r0, r0, r9
    and      r7, r8, r7
    xor      r7, r7, r9
    xor      r0, r0, r9
    or.      r0, r7, r0
    beq      .L_00005B10
    lwz      r8, 0x30(r10)
    lis      r7, shotEvents@ha
    stw      r10, 0x28(r8)
    lwz      r0, 0x2c(r10)
    lwz      r9, 0x30(r10)
    slwi     r8, r0, 2
    addi     r8, r8, 0x20
    lwz      r0, 0x10(r9)
    lwzx     r8, r9, r8
    add      r0, r0, r8
    stw      r0, 0x10(r9)
    lwzu     r0, shotEvents@l(r7)
    ori      r0, r0, 0x1
    stw      r0, 0x0(r7)
.L_00005B10:
    lwz      r0, 0xb4(r5)
    ori      r0, r0, 0x1
    stw      r0, 0xb4(r5)
    b        .L_00005C48
.L_00005B20:
    lwz      r0, 0xb4(r5)
    ori      r0, r0, 0x2
    stw      r0, 0xb4(r5)
    b        .L_00005C48
.L_00005B30:
    lis      r7, 0x4330
    lis      r8, lbl_5_rodata_108@ha
    stw      r4, 0xc(r1)
    li       r0, 0x3
    lfd      f4, lbl_5_rodata_108@l(r8)
    addi     r10, r5, 0x6c
    stw      r7, 0x8(r1)
    li       r8, 0x9
    lfs      f2, 0x28(r5)
    lfd      f1, 0x8(r1)
    lfs      f0, 0x1c(r5)
    fsubs    f1, f1, f4
    stw      r4, 0x14(r1)
    stw      r7, 0x10(r1)
    fmadds   f1, f2, f1, f0
    lfd      f0, 0x10(r1)
    stw      r4, 0x1c(r1)
    fsubs    f2, f0, f4
    stfs     f1, 0x1c(r5)
    stw      r7, 0x18(r1)
    lfs      f3, 0x2c(r5)
    lfs      f1, 0x20(r5)
    lfd      f0, 0x18(r1)
    fmadds   f2, f3, f2, f1
    fsubs    f1, f0, f4
    stfs     f2, 0x20(r5)
    lfs      f2, 0x30(r5)
    lfs      f0, 0x24(r5)
    fmadds   f0, f2, f1, f0
    stfs     f0, 0x24(r5)
    mtctr    r0
.L_00005BAC:
    subi     r0, r8, 0x1
    mulli    r7, r0, 0xc
    subi     r0, r8, 0x2
    addi     r9, r7, 0x38
    add      r9, r5, r9
    lfs      f0, 0x0(r9)
    mulli    r7, r0, 0xc
    subi     r0, r8, 0x3
    stfs     f0, 0x38(r10)
    subi     r8, r8, 0x3
    lfs      f0, 0x4(r9)
    stfs     f0, 0x3c(r10)
    lfs      f0, 0x8(r9)
    addi     r9, r7, 0x38
    add      r9, r5, r9
    stfs     f0, 0x40(r10)
    mulli    r7, r0, 0xc
    lfs      f0, 0x0(r9)
    stfs     f0, 0x2c(r10)
    lfs      f0, 0x4(r9)
    stfs     f0, 0x30(r10)
    lfs      f0, 0x8(r9)
    addi     r9, r7, 0x38
    add      r9, r5, r9
    stfs     f0, 0x34(r10)
    lfs      f0, 0x0(r9)
    stfs     f0, 0x20(r10)
    lfs      f0, 0x4(r9)
    stfs     f0, 0x24(r10)
    lfs      f0, 0x8(r9)
    stfs     f0, 0x28(r10)
    subi     r10, r10, 0x24
    bdnz     .L_00005BAC
    lfs      f0, 0x1c(r5)
    stfs     f0, 0x38(r5)
    lfs      f0, 0x20(r5)
    stfs     f0, 0x3c(r5)
    lfs      f0, 0x24(r5)
    stfs     f0, 0x40(r5)
.L_00005C48:
    lwz      r7, 0xb4(r5)
    clrlwi.  r0, r7, 31
    beq      .L_00005C8C
    lwz      r7, 0x3ac(r3)
    li       r8, 0x0
    addi     r0, r7, 0x1
    stw      r0, 0x3ac(r3)
    stw      r8, 0x3b0(r3)
    lwz      r0, 0x3ac(r3)
    cmpwi    r0, 0x5
    blt      .L_00005CC8
    lis      r7, shotEvents@ha
    lwzu     r0, shotEvents@l(r7)
    ori      r0, r0, 0x4
    stw      r0, 0x0(r7)
    stw      r8, 0x3ac(r3)
    b        .L_00005CC8
.L_00005C8C:
    rlwinm.  r0, r7, 0, 30, 30
    beq      .L_00005CC8
    li       r8, 0x0
    stw      r8, 0x3ac(r3)
    lwz      r7, 0x3b0(r3)
    addi     r0, r7, 0x1
    stw      r0, 0x3b0(r3)
    lwz      r0, 0x3b0(r3)
    cmpwi    r0, 0x5
    blt      .L_00005CC8
    lis      r7, shotEvents@ha
    lwzu     r0, shotEvents@l(r7)
    ori      r0, r0, 0x8
    stw      r0, 0x0(r7)
    stw      r8, 0x3b0(r3)
.L_00005CC8:
    li       r0, 0x0
    stw      r0, 0xb4(r5)
.L_00005CD0:
    addi     r6, r6, 0x1
    addi     r5, r5, 0xbc
    cmplwi   r6, 0x5
    blt      .L_00005A54
    addi     r1, r1, 0x20
    blr
*/

/**
 * @note Address: stage02 .text 0x5CE8
 * @note Size: 0x170
 */
void fighter::CCameraActionFighter::Update()
{
	Vec v;

	if (_40 == NULL) {
		return;
	}
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x800) {
		if (_68) {
			_68 = false;
		} else {
			_68 = true;
		}
	}
	SetFov(40.0f);
	if (_68) {
		v.x = v.y = v.z = 0.0f;
		v.z             = 80.0f;
		PSMTXMultVec(_40, &v, &v);
		SetInterest(v);
		v.x = v.z = 0.0f;
		SetEye(v);
	} else {
		PSMTXMultVec(_40, &_50, &v);
		SetEye(v);
		PSMTXMultVec(_40, &_44, &v);
		SetInterest(v);
	}
}

/**
 * @note Address: stage02 .text 0x5E58
 * @note Size: 0x88
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x5EE0
 * @note Size: 0x4
 */
// inline

/**
 * @note Address: stage02 .text 0x5EE4
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x5EEC
 * @note Size: 0x8
 */
// inline

/**
 * @note Address: stage02 .text 0x5EF4
 * @note Size: 0xC
 */
// inline

/**
 * @note Address: stage02 .text 0x5F00
 * @note Size: 0x134
 */
// fn_5_5F00
/*
    stwu     r1, -0x10(r1)
    mflr     r0
    lis      r4, __vt__7CStatus@ha
    lis      r3, shotEvents@ha
    stw      r0, 0x14(r1)
    addi     r6, r4, __vt__7CStatus@l
    addi     r3, r3, shotEvents@l
    li       r0, 0x0
    lis      r4, fn_5_6128@ha
    lis      r5, sShotEventsDestructorChain@ha
    stw      r6, 0x4(r3)
    addi     r4, r4, fn_5_6128@l
    addi     r5, r5, sShotEventsDestructorChain@l
    stw      r0, 0x0(r3)
    bl       __register_global_object
    lis      r6, __vt__22CInterfaceCameraAction@ha
    lis      r3, fighterCamera@ha
    lis      r4, lbl_5_rodata_A4@ha
    lis      r5, __vt__4CVec@ha
    addi     r9, r6, __vt__22CInterfaceCameraAction@l
    addi     r3, r3, fighterCamera@l
    lfs      f0, lbl_5_rodata_A4@l(r4)
    addi     r8, r5, __vt__4CVec@l
    lis      r7, __vt__Q27fighter20CCameraActionFighter@ha
    li       r6, 0x0
    li       r0, 0x1
    lis      r4, fn_5_C@ha
    lis      r5, sFighterCameraDestructorChain@ha
    stw      r9, 0x3c(r3)
    addi     r7, r7, __vt__Q27fighter20CCameraActionFighter@l
    addi     r4, r4, fn_5_C@l
    stw      r8, 0xc(r3)
    addi     r5, r5, sFighterCameraDestructorChain@l
    stw      r8, 0x1c(r3)
    stw      r8, 0x2c(r3)
    stw      r7, 0x3c(r3)
    stw      r6, 0x40(r3)
    stfs     f0, 0x4c(r3)
    stfs     f0, 0x48(r3)
    stfs     f0, 0x44(r3)
    stfs     f0, 0x58(r3)
    stfs     f0, 0x54(r3)
    stfs     f0, 0x50(r3)
    stb      r0, 0x68(r3)
    bl       __register_global_object
    lis      r4, matoList@ha
    lis      r3, fn_5_610C@ha
    addi     r7, r4, matoList@l
    li       r5, 0x0
    addi     r4, r3, fn_5_610C@l
    li       r6, 0x34
    addi     r3, r7, 0x2c
    li       r7, 0x50
    bl       __construct_array
    lis      r3, fighterWork@ha
    lis      r4, fn_5_60F4@ha
    addi     r6, r3, fighterWork@l
    li       r0, 0x0
    lis      r3, fn_5_60A0@ha
    stw      r0, 0x10c(r6)
    addi     r5, r3, fn_5_60A0@l
    addi     r4, r4, fn_5_60F4@l
    addi     r3, r6, 0x114
    li       r6, 0xbc
    li       r7, 0x5
    bl       __construct_array
    lis      r3, fighterWork@ha
    lis      r4, fn_5_6034@ha
    lis      r5, sFighterWorkDestructorChain@ha
    addi     r3, r3, fighterWork@l
    addi     r4, r4, fn_5_6034@l
    addi     r5, r5, sFighterWorkDestructorChain@l
    bl       __register_global_object
    lwz      r0, 0x14(r1)
    mtlr     r0
    addi     r1, r1, 0x10
    blr
*/

/**
 * @note Address: stage02 .text 0x6034
 * @note Size: 0x6C
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x60A0
 * @note Size: 0x54
 */
// generated dtor

/**
 * @note Address: stage02 .text 0x60F4
 * @note Size: 0x18
 */
// inline ctor

/**
 * @note Address: stage02 .text 0x610C
 * @note Size: 0x1C
 */
// inline ctor

/**
 * @note Address: stage02 .text 0x6128
 * @note Size: 0x48
 */
// generated dtor
