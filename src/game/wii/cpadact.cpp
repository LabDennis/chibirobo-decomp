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

void CPadAct::Init()
{
	mFlags         = 0;
	const Vec zero = { 0.0f, 0.0f, 0.0f };
	for (int i = 0; i < 5; ++i) {
		mRemoteAcceleration[i] = mNunchukAcceleration[i] = zero;
	}
	mRight     = gScreenRight - 16.0f;
	mLeft      = gScreenLeft + 16.0f;
	mBottom    = gScreenBottom - 16.0f;
	mTop       = gScreenTop + 16.0f;
	mPointer.x = gScreenSafeRight - 12;
	mPointer.y = gScreenSafeBottom - 12;
	mPointer.z = mPointerAngle = 0.0f;
	mPointerScale = mPointerFade = mPointerOpacity = 0.0f;
	mInBounds = mPointerValid = FALSE;
	mTransitionFrames         = 0;
	mPointerSeen              = false;
	mFadeFrames               = 0;
	mTransitionCooldown       = 0.0f;
	mStablePointer            = mPointer;
	for (s32 channel = 1; channel < 4; ++channel) {
		KPADSetConnectCallback(channel, SecondaryRemoteConnected);
	}
}

void CPadAct::ResetRumble(BOOL resetAll)
{
	gGame.mPad.ResetRumble(resetAll);
}

static f32 Limit(f32 value, f32 lo, f32 hi)
{
	if (value < lo) {
		value = lo;
	}
	if (value > hi) {
		value = hi;
	}
	return value;
}

static void CopyNonzeroAxes(StickPosition& dest, const StickPosition& source)
{
	if (source.x != 0.0f) {
		dest.x = source.x;
	}
	if (source.y != 0.0f) {
		dest.y = source.y;
	}
}

static u64 DirectionActions(u32 held, u32 edge)
{
	held &= 15;
	edge &= 15;
	// Left/right occupy the lower pair, up/down are reversed.
	const u32 heldDirs = (held & 3) | ((held & 4) << 1) | ((held & 8) >> 1);
	const u32 edgeDirs = (edge & 3) | ((edge & 4) << 1) | ((edge & 8) >> 1);
	// The action word stores up/down, then left/right for both held and pressed.
	return (u64)heldDirs << 26 | (u64)(edgeDirs >> 2) << 30 | (u64)(edgeDirs & 3) << 32;
}

static u64 GameCubeActions(CPadAct& act, const CPadOne& pad)
{
	const u64 held    = pad.mButton;
	const u64 trigger = pad.mTrigger;
	u64 actions       = 0;
	if (!(held & ((1ULL << RemoteMinus) | (1ULL << NunchukC)))) {
		CopyNonzeroAxes(act.mStick, pad.mStick);
	}
	CopyNonzeroAxes(act.mSubStick, pad.mSubStick);
	if ((trigger & (1ULL << GameCubeY)) && !(held & (1ULL << GameCubeB))) {
		actions |= 1ULL << 16;
	}
	if ((trigger & (1ULL << GameCubeZ)) && !(trigger & (1ULL << GameCubeB))) {
		actions |= 1ULL << 6;
	}
	if ((trigger & (1ULL << GameCubeStart)) && !(held & (1ULL << GameCubeB))) {
		actions |= 1ULL << 10;
	}
	if ((trigger & (1ULL << GameCubeX)) && !(held & (1ULL << GameCubeB))) {
		actions |= 1ULL << 8;
	}
	const bool triggerR = (trigger & ((1ULL << GameCubeR) | (1ULL << 16))) != 0;
	if (gGame.GetStatusFlags(0x80000000ULL)) {
		actions |= DirectionActions((u32)(held >> GameCubeDPad), (u32)(trigger >> GameCubeDPad));
		if (pad.mSubStick.y != 0.0f) {
			act.mCameraInput = 0.5f * pad.mSubStick.y;
		}
		if (triggerR && !(held & (1ULL << GameCubeL)) && act.mAnalogL > 0.0f) {
			actions |= 1ULL << 9;
		}
	} else {
		actions |= DirectionActions((u32)(held >> GameCubeSubStick), (u32)(trigger >> GameCubeSubStick));
		if (held & (1ULL << (GameCubeDPad + 3))) {
			act.mCameraInput = 0.5f;
		}
		if (held & (1ULL << (GameCubeDPad + 2))) {
			act.mCameraInput = -0.5f;
		}
		if (triggerR && !(held & (1ULL << GameCubeB))) {
			actions |= 1ULL << 9;
		}
	}
	if ((trigger & (1ULL << GameCubeL)) && !gGame.GetStatusFlags(0x0600000000000000ULL)) {
		actions |= 1ULL << 34;
	}
	if (held & (1ULL << GameCubeB)) {
		if ((trigger & (1ULL << GameCubeStart)) && !(held & (1ULL << GameCubeX))) {
			actions |= 1ULL << 13;
		}
		if (trigger & (1ULL << GameCubeR)) {
			actions |= 1ULL << 14;
		}
		if (trigger & (1ULL << GameCubeL)) {
			actions |= 1ULL << 15;
		}
	} else if ((trigger & (1ULL << GameCubeR)) && pad.mAnalogL < 30.0f) {
		actions |= 1ULL << 35;
	}
	if (pad.mRepeat & (1ULL << GameCubeL)) {
		actions |= 1ULL << 36;
	}
	return actions;
}

static f32 sLastPointerY;

static void UpdatePointer(CPadAct& act, const CPadOne& pad, u32 frames)
{
	act.mPointer.y = Limit(act.mPointer.y, act.mTop, act.mBottom);
	act.mPointer.x = Limit(act.mPointer.x, act.mLeft, act.mRight);
	BOOL visible   = FALSE;
	BOOL inBounds  = FALSE;
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
			if (__fabsf(y - sLastPointerY) > 40.0f) {
				sLastPointerY = y;
			}
			act.mStablePointer.x = x;
			act.mStablePointer.y = sLastPointerY;
			inBounds             = TRUE;
		} else {
			x = Limit(x, act.mLeft, act.mRight);
			y = Limit(y, act.mTop, act.mBottom);
		}
		act.mPointer.x    = x;
		act.mPointer.y    = y;
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
	act.mInBounds       = inBounds;
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
	if (act.mPointerValid) {
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
	mButton            = pad.mButton;
	mDirButton         = pad.mDirButton;
	mTrigger           = pad.mTrigger;
	mDirTrigger        = pad.mDirTrigger;
	mRepeat            = pad.mRepeat;
	mDirRepeat         = pad.mDirRepeat;
	mStick.x = mStick.y = mSubStick.x = mSubStick.y = 0.0f;
	mAnalogL                                        = pad.mAnalogL == 0.0f ? 0.0f : Limit(pad.mAnalogL - 30.0f, 0.0f, 180.0f);
	mAnalogR                                        = pad.mAnalogR == 0.0f ? 0.0f : Limit(pad.mAnalogR - 30.0f, 0.0f, 180.0f);
	mCameraInput                                    = NAN;

	if (mTrigger & ((1ULL << GameCubeA) | (1ULL << RemoteA))) {
		mFlags |= 1ULL << 0;
	}
	if (mTrigger & ((1ULL << GameCubeB) | (1ULL << RemoteB))) {
		mFlags |= 1ULL << 1;
	}
	const u32 repeatDirs = (u32)((mRepeat >> GameCubeStick) | (mRepeat >> NunchukStick)) & 15;
	mFlags |= (u64)(repeatDirs >> 3) << 2 | (u64)((repeatDirs >> 2) & 1) << 3 | (u64)(repeatDirs & 3) << 4;

	if ((pad.mDevices & 2) && (pad.mInputMask & 1)) {
		CopyNonzeroAxes(mStick, pad.mStick);
	}
	if (pad.IsGameCubePadActive()) {
		mFlags |= GameCubeActions(*this, pad);
	}
	if ((pad.mDevices & 1) && (pad.mInputMask & 1)) {
		if (!((gGame.mDvdStatus || gGame.mPrevDvdStatus) && !gGame.mDiskOk)) {
			if (mTrigger & (1ULL << NunchukC)) {
				mFlags |= 1ULL << 16;
			}
			if (mTrigger & (1ULL << NunchukZ)) {
				mFlags |= 1ULL << 6;
			}
			if (mTrigger & (1ULL << RemotePlus)) {
				mFlags |= 1ULL << 7;
			}
			if (mTrigger & (1ULL << RemoteMinus)) {
				mFlags |= 1ULL << 8;
			}
			if (mTrigger & (1ULL << RemoteOne)) {
				mFlags |= 1ULL << 9;
			}
			mFlags |= DirectionActions((u32)mButton, (u32)mTrigger);
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

static u32 EventButtons(u64 buttons)
{
	// The event VM exposes the low 25 bits. GameCube face and trigger inputs
	// are translated into the same namespace, including four dedicated bits.
	u32 result = (u32)buttons;
	result |= (u32)(buttons >> GameCubeDPad) & 0x3f;
	if (buttons & (1ULL << GameCubeZ)) {
		result |= 1u << NunchukZ;
	}
	if (buttons & (1ULL << GameCubeStart)) {
		result |= 1u << RemotePlus;
	}
	if (buttons & (1ULL << GameCubeX)) {
		result |= 1u << 13;
	}
	if (buttons & (1ULL << GameCubeY)) {
		result |= 1u << 14;
	}
	if (buttons & (1ULL << GameCubeL)) {
		result |= 1u << 15;
	}
	if (buttons & (1ULL << GameCubeR)) {
		result |= 1u << 16;
	}
	return result & 0x1ffffff;
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
