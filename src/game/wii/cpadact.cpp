#include "game/wii/cpadact.h"
#include "game/wii/cgame.h"
#include <math.h>

extern "C" {
extern s32 gPointerScreenWidth, gPointerScreenHeight;
extern s32 gScreenLeft, gScreenRight, gScreenTop, gScreenBottom;
extern s32 gScreenSafeRight, gScreenSafeBottom, gRefreshRate;
void KPADSetConnectCallback(s32 channel, void (*callback)(s32, s32));
void fn_801EA484(s32 channel);
}

static void SecondaryRemoteConnected(s32 channel, s32 result)
{
	if (result == 0) {
		fn_801EA484(channel);
	}
}

// NONMATCHING: identical instruction multiset; only FP register naming and
// constant-load scheduling differ.
void CPadAct::Init()
{
	mFlags = 0;
	for (int i = 0; i < 5; ++i) {
		mRemoteAcceleration[i].x = mRemoteAcceleration[i].y = mRemoteAcceleration[i].z = 0.0f;
		mNunchukAcceleration[i].x = mNunchukAcceleration[i].y = mNunchukAcceleration[i].z = 0.0f;
	}
	const f32 right  = gScreenRight - 16.0f;
	const f32 left   = gScreenLeft + 16.0f;
	const f32 bottom = gScreenBottom - 16.0f;
	const f32 top    = gScreenTop + 16.0f;
	mRight           = right;
	mLeft            = left;
	mBottom          = bottom;
	mTop             = top;
	mPointer.x       = gScreenSafeRight - 12;
	mPointer.y       = gScreenSafeBottom - 12;
	mPointerAngle = mPointer.z = 0.0f;
	mPointerScale = mPointerFade = mPointerOpacity = 0.0f;
	mInBounds = mPointerValid = FALSE;
	mTransitionFrames         = 0;
	mPointerSeen              = false;
	mFadeFrames               = 0;
	mTransitionCooldown       = 0.0f;
	mStablePointer            = mPointer;
	KPADSetConnectCallback(1, SecondaryRemoteConnected);
	KPADSetConnectCallback(2, SecondaryRemoteConnected);
	KPADSetConnectCallback(3, SecondaryRemoteConnected);
}

void CPadAct::ResetRumble(BOOL resetAll)
{
	gGame.mPad.ResetRumble(resetAll);
}

static inline f32 Limit(f32 value, f32 lo, f32 hi)
{
	if (value < lo) {
		value = lo;
	}
	if (value > hi) {
		value = hi;
	}
	return value;
}

static inline void CopyNonzeroAxes(StickPosition& dest, const StickPosition& source)
{
	if (source.x) {
		dest.x = source.x;
	}
	if (source.y) {
		dest.y = source.y;
	}
}

static inline void GameCubeActions(CPadAct& act, const CPadOne& pad)
{
	if (!(pad.mButton & ((1ULL << RemoteMinus) | (1ULL << NunchukC)))) {
		CopyNonzeroAxes(act.mStick, pad.mStick);
	}
	CopyNonzeroAxes(act.mSubStick, pad.mSubStick);
	if ((pad.mTrigger & (1ULL << GameCubeY)) && !(pad.mButton & (1ULL << GameCubeB))) {
		act.mFlags |= 1ULL << 16;
	}
	if ((pad.mTrigger & (1ULL << GameCubeZ)) && !(pad.mTrigger & (1ULL << GameCubeB))) {
		act.mFlags |= 1ULL << 6;
	}
	if ((pad.mTrigger & (1ULL << GameCubeStart)) && !(pad.mButton & (1ULL << GameCubeB))) {
		act.mFlags |= 1ULL << 10;
	}
	if ((pad.mTrigger & (1ULL << GameCubeX)) && !(pad.mButton & (1ULL << GameCubeB))) {
		act.mFlags |= 1ULL << 8;
	}
	if (gGame.GetStatusFlags(0x80000000ULL)) {
		if (pad.mButton & (1ULL << (GameCubeDPad + 3))) {
			act.mFlags |= 1ULL << 28;
		}
		if (pad.mButton & (1ULL << (GameCubeDPad + 2))) {
			act.mFlags |= 1ULL << 29;
		}
		if (pad.mButton & (1ULL << GameCubeDPad)) {
			act.mFlags |= 1ULL << 26;
		}
		if (pad.mButton & (1ULL << (GameCubeDPad + 1))) {
			act.mFlags |= 1ULL << 27;
		}
		if (pad.mTrigger & (1ULL << (GameCubeDPad + 3))) {
			act.mFlags |= 1ULL << 30;
		}
		if (pad.mTrigger & (1ULL << (GameCubeDPad + 2))) {
			act.mFlags |= 1ULL << 31;
		}
		if (pad.mTrigger & (1ULL << GameCubeDPad)) {
			act.mFlags |= 1ULL << 32;
		}
		if (pad.mTrigger & (1ULL << (GameCubeDPad + 1))) {
			act.mFlags |= 1ULL << 33;
		}
		if (pad.mSubStick.y) {
			act.mCameraInput = pad.mSubStick.y * 0.5f;
		}
		if ((pad.mTrigger & ((1ULL << GameCubeR) | (1ULL << 16))) && !(pad.mButton & (1ULL << GameCubeL)) && act.mAnalogL > 0.0f) {
			act.mFlags |= 1ULL << 9;
		}
	} else {
		if (pad.mButton & (1ULL << (GameCubeSubStick + 3))) {
			act.mFlags |= 1ULL << 28;
		}
		if (pad.mButton & (1ULL << (GameCubeSubStick + 2))) {
			act.mFlags |= 1ULL << 29;
		}
		if (pad.mButton & (1ULL << GameCubeSubStick)) {
			act.mFlags |= 1ULL << 26;
		}
		if (pad.mButton & (1ULL << (GameCubeSubStick + 1))) {
			act.mFlags |= 1ULL << 27;
		}
		if (pad.mTrigger & (1ULL << (GameCubeSubStick + 3))) {
			act.mFlags |= 1ULL << 30;
		}
		if (pad.mTrigger & (1ULL << (GameCubeSubStick + 2))) {
			act.mFlags |= 1ULL << 31;
		}
		if (pad.mTrigger & (1ULL << GameCubeSubStick)) {
			act.mFlags |= 1ULL << 32;
		}
		if (pad.mTrigger & (1ULL << (GameCubeSubStick + 1))) {
			act.mFlags |= 1ULL << 33;
		}
		if (pad.mButton & (1ULL << (GameCubeDPad + 3))) {
			act.mCameraInput = 0.5f;
		}
		if (pad.mButton & (1ULL << (GameCubeDPad + 2))) {
			act.mCameraInput = -0.5f;
		}
		if ((pad.mTrigger & ((1ULL << GameCubeR) | (1ULL << 16))) && !(pad.mButton & (1ULL << GameCubeB))) {
			act.mFlags |= 1ULL << 9;
		}
	}
	if ((pad.mTrigger & (1ULL << GameCubeL)) && !gGame.GetStatusFlags(0x0600000000000000ULL)) {
		act.mFlags |= 1ULL << 34;
	}
	if ((pad.mButton & (1ULL << GameCubeB)) && (pad.mTrigger & (1ULL << GameCubeStart)) && !(pad.mButton & (1ULL << GameCubeX))) {
		act.mFlags |= 1ULL << 13;
	}
	if ((pad.mButton & (1ULL << GameCubeB)) && (pad.mTrigger & (1ULL << GameCubeR))) {
		act.mFlags |= 1ULL << 14;
	}
	if ((pad.mButton & (1ULL << GameCubeB)) && (pad.mTrigger & (1ULL << GameCubeL))) {
		act.mFlags |= 1ULL << 15;
	} else if ((pad.mTrigger & (1ULL << GameCubeR)) && !(pad.mButton & (1ULL << GameCubeB)) && pad.mAnalogL < 30.0f) {
		act.mFlags |= 1ULL << 35;
	}
	if (pad.mRepeat & (1ULL << GameCubeL)) {
		act.mFlags |= 1ULL << 36;
	}
}

static f32 sLastPointerY;

static inline void UpdatePointer(CPadAct& act, const CPadOne& pad, u32 frames)
{
	if (act.mPointer.y < act.mTop) {
		act.mPointer.y = act.mTop;
	}
	if (act.mPointer.y > act.mBottom) {
		act.mPointer.y = act.mBottom;
	}
	if (act.mPointer.x < act.mLeft) {
		act.mPointer.x = act.mLeft;
	}
	if (act.mPointer.x > act.mRight) {
		act.mPointer.x = act.mRight;
	}
	BOOL visible  = FALSE;
	BOOL inBounds = FALSE;
	for (s32 i = 0; i < pad.mSampleCount; ++i) {
		const WiiPadSample& sample = pad.mSamples[i];
		if (sample.pointerValid < 2) {
			continue;
		}
		f32 x     = ((1.0f + sample.pointer.position.x) * gPointerScreenWidth) * 0.5f;
		f32 y     = ((1.0f + sample.pointer.position.y) * gPointerScreenHeight) * 0.5f;
		f32 angle = (f32)atan2(sample.horizon.position.y, sample.horizon.position.x);
		while (angle < 0.0f) {
			angle += 2.0f * PI;
		}
		while (angle >= 2.0f * PI) {
			angle -= 2.0f * PI;
		}
		if (x >= act.mLeft && x <= act.mRight && y >= act.mTop && y <= act.mBottom) {
			act.mPointer.x = x;
			act.mPointer.y = y;
			if (__fabsf(y - sLastPointerY) > 40.0f) {
				sLastPointerY = y;
			}
			act.mStablePointer.x = x;
			act.mStablePointer.y = sLastPointerY;
			inBounds             = TRUE;
		} else {
			x              = Limit(x, act.mLeft, act.mRight);
			y              = Limit(y, act.mTop, act.mBottom);
			act.mPointer.x = x;
			act.mPointer.y = y;
		}
		act.mPointerAngle = angle;
		visible           = TRUE;
		break;
	}
	if (act.mTransitionCooldown > 0.0f) {
		act.mTransitionCooldown -= (f32)frames;
		if (act.mTransitionCooldown < 0.0f) {
			act.mTransitionCooldown = 0.0f;
		}
	}
	if (act.mInBounds != inBounds) {
		act.mInBounds = inBounds;
	}
	act.mPointerChanged = FALSE;
	if (act.mPointerValid != visible) {
		act.mTransitionFrames += frames;
		if (act.mTransitionFrames >= 4) {
			act.mTransitionFrames = 0;
			act.mPointerValid     = visible;
			act.mPointerChanged   = TRUE;
			const u8* game        = reinterpret_cast<const u8*>(&gGame);
			const bool hasSelection
			    = *reinterpret_cast<const s32*>(game + 0xb5e8) >= 1 && *reinterpret_cast<const s32*>(game + 0xb79c) >= 0;
			if (!hasSelection && !(gGame.GetStatusFlags(4ULL << 32) && *reinterpret_cast<const s32*>(game + 0x5978) != 12)
			    && gGame.mLoadStep >= 2 && act.mTransitionCooldown == 0.0f) {
				act.mTransitionCooldown = 0.3f * gRefreshRate;
			}
		}
	}
	if (act.mPointerValid && !act.mPointerSeen) {
		act.mPointerSeen = true;
	}
	if (act.mPointerSeen) {
		act.mPointerScale = 1.0f;
		if (act.mPointerValid) {
			act.mPointerFade = 1.0f;
			act.mFadeFrames  = 0;
		}
		act.mPointerOpacity = act.mPointerScale * act.mPointerFade;
	} else {
		act.mPointerOpacity = 0.0f;
	}
}

void CPadAct::Update(u32 frames)
{
	const CPadOne& pad = gWiiPads[0];
	mFlags             = 0;
	mButton = mDirButton = 0;
	mTrigger = mDirTrigger = mRepeat = mDirRepeat = 0;
	mAnalogL = mAnalogR = mSubStick.x = mSubStick.y = mStick.x = mStick.y = 0.0f;
	mCameraInput                                                          = NAN;
	mButton                                                               = pad.mButton;
	mDirButton                                                            = pad.mDirButton;
	mTrigger                                                              = pad.mTrigger;
	mDirTrigger                                                           = pad.mDirTrigger;
	mRepeat                                                               = pad.mRepeat;
	mDirRepeat                                                            = pad.mDirRepeat;

	if (pad.mAnalogL) {
		const f32 analogL = pad.mAnalogL - 30.0f;
		mAnalogL          = analogL;
		if (analogL < 0.0f) {
			mAnalogL = 0.0f;
		} else if (analogL > 180.0f) {
			mAnalogL = 180.0f;
		}
	}
	if (pad.mAnalogR) {
		const f32 analogR = pad.mAnalogR - 30.0f;
		mAnalogR          = analogR;
		if (analogR < 0.0f) {
			mAnalogR = 0.0f;
		} else if (analogR > 180.0f) {
			mAnalogR = 180.0f;
		}
	}

	if (pad.mTrigger & ((1ULL << GameCubeA) | (1ULL << RemoteA))) {
		mFlags |= 1ULL << 0;
	}
	if (pad.mTrigger & ((1ULL << GameCubeB) | (1ULL << RemoteB))) {
		mFlags |= 1ULL << 1;
	}
	if (pad.mRepeat & ((1ULL << (GameCubeStick + 3)) | (1ULL << (NunchukStick + 3)))) {
		mFlags |= 1ULL << 2;
	}
	if (pad.mRepeat & ((1ULL << (GameCubeStick + 2)) | (1ULL << (NunchukStick + 2)))) {
		mFlags |= 1ULL << 3;
	}
	if (pad.mRepeat & ((1ULL << GameCubeStick) | (1ULL << NunchukStick))) {
		mFlags |= 1ULL << 4;
	}
	if (pad.mRepeat & ((1ULL << (GameCubeStick + 1)) | (1ULL << (NunchukStick + 1)))) {
		mFlags |= 1ULL << 5;
	}

	if (pad.IsNunchukActive()) {
		CopyNonzeroAxes(mStick, pad.mStick);
	}
	if (pad.IsGameCubePadActive()) {
		GameCubeActions(*this, pad);
	}
	if (pad.IsRemoteActive()) {
		const BOOL dvdBusy = (gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk;
		if (!dvdBusy) {
			if (pad.mTrigger & (1ULL << NunchukC)) {
				mFlags |= 1ULL << 16;
			}
			if (pad.mTrigger & (1ULL << NunchukZ)) {
				mFlags |= 1ULL << 6;
			}
			if (pad.mTrigger & (1ULL << RemotePlus)) {
				mFlags |= 1ULL << 7;
			}
			if (pad.mTrigger & (1ULL << RemoteMinus)) {
				mFlags |= 1ULL << 8;
			}
			if (pad.mTrigger & (1ULL << RemoteOne)) {
				mFlags |= 1ULL << 9;
			}
			if (pad.mButton & (1ULL << RemoteUp)) {
				mFlags |= 1ULL << 28;
			}
			if (pad.mButton & (1ULL << RemoteDown)) {
				mFlags |= 1ULL << 29;
			}
			if (pad.mButton & (1ULL << RemoteLeft)) {
				mFlags |= 1ULL << 26;
			}
			if (pad.mButton & (1ULL << RemoteRight)) {
				mFlags |= 1ULL << 27;
			}
			if (pad.mTrigger & (1ULL << RemoteUp)) {
				mFlags |= 1ULL << 30;
			}
			if (pad.mTrigger & (1ULL << RemoteDown)) {
				mFlags |= 1ULL << 31;
			}
			if (pad.mTrigger & (1ULL << RemoteLeft)) {
				mFlags |= 1ULL << 32;
			}
			if (pad.mTrigger & (1ULL << RemoteRight)) {
				mFlags |= 1ULL << 33;
			}
			UpdatePointer(*this, pad, frames);
		}
		for (int i = 4; i > 0; --i) {
			mRemoteAcceleration[i]  = mRemoteAcceleration[i - 1];
			mNunchukAcceleration[i] = mNunchukAcceleration[i - 1];
		}
		mRemoteAcceleration[0]  = pad.mSamples[0].acceleration.value;
		mNunchukAcceleration[0] = pad.mSamples[0].extension.nunchuk.acceleration.value;
	}
	const u64 other = gWiiPads[3].mTrigger;
	if (other & (1ULL << GameCubeA)) {
		mFlags |= 1ULL << 13;
	}
	if (other & (1ULL << (GameCubeDPad + 3))) {
		mFlags |= 1ULL << 14;
	}
	if (other & (1ULL << (GameCubeDPad + 2))) {
		mFlags |= 1ULL << 15;
	}
}

static inline u32 EventButtons(u64 buttons)
{
	// The event VM exposes the low 25 bits. GameCube face and trigger inputs
	// are translated into the same namespace, including four dedicated bits.
	u64 result = buttons;
	if (buttons & (1ULL << GameCubeDPad)) {
		result |= 1ULL << 0;
	}
	if (buttons & (1ULL << (GameCubeDPad + 1))) {
		result |= 1ULL << 1;
	}
	if (buttons & (1ULL << (GameCubeDPad + 2))) {
		result |= 1ULL << 2;
	}
	if (buttons & (1ULL << (GameCubeDPad + 3))) {
		result |= 1ULL << 3;
	}
	if (buttons & (1ULL << GameCubeA)) {
		result |= 1ULL << RemoteA;
	}
	if (buttons & (1ULL << GameCubeB)) {
		result |= 1ULL << RemoteB;
	}
	if (buttons & (1ULL << GameCubeZ)) {
		result |= 1ULL << NunchukZ;
	}
	if (buttons & (1ULL << GameCubeStart)) {
		result |= 1ULL << RemotePlus;
	}
	if (buttons & (1ULL << GameCubeX)) {
		result |= 1ULL << 13;
	}
	if (buttons & (1ULL << GameCubeY)) {
		result |= 1ULL << 14;
	}
	if (buttons & (1ULL << GameCubeL)) {
		result |= 1ULL << 15;
	}
	if (buttons & (1ULL << GameCubeR)) {
		result |= 1ULL << 16;
	}
	return result & 0x1ffffffULL;
}

u32 CPadAct::GetEventButtons() const
{
	return EventButtons(mButton);
}

u32 CPadAct::GetEventTrigger() const
{
	return EventButtons(mTrigger);
}

BOOL CPadAct::GetSwing(Vec* direction, f32* strength) const
{
	return gWiiPads[0].GetSwing(direction, strength);
}
