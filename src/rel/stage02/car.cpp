#include "Version.h"

// the two cars of the hallway race and the camera that follows them. retail built this as a file
// of its own: its data is aligned apart from rouka.cpp's, and rouka.cpp calls into it without
// inlining anything. the functions are in the order they were written, retail's code runs the
// other way (-inline deferred)

#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/csound.h"
#include "game/ut.h"
#include "game/xchr.h"

#include <dolphin/mtx/mtx.h>

namespace Car {

class CCameraActionCar : public CInterfaceCameraAction {
public:
	CCameraActionCar()
	{
		mCarMtx           = NULL;
		mInterestOffset.x = mInterestOffset.y = mInterestOffset.z = 0.0f;
		mTarget.x = mTarget.y = mTarget.z = 0.0f;
	}
	virtual ~CCameraActionCar() { }

	void SetTarget(Vec pos) { mTarget = pos; }
	void SetOffset(Vec pos) { mInterestOffset = pos; }

	virtual void Update(u32 step);
	virtual u32 GetMode() { return 0; }
	virtual u32 GetCameraFlags() { return 11; }
	virtual f32 GetFollowDivisor() { return 0.1f; }

	MtxPtr mCarMtx;      // _40, matrix of the car the camera follows, NULL stops it
	Vec mInterestOffset; // _44, interest point in the car's space, set by SetOffset
	Vec mTarget;         // _50, set by SetTarget (0,30,-120 in StartCarCamera); never read
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

static Car::CCameraActionCar carCamera;

static Car::PlayerCarRoot playerCar;
static Car::CarRoot racerCar;
static u8 carsStarted;
static Vec carCameraTrail[32];
static u32 carCameraTrailIndex;
static u8* carParams;
static u8* carSoundCues;

extern "C" {

void RestoreDefaultCamera(void);
void Terminate(void);
s32 GetPlayerCarFrame(void);
s32 GetPlayerCarResult(void);
void SetCarSoundCues(u8* data);
void SetCarParams(u8* data);
void DrawCarHud(s32 frames);
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

inline void Car::CarRoot::Register(XObj* car, XObj* path, XObj* attached)
{
#if !defined(VERSION_PAL)
	OSReport("Car::Register\n");
#endif
	mCar      = car;
	mPath     = path;
	mAttached = attached;
	car->mStatus.mFlags |= 0x10000;       // drawn
	mPath->mStatus.mFlags &= ~0x10000ULL; // the hidden mover
	mFrame     = 0.0f;
	mPrevFrame = 0.0f;
	mSpeed     = 1.0f;
	mBraking   = 0;
	mStarted   = 0;
}

// the one virtual that is inline: retail has its copy at the end, with the camera's
inline void Car::CarRoot::Start()
{
	mStarted = 1;
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
// Car::CCameraActionCar::GetFollowDivisor, inline in the class

/**
 * @note Address: stage02 .text 0x8650
 * @note Size: 0x8
 */
// Car::CCameraActionCar::GetCameraFlags, inline in the class

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
void Car::CCameraActionCar::Update(u32 step)
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
	jobj        = GetJointByPath(mPath->mJObj, path, 1);
	PSMTXScale(scale, 0.5f, 0.5f, 0.5f);
	PSMTXConcat(HSD_JObjGetMtxPtr(jobj), scale, mCarMtx);
	HSD_JObjCopyMtx(mCar->mJObj, mCarMtx);
	if (mAttached != NULL) {
		PSMTXTrans(m, 3.75f, 5.0f, -1.0f);
		PSMTXConcat(mCarMtx, m, mAttachedMtx);
		HSD_JObjCopyMtx(mAttached->mJObj, mAttachedMtx);
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

/**
 * @note Address: stage02 .text 0x7FB8
 * @note Size: 0xE8
 */
void Car::CarRoot::Brake()
{
	mBraking = 1;
	mCar->SetAnim(2, FALSE, NULL, NULL, NULL, 0.0f, 1.0f, 0.0f, 10.0f);
	mCar->PushAnim(3, 0, 100);
}

// Brake__Q23Car7CarRootFv

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
	PlayGameSound(&gGame.mSound, 0x2000D, 0xFF);
}

/**
 * @note Address: stage02 .text 0x7BB0
 * @note Size: 0x3A8
 */
void Car::PlayerCarRoot::Update(u32 frames)
{
	CarRoot::Update(frames);
	if (!mStarted) {
		return;
	}
	CPad pad(0);
	XSound* sound;
	if (!mBraking && (pad.mPad->mTrigger & 0x200)) { // B brakes
		Brake();
		mBrakeFrame = mFrame;
		sound       = &gGame.mSound;
		StopGameSfx(sound, 0x2000D);
		if (mBrakeFrame < ReadS32LE(carParams + CAR_PARAM_BRAKE_JUDGE)) {
			PlayGameSound(sound, 0x2000E, 0xFF);
		} else {
			PlayGameSound(sound, 0x20011, 0xFF);
		}
	}
	sound = &gGame.mSound;
	// cue records of (frame, sound id); a non-positive frame ends the list
	for (s32 i = 0; ReadS32LE(carSoundCues + i * 8) > 0; i++) {
		s32 frame = ReadS32LE(carSoundCues + i * 8);
		s32 id    = ReadS32LE(carSoundCues + (i * 2 + 1) * 4);
		if (mPrevFrame < frame && frame <= mFrame) {
			PlayGameSound(sound, id, 0xFF);
			if (id == 0x2001F) { // starting 0x2001F fades the music (track 0xFFFF0000) out
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
	gGame.GetPlayer()->LockControl();
	gGame.mMotionControlCamera.ChangeAction(&carCamera);
	Vec v;
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
// NONMATCHING: retail constructs an otherwise unused CPad(0), as the Wii source also does; the clean GC
// source omits it. Car updates, camera trail and braking logic are otherwise present.
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
		Vec pos = { 0.0f, 20.0f, 0.0f }; // the camera rides 20 above the car
		PSMTXMultVec(playerCar.mCarMtx, &pos, &pos);
		carCameraTrail[carCameraTrailIndex] = pos;
		s32 prev                            = carCameraTrailIndex - 10; // the eye trails 10 positions back
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

/**
 * @note Address: stage02 .text 0x7490
 * @note Size: 0x4
 */
extern "C" void DrawCarHud(s32 frames)
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
	aobj = GetAObj(playerCar.mPath->mJObj);
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
	HSD_JObjClearMtxFlags(playerCar.mCar->mJObj);
	if (playerCar.mAttached != NULL) {
		HSD_JObjClearMtxFlags(playerCar.mAttached->mJObj);
	}
	playerCar.mStarted = 0;
	playerCar.mBraking = 0;
	HSD_JObjClearMtxFlags(racerCar.mCar->mJObj);
	if (racerCar.mAttached != NULL) {
		HSD_JObjClearMtxFlags(racerCar.mAttached->mJObj);
	}
	racerCar.mStarted = 0;
	racerCar.mBraking = 0;
	gGame.GetPlayer()->RestoreControl();
	HSD_JObjClearMtxFlags(gGame.GetPlayer()->mJObj);
	gGame.mMotionControlCamera.ChangeAction(gGame.mMotionControlCamera.mPrevAction);
	Car::Init();
}

// Terminate

/**
 * @note Address: stage02 .text 0x6E5C
 * @note Size: 0xCC
 */
extern "C" void RestoreDefaultCamera(void)
{
	HSD_JObjClearMtxFlags(gGame.GetPlayer()->mJObj);
	gGame.mMotionControlCamera.ChangeAction(gGame.mMotionControlCamera.mPrevAction);
}
