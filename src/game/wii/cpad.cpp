#include "game/wii/cpad.h"
#include "Dolphin/pad.h"
#include "revolution/mem.h"

struct WiiRawSample {
	u16 buttons;
	s16 acceleration[3];
	struct Point {
		s16 x, y;
		u16 size;
		u8 id, reserved;
	} pointer[4];
	u8 device;
	s8 error;
	union {
		struct {
			s16 acceleration[3];
			s8 stickX, stickY;
		} nunchuk;
		// KPAD transfers the extension-specific raw payload in full.
		u8 report[12];
	} extension;
	u8 dataFormat;
};
STATIC_ASSERT(sizeof(WiiRawSample) == 0x38);

extern "C" {
s32 KPADRead(s32 channel, WiiPadSample* out, u32 count);
void KPADGetUnifiedWpadStatus(s32 channel, WiiRawSample* out, u32 count);
void WPADControlMotor(s32 channel, u32 command);
void NormalizeWiiStick(StickPosition*, f32 x, f32 y, f32 innerRadius, f32 outerRadius);

extern MEMAllocator gWiiPadAllocator;
void WPADRegisterAllocator(void* (*alloc)(u32), BOOL (*free)(void*));
BOOL WPADIsMotorEnabled();
void KPADInit();
void KPADReset();
void KPADSetBtnRepeat(s32 channel, f32 delay, f32 interval);
void KPADSetPosParam(s32 channel, f32 playRadius, f32 sensitivity);
void KPADSetHoriParam(s32 channel, f32 playRadius, f32 sensitivity);
void KPADSetDistParam(s32 channel, f32 playRadius, f32 sensitivity);
void KPADSetAccParam(s32 channel, f32 playRadius, f32 sensitivity);
void KPADSetConnectCallback(s32 channel, void (*callback)(s32, s32));
void KPADEnableAimingMode(s32 channel);
s32 KPADCalibrateDPD(s32 channel);
void KPADDisableStickCrossClamp();
void KPADSetObjInterval(f32 value);
void KPADSetFSStickClamp(s8 inner, s8 outer);
BOOL EnableWiiRemoteSpeaker(RemoteSpeakerChannel*, void (*completion)(s32, s32));
BOOL DisableWiiRemoteSpeaker(RemoteSpeakerChannel*, void (*completion)(s32, s32));
}

struct WiiStickRange {
	f32 min, max;
};
static WiiStickRange sStickRanges[2];
static WiiRawSample sRawSamples[16];

// Hardware masks returned by KPAD for the Wii Remote and its extensions.
enum {
	WPAD_BUTTON_LEFT  = 0x0001,
	WPAD_BUTTON_RIGHT = 0x0002,
	WPAD_BUTTON_DOWN  = 0x0004,
	WPAD_BUTTON_UP    = 0x0008,
	WPAD_BUTTON_PLUS  = 0x0010,
	WPAD_BUTTON_2     = 0x0100,
	WPAD_BUTTON_1     = 0x0200,
	WPAD_BUTTON_B     = 0x0400,
	WPAD_BUTTON_A     = 0x0800,
	WPAD_BUTTON_MINUS = 0x1000,
	WPAD_BUTTON_Z     = 0x2000,
	WPAD_BUTTON_C     = 0x4000,
	WPAD_BUTTON_HOME  = 0x8000
};

enum {
	WPAD_CL_BUTTON_UP    = 0x0001,
	WPAD_CL_BUTTON_LEFT  = 0x0002,
	WPAD_CL_TRIGGER_ZR   = 0x0004,
	WPAD_CL_BUTTON_X     = 0x0008,
	WPAD_CL_BUTTON_A     = 0x0010,
	WPAD_CL_BUTTON_Y     = 0x0020,
	WPAD_CL_BUTTON_B     = 0x0040,
	WPAD_CL_TRIGGER_ZL   = 0x0080,
	WPAD_CL_TRIGGER_R    = 0x0200,
	WPAD_CL_BUTTON_PLUS  = 0x0400,
	WPAD_CL_BUTTON_HOME  = 0x0800,
	WPAD_CL_BUTTON_MINUS = 0x1000,
	WPAD_CL_TRIGGER_L    = 0x2000,
	WPAD_CL_BUTTON_DOWN  = 0x4000,
	WPAD_CL_BUTTON_RIGHT = 0x8000
};

// In table order: Remote directions, two extra repeat masks, +, -; then GC directions, L, R, X, Y.
static const u64 sRepeatGroups[]
    = { 0x01100008ULL,      0x00880004ULL,       0x00220001ULL,         0x00440002ULL,         0x00008000ULL,         0x00010000ULL,
	    1ULL << RemotePlus, 1ULL << RemoteMinus, 0x8800800000000000ULL, 0x4400400000000000ULL, 0x1100100000000000ULL, 0x2200200000000000ULL,
	    1ULL << GameCubeL,  1ULL << GameCubeR,   1ULL << GameCubeX,     1ULL << GameCubeY };

#include "arithmetic.h"

extern "C" {
extern f64 atan2(f64 y, f64 x);

static inline f32 StickLength(f32 x, f32 y)
{
	return FSqrt(x * x + y * y);
}

void NormalizeWiiStick(StickPosition* result, f32 x, f32 y, f32 innerRadius, f32 outerRadius)
{
	f32 length = StickLength(x, y);
	if (length <= innerRadius) {
		result->x = result->y = 0.0f;
	} else if (length >= outerRadius) {
		result->x = x / length;
		result->y = y / length;
	} else {
		f32 scale = ((length - innerRadius) / (outerRadius - innerRadius)) / length;
		result->x = x * scale;
		result->y = y * scale;
	}
}
}

static void SpeakerEnabled(s32 channel, s32 result);
static void SpeakerDisabled(s32 channel, s32 result);
static void OnPadConnection(s32 channel, s32 result);

void CPadOne::Init(u8 port, u8 inputMask)
{
	mPort      = port;
	mInputMask = inputMask;
	Clear();
	KPADSetPosParam(mPort, 0.0f, 1.0f);
	KPADSetHoriParam(mPort, 0.0f, 1.0f);
	KPADSetDistParam(mPort, 0.0f, 1.0f);
	KPADSetAccParam(mPort, 0.1f, 1.0f);
	KPADSetBtnRepeat(mPort, 0.0f, 0.0f);
	KPADEnableAimingMode(mPort);
	KPADCalibrateDPD(mPort);
	SetConnectCallback();
}

void CPadOne::Clear()
{
	mButton = mDirButton = 0;
	mLastButton = mLastDirButton = 0;
	mTrigger = mDirTrigger = mRepeat = mDirRepeat = 0;
	for (int i = 0; i < 16; ++i) {
		mRepeatSettings[i].delay    = 24;
		mRepeatSettings[i].interval = mRepeatSettings[i].startingInterval = 6;
		mRepeatSettings[i].minimumInterval                                = 6;
	}
	for (int i = 0; i < 16; ++i) {
		mRepeatTimer[i] = 24;
	}
	mStick.x = mStick.y = 0.0f;
	mSubStick.x = mSubStick.y = 0.0f;
	mAnalogL = mAnalogR                     = 0.0f;
	mStickDirections.thresholdPercent       = 50.0f;
	mStickDirections.diagonalOverlapDegrees = 30.0f;
	mSampleCount                            = 0;
	mDevices                                = 0;
	ResetRumble(TRUE);
}

void CPadOne::ResetRumble(BOOL resetAll)
{
	if (resetAll) {
		for (int i = 0; i < 3; ++i) {
			mRumble[i].flags = mRumble[i].previousFlags = 0;
			mRumble[i].remaining                        = 0;
		}
	} else {
		for (int i = 0; i < 3; ++i) {
			mRumble[i].flags &= ~(RumbleRunning | RumblePaused);
			mRumble[i].previousFlags &= ~(RumbleRunning | RumblePaused);
			mRumble[i].remaining = 0;
		}
	}
	WPADControlMotor(mPort, 0);
}

enum StickDirection {
	StickUp    = 1,
	StickLeft  = 2,
	StickDown  = 4,
	StickRight = 8,
};

// whether the GameCube pad's buttons take part in the repeat groups. Its place here, in front of Update, is
// what puts IsGameCubePadActive's own copy behind ResetRumble as in retail (0x802B1D84)
static BOOL (CPadOne::*sGameCubePadCheck)() const = &CPadOne::IsGameCubePadActive;

void CPadOne::Update(const PADStatus* status, s32 frames)
{
	mSampleCount = KPADRead(mPort, mSamples, ARRAY_SIZE(mSamples));
	u32 device;
	s32 probe = WPADProbe(mPort, &device);
	if ((probe == 0 || probe == -2) && device != 0xfd) {
		if (GetRemoteSpeakerChannel(GetRemoteSpeakerManager(), mPort)->state != 4) {
			return;
		}
	}
	switch (probe) {
	case 0:
		switch (device) {
		case 0:
			mDevices = DeviceRemote;
			break;
		case 1:
			mDevices = DeviceRemote | DeviceNunchuk;
			break;
		case 2:
			mDevices = DeviceRemote | DeviceClassic;
			break;
		case 0xfb:
		case 0xfc:
		case 0xff:
			mDevices = DeviceRemote;
			break;
		case 0xfd:
			mDevices = 0;
			ResetRumble(FALSE);
			break;
		}
		break;
	case -1:
		mDevices = 0;
		ResetRumble(FALSE);
		break;
	case -3:
		ResetRumble(FALSE);
		break;
	}
	if (status->err == PAD_ERR_NONE) {
		mDevices |= DeviceGameCube;
	}
	mProbeResult = probe;
	mDeviceType  = device;

	u64 buttons = 0;
	u32 claimed = 0;
	StickPosition stick, substick;
	if (IsRemoteActive()) {
		u32 hold = mSamples[0].buttons.hold;
		UpdateSwing();
		UpdateRemoteMotion();
		if (hold & WPAD_BUTTON_LEFT) {
			buttons |= 1ULL << RemoteLeft;
		}
		if (hold & WPAD_BUTTON_RIGHT) {
			buttons |= 1ULL << RemoteRight;
		}
		if (hold & WPAD_BUTTON_DOWN) {
			buttons |= 1ULL << RemoteDown;
		}
		if (hold & WPAD_BUTTON_UP) {
			buttons |= 1ULL << RemoteUp;
		}
		if (hold & WPAD_BUTTON_A) {
			buttons |= 1ULL << RemoteA;
		}
		if (hold & WPAD_BUTTON_B) {
			buttons |= 1ULL << RemoteB;
		}
		if (hold & WPAD_BUTTON_1) {
			buttons |= 1ULL << RemoteOne;
		}
		if (hold & WPAD_BUTTON_2) {
			buttons |= 1ULL << RemoteTwo;
		}
		if (hold & WPAD_BUTTON_PLUS) {
			buttons |= 1ULL << RemotePlus;
		}
		if (hold & WPAD_BUTTON_MINUS) {
			buttons |= 1ULL << RemoteMinus;
		}
		if (hold & WPAD_BUTTON_HOME) {
			buttons |= 1ULL << RemoteHome;
		}
		if (IsNunchukActive()) {
			UpdateNunchukMotion();
			if (hold & WPAD_BUTTON_C) {
				buttons |= 1ULL << NunchukC;
			}
			if (hold & WPAD_BUTTON_Z) {
				buttons |= 1ULL << NunchukZ;
			}
			KPADGetUnifiedWpadStatus(mPort, sRawSamples, ARRAY_SIZE(sRawSamples));
			NormalizeWiiStick(&mSamples[0].extension.nunchuk.stick, sRawSamples[0].extension.nunchuk.stickX,
			                  sRawSamples[0].extension.nunchuk.stickY, sStickRanges[0].min, sStickRanges[0].max);
			StickPosition nunchuk = mSamples[0].extension.nunchuk.stick;
			u32 directions        = GetStickDirections(mStickDirections, nunchuk.x, nunchuk.y);
			if (directions & StickUp) {
				buttons |= 1ULL << (NunchukStick + 3);
			}
			if (directions & StickLeft) {
				buttons |= 1ULL << (NunchukStick + 0);
			}
			if (directions & StickDown) {
				buttons |= 1ULL << (NunchukStick + 2);
			}
			if (directions & StickRight) {
				buttons |= 1ULL << (NunchukStick + 1);
			}
			if ((mStick.x = mSamples[0].extension.nunchuk.stick.x) != 0.0f) {
				claimed |= 1;
			}
			if ((mStick.y = mSamples[0].extension.nunchuk.stick.y) != 0.0f) {
				claimed |= 2;
			}
		} else {
			mStick.x = mStick.y = 0.0f;
		}
		mSubStick.x = mSubStick.y = mAnalogL = mAnalogR = 0.0f;
	}
	if (IsClassicPadActive()) {
		u32 hold = mSamples[0].extension.classic.buttons.hold;
		if (hold & WPAD_CL_BUTTON_LEFT) {
			buttons |= 1ULL << ClassicLeft;
		}
		if (hold & WPAD_CL_BUTTON_RIGHT) {
			buttons |= 1ULL << ClassicRight;
		}
		if (hold & WPAD_CL_BUTTON_DOWN) {
			buttons |= 1ULL << ClassicDown;
		}
		if (hold & WPAD_CL_BUTTON_UP) {
			buttons |= 1ULL << ClassicUp;
		}
		if (hold & WPAD_CL_BUTTON_A) {
			buttons |= 1ULL << ClassicA;
		}
		if (hold & WPAD_CL_BUTTON_B) {
			buttons |= 1ULL << ClassicB;
		}
		if (hold & WPAD_CL_BUTTON_X) {
			buttons |= 1ULL << ClassicX;
		}
		if (hold & WPAD_CL_BUTTON_Y) {
			buttons |= 1ULL << ClassicY;
		}
		if (hold & WPAD_CL_BUTTON_PLUS) {
			buttons |= 1ULL << ClassicPlus;
		}
		if (hold & WPAD_CL_BUTTON_MINUS) {
			buttons |= 1ULL << ClassicMinus;
		}
		if (hold & WPAD_CL_BUTTON_HOME) {
			buttons |= 1ULL << ClassicHome;
		}
		if (hold & WPAD_CL_TRIGGER_L) {
			buttons |= 1ULL << ClassicL;
		}
		if (hold & WPAD_CL_TRIGGER_R) {
			buttons |= 1ULL << ClassicR;
		}
		if (hold & WPAD_CL_TRIGGER_ZL) {
			buttons |= 1ULL << ClassicZL;
		}
		if (hold & WPAD_CL_TRIGGER_ZR) {
			buttons |= 1ULL << ClassicZR;
		}
	}
	if (IsGameCubePadActive()) {
		u16 hold = status->button;
		if (hold & PAD_BUTTON_LEFT) {
			buttons |= 1ULL << (GameCubeDPad + 0);
		}
		if (hold & PAD_BUTTON_RIGHT) {
			buttons |= 1ULL << (GameCubeDPad + 1);
		}
		if (hold & PAD_BUTTON_DOWN) {
			buttons |= 1ULL << (GameCubeDPad + 2);
		}
		if (hold & PAD_BUTTON_UP) {
			buttons |= 1ULL << (GameCubeDPad + 3);
		}
		if (hold & PAD_BUTTON_A) {
			buttons |= 1ULL << GameCubeA;
		}
		if (hold & PAD_BUTTON_B) {
			buttons |= 1ULL << GameCubeB;
		}
		if (hold & PAD_BUTTON_X) {
			buttons |= 1ULL << GameCubeX;
		}
		if (hold & PAD_BUTTON_Y) {
			buttons |= 1ULL << GameCubeY;
		}
		if (hold & PAD_TRIGGER_Z) {
			buttons |= 1ULL << GameCubeZ;
		}
		if (hold & PAD_TRIGGER_R) {
			buttons |= 1ULL << GameCubeR;
		}
		if (hold & PAD_TRIGGER_L) {
			buttons |= 1ULL << GameCubeL;
		}
		if (hold & PAD_BUTTON_START) {
			buttons |= 1ULL << GameCubeStart;
		}
		NormalizeWiiStick(&stick, status->stickX, status->stickY, sStickRanges[0].min, sStickRanges[0].max);
		NormalizeWiiStick(&substick, status->substickX, status->substickY, sStickRanges[1].min, sStickRanges[1].max);
		u32 directions = GetStickDirections(mStickDirections, stick.x, stick.y);
		if (directions & StickUp) {
			buttons |= 1ULL << (GameCubeStick + 3);
		}
		if (directions & StickLeft) {
			buttons |= 1ULL << (GameCubeStick + 0);
		}
		if (directions & StickDown) {
			buttons |= 1ULL << (GameCubeStick + 2);
		}
		if (directions & StickRight) {
			buttons |= 1ULL << (GameCubeStick + 1);
		}
		directions = GetStickDirections(mStickDirections, substick.x, substick.y);
		if (directions & StickUp) {
			buttons |= 1ULL << (GameCubeSubStick + 3);
		}
		if (directions & StickLeft) {
			buttons |= 1ULL << (GameCubeSubStick + 0);
		}
		if (directions & StickDown) {
			buttons |= 1ULL << (GameCubeSubStick + 2);
		}
		if (directions & StickRight) {
			buttons |= 1ULL << (GameCubeSubStick + 1);
		}
		if (!(claimed & 1)) {
			mStick.x = stick.x;
		}
		if (!(claimed & 2)) {
			mStick.y = stick.y;
		}
		if (!(claimed & 4)) {
			mSubStick.x = substick.x;
		}
		if (!(claimed & 8)) {
			mSubStick.y = substick.y;
		}
		if (!(claimed & 16)) {
			mAnalogL = status->triggerLeft;
		}
		if (!(claimed & 32)) {
			mAnalogR = status->triggerRight;
		}
	}

	mButton = mDirButton = buttons;
	if (mButton & ((1ULL << (NunchukStick + 3)) | (1ULL << (RemoteDirections2 + 3)))) {
		mDirButton |= 1ULL << RemoteUp;
	}
	if (mButton & ((1ULL << (NunchukStick + 2)) | (1ULL << (RemoteDirections2 + 2)))) {
		mDirButton |= 1ULL << RemoteDown;
	}
	if (mButton & ((1ULL << (NunchukStick + 0)) | (1ULL << (RemoteDirections2 + 0)))) {
		mDirButton |= 1ULL << RemoteLeft;
	}
	if (mButton & ((1ULL << (NunchukStick + 1)) | (1ULL << (RemoteDirections2 + 1)))) {
		mDirButton |= 1ULL << RemoteRight;
	}
	if (IsGameCubePadActive()) {
		if (mButton & ((1ULL << (GameCubeStick + 3)) | (1ULL << (GameCubeSubStick + 3)))) {
			mDirButton |= 1ULL << (GameCubeDPad + 3);
		}
		if (mButton & ((1ULL << (GameCubeStick + 2)) | (1ULL << (GameCubeSubStick + 2)))) {
			mDirButton |= 1ULL << (GameCubeDPad + 2);
		}
		if (mButton & ((1ULL << (GameCubeStick + 0)) | (1ULL << (GameCubeSubStick + 0)))) {
			mDirButton |= 1ULL << (GameCubeDPad + 0);
		}
		if (mButton & ((1ULL << (GameCubeStick + 1)) | (1ULL << (GameCubeSubStick + 1)))) {
			mDirButton |= 1ULL << (GameCubeDPad + 1);
		}
	}
	mTrigger = mRepeat = (mButton ^ mLastButton) & ~mLastButton;
	mDirTrigger = mDirRepeat = (mDirButton ^ mLastDirButton) & ~mLastDirButton;
	const int repeatGroups   = (sGameCubePadCheck != NULL) ? 16 : 8;
	for (int i = 0; i < repeatGroups; ++i) {
		if (mRepeatSettings[i].startingInterval <= 0) {
			continue;
		}
		if ((mButton | mDirButton) & sRepeatGroups[i]) {
			if ((mRepeatTimer[i] -= frames) <= 0) {
				if (mButton & sRepeatGroups[i]) {
					mRepeat |= mButton & sRepeatGroups[i];
				}
				if (mDirButton & sRepeatGroups[i]) {
					mDirRepeat |= mDirButton & sRepeatGroups[i];
				}
				mRepeatTimer[i] = mRepeatSettings[i].interval;
				if (mRepeatSettings[i].interval > mRepeatSettings[i].minimumInterval) {
					--mRepeatSettings[i].interval;
				}
			}
		} else {
			mRepeatTimer[i]             = mRepeatSettings[i].delay;
			mRepeatSettings[i].interval = mRepeatSettings[i].startingInterval;
		}
	}
	mLastButton    = mButton;
	mLastDirButton = mDirButton;

	if (mSamples[0].error == 0) {
		for (int i = 0; i < 3; ++i) {
			if (mRumble[i].IsEnabled()) {
				if (i == 2 && (mRumble[i].flags & RumblePaused)) {
					mRumble[i].flags &= ~RumblePaused;
				}
				if ((mRumble[i].flags & RumbleRunning) && (mRumble[i].flags & RumblePaused) && !(mRumble[i].previousFlags & RumblePaused)) {
					WPADControlMotor(mPort, 0);
				} else if ((mRumble[i].flags & RumbleRunning) && !(mRumble[i].flags & RumblePaused)
				           && (mRumble[i].previousFlags & RumblePaused)) {
					WPADControlMotor(mPort, 1);
				}
				if ((mRumble[i].flags & RumbleRunning) && !(mRumble[i].flags & RumblePaused) && (mRumble[i].remaining -= frames) <= 0) {
					mRumble[i].remaining = 0;
					mRumble[i].flags &= ~RumbleRunning;
				}
				if ((mRumble[i].flags & RumbleRunning) && !(mRumble[i].previousFlags & RumbleRunning)) {
					WPADControlMotor(mPort, 1);
				} else if (!(mRumble[i].flags & RumbleRunning) && (mRumble[i].previousFlags & RumbleRunning)) {
					WPADControlMotor(mPort, 0);
				}
			}
			if (mRumble[i].flags != mRumble[i].previousFlags) {
				mRumble[i].previousFlags = mRumble[i].flags;
			}
		}
	}
}

u32 CPadOne::GetStickDirections(StickDirectionSettings& settings, f32 x, f32 y) const
{
	u32 result = 0;
	f32 length = StickLength(x, y);
	if (100.0f * length >= settings.thresholdPercent) {
		f32 angle = -(f32)atan2(x, y);
		if (angle < 0.0f) {
			angle += 6.2831855f;
		}
		angle = 57.295776f * angle;
		if (angle < 45.0f + settings.diagonalOverlapDegrees / 2.0f || angle > 315.0f - settings.diagonalOverlapDegrees / 2.0f) {
			result |= StickUp;
		}
		if (angle > 45.0f - settings.diagonalOverlapDegrees / 2.0f && angle < 135.0f + settings.diagonalOverlapDegrees / 2.0f) {
			result |= StickLeft;
		}
		if (angle > 135.0f - settings.diagonalOverlapDegrees / 2.0f && angle < 225.0f + settings.diagonalOverlapDegrees / 2.0f) {
			result |= StickDown;
		}
		if (angle > 225.0f - settings.diagonalOverlapDegrees / 2.0f && angle < 315.0f + settings.diagonalOverlapDegrees / 2.0f) {
			result |= StickRight;
		}
	}
	return result;
}

void CPadOne::SetConnectCallback()
{
	KPADSetConnectCallback(mPort, OnPadConnection);
}

void CPadOne::EnableRemoteSpeaker()
{
	EnableWiiRemoteSpeaker(GetRemoteSpeakerChannel(GetRemoteSpeakerManager(), mPort), SpeakerEnabled);
}

void CPadOne::DisableRemoteSpeaker()
{
	DisableWiiRemoteSpeaker(GetRemoteSpeakerChannel(GetRemoteSpeakerManager(), mPort), SpeakerDisabled);
}

static void SpeakerEnabled(s32, s32)
{
}

static void SpeakerDisabled(s32, s32)
{
}

static void OnPadConnection(s32 channel, s32 result)
{
	if (result == 0) {
		EnableWiiRemoteSpeaker(GetRemoteSpeakerChannel(GetRemoteSpeakerManager(), channel), SpeakerEnabled);
	} else if (result == -1) {
		DisableWiiRemoteSpeaker(GetRemoteSpeakerChannel(GetRemoteSpeakerManager(), channel), SpeakerDisabled);
	}
}

extern "C" f64 sqrt(f64 x);

BOOL CPadOne::GetSwing(Vec* direction, f32* strength)
{
	if (!mSwing.previouslyRecognized && mSwing.recognized == TRUE) {
		if (direction) {
			direction->x = mSwing.direction.x;
			direction->y = mSwing.direction.y;
			direction->z = mSwing.direction.z;
		}
		if (strength) {
			*strength = mSwing.strength;
		}
		return TRUE;
	}
	return FALSE;
}

void CPadOne::ResetSwing()
{
	for (int i = 0; i < 30; ++i) {
		mSwing.acceleration[i].x = mSwing.acceleration[i].y = mSwing.acceleration[i].z = 0.0f;
		mSwing.magnitude[i]                                                            = 0.0f;
		mSwing.change[i]                                                               = 0.0f;
	}
	mSwing.count                = 0;
	mSwing.started              = FALSE;
	mSwing.previouslyStarted    = FALSE;
	mSwing.recognized           = FALSE;
	mSwing.previouslyRecognized = FALSE;
	mSwing.timedOut             = FALSE;
	mSwing.timing               = FALSE;
	mSwing.elapsed              = 0;
	mSwing.timeout              = 0;
}

void CPadOne::AppendSwingSample(s32 index)
{
	if (mSwing.count < 10) {
		mSwing.acceleration[mSwing.count] = mSamples[index].acceleration.value;
		mSwing.magnitude[mSwing.count]    = mSamples[index].acceleration.magnitude;
		mSwing.change[mSwing.count]       = mSamples[index].acceleration.change;
		++mSwing.count;
	} else {
		int dst;
		for (dst = 0; dst < mSwing.count - 1; ++dst) {
			mSwing.acceleration[dst] = mSwing.acceleration[dst + 1];
			mSwing.magnitude[dst]    = mSwing.magnitude[dst + 1];
			mSwing.change[dst]       = mSwing.change[dst + 1];
		}
		mSwing.acceleration[dst] = mSamples[index].acceleration.value;
		mSwing.magnitude[dst]    = mSamples[index].acceleration.magnitude;
		mSwing.change[dst]       = mSamples[index].acceleration.change;
	}
	if (mSwing.count >= 30) {
		mSwing.count = 29;
	}
}

void CPadOne::UpdateSwing()
{
	if (mSwing.timing && ++mSwing.elapsed >= mSwing.timeout) {
		mSwing.timing   = FALSE;
		mSwing.elapsed  = 0;
		mSwing.timedOut = TRUE;
	}
	mSwing.previouslyStarted    = mSwing.started;
	mSwing.previouslyRecognized = mSwing.recognized;
	DetectSwing();
	if (mSwing.previouslyStarted != mSwing.started && mSwing.started == TRUE) {
		CalculateSwing(0, mSwing.count - 1);
	}
}

void CPadOne::DetectSwing()
{
	// These counters are shared by the ports in the Wii executable.
	static s32 waitingSamples;
	static s32 quietSamples;
	for (int i = mSampleCount - 1; i >= 0; --i) {
		AppendSwingSample(i);
		if (!mSwing.recognized) {
			if (!mSwing.started) {
				Vec delta;
				delta.x = mSwing.acceleration[mSwing.count - 1].x - mSwing.acceleration[0].x;
				delta.y = mSwing.acceleration[mSwing.count - 1].y - mSwing.acceleration[0].y;
				if (delta.x >= 0.2f || delta.x <= -0.2f || delta.y >= 0.2f || delta.y <= -0.2f) {
					mSwing.started = TRUE;
					break;
				}
			} else if (++waitingSamples < 40) {
				if (mSwing.acceleration[mSwing.count - 1].z >= 1.1f) {
					waitingSamples    = 0;
					mSwing.recognized = TRUE;
					mSwing.timing     = TRUE;
					mSwing.elapsed    = 0;
					mSwing.timeout    = 15;
					break;
				}
			} else {
				waitingSamples = 0;
				mSwing.started = FALSE;
				mSwing.timing  = FALSE;
				break;
			}
		} else {
			if (mSwing.magnitude[mSwing.count - 1] < 1.1f) {
				if (++quietSamples >= 5) {
					quietSamples      = 0;
					mSwing.recognized = FALSE;
					mSwing.started    = FALSE;
					mSwing.timing     = FALSE;
					break;
				}
			} else {
				quietSamples = 0;
			}
			if (mSwing.timedOut) {
				mSwing.timedOut   = FALSE;
				quietSamples      = 0;
				mSwing.recognized = FALSE;
				mSwing.started    = FALSE;
				mSwing.timing     = FALSE;
				break;
			}
		}
	}
}

void CPadOne::CalculateSwing(s32 first, s32 last)
{
	Vec delta;
	Vec movement;
	Vec origin;
	movement.x = movement.y = movement.z = 0.0f;
	origin.x = origin.y = origin.z = 0.0f;
	mSwing.direction.x = mSwing.direction.y = mSwing.direction.z = 0.0f;
	mSwing.strength                                              = 0.0f;
	for (int i = first + 1; i <= last; ++i) {
		delta.x = mSwing.acceleration[i].x - mSwing.acceleration[i - 1].x;
		delta.y = mSwing.acceleration[i].y - mSwing.acceleration[i - 1].y;
		delta.z = 0.0f;
		movement.x += delta.x * (f32)sqrt(delta.x * delta.x + delta.y * delta.y);
		movement.y += delta.y * (f32)sqrt(delta.x * delta.x + delta.y * delta.y);
		mSwing.strength += mSwing.magnitude[i] + mSwing.change[i];
	}
	origin.x = mSwing.acceleration[0].x;
	origin.y = mSwing.acceleration[0].y;
	if (movement.x != 0.0f || (movement.y != 0.0f && origin.x != 0.0f) || origin.y != 0.0f) {
		PSVECNormalize(&movement, &movement);
		PSVECNormalize(&origin, &origin);
		mSwing.direction.x = -origin.y * movement.x + origin.x * movement.y;
		mSwing.direction.y = -origin.x * movement.x - origin.y * movement.y;
	}
}

static inline void AppendChange(WiiMotionWindow& motion, f32 change)
{
	motion.changes[motion.count] = change;
	++motion.count;
	if (motion.count >= ARRAY_SIZE(motion.changes)) {
		motion.count = ARRAY_SIZE(motion.changes) - 1;
	}
}

void CPadOne::ResetRemoteMotion()
{
	for (unsigned int i = 0; i < ARRAY_SIZE(mRemoteMotion.changes); ++i) {
		mRemoteMotion.changes[i] = 0.0f;
	}
	mRemoteMotion.count            = 0;
	mRemoteMotion.previouslyActive = mRemoteMotion.active = FALSE;
	mRemoteMotion.activeFrames                            = 1;
}

void CPadOne::CollectRemoteMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		AppendChange(mRemoteMotion, mSamples[i].acceleration.change);
	}
}

// every 20th active frame the window's changes are summed up and the window starts over
void CPadOne::UpdateRemoteMotion()
{
	DetectRemoteMotion();
	if (mRemoteMotion.active == TRUE) {
		CollectRemoteMotion();
		if (++mRemoteMotion.activeFrames >= 20) {
			mRemoteMotion.activeFrames = 1;
			mRemoteMotion.sum          = 0.0f;
			for (u32 i = 0; i < mRemoteMotion.count; ++i) {
				mRemoteMotion.sum += mRemoteMotion.changes[i];
			}
			mRemoteMotion.count = 0;
		}
	}
}

void CPadOne::DetectRemoteMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		f32 change = mSamples[i].acceleration.change;
		if (change >= 0.2f) {
			mRemoteMotion.previouslyActive = mRemoteMotion.active;
			mRemoteMotion.active           = TRUE;
			return;
		}
		if (change < 0.1f) {
			if (++mRemoteMotion.quietSamples >= 15) {
				mRemoteMotion.previouslyActive = mRemoteMotion.active;
				mRemoteMotion.active           = FALSE;
				return;
			}
		} else {
			mRemoteMotion.quietSamples = 0;
		}
	}
}

void CPadOne::ResetNunchukMotion()
{
	for (unsigned int i = 0; i < ARRAY_SIZE(mNunchukMotion.changes); ++i) {
		mNunchukMotion.changes[i] = 0.0f;
	}
	mNunchukMotion.count            = 0;
	mNunchukMotion.previouslyActive = mNunchukMotion.active = FALSE;
	mNunchukMotion.activeFrames                             = 1;
}

void CPadOne::CollectNunchukMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		AppendChange(mNunchukMotion, mSamples[i].extension.nunchuk.acceleration.change);
	}
}

// every 20th active frame the window's changes are summed up and the window starts over
void CPadOne::UpdateNunchukMotion()
{
	DetectNunchukMotion();
	if (mNunchukMotion.active == TRUE) {
		CollectNunchukMotion();
		if (++mNunchukMotion.activeFrames >= 20) {
			mNunchukMotion.activeFrames = 1;
			mNunchukMotion.sum          = 0.0f;
			for (u32 i = 0; i < mNunchukMotion.count; ++i) {
				mNunchukMotion.sum += mNunchukMotion.changes[i];
			}
			mNunchukMotion.count = 0;
		}
	}
}

void CPadOne::DetectNunchukMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		f32 change = mSamples[i].extension.nunchuk.acceleration.change;
		if (change >= 0.2f) {
			mNunchukMotion.previouslyActive = mNunchukMotion.active;
			mNunchukMotion.active           = TRUE;
			return;
		}
		if (change < 0.1f) {
			if (++mNunchukMotion.quietSamples >= 15) {
				mNunchukMotion.previouslyActive = mNunchukMotion.active;
				mNunchukMotion.active           = FALSE;
				return;
			}
		} else {
			mNunchukMotion.quietSamples = 0;
		}
	}
}

static void* AllocatePadMemory(u32 size)
{
	return MEMAllocFromAllocator(&gWiiPadAllocator, size);
}

static BOOL FreePadMemory(void* object)
{
	MEMFreeToAllocator(&gWiiPadAllocator, object);
	return TRUE;
}

CPadOne gWiiPads[4];
PADStatus gWiiGameCubePads[4];

void CPadEx::Init(u8 inputMask)
{
	mInputMask = inputMask;
	if (mInputMask & InputGameCube) {
		PADInit();
	}
	WPADRegisterAllocator(AllocatePadMemory, FreePadMemory);
	KPADInit();
	KPADDisableStickCrossClamp();
	SetMainStickRange(15.0f, 71.0f);
	SetSubStickRange(15.0f, 60.0f);
	for (int port = 0; port < 4; ++port) {
		gWiiPads[port].Init(port, inputMask);
	}
	Clear();
	KPADSetObjInterval(0.2f);
}

void CPadEx::Clear()
{
	if (mInputMask & InputGameCube) {
		PADRecalibrate(0xf0000000);
	}
	KPADReset();
	for (int port = 0; port < 4; ++port) {
		gWiiPads[port].Clear();
	}
}

void CPadEx::ResetRumble(BOOL resetAll)
{
	if (resetAll) {
		mMotorEnabled = WPADIsMotorEnabled();
	}
	for (int port = 0; port < 4; ++port) {
		gWiiPads[port].ResetRumble(resetAll);
		for (int i = 0; i < 3; ++i) {
			if (mMotorEnabled) {
				gWiiPads[port].mRumble[i].flags |= RumbleEnabled;
			} else {
				gWiiPads[port].mRumble[i].flags &= ~RumbleEnabled;
			}
		}
	}
}

void CPadEx::Update(s32 frames)
{
	if (mInputMask & InputGameCube) {
		PADRead(gWiiGameCubePads);
	}
	for (u8 port = 0; port < 4; ++port) {
		gWiiPads[port].Update(&gWiiGameCubePads[port], frames);
	}
}

void CPadEx::SetMainStickRange(f32 inner, f32 outer)
{
	sStickRanges[0].min = inner;
	sStickRanges[0].max = outer;
	KPADSetFSStickClamp((s8)sStickRanges[0].min, (s8)sStickRanges[0].max);
}

void CPadEx::SetSubStickRange(f32 inner, f32 outer)
{
	sStickRanges[1].min = inner;
	sStickRanges[1].max = outer;
}
