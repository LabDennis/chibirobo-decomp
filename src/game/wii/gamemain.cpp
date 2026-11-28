#include "game/wii/cgame.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include "Dolphin/dvd.h"
#include "Dolphin/gx.h"
#include "revolution/wpad.h"
#include "nw4r/snd/snd_RemoteSpeakerManager.h"
#include "revolution/mem.h"
#include <string.h>
#include <math.h>
#include "nw4r/snd/snd_BasicSound.h"

extern "C" {
void OSResetStopwatch(void* sw);
void OSStartStopwatch(void* sw);
void OSStopStopwatch(void* sw);
s64 OSCheckStopwatch(void* sw);
BOOL OSGetResetButtonState();
void OSInitStopwatch(void* sw, const char* name);
void* OSSetPowerCallback(void (*callback)());
void InitGameRuntime(CBase* base);
CCamera* InitGamePerspectiveCamera(CCamera* camera, const GXRenderModeObj* rmode);
CCamera* InitGameOrthoCamera(CCamera* camera, const GXRenderModeObj* rmode);
CLight* InitFourLightSet(CLight* lights);
CLight* InitTwoLightSet(CLight* lights);
CFog* InitGameFog(CFog* fog);
void OSRebootSystem();
void OSShutdownSystem();
void OSRestart(u32 code);
void OSReturnToMenu();
void OSReturnToDataManager();
BOOL DVDCheckDiskAsync(DVDCommandBlock* block, DVDCBCallback callback);
void OnDiskChecked(s32 result, DVDCommandBlock* block);
const char* GetLocalizedInterfaceText(s32 id);
int sprintf(char* buf, const char* fmt, ...);
}

static void OnBatteryInfo(s32 chan, s32 result);

// the expanded heap mm::InitMem1 creates over the MEM1 arena
extern MEMiHeapHead* gWiiMem1Heap;

// the original file defines operator delete itself (0x80019A68, game/wii/delete.cpp) and the
// compiler expands it in the destructors it emits; this piece of the file sees it as an inline
inline void operator delete(void* ptr) throw()
{
	MEMFreeToExpHeap(gWiiMem1Heap, ptr);
}
extern s32 gRefreshRate;
extern s32 gPointerScreenWidth;
extern s32 gPointerScreenHeight;
extern s32 gScreenLeft;
extern s32 gScreenRight;
extern s32 gScreenSafeLeft;
extern s32 gScreenSafeTop;
extern s32 gScreenSafeBottom;

// the remote's battery query: Update asks WPAD every three seconds while pad 0 holds a remote
struct WiiBatteryCheck {
	bool mDone;     // _00, the answer is in mInfo
	s32 mWait;      // _04, frames to the next query
	WPADInfo mInfo; // _08
};
WiiBatteryCheck gBatteryCheck;

// the TPL with the corner icons, built into the game; it lies in this file's .rodata, which is not split yet
// shortcut: declared here, defined where the rodata of the whole file lands
extern u8 gCornerIconTplData[];

// set by the power button's callback, CBase::Update turns it into a shutdown request
extern u8 gPowerButtonPressed;

inline void CSound::StopAllTracks(f32 fade)
{
	s32 frames = gRefreshRate * fade;
	for (int i = 0; i < 0x40; ++i) {
		if (mArchivePlayer.mTracks.mHandles[i].IsAttachedSound()) {
			mArchivePlayer.mTracks.mHandles[i].Stop(frames);
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
 * @note Address: 0x80019540
 * @note Size: 0x39C
 */
// NONMATCHING: mTextFont now precedes the table copy as in retail. The remaining difference is
// a 0x30 larger frame and four rather than five callee-saved floats (96.82%)
// the drive's message for the status DVDGetDriveStatus reports, centred on a dark box
void CBase::DrawDiscError()
{
	if (IsDiscError()) {
		f32 width           = gPointerScreenWidth;
		f32 height          = gPointerScreenHeight;
		gGame.mGs.mTextFont = 1;
		// the drive statuses that have a message, and the interface text each shows
		struct {
			s32 status;
			s32 text;
		} texts[6] = {
			{ 5, 0x55 }, { 4, 0x56 }, { 6, 0x57 }, { 11, 0x58 }, { 1, 0x59 }, { 0, -1 },
		};
		const char* text = NULL;
		for (s32 i = 0; texts[i].text >= 0; i++) {
			if (texts[i].status == mDvdStatus) {
				text = GetLocalizedInterfaceText(texts[i].text);
			}
		}
		if (text != NULL) {
			gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			char buf[0x138];
			sprintf(buf, text, GetLocalizedInterfaceText(0));
			f32 size       = 24.0f;
			f32 textWidth  = MeasureGameTextWidth(&gGame.mGs, buf, TRUE, size);
			f32 textHeight = MeasureGameTextHeight(&gGame.mGs, buf, size);
			f32 y          = height * 0.5f - textHeight * 0.5f;
			f32 x          = width * 0.5f - textWidth * 0.5f;
			Vec quad[4]    = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
			quad[0].x      = x - size;
			quad[0].y      = y - size;
			quad[1].x      = x + textWidth + size;
			quad[1].y      = y - size;
			quad[2].x      = x + textWidth + size;
			quad[2].y      = y + textHeight + size;
			quad[3].x      = x - size;
			quad[3].y      = y + textHeight + size;
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, 200, quad[1].x, quad[1].y, quad[1].z, 200, quad[2].x, quad[2].y,
			                 quad[2].z, 200, quad[3].x, quad[3].y, quad[3].z, 200);
			DrawGsProportionalTextFormat(&gGame.mGs, 0xFFFFFFFF, buf, x, y, 0.0f, size, size);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		}
		gGame.mGs.mTextFont = 0;
	}
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
 * @note Address: 0x80019414
 * @note Size: 0x14
 */
struct DiskCheckState {
	s32 mDvdStatus;
	s32 mPrevDvdStatus;
	BOOL mDiskOk;
	bool mDvdError;
	DVDCommandBlock mDiskCheckBlock;
	bool mDiskChecking;
};
STATIC_ASSERT(offsetof(DiskCheckState, mDiskOk) == offsetof(CBase, mDiskOk) - offsetof(CBase, mDvdStatus));
STATIC_ASSERT(offsetof(DiskCheckState, mDiskChecking) == offsetof(CBase, mDiskChecking) - offsetof(CBase, mDvdStatus));

void OnDiskChecked(s32 result, DVDCommandBlock* block)
{
	DiskCheckState* state = static_cast<DiskCheckState*>(block->userData);
	state->mDiskOk        = result;
	state->mDiskChecking  = false;
}

/**
 * @note Address: 0x80019300
 * @note Size: 0x114
 */
// where the icons of mIconTpl go: the safe area's lower left corner, the battery icon's height above
// it while it is shown, sliding in as mBatteryAlpha rises
extern "C" Vec GetCornerIconPosition(CGame* game)
{
	f32 alpha = game->mBatteryAlpha;
	game->mIconTpl.mTex.Init(&game->mIconTpl.mPalette->descriptorArray[1], 0);
	f32 height = GXGetTexObjHeight(&game->mIconTpl.mTex);
	Vec pos    = { 0.0f, 0.0f, 0.0f };
	pos.x      = gScreenSafeLeft;
	f32 shown  = sin(3.1415927f * alpha / 2.0f);
	pos.y      = gScreenSafeBottom - (height - height * (1.0f - shown));
	return pos;
}

/**
 * @note Address: 0x800187B0
 * @note Size: 0xB50
 */
// NONMATCHING: stack slot sharing across if/else scopes and callee-saved FPR allocation (f21..f31)
void CBase::DrawSystemMessage()
{
	f32 size = 20.0f;
	if (mSystemMessage != NULL && mSystemMessageAlpha > 0.0f) {
		gGame.mGs.mTextFont = 1;
		const char* text    = mSystemMessage;
		f32 alpha           = mSystemMessageAlpha;
		gGame.mGs.mCurRenderState->SetColorUpdate(GX_TRUE);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		f32 textWidth = MeasureGameTextWidth(&gGame.mGs, text, TRUE, size);
		f32 extra     = 0.0f;
		if (mSystemMessage == GetLocalizedInterfaceText(5) && _41E0 != NULL) {
			extra = size;
		}
		f32 x;
		f32 y;
		if (_41C0) {
			x           = (f32)(gPointerScreenWidth / 2) - textWidth * 0.5f;
			y           = (f32)(gPointerScreenHeight / 2) - size * 0.5f;
			Vec quad[4] = {
				{ x - size - extra, y - size, 0.0f },
				{ x + textWidth + size + extra, y - size, 0.0f },
				{ x + textWidth + size + extra, y + size + size, 0.0f },
				{ x - size - extra, y + size + size, 0.0f },
			};
			DrawGsVertexQuad(&gGame.mGs, quad[0].x, quad[0].y, quad[0].z, (u32)(200.0f * alpha), quad[1].x, quad[1].y, quad[1].z,
			                 (u32)(200.0f * alpha), quad[2].x, quad[2].y, quad[2].z, (u32)(200.0f * alpha), quad[3].x, quad[3].y, quad[3].z,
			                 (u32)(200.0f * alpha));
			DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xFFFFFF00, text, x, y, 0.0f, size, size);
		} else {
			x                  = (f32)(gPointerScreenWidth / 2) - textWidth * 0.5f;
			f32 slide          = sin(3.1415927f * alpha * 0.5f);
			y                  = (f32)gScreenSafeTop - (1.0f - slide) * 24.0f;
			u32 textColor      = (u32)(255.0f * alpha) | 0xFFFFFF00;
			f32 bottom         = (y + size) + 2.0f;
			f32 top            = y - 2.0f;
			f32 midX           = (f32)(gPointerScreenWidth / 2);
			f32 halfW          = (textWidth * alpha) * 0.5f;
			f32 x0             = midX - halfW;
			f32 x1             = midX + halfW;
			f32 left           = (f32)gScreenLeft;
			f32 right          = (f32)gScreenRight;
			u32 bgColor        = (u32)(160.0f * alpha);
			PrimVertex vtx[12] = {
				{ (Vec) { left, top, 0.0f }, 0 },        { (Vec) { x0, top, 0.0f }, bgColor },    { (Vec) { left, bottom, 0.0f }, 0 },
				{ (Vec) { x0, bottom, 0.0f }, bgColor }, { (Vec) { x0, top, 0.0f }, bgColor },    { (Vec) { x1, top, 0.0f }, bgColor },
				{ (Vec) { x0, bottom, 0.0f }, bgColor }, { (Vec) { x1, bottom, 0.0f }, bgColor }, { (Vec) { x1, top, 0.0f }, bgColor },
				{ (Vec) { right, top, 0.0f }, 0 },       { (Vec) { x1, bottom, 0.0f }, bgColor }, { (Vec) { right, bottom, 0.0f }, 0 },
			};
			DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLESTRIP, gGame.mGs.mCamera->mViewMtx, vtx, 12);
			DrawGsProportionalTextFormat(&gGame.mGs, textColor, text, x, y, 0.0f, size, size);
		}
		if (mSystemMessage == GetLocalizedInterfaceText(5) && _41E0 != NULL) {
			f32 iconX = x - 0.8f * size;
			f32 iconY = y + 0.5f * size;
			f32 blink = (mSystemMessageBlink > 0.5f) ? 220.0f : 150.0f;
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			DrawGsTexturedQuad(&gGame.mGs, _41E0, (u32)(blink * alpha) | 0xFFFFFF00, (u32)(blink * alpha) | 0xFFFFFF00,
			                   (u32)(blink * alpha) | 0xFFFFFF00, (u32)(blink * alpha) | 0xFFFFFF00, iconX, iconY, 0.0f, size, size, 0.0f,
			                   0.0f, 0.0f);
			DrawGsTexturedQuad(&gGame.mGs, _41E0, (u32)(blink * alpha) | 0xFFFFFF00, (u32)(blink * alpha) | 0xFFFFFF00,
			                   (u32)(blink * alpha) | 0xFFFFFF00, (u32)(blink * alpha) | 0xFFFFFF00, iconX + textWidth + 1.6f * size, iconY,
			                   0.0f, size, size, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mTextFont = 0;
	}
	if (mBatteryAlpha > 0.0f) {
		mIconTpl.mTex.Init(&mIconTpl.mPalette->descriptorArray[1], 0);
		CTexObj tex = mIconTpl.mTex;
		u32 color   = (u32)(200.0f * mBatteryAlpha * mBatteryBlink) | 0xFF808000;
		f32 w       = GXGetTexObjWidth(&tex);
		f32 h       = GXGetTexObjHeight(&tex);
		f32 slide   = sin(3.1415927f * mBatteryAlpha * 0.5f);
		f32 x       = (f32)gScreenSafeLeft + w * 0.5f - (1.0f - slide) * 24.0f;
		f32 y       = (f32)gScreenSafeBottom - h;
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	}
}

/**
 * @note Address: 0x80018364
 * @note Size: 0x44C
 */
// NONMATCHING: only the __FILE__ string's position in this unit's incomplete string pool remains
// the interface message in the screen's corner (3 while the remote is away, 5 when it points off
// the screen, 4 for an unsupported controller) and the low-battery icon, both fading in and out
void CBase::UpdateSystemMessage()
{
	if (!(_41B0 & 1)) {
		return;
	}
	CPadOne* pad = &gWiiPads[0];
	u32 step     = mRetraceStep;
	u32 device   = pad->mDeviceType;
	s32 probe    = pad->mProbeResult;
	switch (mLastProbe) {
	case -1:
		if (probe != -2 && probe != -3) {
			mLastProbe           = probe;
			mLastDeviceType      = device;
			mSystemMessage       = GetLocalizedInterfaceText(3);
			_41C0                = _41C1;
			mNoRemoteFrames      = 0.0f;
			mSystemMessageTarget = -(s32)((_41B0 >> 1) & 1);
			mBatteryLow          = false;
		}
		break;
	default:
		if (probe != -2 && probe != -3) {
			mLastProbe      = probe;
			mLastDeviceType = device;
		}
		if (probe == 0) {
			mNoRemoteFrames += 1.0f;
			if (mNoRemoteFrames > 10.0f) {
				mNoRemoteFrames = 10.0f;
			}
		} else {
			mNoRemoteFrames = 0.0f;
		}
		if (mNoRemoteFrames > 1.0f) {
			if (probe == 0 || probe == -2) {
				if (pad->mSamples[0].pointerValid < 2 && (_41B0 & 8)) {
					mSystemMessage       = GetLocalizedInterfaceText(5);
					_41C0                = _41C3;
					mSystemMessageTarget = -1.0f;
					mNoRemoteFrames      = 0.0f;
				} else if (mLastDeviceType != 1 && (_41B0 & 4)) {
					mSystemMessage       = GetLocalizedInterfaceText(4);
					_41C0                = _41C2;
					mSystemMessageTarget = -1.0f;
				} else {
					mSystemMessageTarget = 0.0f;
				}
			} else {
				mSystemMessageTarget = 0.0f;
			}
		}
		if (mLastProbe == 0) {
			if (gBatteryCheck.mWait > 0 && (gBatteryCheck.mWait -= step) <= 0) {
				gBatteryCheck.mWait = 0;
			} else if (gBatteryCheck.mWait == 0
			           && !nw4r::snd::detail::RemoteSpeakerManager::GetInstance().GetRemoteSpeaker(pad->mPort).IsEnabledOutput()) {
				gBatteryCheck.mDone = false;
				if (WPADGetInfoAsync(0, &gBatteryCheck.mInfo, OnBatteryInfo) == 0) {
					gBatteryCheck.mWait = gRefreshRate * 3;
				}
			}
		} else {
			gBatteryCheck.mDone = false;
		}
		if (gBatteryCheck.mDone) {
			if (gBatteryCheck.mInfo.battery <= 1) {
				mBatteryLow = true;
			} else {
				mBatteryLow = false;
			}
		}
		break;
	}

	if (0.0f != mSystemMessageTarget) {
		if (mSystemMessageAlpha < 1.0f) {
			mSystemMessageAlpha += 0.1f * step;
			if (mSystemMessageAlpha > 1.0f) {
				mSystemMessageAlpha = 1.0f;
			}
		}
	} else if (mSystemMessageAlpha > 0.0f) {
		mSystemMessageAlpha -= 0.1f * step;
		if (mSystemMessageAlpha < 0.0f) {
			mSystemMessageAlpha = 0.0f;
			mSystemMessage      = NULL;
		}
	}
	if (mBatteryLow) {
		if (mBatteryAlpha < 1.0f) {
			mBatteryAlpha += 0.1f * step;
			if (mBatteryAlpha > 1.0f) {
				mBatteryAlpha = 1.0f;
			}
		}
	} else if (mBatteryAlpha > 0.0f) {
		mBatteryAlpha -= 0.1f * step;
		if (mBatteryAlpha < 0.0f) {
			mBatteryAlpha = 0.0f;
		}
	}
	if (1.0f == mBatteryAlpha) {
		mBatteryBlink -= 0.025f * step;
		if (mBatteryBlink <= 0.0f) {
			mBatteryBlink = 1.0f;
		}
	} else {
		mBatteryBlink = 1.0f;
	}
	if (mSystemMessageAlpha > 0.0f) {
		mSystemMessageBlink -= 0.025f * step;
		if (mSystemMessageBlink <= 0.0f) {
			mSystemMessageBlink = 1.0f;
		}
	} else {
		mSystemMessageBlink = 1.0f;
	}
}

/**
 * @note Address: 0x8001834C
 * @note Size: 0x18
 */
static void OnBatteryInfo(s32 chan, s32 result)
{
	if (result == 0) {
		gBatteryCheck.mDone = true;
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
	UpdateSystemMessage();
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
 * @note Address: 0x800182C0
 * @note Size: 0x8
 */
// the base game is ready once the font is read
BOOL CBase::BeforeUpdate()
{
	return mGs.mFont.LoadResource();
}

/**
 * @note Address: 0x800182BC
 * @note Size: 0x4
 */
void CBase::ResetNewGameState()
{
}

/**
 * @note Address: 0x8001829C
 * @note Size: 0x20
 */
void CBase::InitFont(const char* path)
{
	mGs.mFont.Init(path, gGame.mLanguage != 0);
}

/**
 * @note Address: 0x80017D4C
 * @note Size: 0x344
 */
// NONMATCHING: retail keeps the local CTpl's address in r29 from its construction on and loads the TPL
// data's address after the inline CFile constructor; the copy and the rest follow
// CBase's start: the runtime, the corner icons, the cameras, lights and fog, the timers, and the state
// the drive, reset and corner-message code keeps
void CBase::Start()
{
	InitGameRuntime(this);
	mLastRetrace   = 0;
	mRetraceStep   = 0;
	mStatus.mFlags = 0;
	{
		CTpl tpl;
		tpl.Bind(gCornerIconTplData);
		mIconTpl = tpl;
	}
	InitGamePerspectiveCamera(&mCameras[0], &gGame.mGs.mScreen);
	InitGameOrthoCamera(&mCameras[1], &gGame.mGs.mScreen);
	InitFourLightSet(&mLights[0]);
	InitTwoLightSet(&mLights[8]);
	InitGameFog(&mFog);
	OSInitStopwatch(&mStopwatch[0], "__$update$");
	OSInitStopwatch(&mStopwatch[1], "__$draw$  ");
	_41B0                    = 0;
	mLastDeviceType          = 0xFF;
	mLastProbe               = 0;
	mSystemMessage           = NULL;
	_41C3                    = 0;
	_41C2                    = 0;
	_41C1                    = 0;
	_41C0                    = 0;
	mBatteryLow              = false;
	mSystemMessageBlink      = 0.0f;
	mBatteryBlink            = 0.0f;
	mBatteryAlpha            = 0.0f;
	mSystemMessageAlpha      = 0.0f;
	mNoRemoteFrames          = 0.0f;
	mSystemMessageTarget     = 0.0f;
	_41E0                    = NULL;
	gBatteryCheck.mDone      = false;
	gBatteryCheck.mWait      = 0;
	mPrevDvdStatus           = 0;
	mDvdStatus               = 0;
	mDiskOk                  = TRUE;
	mDvdError                = false;
	mDiskChecking            = false;
	mDiskCheckBlock.userData = &mDvdStatus;
	gPowerButtonPressed      = FALSE;
	mResetRequested          = false;
	mResetType               = 0;
	OSSetPowerCallback(OnPowerButton);
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
