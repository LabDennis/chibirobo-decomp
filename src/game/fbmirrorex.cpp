#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/progressive.h"
#include "game/cpad.h"
#include "game/video.h"
#include "game/globals.h"
#include "game/ut.h"

extern "C" {
#include <sysdolphin/video.h>
}

/**
 * @note Address: 0x8001BC34
 * @note Size: 0xC8
 */
// sets the mirror's camera up for its 256 square texture
void FbMirrorEx::Init()
{
	mJoint = NULL;
	mCamera.Init(0.1f, 32768.0f, 0.5f, -0.5f, -0.5f, 0.5f, &gGame.mGs.mScreen);
	mCamera.SetViewport(0.0f, 256.0f, 0.0f, 256.0f);
	mCamera.SetScissor(0, 0x100, 0, 0x100);
	Reset();
	mOwner = NULL;
}

static inline void DrawScanModeConfirmation(u32 color, bool alternate)
{
#if defined(VERSION_PAL)
	const s32 first     = alternate ? 0x112 : 0x10f;
	const char* lines[] = { GetLocalizedInterfaceText(first), GetLocalizedInterfaceText(first + 1), NULL };
#elif defined(VERSION_JP)
	const char* lines[]
	    = { GetLocalizedInterfaceText(3), GetLocalizedInterfaceText(alternate ? 5 : 4), GetLocalizedInterfaceText(6), NULL };
#else
	const s32 first = alternate ? 6 : 3;
	const char* lines[]
	    = { GetLocalizedInterfaceText(first), GetLocalizedInterfaceText(first + 1), GetLocalizedInterfaceText(first + 2), NULL };
#endif
	s32 count = 0;
	for (const char* const* line = lines; *line; ++line)
		++count;
	f32 height = 24.0f * count;
	f32 top    = 224.0f - height * 0.5f;
	for (s32 i = 0; i < count; ++i) {
		f32 width = MeasureGameTextWidth(&gGame.mGs, lines[i], TRUE, 24.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, color, lines[i], 320.0f - width * 0.5f, 24.0f * i + top, 0.0f, 24.0f, 24.0f);
	}
}

/**
 * @note Address: 0x8001BCFC
 * @note Size: 0x5C8
 */
extern "C" void DrawScanModePrompt()
{
	if (!gVideoModePrompt.step)
		return;
	CGs& gs = gGame.mGs;
	SetGsChannelEnabled(&gs, 0, 0);
	SetGsColorUpdate(gs.mCurRenderMode, TRUE);
	SetGsAlphaUpdate(gs.mCurRenderMode, FALSE);
	SetGsAlphaCompare(gs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 0, 7, 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	u8 alpha  = (u8)(s32)(255.0f * gVideoModePrompt.alpha);
	u32 color = 0xffffff00 | alpha;
	switch (gVideoModePrompt.step) {
	case 1:
	case 10: {
#if defined(VERSION_PAL)
		const char* lines[]  = { GetLocalizedInterfaceText(0x107),
			                     GetLocalizedInterfaceText(0x108),
			                     GetLocalizedInterfaceText(0x109),
			                     "",
			                     GetLocalizedInterfaceText(0x10a),
			                     GetLocalizedInterfaceText(0x10b),
			                     GetLocalizedInterfaceText(0x10c),
			                     GetLocalizedInterfaceText(0x10d),
			                     "",
			                     "",
			                     NULL };
		const s32 choiceText = 0x10e;
#else
		const char* lines[]  = { GetLocalizedInterfaceText(0), GetLocalizedInterfaceText(1), "", "", NULL };
		const s32 choiceText = 2;
#endif
		s32 count = 0;
		for (const char* const* line = lines; *line; ++line)
			++count;
		f32 height = 24.0f * count;
		f32 top    = 224.0f - height * 0.5f;
		for (s32 i = 0; i < count; ++i) {
			f32 width = MeasureGameTextWidth(&gGame.mGs, lines[i], TRUE, 24.0f);
			DrawGsProportionalTextFormat(&gGame.mGs, color, lines[i], 320.0f - width * 0.5f, 24.0f * i + top, 0.0f, 24.0f, 24.0f);
		}
		const char* format  = GetLocalizedInterfaceText(choiceText);
		const s32 selected  = gVideoModePrompt.useStandardMode;
		const char* yes     = selected == 0 ? "\\B404040" : "\\B000000";
		const char* no      = selected == 1 ? "\\B404040" : "\\B000000";
		const u8 yesAlpha   = selected == 0 ? alpha : 0;
		const u8 noAlpha    = selected == 1 ? alpha : 0;
		const char* choices = FormatToStaticBuffer(format, yes, yesAlpha, "\\B00000000", no, noAlpha, "\\B00000000");
		f32 width           = MeasureGameTextWidth(&gs, choices, TRUE, 24.0f);

		const f32 y = 24.0f * (count - 1) + (224.0f - (24.0f * count) * 0.5f);
		DrawGsProportionalTextFormat(&gGame.mGs, color, choices, 320.0f - width * 0.5f, y, 0.0f, 24.0f, 24.0f, yes, no);
		break;
	}
	case 4:
	case 40: {
		if (gVideoModePrompt.useStandardMode == 0)
			DrawScanModeConfirmation(color, false);
		else
			DrawScanModeConfirmation(color, true);
		break;
	}
	}
	SetGsTevMode(gs.mCurTevStage, 4);
}

// the pad as the progressive question reads it
struct ResetPad {
	u8 _00[0x4];  // _00
	u32 mButton;  // _04
	u8 _08[0xC];  // _08
	u32 mTrigger; // _14
	u32 mRepeat;  // _18
};

extern "C" {
u32 VIGetDTVStatus(void);
u32 OSGetResetCode(void);
u32 OSGetProgressiveMode(void);
void OSSetProgressiveMode(u32 mode);
u32 OSGetEuRgb60Mode(void);
void OSSetEuRgb60Mode(u32 mode);
}

#if defined(VERSION_PAL)
#define PROGRESSIVE_FADE_STEP (1.0f / gProgressiveFadeFrames)
#define VIDEO_FPS             gVideoFramesPerSecond
#else
#define PROGRESSIVE_FADE_STEP 0.11111111f
#define VIDEO_FPS             60
#endif

/**
 * @note Address: 0x8001C2C4
 * @note Size: 0x590
 */
// the progressive scan question: asked when the cable allows it and the console is already
// in progressive mode or B is held, the answer goes to the scan mode and the saved setting
extern "C" BOOL UpdateProgressiveScanPrompt(u32 frames)
{
	CPad pad(0);

	switch (gVideoModePrompt.step) {
	case 0:
#if !defined(VERSION_JP)
		gVideoModePrompt.step = 100;
		break;
	case 100:
#endif
#if defined(VERSION_PAL)
		if ((OSGetResetCode() >> 31) && gVideoModeResetState->hasSelection) {
			gVideoModePrompt.step              = 10;
			gVideoModePrompt.restoredSelection = gVideoModeResetState->hasSelection;
			gVideoModePrompt.useStandardMode   = gVideoModeResetState->useStandardMode;
			break;
		}
		gVideoModePrompt.step            = 1;
		gVideoModePrompt.framesRemaining = 10 * VIDEO_FPS;
		gVideoModePrompt.alpha           = 0.0f;
		if (OSGetEuRgb60Mode() != 1) {
			gVideoModePrompt.useStandardMode = TRUE;
		}
#else
		if (VIGetDTVStatus() != 0) {
			if ((OSGetResetCode() >> 31) && gVideoModeResetState->hasSelection) {
				gVideoModePrompt.step              = 10;
				gVideoModePrompt.restoredSelection = gVideoModeResetState->hasSelection;
				gVideoModePrompt.useStandardMode   = gVideoModeResetState->useStandardMode;
				break;
			}
			BOOL ask = FALSE;
			if (OSGetProgressiveMode() == 1) {
				ask = TRUE;
			} else if (((ResetPad*)pad.mPad)->mButton & 0x200) {
				ask = TRUE;
			}
			if (ask) {
				gVideoModePrompt.framesRemaining = 600.0f;
				gVideoModePrompt.step            = 1;
				gVideoModePrompt.alpha           = 0.0f;
			} else {
#if !defined(VERSION_JP)
				gVideoModePrompt.step = 0;
#endif
				OSSetProgressiveMode(0);
				gVideoModeResetState->hasSelection    = TRUE;
				gVideoModeResetState->useStandardMode = TRUE;
			}
		} else {
#if !defined(VERSION_JP)
			gVideoModePrompt.step = 0;
#endif
			OSSetProgressiveMode(0);
			gVideoModeResetState->hasSelection    = TRUE;
			gVideoModeResetState->useStandardMode = TRUE;
		}
#endif
		break;
	case 1: {
		if (gVideoModePrompt.alpha < 1.0f && (gVideoModePrompt.alpha += PROGRESSIVE_FADE_STEP * frames) > 1.0f) {
			gVideoModePrompt.alpha = 1.0f;
		}
		BOOL done = FALSE;
		if ((gVideoModePrompt.framesRemaining -= frames) <= 0.0f) {
			done = TRUE;
		} else if (((ResetPad*)pad.mPad)->mTrigger & 0x100) {
			done = TRUE;
		} else if (((ResetPad*)pad.mPad)->mRepeat & 1) {
			gVideoModePrompt.useStandardMode = FALSE;
		} else if (((ResetPad*)pad.mPad)->mRepeat & 2) {
			gVideoModePrompt.useStandardMode = TRUE;
		}
		if (done) {
			gVideoModePrompt.step = 10;
		}
		break;
	}
	case 10:
		if (gVideoModePrompt.alpha > 0.0f && (gVideoModePrompt.alpha -= PROGRESSIVE_FADE_STEP * frames) < 0.0f) {
			gVideoModePrompt.alpha = 0.0f;
		}
		if (gVideoModePrompt.alpha == 0.0f) {
			if (!gVideoModePrompt.useStandardMode) {
				gVideoModePrompt.step            = 2;
				gVideoModePrompt.framesRemaining = 2.0f;
				HSD_VISetBlack(TRUE);
			} else {
				gVideoModePrompt.step            = 4;
				gVideoModePrompt.pendingMode     = gVideoModePrompt.useStandardMode ? FALSE : TRUE;
				gVideoModePrompt.framesRemaining = 10 * VIDEO_FPS;
			}
		}
		break;
	case 2:
		if ((gVideoModePrompt.framesRemaining -= frames) <= 0.0f) {
			gVideoModePrompt.step            = 3;
			gVideoModePrompt.pendingMode     = gVideoModePrompt.useStandardMode ? FALSE : TRUE;
			gVideoModePrompt.framesRemaining = 2 * VIDEO_FPS;
		}
		break;
	case 3:
		if ((gVideoModePrompt.framesRemaining -= frames) <= 0.0f) {
			gVideoModePrompt.step            = 4;
			gVideoModePrompt.framesRemaining = 10 * VIDEO_FPS;
			HSD_VISetBlack(FALSE);
		}
		break;
	case 4:
		if ((OSGetResetCode() >> 31) && gVideoModePrompt.restoredSelection) {
			gVideoModePrompt.step = 40;
		} else {
			if (gVideoModePrompt.alpha < 1.0f && (gVideoModePrompt.alpha += PROGRESSIVE_FADE_STEP * frames) > 1.0f) {
				gVideoModePrompt.alpha = 1.0f;
			}
			BOOL done = FALSE;
			if ((gVideoModePrompt.framesRemaining -= frames) <= 0.0f) {
				done = TRUE;
			} else if (((ResetPad*)pad.mPad)->mTrigger & 0x100) {
				done = TRUE;
			}
			if (done) {
				gVideoModePrompt.step = 40;
			}
		}
		break;
	case 40:
		if (gVideoModePrompt.alpha > 0.0f && (gVideoModePrompt.alpha -= PROGRESSIVE_FADE_STEP * frames) < 0.0f) {
			gVideoModePrompt.alpha = 0.0f;
		}
		if (gVideoModePrompt.alpha == 0.0f) {
			gVideoModePrompt.step = 0;
		}
		break;
	}

	if (gVideoModePrompt.pendingMode >= 0) {
		if (gVideoModePrompt.pendingMode != 0) {
#if defined(VERSION_PAL)
			OSSetEuRgb60Mode(1);
			CGsScreen* screen = &gGame.mGs.mScreen;
			ConfigureGsScreenMode(screen, 28, 0, 18);
			screen->mHeightScale  = screen->xfbHeight / 444.0f;
			gVideoFramesPerSecond = 60;
			CCamera* first        = &gGame.mCameras[0];
			CCamera* second       = &gGame.mCameras[1];
			first->SetViewport(0.0f, 0.0f, screen);
			second->SetViewport(0.0f, 0.0f, screen);
#else
			OSSetProgressiveMode(1);
			ConfigureGsScreenMode(&gGame.mGs.mScreen, 8, 0, 16);
#endif
		} else {
#if defined(VERSION_PAL)
			OSSetEuRgb60Mode(0);
#else
			OSSetProgressiveMode(0);
#endif
		}
		gVideoModePrompt.pendingMode          = -1;
		gVideoModeResetState->hasSelection    = TRUE;
		gVideoModeResetState->useStandardMode = gVideoModePrompt.useStandardMode;
	}
	return !gVideoModePrompt.step;
}
