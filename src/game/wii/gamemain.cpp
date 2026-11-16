#include "game/wii/cgame.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include "Dolphin/dvd.h"
#include "Dolphin/gx.h"
#include "revolution/mem.h"
#include "nw4r/snd/snd_BasicSound.h"

extern "C" {
void OSResetStopwatch(void* sw);
void OSStartStopwatch(void* sw);
void OSStopStopwatch(void* sw);
s64 OSCheckStopwatch(void* sw);
BOOL OSGetResetButtonState();
void OSRebootSystem();
void OSShutdownSystem();
void OSRestart(u32 code);
void OSReturnToMenu();
void OSReturnToDataManager();
BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback);
void OnDiskChecked(s32 result, DVDCommandBlock* block); // 0x80019414, no source yet
}

// the expanded heap mm::InitMem1 creates over the MEM1 arena
extern MEMiHeapHead* gWiiMem1Heap;
extern s32 gRefreshRate;

// set by the power button's callback, CBase::Update turns it into a shutdown request
extern u8 gPowerButtonPressed;

inline void CSoundTrackSlot::Stop(int fadeFrames)
{
	if (IsAttachedSound()) {
		((nw4r::snd::detail::BasicSound*)mTrack)->Stop(fadeFrames);
	}
}

inline void CSound::StopAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mTracks[i].IsAttachedSound()) {
			mTracks[i].Stop(frames);
		}
	}
}

// The functions stand in the order of the Wii file: the binary lays them out backwards, main first.

/**
 * @note Address: 0x80019A18
 * @note Size: 0x50
 */
// The game's own operator new: four-byte aligned blocks from the MEM1 heap.
void* operator new(size_t size) throw()
{
	return MEMAllocFromExpHeapEx(gWiiMem1Heap, size, 4);
}

/**
 * @note Address: 0x800198E8
 * @note Size: 0x130
 */
// stops the disc, the sound and the video, then leaves the game the way code says
void CBase::Reset(u32 code)
{
	mResetRequested = true;
	DVDCancelAll();
	gGame.mSound.StopAllTracks(0.0f);
	GXDrawDone();
	GXAbortFrame();
	VISetBlack(TRUE);
	VIFlush();
	VIWaitForRetrace();
	ShutdownAudio(&gGame.mSound);
	switch (code) {
	case 1:
		OSRebootSystem();
		break;
	case 2:
		OSShutdownSystem();
		break;
	case 3:
		OSRestart(0);
		break;
	case 4:
		OSReturnToMenu();
		break;
	case 5:
		OSReturnToDataManager();
		break;
	}
}

/**
 * @note Address: 0x800198DC
 * @note Size: 0xC
 */
// the callback Start gives OSSetPowerCallback
void OnPowerButton()
{
	gPowerButtonPressed = TRUE;
}

/**
 * @note Address: 0x80019428
 * @note Size: 0x118
 */
// follows the drive: a cover that opens or a disc that stops reading sets mDvdError and takes the
// rumble off every remote, a drive that reads again clears it
void CBase::UpdateDvdStatus()
{
	mPrevDvdStatus = mDvdStatus;
	mDvdStatus     = DVDGetDriveStatus();
	if (!mDiskChecking) {
		mDiskChecking = true;
		DVDCheckDiskAsync(&mDiskCheckBlock, OnDiskChecked);
	}
	switch (mDvdStatus) {
	case -1:
	case 4:
	case 5:
	case 6:
	case 7:
	case 11:
		if (mDvdStatus != mPrevDvdStatus) {
			mDvdError = true;
			for (int i = 0; i < 4; ++i) {
				gWiiPads[i].mRumble[0].flags &= ~RumbleRunning;
				gWiiPads[i].mRumble[0].remaining = 0;
			}
		}
		break;
	case 0:
		if (mPrevDvdStatus != 0 && mDvdError) {
			mDvdError = false;
		}
		break;
	}
}

/**
 * @note Address: 0x80018348
 * @note Size: 0x4
 */
void CBase::DrawHud()
{
}

/**
 * @note Address: 0x80018344
 * @note Size: 0x4
 */
void CBase::DrawScene()
{
}

/**
 * @note Address: 0x800182C8
 * @note Size: 0x7C
 */
// the corner icon, the drive, and the two buttons on the console: power shuts down, reset restarts
void CBase::Update(u32 frames)
{
	fn_80018364();
	UpdateDvdStatus();
	if (!mResetRequested) {
		BOOL reset = OSGetResetButtonState();
		if (gPowerButtonPressed) {
			mResetRequested = true;
			mResetType      = 2;
		} else if (reset) {
			mResetRequested = true;
			mResetType      = 3;
		}
	}
}

/**
 * @note Address: 0x800182BC
 * @note Size: 0x4
 */
void CBase::ResetNewGameState()
{
}

/**
 * @note Address: 0x80017D48
 * @note Size: 0x4
 */
void CBase::Initialize(int argc, char** argv)
{
}

/**
 * @note Address: 0x80017D40
 * @note Size: 0x8
 */
s32 CBase::FrameInterval()
{
	return 1;
}

/**
 * @note Address: 0x80017D3C
 * @note Size: 0x4
 */
void CBase::Draw()
{
}

/**
 * @note Address: 0x80017BD8
 * @note Size: 0x164
 */
// the game's loop, it never returns: a frame is the pad, the update between the draw's two halves,
// and a wait that pads it to FrameInterval retraces
void CBase::Run(int argc, char** argv)
{
	Initialize(argc, argv);
	Start();
	gGame.mGs.StartFrameThread();
	VISetBlack(FALSE);
	while (true) {
		gGame.mGs.EndFrame();
		gGame.mGs.SwapBuffers();
		mRetraceStep = VIGetRetraceCount() - mLastRetrace;
		mLastRetrace = VIGetRetraceCount();
		if (mRetraceStep > 4) { // a hitch only counts four frames
			mRetraceStep = 4;
		}
		OSResetStopwatch(&mStopwatch[0]);
		OSStartStopwatch(&mStopwatch[0]);
		OSResetStopwatch(&mStopwatch[1]);
		OSStartStopwatch(&mStopwatch[1]);
		mPad.Update(mRetraceStep);
		BeforeUpdate();
		Draw();
		mStopwatch[1].mElapsed = OSCheckStopwatch(&mStopwatch[1]);
		OSStopStopwatch(&mStopwatch[1]);
		Update(mRetraceStep);
		mStopwatch[0].mElapsed = OSCheckStopwatch(&mStopwatch[0]);
		OSStopStopwatch(&mStopwatch[0]);
		s32 interval = FrameInterval();
		while (VIGetRetraceCount() - mLastRetrace < (u32)(interval - 1)) { }
	}
}

/**
 * @note Address: 0x80017BC0
 * @note Size: 0x18
 */
void main(int argc, char** argv)
{
	gGame.Run(argc, argv);
}
