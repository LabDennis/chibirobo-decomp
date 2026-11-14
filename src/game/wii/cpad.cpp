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

struct RemoteSpeakerManager;
// Prefix read by the input code. Channel selection and ownership stay with the speaker manager.
struct RemoteSpeakerChannel {
	u8 flags[8];
	s32 state;
};

extern "C" {
s32 KPADRead(s32 channel, WiiPadSample* out, u32 count);
void KPADGetUnifiedWpadStatus(s32 channel, WiiRawSample* out, u32 count);
s32 WPADProbe(s32 channel, u32* type);
void WPADControlMotor(s32 channel, u32 command);
RemoteSpeakerManager* GetRemoteSpeakerManager();
RemoteSpeakerChannel* GetRemoteSpeakerChannel(RemoteSpeakerManager*, s32 channel);
void NormalizeWiiStick(StickPosition*, f32 x, f32 y, f32 innerRadius, f32 outerRadius);
}

struct WiiStickRange {
	f32 min, max;
};
static WiiStickRange sStickRanges[2];
static WiiRawSample sRawSamples[16];
static BOOL (CPadOne::*sGameCubePadCheck)() const = &CPadOne::IsGameCubePadActive;

enum {
	RumbleEnabled = 1,
	RumbleRunning = 2,
	RumblePaused  = 4
};

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

void CPadOne::Clear()
{
	mButton     = mDirButton = 0;
	mLastButton = mLastDirButton = 0;
	mTrigger = mDirTrigger = mRepeat = mDirRepeat = 0;
	for (int i = 0; i < 16; ++i) {
		mRepeatSettings[i].delay = 24;
		mRepeatSettings[i].interval = mRepeatSettings[i].startingInterval = 6;
		mRepeatSettings[i].minimumInterval                              = 6;
	}
	for (int i = 0; i < 16; ++i) {
		mRepeatTimer[i] = 24;
	}
	mStick.x    = mStick.y = 0.0f;
	mSubStick.x = mSubStick.y = 0.0f;
	mAnalogL    = mAnalogR = 0.0f;
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
			mRumble[i].remaining                       = 0;
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
	// NONMATCHING: the probe/device dispatch, frame layout and register
	// allocation still differ from retail even though the unit is built with
	// the matching GC/3.0a5.2 profile; remaining diffs are scheduling and
	// local-placement residue, not missing cases.
	switch (probe) {
	case -1:
		mDevices = 0;
		ResetRumble(FALSE);
		break;
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
			mDevices = DeviceRemote;
			break;
		case 0xfc:
			mDevices = DeviceRemote;
			break;
		case 0xfd:
			mDevices = 0;
			ResetRumble(FALSE);
			break;
		case 0xff:
			mDevices = DeviceRemote;
			break;
		}
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
			StickPosition& stick = mSamples[0].extension.nunchuk.stick;
			NormalizeWiiStick(&stick, sRawSamples[0].extension.nunchuk.stickX, sRawSamples[0].extension.nunchuk.stickY, sStickRanges[0].min,
			                  sStickRanges[0].max);
			u32 directions = GetStickDirections(mStickDirections, stick.x, stick.y);
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
			mStick.x = stick.x;
			if (stick.x != 0.0f) {
				claimed |= 1;
			}
			mStick.y = stick.y;
			if (stick.y != 0.0f) {
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
		StickPosition stick, substick;
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
	mTrigger = mRepeat = mButton & ~mLastButton;
	mDirTrigger = mDirRepeat = mDirButton & ~mLastDirButton;
	const int repeatGroups = (sGameCubePadCheck != NULL) ? 16 : 8;
	for (int i = 0; i < repeatGroups; ++i) {
		WiiButtonRepeat& settings = mRepeatSettings[i];
		if (settings.startingInterval <= 0) {
			continue;
		}
		u64 mask = sRepeatGroups[i];
		if ((mButton | mDirButton) & mask) {
			if ((mRepeatTimer[i] -= frames) <= 0) {
				if (mButton & mask) {
					mRepeat |= mButton & mask;
				}
				if (mDirButton & mask) {
					mDirRepeat |= mDirButton & mask;
				}
				mRepeatTimer[i] = settings.interval;
				if (settings.interval > settings.minimumInterval) {
					--settings.interval;
				}
			}
		} else {
			mRepeatTimer[i]   = settings.delay;
			settings.interval = settings.startingInterval;
		}
	}
	mLastButton    = mButton;
	mLastDirButton = mDirButton;

	if (mSamples[0].error == 0) {
		for (int i = 0; i < 3; ++i) {
			WiiRumble& rumble = mRumble[i];
			if (rumble.flags & RumbleEnabled) {
				if (i == 2 && (rumble.flags & RumblePaused)) {
					rumble.flags &= ~(RumbleRunning | RumblePaused);
				}
				if ((rumble.flags & RumbleRunning) && (rumble.flags & RumblePaused) && !(rumble.previousFlags & RumblePaused)) {
					WPADControlMotor(mPort, 0);
				}
				if ((rumble.flags & RumbleRunning) && !(rumble.flags & RumblePaused) && (rumble.previousFlags & RumblePaused)) {
					WPADControlMotor(mPort, 1);
				}
				if ((rumble.flags & RumbleRunning) && !(rumble.flags & RumblePaused) && (rumble.remaining -= frames) <= 0) {
					rumble.remaining = 0;
					rumble.flags &= ~RumbleRunning;
				}
				if ((rumble.flags & RumbleRunning) && !(rumble.previousFlags & RumbleRunning)) {
					WPADControlMotor(mPort, 1);
				}
				if (!(rumble.flags & RumbleRunning) && (rumble.previousFlags & RumbleRunning)) {
					WPADControlMotor(mPort, 0);
				}
			}
			if (rumble.flags != rumble.previousFlags) {
				rumble.previousFlags = rumble.flags;
			}
		}
	}
}
extern "C" {
extern f32 WiiReciprocalSquareRoot(f32 value);
extern void WiiReportMathError(const char* file, int line, const char* format, ...);
extern f64 atan2(f64 y, f64 x);

static f32 StickLength(f32 x, f32 y)
{
	f32 squared = x * x + y * y;
	if (!(squared >= 0.0f)) {
		WiiReportMathError("arithmetic.h", 627, "FSqrt: Input is out of the domain.");
	}
	if (squared <= 0.0f) {
		return 0.0f;
	}
	return squared * WiiReciprocalSquareRoot(squared);
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

u32 CPadOne::GetStickDirections(const StickDirectionSettings& settings, f32 x, f32 y) const
{
	u32 result = 0;
	f32 length = StickLength(x, y);
	if (100.0f * length >= settings.thresholdPercent) {
		f32 angle = -(f32)atan2(x, y);
		if (angle < 0.0f) {
			angle += 6.2831855f;
		}
		angle *= 57.295776f;
		if (angle < 45.0f + settings.diagonalOverlapDegrees * 0.5f || angle > 315.0f - settings.diagonalOverlapDegrees * 0.5f) {
			result |= StickUp;
		}
		if (angle > 45.0f - settings.diagonalOverlapDegrees * 0.5f && angle < 135.0f + settings.diagonalOverlapDegrees * 0.5f) {
			result |= StickLeft;
		}
		if (angle > 135.0f - settings.diagonalOverlapDegrees * 0.5f && angle < 225.0f + settings.diagonalOverlapDegrees * 0.5f) {
			result |= StickDown;
		}
		if (angle > 225.0f - settings.diagonalOverlapDegrees * 0.5f && angle < 315.0f + settings.diagonalOverlapDegrees * 0.5f) {
			result |= StickRight;
		}
	}
	return result;
}

extern "C" f64 sqrt(f64 x);

BOOL CPadOne::GetSwing(Vec* direction, f32* strength) const
{
	if (mSwing.previouslyRecognized || mSwing.recognized != TRUE) {
		return FALSE;
	}
	if (direction) {
		*direction = mSwing.direction;
	}
	if (strength) {
		*strength = mSwing.strength;
	}
	return TRUE;
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
	int dst;
	if (mSwing.count < 10) {
		dst = mSwing.count++;
	} else {
		for (dst = 0; dst < mSwing.count - 1; ++dst) {
			mSwing.acceleration[dst] = mSwing.acceleration[dst + 1];
			mSwing.magnitude[dst]    = mSwing.magnitude[dst + 1];
			mSwing.change[dst]       = mSwing.change[dst + 1];
		}
	}
	const WiiAcceleration& sample = mSamples[index].acceleration;
	mSwing.acceleration[dst]      = sample.value;
	mSwing.magnitude[dst]         = sample.magnitude;
	mSwing.change[dst]            = sample.change;
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
	if (mSwing.started != mSwing.previouslyStarted && mSwing.started == TRUE) {
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
		const int last = mSwing.count - 1;
		if (!mSwing.recognized) {
			if (!mSwing.started) {
				f32 x = mSwing.acceleration[last].x - mSwing.acceleration[0].x;
				f32 y = mSwing.acceleration[last].y - mSwing.acceleration[0].y;
				if (x >= 0.2f || x <= -0.2f || y >= 0.2f || y <= -0.2f) {
					mSwing.started = TRUE;
					break;
				}
			} else if (++waitingSamples >= 40) {
				waitingSamples = 0;
				mSwing.started = FALSE;
				mSwing.timing  = FALSE;
				break;
			} else if (mSwing.acceleration[last].z >= 1.1f) {
				waitingSamples    = 0;
				mSwing.recognized = TRUE;
				mSwing.timing     = TRUE;
				mSwing.elapsed    = 0;
				mSwing.timeout    = 15;
				break;
			}
		} else {
			if (mSwing.magnitude[last] < 1.1f) {
				if (++quietSamples >= 5) {
					quietSamples      = 0;
					mSwing.recognized = mSwing.started = mSwing.timing = FALSE;
					break;
				}
			} else {
				quietSamples = 0;
			}
			if (mSwing.timedOut) {
				mSwing.timedOut   = FALSE;
				quietSamples      = 0;
				mSwing.recognized = mSwing.started = mSwing.timing = FALSE;
				break;
			}
		}
	}
}

void CPadOne::CalculateSwing(s32 first, s32 last)
{
	Vec movement       = { 0.0f, 0.0f, 0.0f };
	Vec origin         = { 0.0f, 0.0f, 0.0f };
	mSwing.direction.x = mSwing.direction.y = mSwing.direction.z = mSwing.strength = 0.0f;
	for (int i = first + 1; i <= last; ++i) {
		f32 x      = mSwing.acceleration[i].x - mSwing.acceleration[i - 1].x;
		f32 y      = mSwing.acceleration[i].y - mSwing.acceleration[i - 1].y;
		f32 length = (f32)sqrt(x * x + y * y);
		movement.x += x * length;
		movement.y += y * length;
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

static inline void ResetMotion(WiiMotionWindow& motion)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(motion.changes); ++i) {
		motion.changes[i] = 0.0f;
	}
	motion.count  = 0;
	motion.active = motion.previouslyActive = FALSE;
	motion.activeFrames                     = 1;
}

static inline void AppendChange(WiiMotionWindow& motion, f32 change)
{
	motion.changes[motion.count++] = change;
	if (motion.count >= ARRAY_SIZE(motion.changes)) {
		motion.count = ARRAY_SIZE(motion.changes) - 1;
	}
}

static inline bool DetectChange(WiiMotionWindow& motion, f32 change)
{
	if (change >= 0.2f) {
		motion.previouslyActive = motion.active;
		motion.active           = TRUE;
		return true;
	}
	if (change < 0.1f) {
		if (++motion.quietSamples >= 15) {
			motion.previouslyActive = motion.active;
			motion.active           = FALSE;
			return true;
		}
	} else {
		motion.quietSamples = 0;
	}
	return false;
}

static inline void FinishMotionFrame(WiiMotionWindow& motion)
{
	if (++motion.activeFrames >= 20) {
		motion.activeFrames = 1;
		motion.sum          = 0.0f;
		for (u32 i = 0; i < motion.count; ++i) {
			motion.sum += motion.changes[i];
		}
		motion.count = 0;
	}
}

void CPadOne::ResetRemoteMotion()
{
	ResetMotion(mRemoteMotion);
}

void CPadOne::CollectRemoteMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		AppendChange(mRemoteMotion, mSamples[i].acceleration.change);
	}
}

void CPadOne::UpdateRemoteMotion()
{
	DetectRemoteMotion();
	if (mRemoteMotion.active == TRUE) {
		CollectRemoteMotion();
		FinishMotionFrame(mRemoteMotion);
	}
}

void CPadOne::DetectRemoteMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		if (DetectChange(mRemoteMotion, mSamples[i].acceleration.change)) {
			break;
		}
	}
}

void CPadOne::ResetNunchukMotion()
{
	ResetMotion(mNunchukMotion);
}

void CPadOne::CollectNunchukMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		AppendChange(mNunchukMotion, mSamples[i].extension.nunchuk.acceleration.change);
	}
}

void CPadOne::UpdateNunchukMotion()
{
	DetectNunchukMotion();
	if (mNunchukMotion.active == TRUE) {
		CollectNunchukMotion();
		FinishMotionFrame(mNunchukMotion);
	}
}

void CPadOne::DetectNunchukMotion()
{
	for (int i = mSampleCount - 1; i >= 0; --i) {
		if (DetectChange(mNunchukMotion, mSamples[i].extension.nunchuk.acceleration.change)) {
			break;
		}
	}
}

extern "C" {
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

CPadOne::CPadOne()
{
	ResetSwing();
	ResetRemoteMotion();
	ResetNunchukMotion();
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
		PADReset(0xf0000000);
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
		CPadOne& pad = gWiiPads[port];
		pad.ResetRumble(resetAll);
		for (int i = 0; i < 3; ++i) {
			if (mMotorEnabled) {
				pad.mRumble[i].flags |= RumbleEnabled;
			} else {
				pad.mRumble[i].flags &= ~RumbleEnabled;
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
