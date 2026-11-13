#include "Version.h"

// the two cars of the hallway race and the camera that follows them. retail built this as a file
// of its own: its data is aligned apart from rouka.cpp's, and rouka.cpp calls into it without
// inlining anything. the functions are in the order they were written, retail's code runs the
// other way (-inline deferred)

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
	u8 _04[0x30];   // _04, padding over CJObj's mTranslate/mRotate/mScale (_04-_34); not accessed
	virtual void vf08();
	virtual void SetTranslateX();
	virtual void SetTranslateY();
	virtual void SetTranslateZ();
	virtual void SetScaleX();
	virtual void SetScaleY();
	virtual void SetScaleZ();
	virtual void SetRotationX();
	virtual void SetRotationY();
	virtual void SetRotationZ();
	virtual void SetTranslate(const Vec& v, s32 force);
	virtual void SetTranslate(f32 x, f32 y, f32 z, s32 force);
	virtual void SetScale(const Vec& v, s32 force);
	virtual void SetScale(f32 x, f32 y, f32 z, s32 force);
	virtual void SetRotation(const Vec& v, s32 force);
	virtual void SetRotation(f32 x, f32 y, f32 z, s32 force);
	virtual void SetAnimRate();
	virtual void ReadAsync();
	virtual void ReadCheck();
	virtual void Release();
	virtual void StepRotate();
	virtual void StepTranslate();
	virtual void StepScale();
	virtual void SetPos(const Vec& pos, s32 force);
	virtual void SetPos(f32 x, f32 y, f32 z, s32 force);
	virtual void SetScale(f32 scale, s32 force);
	virtual void SetRot(const Vec& rot, s32 force);
	virtual void SetRot(f32 x, f32 y, f32 z, s32 force);
	virtual void SetMoveDir(const Vec* dir, f32 speed);
	virtual void SetRotateY();
	virtual void SetAnim(s32 id, BOOL force, void* jointAnim, void* matAnim, void* shapeAnim, f32 frame, f32 rate, f32 blend,
	                     f32 blendRate);
#if defined(VERSION_JP)
	u8 _38[0x88]; // _38, mode/flag word returned by GetFlags (_54); the real class also leaves it _38
#else
	u8 _38[0x90]; // _38, the mode word GetFlags (_54) returns. The dol class leaves it unnamed too: its SetEye/SetInterest(v, mode)
	              // write it, and GetCameraFlags returns it
#endif
	u8 _C8[0x90];      // _C8 (JP: _C0)
	s32 mAnimId;       // _158 (JP: _150)
	u8 _15C[0xCC];     // _15C (JP: _154)
	u64 mStatus;       // _228 (JP: _220)
	u8 _230[0x8];      // _230 (JP: _228)
	s32* mCoord;       // _238 (JP: _230)
	u8 _23C[0x5D4];    // _23C (JP: _234)
	s32 mAnmQueueNext; // _810 (JP: _808)
	struct {
		s32 type;  // _00
		s16 _04;   // _04, CJObj's mTranslate, mRotate and mScale (three CVecs, _04-_34) as one byte block; not accessed in the REL
		s16 mRate; // _06, queued anim rate in hundredths (CAnmQueueEntry::mRate)
	} _814[16];    // _814 (JP: _80C)
};

class CCamera;

class CInterfaceCameraAction {
public:
	CVec mEye;      // _00, eye position
	CVec mInterest; // _10, interest (look-at) point
	CVec mAngle;    // _20, camera angle
	f32 mDistance;  // _30, camera distance
	f32 mFov;       // _34, field of view
	s32 _38;        // _38

	virtual ~CInterfaceCameraAction() { }       // _08
	virtual void PreviousUpdate() { }           // _0C
	virtual void Update() = 0;                  // _10
	virtual void Reset() { }                    // _14
	virtual void SetCamera(CCamera* camera) { } // _18
	virtual void Init() { }                     // _1C
	virtual void Draw() { }                     // _20
	virtual void DrawDebug2d() { }              // _24
	virtual void DrawDebug3d() { }              // _28
	virtual void SetEye(f32 x, f32 y, f32 z)    // _2C
	{
		mEye.x = x;
		mEye.y = y;
		mEye.z = z;
	}
	virtual void SetInterest(f32 x, f32 y, f32 z) // _30
	{
		mInterest.x = x;
		mInterest.y = y;
		mInterest.z = z;
	}
	virtual void SetAngle(f32 x, f32 y, f32 z) // _34
	{
		mAngle.x = x;
		mAngle.y = y;
		mAngle.z = z;
	}
	virtual void SetEye(const Vec& v) { SetEye(v.x, v.y, v.z); }           // _38
	virtual void SetInterest(const Vec& v) { SetInterest(v.x, v.y, v.z); } // _3C
	virtual void SetAngle(const Vec& v) { SetAngle(v.x, v.y, v.z); }       // _40
	virtual void SetDistance(f32 v) { mDistance = v; }                     // _44
	virtual void SetFov(f32 v) { mFov = v; }                               // _48
	virtual void SetAngleZ(f32 v) { mAngle.z = v; }                        // _4C
	virtual s32 GetMode() { return 0; }                                    // _50
	virtual s32 GetFlags() { return _38; }                                 // _54
	virtual f32 GetEaseDivisor() { return 12.0f; }                         // _58
	virtual s32 GetInterestMode() { return 0; }                            // _5C
	virtual s32 GetTarget() { return 0; }                                  // _60
};

namespace Car {

class CCameraActionCar : public CInterfaceCameraAction {
public:
	CCameraActionCar()
	{
		mCarMtx           = NULL;
		mInterestOffset.x = mInterestOffset.y = mInterestOffset.z = 0.0f;
		_50.x = _50.y = _50.z = 0.0f;
	}
	virtual ~CCameraActionCar() { }

	void SetTarget(Vec pos) { _50 = pos; }
	void SetOffset(Vec pos) { mInterestOffset = pos; }

	virtual void Update();
	virtual s32 GetMode() { return 0; }
	virtual s32 GetFlags() { return 11; }
	virtual f32 GetEaseDivisor() { return 0.1f; }

	MtxPtr mCarMtx;      // _40, matrix of the car the camera follows, NULL stops it
	Vec mInterestOffset; // _44, interest point in the car's space, set by SetOffset
	Vec _50;             // _50, set by SetTarget (0,30,-120 in StartCarCamera) and zeroed in the ctor; never read
	u8 _5C[0xC];         // _5C
};

class CarRoot {
public:
	virtual void Update(u32 frames); // _08
	virtual void Brake();            // _0C
	virtual void Start();            // _10

	void Register(XObj* car, XObj* path, XObj* attached);

	XObj* mCar;       // _04, the car model: drawn, follows _1C, plays the drive/brake/stop anims
	XObj* mPath;      // _08, hidden object whose anim 0 at mFrame carries the car along the course
	XObj* mAttached;  // _0C, optional object carried on the car at (3.75, 5, -1)
	f32 mFrame;       // _10, current frame along the course
	f32 mPrevFrame;   // _14, frame before the last advance
	f32 mSpeed;       // _18, frames per tick multiplier, 1 normally, decays to 0 after braking
	Mtx mCarMtx;      // _1C, car matrix: path joint * scale 0.5, copied into the car's jobj
	Mtx mAttachedMtx; // _4C, _1C * trans(3.75, 5, -1), copied into _0C's jobj
	u8 mBraking;      // _7C, set by Brake; stops the drive anim and lets mSpeed decay
	u8 mStarted;      // _7D, set by Start; the frame only advances while set
};

class PlayerCarRoot : public CarRoot {
public:
	virtual void Update(u32 frames); // _08
	virtual void Start();            // _10
	virtual void Clear();            // _14

	f32 mBrakeFrame; // _80, frame at which the player braked
};

// prints "Car::Init"
void Init(void);

} // namespace Car

struct RoboView {
#if defined(VERSION_JP)
	u8 _00[0x970];     // _00
	u32 mPlayerStatus; // _970
	u8 _984[0x12380];  // _974
	s32 mPadReset;     // _12CF4
#else
	u8 _00[0x980];     // _00
	u32 mPlayerStatus; // _980, XObjChr::mPlayerStatus.mFlags (JP _970); 0x40000000 hands him to a script, 0x20000000 | 7 gives control back
	u8 _984[0x12470];  // _984
	s32 mPadReset;     // _12DF4, XObjChr::mPad.mReset (XPad at _12DA0, mReset at +0x54; JP _12CF4); 1 with the give-back flags, 0 with the
	                   // script flags
#endif
};

#define GAME_OBJ() (*(RoboView**)GAME_AT(GAME_OFS(0x96BC, 0x9674)))
#define SOUND()    GAME_AT(GAME_OFS(0xE7E24, 0xE713C))

static Car::CCameraActionCar carCamera;

// see game/cmotioncontrolcamera.h
class CMotionControlCamera {
public:
	void ChangeAction(CInterfaceCameraAction* action);
};

static Car::PlayerCarRoot playerCar;
static Car::CarRoot racerCar;
static u8 carsStarted;
static Vec carCameraTrail[32];
static u32 carCameraTrailIndex;
static u8* carParams;
static u8* carSoundCues;

extern "C" {
void PlayGameSound(void* p, s32 id, s32 v);
void StopGameSfx(void* p, s32 id);
void FadeOutGameSfx(void* p, s32 a, s32 b);

void RestoreDefaultCamera(void);
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void DrawCarHud(void* arg);
void UpdateCars(u32 frames);
void StartCars(void);
void StartCarCamera(void);
s32 RegisterCar(XObj* car, XObj* path, XObj* attached);
}

// carParams: little endian s32s out of the stage's car data
#define CAR_PARAM_BRAKE_DECEL 0x0 // speed loss per frame while braking, thousandths
#define CAR_PARAM_FINISH      0x4 // past it the run counts as done: speed restores, camera releases
#define CAR_PARAM_BRAKE_JUDGE 0x8 // braking before this frame plays 0x2000E, after it 0x20011
#define CAR_PARAM_RACER_BRAKE 0xC // the frame the rival's car brakes at

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

inline void Car::CarRoot::Register(XObj* car, XObj* path, XObj* attached)
{
#if !defined(VERSION_PAL)
	OSReport("Car::Register\n");
#endif
	mCar      = car;
	mPath     = path;
	mAttached = attached;
	car->mStatus |= 0x10000;       // drawn
	mPath->mStatus &= ~0x10000ULL; // the hidden mover
	mFrame     = 0.0f;
	mPrevFrame = 0.0f;
	mSpeed     = 1.0f;
	mBraking   = 0;
	mStarted   = 0;
}

// what Brake does. the copy retail has of the virtual holds this one inline level down
inline void CarRoot_Brake(Car::CarRoot* car)
{
	s32 i;
	XObj* obj;

	car->mBraking = 1;
	car->mCar->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	obj = car->mCar;
	// takes the first free slot in the request ring
	for (i = 0; i < 16; i++) {
		s32 idx = (obj->mAnmQueueNext + i) % 16;
		if (obj->_814[idx].type == -1) { // -1 marks it free
			obj->_814[idx].type  = 3;
			obj->_814[idx]._04   = 0;
			obj->_814[idx].mRate = 100;
			break;
		}
	}
}

// the one virtual that is inline: retail has its copy at the end, with the camera's
inline void Car::CarRoot::Start()
{
	mStarted = 1;
}

inline void RoboView_ResetFlags(RoboView* game)
{
	game->mPlayerStatus = 0;
	game->mPlayerStatus |= 0x20000000;
	game->mPlayerStatus |= 7;
	game->mPadReset = 1;
}

/**
 * @note Address: stage02 .text 0x8664
 * @note Size: 0xB4
 */
// fn_5_8664
/**
 * @note Address: stage02 .text 0x8658
 * @note Size: 0xC
 */
// Car::CCameraActionCar::GetEaseDivisor, inline in the class

/**
 * @note Address: stage02 .text 0x8650
 * @note Size: 0x8
 */
// Car::CCameraActionCar::GetFlags, inline in the class

/**
 * @note Address: stage02 .text 0x8648
 * @note Size: 0x8
 */
// Car::CCameraActionCar::GetMode, inline in the class

/**
 * @note Address: stage02 .text 0x85B0
 * @note Size: 0x98
 */
// fn_5_85B0

/**
 * @note Address: stage02 .text 0x85A4
 * @note Size: 0xC
 */
// Car::CarRoot::Start, inline above the functions

/**
 * @note Address: stage02 .text 0x8530
 * @note Size: 0x74
 */
void Car::CCameraActionCar::Update()
{
	Vec pos;

	if (mCarMtx != NULL) {
		SetFov(40.0f);
		PSMTXMultVec(mCarMtx, &mInterestOffset, &pos);
		SetInterest(pos);
	}
}

/**
 * @note Address: stage02 .text 0x80A0
 * @note Size: 0x490
 */
void Car::CarRoot::Update(u32 frames)
{
	HSD_JObj* jobj;
	Mtx scale;
	Mtx m;

	if (mCar == NULL || mPath == NULL) {
		return;
	}
	s32 path[2] = { 0, 0 }; // the path object's first joint
	jobj        = GetJointByPath(mPath->jobj, path, 1);
	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, mCarMtx);
	HSD_JObjCopyMtx(mCar->jobj, mCarMtx);
	if (mAttached != NULL) {
		PSMTXTrans(m, 3.75f, 5.0f, -1.0f);
		PSMTXConcat(mCarMtx, m, mAttachedMtx);
		HSD_JObjCopyMtx(mAttached->jobj, mAttachedMtx);
	}
	if (mStarted) {
		mPrevFrame = mFrame;
		mFrame     = mSpeed * frames + mFrame;
	}
	mPath->SetAnim(0, TRUE, NULL, NULL, NULL, mFrame, 1.0f, 0.0f, 10.0f);
	if (!mBraking) {
		mCar->SetAnim(1, TRUE, NULL, NULL, NULL, mFrame, 1.0f, 0.0f, 10.0f);
		return;
	}
	if (0.0f < mSpeed) {
		f32 rate = mSpeed - ReadS32LE(carParams + CAR_PARAM_BRAKE_DECEL) / 1000.0f;
		if (rate < 0.0f) {
			rate = 0.0f;
		}
		mSpeed = rate;
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
 * @note Address: stage02 .text 0x7FB8
 * @note Size: 0xE8
 */
void Car::CarRoot::Brake()
{
	CarRoot_Brake(this);
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
 * @note Address: stage02 .text 0x7F9C
 * @note Size: 0x1C
 */
void Car::PlayerCarRoot::Clear()
{
	mPath       = NULL;
	mCar        = NULL;
	mBrakeFrame = 0.0f;
}

/**
 * @note Address: stage02 .text 0x7F58
 * @note Size: 0x44
 */
void Car::PlayerCarRoot::Start()
{
	mStarted = 1;
	PlayGameSound(SOUND(), 0x2000D, 0xFF);
}

/**
 * @note Address: stage02 .text 0x7BB0
 * @note Size: 0x3A8
 */
void Car::PlayerCarRoot::Update(u32 frames)
{
	s32 i;

	CarRoot::Update(frames);
	if (!mStarted) {
		return;
	}
	CPad pad(0);
	void* sound;
	if (!mBraking && (pad.mPad->mTrigger & 0x200)) { // B brakes
		Brake();
		mBrakeFrame = mFrame;
		sound       = SOUND();
		StopGameSfx(sound, 0x2000D);
		if (mBrakeFrame < ReadS32LE(carParams + CAR_PARAM_BRAKE_JUDGE)) {
			PlayGameSound(sound, 0x2000E, 0xFF);
		} else {
			PlayGameSound(sound, 0x20011, 0xFF);
		}
	}
	sound = SOUND();
	// cue records of (frame, sound id); a non-positive frame ends the list
	for (i = 0; ReadS32LE(carSoundCues + i * 8) > 0; i++) {
		s32 frame = ReadS32LE(carSoundCues + i * 8);
		s32 id    = ReadS32LE(carSoundCues + (i * 2 + 1) * 4);
		if (mPrevFrame < frame && frame <= mFrame) {
			PlayGameSound(sound, id, 0xFF);
			if (id == 0x2001F) { // it gets faded out
				FadeOutGameSfx(sound, -0x10000, 20);
			} else if (id == 0x2000F) { // it stops 0x2001F
				StopGameSfx(sound, 0x2001F);
			}
		}
	}
	if (ReadS32LE(carParams + CAR_PARAM_FINISH) <= mFrame) {
		mSpeed = 1.0f;
		if (mBraking && mCar->mAnimId != 4) {
			mCar->SetAnim(4, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
		}
	}
}

/**
 * @note Address: stage02 .text 0x7B38
 * @note Size: 0x78
 */
void Car::Init(void)
{
#if !defined(VERSION_PAL)
	OSReport("Car::Init\n");
#endif
	playerCar.mPath       = NULL;
	playerCar.mCar        = NULL;
	playerCar.mBrakeFrame = 0.0f;
	racerCar.mPath        = NULL;
	racerCar.mCar         = NULL;
	carsStarted           = 0;
	carCameraTrailIndex   = 0;
	carParams             = NULL;
}

/**
 * @note Address: stage02 .text 0x799C
 * @note Size: 0x19C
 */
extern "C" s32 RegisterCar(XObj* car, XObj* path, XObj* attached)
{
	// 1 claims the player's car, 2 the rival's
	if (playerCar.mCar == NULL) {
		playerCar.Register(car, path, attached);
		return 1;
	}
	if (racerCar.mCar == NULL) {
		racerCar.Register(car, path, attached);
		return 2;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x7890
 * @note Size: 0x10C
 */
extern "C" void StartCarCamera(void)
{
	RoboView* game = GAME_OBJ();
	Vec v;

	game->mPlayerStatus = 0;
	game->mPlayerStatus |= 0x40000000;
	game->mPadReset = 0;
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))->ChangeAction(&carCamera);
	v.x = 0.0f;
	v.y = 30.0f;
	v.z = -120.0f;
	carCamera.SetTarget(v);
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 60.0f;
	carCamera.SetOffset(v); // the interest sits 60 out on the car's z
	carCamera.mCarMtx = playerCar.mCarMtx;
}

/**
 * @note Address: stage02 .text 0x782C
 * @note Size: 0x64
 */
extern "C" void StartCars(void)
{
	carsStarted = 1;
	playerCar.Start();
	racerCar.Start();
}

/**
 * @note Address: stage02 .text 0x7494
 * @note Size: 0x398
 */
extern "C" void UpdateCars(u32 frames)
{
	CPad pad(0);
	bool done;

	if (!carsStarted) {
		frames = 0;
	}
	if (playerCar.mCar == NULL) {
		return;
	}
	if (racerCar.mCar == NULL) {
		return;
	}
	playerCar.Update(frames);
	racerCar.Update(frames);
	if (!playerCar.mStarted) {
		done = false;
	} else {
		done = ReadS32LE(carParams + CAR_PARAM_FINISH) <= playerCar.mFrame;
	}
	if (done) {
		carCamera.mCarMtx = NULL; // the camera lets go
	} else {
		Vec pos;
		s32 prev;
		pos.y = 0.0f;
		pos.z = 0.0f;
		pos.x = 0.0f;
		pos.y = 20.0f; // the camera rides 20 above the car
		PSMTXMultVec(playerCar.mCarMtx, &pos, &pos);
		carCameraTrail[carCameraTrailIndex] = pos;
		prev                                = carCameraTrailIndex - 10; // the eye trails 10 positions back
		if (prev < 0) {
			prev += 32;
		}
		CInterfaceCameraAction* cam = &carCamera;
		cam->SetEye(carCameraTrail[prev].x, carCameraTrail[prev].y, carCameraTrail[prev].z);
		if (++carCameraTrailIndex >= 32) {
			carCameraTrailIndex = 0;
		}
	}
	if (!racerCar.mBraking && ReadS32LE(carParams + CAR_PARAM_RACER_BRAKE) <= racerCar.mFrame) {
		racerCar.Brake();
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
 * @note Address: stage02 .text 0x7490
 * @note Size: 0x4
 */
extern "C" void DrawCarHud(void* arg)
{
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
 * @note Address: stage02 .text 0x7478
 * @note Size: 0xC
 */
extern "C" void SetCarSoundCues(u8* data)
{
	carSoundCues = data;
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

	// 1: braked to a stop, 2: ran past the course's end
	if (playerCar.mBraking && playerCar.mSpeed <= 0.0f) {
		return 1;
	}
	aobj = GetAObj(playerCar.mPath->jobj);
	end  = aobj != NULL ? HSD_AObjGetEndFrame(aobj) : 0.0f;
	if (end <= playerCar.mFrame) {
		return 2;
	}
	return 0;
}

/**
 * @note Address: stage02 .text 0x7304
 * @note Size: 0x30
 */
extern "C" s32 GetPlayerCarFrame(void)
{
	return 100.0f * playerCar.mFrame;
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
	JObjInline1199(playerCar.mCar->jobj);
	if (playerCar.mAttached != NULL) {
		JObjInline1199(playerCar.mAttached->jobj);
	}
	playerCar.mStarted = 0;
	playerCar.mBraking = 0;
	JObjInline1199(racerCar.mCar->jobj);
	if (racerCar.mAttached != NULL) {
		JObjInline1199(racerCar.mAttached->jobj);
	}
	racerCar.mStarted = 0;
	racerCar.mBraking = 0;
	RoboView_ResetFlags(GAME_OBJ());
	JObjInline1199(((XObj*)GAME_OBJ())->jobj);
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))
	    ->ChangeAction(*(CInterfaceCameraAction**)GAME_AT(GAME_OFS(0xE5F0C, 0xE5224)));
	Car::Init();
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
 * @note Address: stage02 .text 0x6E5C
 * @note Size: 0xCC
 */
extern "C" void RestoreDefaultCamera(void)
{
	JObjInline1199(((XObj*)GAME_OBJ())->jobj);
	((CMotionControlCamera*)GAME_AT(GAME_OFS(0xE5D38, 0xE5050)))
	    ->ChangeAction(*(CInterfaceCameraAction**)GAME_AT(GAME_OFS(0xE5F0C, 0xE5224)));
}
