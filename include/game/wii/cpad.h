#ifndef GAME_WII_CPAD_H
#define GAME_WII_CPAD_H

#include "types.h"
#include "Dolphin/vec.h"
#include <nw4r/snd/snd_RemoteSpeakerManager.h>

// Bit positions in the game's 64-bit button word.
enum {
	RemoteLeft,
	RemoteRight,
	RemoteDown,
	RemoteUp,
	RemoteA,
	RemoteB,
	RemoteOne,
	RemoteTwo,
	RemotePlus,
	RemoteMinus,
	RemoteHome,
	NunchukC,
	NunchukZ,
	NunchukStick      = 17,
	RemoteDirections2 = 21,
	ClassicLeft       = 25,
	ClassicRight,
	ClassicDown,
	ClassicUp,
	ClassicA,
	ClassicB,
	ClassicX,
	ClassicY,
	ClassicPlus,
	ClassicMinus,
	ClassicHome,
	ClassicL,
	ClassicR,
	ClassicZL,
	ClassicZR,
	GameCubeDPad = 44,
	GameCubeA    = 48,
	GameCubeB,
	GameCubeX,
	GameCubeY,
	GameCubeZ,
	GameCubeR,
	GameCubeL,
	GameCubeStart,
	GameCubeStick    = 56,
	GameCubeSubStick = 60
};

enum { InputRemote = 1, InputClassic = 2, InputGameCube = 4 };

enum { DeviceRemote = 1, DeviceNunchuk = 2, DeviceClassic = 4, DeviceGameCube = 8 };

struct PADStatus;

struct StickPosition {
	f32 x, y;
};

struct StickDirectionSettings {
	f32 thresholdPercent;
	f32 diagonalOverlapDegrees;
};

struct WiiButtonState {
	u32 hold, trigger, release;
};

struct WiiAcceleration {
	Vec value;
	f32 magnitude;
	f32 change;
};

struct WiiPointerMotion {
	StickPosition position;
	StickPosition velocity;
	f32 speed;
};

// The extension report overlays the Nunchuk, Classic Controller and Balance Board.
union WiiExtensionReport {
	struct {
		StickPosition stick;
		WiiAcceleration acceleration;
	} nunchuk;
	struct {
		WiiButtonState buttons;
		StickPosition leftStick, rightStick;
		f32 leftTrigger, rightTrigger;
	} classic;
	struct {
		f64 total;
		f64 sensors[4];
		f64 averages[4];
		s32 error;
	} balance;
};

struct WiiPadSample {
	WiiButtonState buttons;
	WiiAcceleration acceleration;
	WiiPointerMotion pointer;
	WiiPointerMotion horizon;
	f32 distance, distanceVelocity, distanceSpeed;
	StickPosition verticalAcceleration;
	u8 device;
	s8 error;
	s8 pointerValid;
	u8 dataFormat;
	WiiExtensionReport extension;
};
STATIC_ASSERT(sizeof(WiiPadSample) == 0xb0);

struct WiiButtonRepeat {
	s32 delay, interval, startingInterval, minimumInterval;
};

enum { RumbleEnabled = 1, RumbleRunning = 2, RumblePaused = 4 };

struct WiiRumble {
	u8 flags;
	u8 previousFlags;
	s32 remaining;

	// CPadOne::Update reads the flag through a const path: retail loads flags again for its next test,
	// which the compiler only does after a read through a pointer to const. The name is ours
	BOOL IsEnabled() const { return flags & RumbleEnabled; }
	// runs the motor for frames, CPadOne::Update counts them down
	void Start(s32 frames)
	{
		remaining = frames;
		flags |= RumbleRunning;
	}
	void Stop()
	{
		remaining = 0;
		flags &= ~RumbleRunning;
	}
};

class CPadOne {
public:
	u8 mPort;
	u8 mInputMask;
	u64 mButton, mDirButton;
	u64 mLastButton, mLastDirButton;
	u64 mTrigger, mDirTrigger;
	u64 mRepeat, mDirRepeat;
	WiiButtonRepeat mRepeatSettings[16];
	s32 mRepeatTimer[16];
	StickPosition mStick, mSubStick;
	f32 mAnalogL, mAnalogR;
	StickDirectionSettings mStickDirections;
	WiiRumble mRumble[3];
	WiiPadSample mSamples[16];
	s32 mSampleCount;
	u32 mDevices, mDeviceType;
	s32 mProbeResult;
	f32 mRemoteMotionChanges[100];
	u32 mRemoteMotionCount;
	BOOL mRemoteMotionActive;
	BOOL mRemoteMotionPreviouslyActive;
	f32 mRemoteMotionSum;
	u8 mRemoteMotionQuietSamples;
	u8 mRemoteMotionActiveFrames;
	f32 mNunchukMotionChanges[100];
	u32 mNunchukMotionCount;
	BOOL mNunchukMotionActive;
	BOOL mNunchukMotionPreviouslyActive;
	f32 mNunchukMotionSum;
	u8 mNunchukMotionQuietSamples;
	u8 mNunchukMotionActiveFrames;
	Vec mSwingAcceleration[30];
	f32 mSwingMagnitude[30];
	f32 mSwingChange[30];
	s32 mSwingCount;
	BOOL mSwingStarted, mSwingPreviouslyStarted;
	BOOL mSwingRecognized, mSwingPreviouslyRecognized;
	Vec mSwingDirection;
	f32 mSwingStrength;
	BOOL mSwingTimedOut, mSwingTiming;
	s32 mSwingElapsed, mSwingTimeout;

	// retail has it behind the file's static initializer, its only caller (0x802B3B10)
	CPadOne()
	{
		ResetSwing();
		ResetRemoteMotion();
		ResetNunchukMotion();
	}
	virtual ~CPadOne();
	virtual void Init(u8 port, u8 inputMask);
	virtual void Clear();
	virtual void ResetRumble(BOOL resetAll);
	virtual void Update(const PADStatus* status, s32 frames);

	void SetConnectCallback();
	void EnableRemoteSpeaker();
	void DisableRemoteSpeaker();
	u32 GetStickDirections(StickDirectionSettings& settings, f32 x, f32 y) const;
	BOOL IsRemoteActive() const { return (mDevices & DeviceRemote) && (mInputMask & InputRemote); }
	BOOL IsNunchukActive() const { return (mDevices & DeviceNunchuk) && (mInputMask & InputRemote); }
	BOOL IsClassicPadActive() const { return (mDevices & DeviceClassic) && (mInputMask & InputClassic); }
	BOOL IsGameCubePadActive() const { return (mDevices & DeviceGameCube) && (mInputMask & InputGameCube); }
	BOOL GetSwing(Vec* direction, f32* strength);
	void ResetSwing();
	void AppendSwingSample(s32 index);
	void UpdateSwing();
	void DetectSwing();
	void CalculateSwing(s32 first, s32 last);
	void ResetRemoteMotion();
	void CollectRemoteMotion();
	void UpdateRemoteMotion();
	void DetectRemoteMotion();
	void ResetNunchukMotion();
	void CollectNunchukMotion();
	void UpdateNunchukMotion();
	void DetectNunchukMotion();
};
STATIC_ASSERT(sizeof(CPadOne) == 0x12a8);

extern CPadOne gWiiPads[4];

// the pad on a port. Retail multiplies the port out even where it is a constant, so the lookup is a
// function of its own; the name is ours
inline CPadOne& GetWiiPad(u8 port)
{
	return gWiiPads[port];
}

// a handle on one port, what the rest of the game keeps. the GameCube's game/cpad.h CPad is a
// different class (its CPadOne is the 0x54-byte GameCube one); the Wii inlines this constructor
class CPad {
public:
	CPadOne* mPad; // _00
	// _04 vtable

	CPad(u8 port) { mPad = &GetWiiPad(port); }
	virtual ~CPad() { } // _08
};
STATIC_ASSERT(sizeof(CPad) == 0x8);

extern PADStatus gWiiGameCubePads[4];

class CPadEx {
public:
	u8 mInputMask;
	BOOL mMotorEnabled;
	virtual ~CPadEx();

	void Init(u8 inputMask);
	void Clear();
	void ResetRumble(BOOL resetAll);
	void Update(s32 frames);
	void SetMainStickRange(f32 inner, f32 outer);
	void SetSubStickRange(f32 inner, f32 outer);
};
STATIC_ASSERT(sizeof(CPadEx) == 0xc);

#endif
