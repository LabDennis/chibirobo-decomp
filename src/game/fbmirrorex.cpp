#include "game/hsd_jobj.h"
#include "game/cgame.h"
#include "game/progressive.h"
#include "game/cpad.h"
#include "game/video.h"
#include "game/globals.h"
#include "game/ut.h"
#include "game/cmdline.h"
#include "game/csound.h"
#include "game/mm.h"
#include "game/cgsdefaults.h"
#include "game/vistate.h"

#include "Dolphin/OS/OSFastCast.h"

extern "C" {
#include <sysdolphin/video.h>
#include <dolphin/gx/gx.h>
#include <sysdolphin/initialize.h>
}

// the scan mode prompt, the boot config, and the game core's start-up pieces (FbMirrorEx::Init at the
// head of this range is game.cpp's weak copy and lives there)

// draws a NULL-terminated list of lines centered on the screen, returns how many there were
static inline s32 DrawCenteredLines(const char** lines, s32 color)
{
	s32 count = 0;
	for (const char** line = lines; *line; line++, count++) { }
	f32 height        = 24.0f * count;
	f32 top           = 224.0f - height / 2.0f;
	const char** line = lines;
	for (s32 i = 0; i < count; ++line, ++i) {
		f32 width = MeasureGameTextWidth(&gGame.mGs, *line, TRUE, 24.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, color, *line, 320.0f - width / 2.0f, 24.0f * i + top, 0.0f, 24.0f, 24.0f);
	}
	return count;
}

/**
 * @note Address: 0x8001BCFC
 * @note Size: 0x5C8
 */
// NONMATCHING: .rodata pool offsets, register coloring in DrawCenteredLines, and 0.0f/*line load order.
// retail has the line pointer in r24 above i in r23 (0x8001BEBC addi r24, r1, 0x28). i declared before line, ++i before ++line,
// lines[i], a while loop for the count or one line pointer for both loops score the same or lower
extern "C" void DrawScanModePrompt()
{
	if (!gVideoModePrompt.step)
		return;
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsColorUpdate(gGame.mGs.mCurRenderMode, TRUE);
	SetGsAlphaUpdate(gGame.mGs.mCurRenderMode, FALSE);
	SetGsAlphaCompare(gGame.mGs.mCurRenderMode, 4, 0, 0, 3, 255);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	s32 alpha = 255.0f * gVideoModePrompt.alpha;
	u32 color = 0xffffff00 | (u8)alpha; // white text at the prompt's alpha
	switch (gVideoModePrompt.step) {
	case 1: // the question and its fade-out
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
		s32 count           = DrawCenteredLines(lines, color);
		const char* format  = GetLocalizedInterfaceText(choiceText);
		const s32 selected  = gVideoModePrompt.useStandardMode;
		const char* yes     = selected == 0 ? "\\B404040" : "\\B000000";
		const char* no      = selected == 1 ? "\\B404040" : "\\B000000";
		const u8 yesAlpha   = selected == 0 ? alpha : 0;
		const u8 noAlpha    = selected == 1 ? alpha : 0;
		const char* choices = FormatToStaticBuffer(format, yes, yesAlpha, "\\B00000000", no, noAlpha, "\\B00000000");
		f32 width           = MeasureGameTextWidth(&gGame.mGs, choices, TRUE, 24.0f);

		const f32 y = 24.0f * (count - 1) + (224.0f - (24.0f * count) / 2.0f);
		DrawGsProportionalTextFormat(&gGame.mGs, color, choices, 320.0f - width / 2.0f, y, 0.0f, 24.0f, 24.0f, yes, no);
		break;
	}
	case 4: // the "did it work" confirmation
	case 40: {
		if (gVideoModePrompt.useStandardMode == 0) {
#if defined(VERSION_PAL)
			const char* lines[] = { GetLocalizedInterfaceText(0x10f), GetLocalizedInterfaceText(0x110), NULL };
#elif defined(VERSION_JP)
			const char* lines[] = { GetLocalizedInterfaceText(3), GetLocalizedInterfaceText(4), GetLocalizedInterfaceText(6), NULL };
#else
			const char* lines[] = { GetLocalizedInterfaceText(3), GetLocalizedInterfaceText(4), GetLocalizedInterfaceText(5), NULL };
#endif
			DrawCenteredLines(lines, color);
		} else {
#if defined(VERSION_PAL)
			const char* lines[] = { GetLocalizedInterfaceText(0x112), GetLocalizedInterfaceText(0x113), NULL };
#elif defined(VERSION_JP)
			const char* lines[] = { GetLocalizedInterfaceText(3), GetLocalizedInterfaceText(5), GetLocalizedInterfaceText(6), NULL };
#else
			const char* lines[] = { GetLocalizedInterfaceText(6), GetLocalizedInterfaceText(7), GetLocalizedInterfaceText(8), NULL };
#endif
			DrawCenteredLines(lines, color);
		}
		break;
	}
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
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
		gVideoModePrompt.step = 100; // non-jp enters at 100
		break;
	case 100:
#endif
#if defined(VERSION_PAL)                                                      // PAL asks about eu-rgb60, not progressive
		if ((OSGetResetCode() >> 31) && gVideoModeResetState->hasSelection) { // a reset brought us back
			gVideoModePrompt.step              = 10;
			gVideoModePrompt.restoredSelection = gVideoModeResetState->hasSelection;
			gVideoModePrompt.useStandardMode   = gVideoModeResetState->useStandardMode;
			break;
		}
		gVideoModePrompt.step            = 1;
		gVideoModePrompt.framesRemaining = 10 * VIDEO_FPS; // ten seconds to answer
		gVideoModePrompt.alpha           = 0.0f;
		if (OSGetEuRgb60Mode() != 1) {
			gVideoModePrompt.useStandardMode = TRUE;
		}
#else
		if (VIGetDTVStatus() != 0) {                                              // a component cable is connected
			if ((OSGetResetCode() >> 31) && gVideoModeResetState->hasSelection) { // a reset brought us back
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
				gVideoModePrompt.framesRemaining = 600.0f; // ten seconds to answer
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
	case 1: { // asking: fade in, read the pad
		if (gVideoModePrompt.alpha < 1.0f && (gVideoModePrompt.alpha += PROGRESSIVE_FADE_STEP * frames) > 1.0f) {
			gVideoModePrompt.alpha = 1.0f;
		}
		BOOL done = FALSE;
		if ((gVideoModePrompt.framesRemaining -= frames) <= 0.0f) { // time's up
			done = TRUE;
		} else if (((ResetPad*)pad.mPad)->mTrigger & 0x100) { // A confirms
			done = TRUE;
		} else if (((ResetPad*)pad.mPad)->mRepeat & 1) { // d-pad left picks progressive
			gVideoModePrompt.useStandardMode = FALSE;
		} else if (((ResetPad*)pad.mPad)->mRepeat & 2) { // right picks standard
			gVideoModePrompt.useStandardMode = TRUE;
		}
		if (done) {
			gVideoModePrompt.step = 10;
		}
		break;
	}
	case 10: // fade out, then act on the choice
		if (gVideoModePrompt.alpha > 0.0f && (gVideoModePrompt.alpha -= PROGRESSIVE_FADE_STEP * frames) < 0.0f) {
			gVideoModePrompt.alpha = 0.0f;
		}
		if (gVideoModePrompt.alpha == 0.0f) {
			if (!gVideoModePrompt.useStandardMode) {
				gVideoModePrompt.step            = 2;
				gVideoModePrompt.framesRemaining = 2.0f;
				HSD_VISetBlack(TRUE); // blank the screen while the mode flips
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
			HSD_VISetBlack(FALSE); // lift the black and show the new mode
		}
		break;
	case 4: // the second prompt: confirm the new mode
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
	case 40: // fade out and finish
		if (gVideoModePrompt.alpha > 0.0f && (gVideoModePrompt.alpha -= PROGRESSIVE_FADE_STEP * frames) < 0.0f) {
			gVideoModePrompt.alpha = 0.0f;
		}
		if (gVideoModePrompt.alpha == 0.0f) {
			gVideoModePrompt.step = 0;
		}
		break;
	}

	if (gVideoModePrompt.pendingMode >= 0) { // apply a switch the steps queued
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

extern "C" {
u32 OSGetSoundMode(void);
char* strtok(char*, const char*);
int strcmp(const char*, const char*);
int atoi(const char*);
s32 GetStageNum();
const char* GetStageFileName(s32);
s32 GetDefaultStageIndex();
}

BootConfigEntry gBootConfigEntries[] = {
	{ "STAGE_NO", BootStage, 0 },
	{ "DRAW_XZGRID", BootFlag, 1ULL << 18 },
	{ "DRAW_WORLDAXIS", BootFlag, 1ULL << 19 },
	{ "DRAW_LIGHTMARK", BootFlag, 1ULL << 20 },
	{ "DRAW_SAFETYFRAME", BootFlag, 1ULL << 21 },
	{ "DRAW_FRAMEMETER", BootFlag, 1ULL << 22 },
	{ "DRAW_HEAPMETER", BootFlag, 1ULL << 23 },
	{ "DRAW_CROSSHAIR", BootCrosshair, 1ULL << 50 },
	{ "DRAW_DEBUGSTRING", BootFlag, 1ULL << 24 },
	{ "DRAW_COLLISION", BootFlag, 1ULL << 25 },
	{ "DRAW_CAMCOLLISION", BootFlag, 1ULL << 53 },
	{ "DRAW_OBJINFO", BootFlag, 1ULL << 26 },
	{ "DRAW_PLAYTIME", BootFlag, 1ULL << 27 },
	{ "PRINT_FILEINFO", BootFlag, 1ULL << 28 },
	{ "ENABLE_LOGO", BootFlag, 1ULL << 34 },
	{ "ENABLE_TITLE", BootFlag, 1ULL << 29 },
	{ "ENABLE_GAMEOVER", BootFlag, 1ULL << 30 },
	{ "ENABLE_TIMEUP", BootFlag, 1ULL << 46 },
	{ "ENABLE_DAYNIGHT", BootFlag, 1ULL << 49 },
	{ "ENABLE_DEBUGOPR", BootFlag, 1ULL << 31 },
	{ "ENABLE_ATC_COPTER", BootSetting, 1 },
	{ "ENABLE_ATC_POPPER", BootSetting, 2 },
	{ "ENABLE_ATC_RADAR", BootSetting, 3 },
	{ "ENABLE_RELMODULE", BootFlag, 1ULL << 32 },
	{ "ENABLE_ARAMCACHE", BootFlag, 1ULL << 41 },
	{ "ENABLE_PRECACHE", BootFlag, 1ULL << 42 },
	{ "ENABLE_ARCHIVER", BootUnrestrictedFlag, 1ULL << 33 },
	{ "ENABLE_SCREENSHOT", BootFlag, 1ULL << 52 },
	{ "ATC_MENU_TYPE", BootSetting, 0 },
	{ "LIFTPLUG_TYPE", BootSetting, 4 },
	{ "ATC_ACTION_TYPE", BootSetting, 5 },
	{ "TOOL_ACTION_TYPE", BootSetting, 6 },
	{ "TITLE_CURSOR", BootSetting, 7 },
	{ "EVENT_LOG", BootEventLog, 0 },
	{ NULL, BootFlag, 0 },
};

VideoModePrompt gVideoModePrompt = { 0, -1, 0.0f, 0.0f, 0, 0 };

/**
 * @note Address: 0x8001C89C
 * @note Size: 0x28
 */
extern "C" void ResetProgressiveScanPrompt(void)
{
	gVideoModePrompt.step              = 0;
	gVideoModePrompt.pendingMode       = -1;
	gVideoModePrompt.framesRemaining   = 0.0f;
	gVideoModePrompt.restoredSelection = 0;
	gVideoModePrompt.useStandardMode   = 0;
}

extern "C" void LoadBootConfig()
{
	CGame& game = gGame;
	u64& flags  = game.mStatus.mFlags;
	flags |= 1ULL << 34; // the defaults: logo, title, game over, rel, archiver
	flags |= 1ULL << 29;
	flags |= 1ULL << 30;
	flags |= 1ULL << 32;
	flags |= 1ULL << 33;
	if (OSGetSoundMode() == 1) // 1 = stereo
		flags |= 1ULL << 36;
	else
		flags &= ~(1ULL << 36);
	flags |= 1ULL << 35;
	flags |= 1ULL << 14;
	flags |= 1ULL << 16;
	flags |= 1ULL << 38;
	flags |= 1ULL << 40;
	flags |= 1ULL << 41;
	flags |= 1ULL << 42;
	flags |= 1ULL << 47;
	flags |= 1ULL << 48;
	flags |= 1ULL << 49;
	flags |= 1ULL << 54;

	CFileText file;
	file.mPos  = 0;
	s32 result = file.Read("/qp.ini", 0, 0);
	if (result == 0) {
		char line[256];
		BOOL restricted = FALSE;
		while (file.GetLine(line) == TRUE) {
			for (char* p = line; *p; ++p)
				if (*p == ';') { // ';' starts a comment
					*p = 0;
					break;
				}
			char* key = strtok(line, " \t,=");
			if (!key)
				continue;
			if (strcmp(key, "&") == 0) // '&' restricts the rest of the file
				restricted = TRUE;
			char* text = strtok(NULL, " \t,=");
			if (!text)
				continue;
			for (s32 i = 0; gBootConfigEntries[i].name; ++i) {
				const BootConfigEntry* entry = &gBootConfigEntries[i];
				if (strcmp(key, entry->name) != 0)
					continue;
				switch (entry->kind) {
				case BootFlag:
					if (restricted) // restricted files only get BootUnrestrictedFlag and BootCrosshair
						break;
				case BootUnrestrictedFlag:
					if (atoi(text))
						flags |= entry->value;
					else
						flags &= ~entry->value;
					break;
				case BootStage:
					if (!restricted) {
						s32 stage = gCmdLineArgs.mStage >= 0 ? gCmdLineArgs.mStage : atoi(text);
						if (stage >= 100) // 100+ indexes the rel stages
							stage += GetStageNum() - 99;
						gCmdLineArgs.mStage = stage;
						gGame.SetStage(stage);
						GetStageFileName(stage);
					}
					break;
				case BootCrosshair: {
					flags |= entry->value;
					s32 x = atoi(text);
					text  = strtok(NULL, " \t,=");
					if (text) {
						gCmdLineArgs.mCrosshairX = x;
						gCmdLineArgs.mCrosshairY = atoi(text);
						atoi(text);
					}
					break;
				}
				// restricted like the settings, but nothing is left of it in this build
				case BootEventLog:
					if (restricted)
						break;
					break;
				case BootSetting:
					if (restricted)
						break;
					switch (entry->value) {
					case 0:
						gCmdLineArgs.mAttachmentMenuType = atoi(text);
						break;
					case 1:
						gCmdLineArgs.mCopterLevelOverride = atoi(text) + 1;
						break;
					case 2:
						gCmdLineArgs.mPopperLevelOverride = atoi(text) + 1;
						break;
					case 3:
						gCmdLineArgs.mRadarLevelOverride = atoi(text) + 1;
						break;
					case 4:
						gCmdLineArgs.mLiftPlugFlags = atoi(text);
						break;
					case 5:
						gCmdLineArgs.mAttachmentActionType = atoi(text);
						break;
					case 6:
						gCmdLineArgs.mToolActionType = atoi(text);
						break;
					case 7:
						gCmdLineArgs.mTitleCursor = atoi(text);
						break;
					}
					atoi(text);
					break;
				}
			}
		}
		file.Close();
	} else if (result == -3) { // no qp.ini on the disc: use the default stage
		s32 stage           = gCmdLineArgs.mStage >= 0 ? gCmdLineArgs.mStage : GetDefaultStageIndex();
		gCmdLineArgs.mStage = stage;
		gGame.SetStage(stage);
	}
}

extern "C" {
extern GXRenderModeObj GXPal528IntDf;
void HSD_SetPixelFormat(s32, s32);
void HSD_SetZSortMode(s32, s32);
void GXSetClipMode(s32);
}

// the game core's start-up pieces: the light sets and cameras the scenes start from and the
// system setup

// the light sets and cameras the scenes start from. each one sets up the objects it gets and
// hands them back

/**
 * @note Address: 0x8001CEFC
 * @note Size: 0xD4
 */
extern "C" CLight* InitTwoLightSet(CLight* lights)
{
	CLight* light;

	lights[0].Init(LOBJ_AMBIENT | LOBJ_DIFFUSE, FALSE);
	lights[0].mColor.r = 0x80;
	lights[0].mColor.g = 0x80;
	lights[0].mColor.b = 0x80;
	lights[0].mColor.a = 0xFF;
	HSD_LObjSetColor(&lights[0], lights[0].mColor);

	lights[0].next = &lights[1];
	light          = &lights[1];
	light->Init(LOBJ_INFINITE | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(1.0f, -1.0f, 1.0f);
	light->mColor.r = 0xFF;
	light->mColor.g = 0xFF;
	light->mColor.b = 0xFF;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);
	light->next = NULL;
	return lights;
}

/**
 * @note Address: 0x8001CFD0
 * @note Size: 0x1D0
 */
extern "C" CLight* InitFourLightSet(CLight* lights)
{
	CLight* light;

	lights[0].Init(LOBJ_AMBIENT | LOBJ_DIFFUSE, FALSE);
	lights[0].mColor.r = 200;
	lights[0].mColor.g = 200;
	lights[0].mColor.b = 200;
	lights[0].mColor.a = 0xFF;
	HSD_LObjSetColor(&lights[0], lights[0].mColor);

	lights[0].next = &lights[1];
	light          = &lights[1];
	light->Init(LOBJ_INFINITE | LOBJ_DIFFUSE, TRUE);
	light->SetPosition(-2.0f, 20.0f, 2.0f);
	light->mColor.r = 200;
	light->mColor.g = 200;
	light->mColor.b = 200;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);

	light->next = NULL;

	light = &lights[2];
	light->Init(LOBJ_POINT | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(0.0f, 20.0f, -20.0f);
	light->mColor.r = 0;
	light->mColor.g = 0xFF;
	light->mColor.b = 0xFF;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);

	light->next = NULL;

	light = &lights[3];
	light->Init(LOBJ_SPOT | LOBJ_DIFFUSE, FALSE);
	light->SetPosition(0.0f, 20.0f, 0.0f);
	light->SetInterest(0.0f, 0.0f, 20.0f);
	light->mColor.r = 0xFF;
	light->mColor.g = 0;
	light->mColor.b = 0;
	light->mColor.a = 0xFF;
	HSD_LObjSetColor(light, light->mColor);
	light->next = NULL;
	return lights;
}

/**
 * @note Address: 0x8001D1D0
 * @note Size: 0xCC
 */
extern "C" CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* rmode)
{
	camera->Init(0.0f, 10000.0f, rmode);
#if defined(VERSION_PAL)
	camera->SetFrustum(0.0f, 448.0f, 0.0f, 640.0f);
#endif
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetEye(0.0f, 0.0f, 5000.0f);
	camera->SetInterest(0.0f, 0.0f, 0.0f);
	camera->CalcAngle();
	camera->CalcDistance();
	return camera;
}

/**
 * @note Address: 0x8001D2D0
 * @note Size: 0xF4
 */
extern "C" CCamera* InitMotionControlCameraDefaults(CCamera* camera)
{
	camera->Init(1.0f, 80000.0f, 30.0f, &gGame.mGs.mScreen);
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f); // pitched 24 degrees, yawed -180
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

/**
 * @note Address: 0x8001D3D4
 * @note Size: 0xEC
 */
extern "C" CCamera* InitGamePerspectiveCamera(CCamera* camera, GXRenderModeObj* rmode)
{
	camera->Init(1.0f, 80000.0f, 30.0f, rmode);
	camera->SetScale(1.0f, 1.0f, 1.0f);
	camera->SetInterest(-4.2f, 0.0f, -64.0f);
	camera->SetAngle(0.41887903f, -3.1415927f, 0.0f);
	camera->SetDistance(400.0f);
	camera->SetFov(18.0f);
	camera->CalcEye();
	return camera;
}

// the system setup the game runs before anything else

extern "C" {
extern GXRenderModeObj GXPal528IntDf;
void srand(u32 seed);
u32 OSGetTick(void);
void* OSInitAlloc(void* arenaStart, void* arenaEnd, int maxHeaps);
u32 OSGetResetCode(void);
void OSSetSaveRegion(void* start, void* end);
void DVDSetAutoFatalMessaging(BOOL enable);
BOOL DVDChangeDir(char* dirName);
u32 ARInit(u32* stackIndexAddr, u32 numEntries);
void ARQInit(void);
void HSD_VIDrawDoneXFB(s32 idx);
}

/**
 * @note Address: 0x8001D4C0
 * @note Size: 0x2C
 */
extern "C" void InitGameDVD(void)
{
	DVDSetAutoFatalMessaging(TRUE);
	DVDChangeDir((char*)"/qp/"); // the game's data root
}

// ARInit's allocation stack
u32 gAramBlockLengths[8];

/**
 * @note Address: 0x8001D4EC
 * @note Size: 0x30
 */
extern "C" void InitGameARAM(void)
{
	ARInit(gAramBlockLengths, 8);
	ARQInit();
}

extern "C" void GameXFBDrawDoneCallback(int idx);

/**
 * @note Address: 0x8001D51C
 * @note Size: 0x384
 */
// NONMATCHING: retail loads the scalar render-mode arguments (color and alpha update, z before
// texture, dither, cull) from one-object .sdata2/.sbss2 constants where this build has li, and its
// struct arguments' .rodata lies in another order behind the unit's 0x48-byte header
extern "C" void InitGameRenderState(CBase* base)
{
	CGs* gs = &base->mGs;
#if defined(VERSION_PAL)
	SetGsScreenModeAndRenderDefaults(gs, 21, 0, 0, 0);
	gs->mScreen.mHeightScale = gGame.mGs.mScreen.xfbHeight / 448.0f;
	gs->mScreen.viHeight     = 480;
#else
	SetGsScreenModeAndRenderDefaults(gs, 4, 0, 16, 0);
#endif
#if !defined(VERSION_JP)
	SetGsFixedGlyphWidthScale(gs, 1.2f);
#endif
	HSD_SetPixelFormat(1, 0);
	GXSetPixelFmt(1, 0);
	GXSetCopyFilter(0, NULL, 0, NULL);

	InitializeVideoClearState((GXColor) { 0, 0, 0, 0 });
	GXSetClipMode(0);
	HSD_SetZSortMode(1, 1);

	ConfigureGsRenderMode(gs, NULL, GX_TRUE, GX_FALSE, (GsBlendMode) { 1, 4, 5, 15 }, (GsAlphaCompare) { 4, 0, 0, 3, 255 },
	                      (GsDstAlpha) { 0, 0 }, (GsZMode) { 1, 7, 1 }, GX_TRUE, GX_TRUE, 2, (GsLineWidth) { 12, 5 },
	                      (GsLineWidth) { 24, 5 });
	ConfigureGsPrimaryTevStage(gs);
	ConfigureGsChannel(gs, NULL, GX_COLOR0, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 2, 2 }, (GXColor) { 0, 0, 0, 255 },
	                   (GXColor) { 255, 255, 255, 255 });
	ConfigureGsChannel(gs, NULL, GX_ALPHA0, (GsChanCtrl) { GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0, 0, 2 }, (GXColor) { 0, 0, 0, 255 },
	                   (GXColor) { 255, 255, 255, 255 });
	if (gs->mCurRenderMode)
		ApplyGsRenderMode(gs->mCurRenderMode, TRUE);
	if (gs->mCurTevStage)
		ApplyGsTevStage(gs->mCurTevStage, TRUE);
	if (gs->mCurChannel)
		ApplyGsChannel(gs->mCurChannel, TRUE);
	HSD_VISetUserGXDrawDoneCallback(GameXFBDrawDoneCallback);
}

/**
 * @note Address: 0x8001D8A0
 * @note Size: 0x20
 */
extern "C" void GameXFBDrawDoneCallback(int idx)
{
	HSD_VIDrawDoneXFB(idx);
}

// the two words the game keeps over a reset
extern "C" {
SavedVideoMode* gVideoModeResetState;
}

/**
 * @note Address: 0x8001D8C0
 * @note Size: 0xE8
 */
extern "C" void InitGameRuntime(void)
{
	OSInitFastCast();
	srand(OSGetTick());
	void* lo = OSGetArenaLo();
	void* hi = OSGetArenaHi();
	OSSetArenaLo(OSInitAlloc(lo, hi, 1));
	HSD_SetInitParameter(HSD_INIT_FIFO_SIZE, 0x40000);
	HSD_SetInitParameter(HSD_INIT_XFB_MAX_NUM, 3);
#if defined(VERSION_PAL)
	HSD_SetInitParameter(HSD_INIT_RENDER_MODE_OBJ, &GXPal528IntDf);
#endif
	HSD_Init();
	mm::Init();
	gVideoModeResetState = (SavedVideoMode*)OSAllocFromHeap(__OSCurrHeap, sizeof(SavedVideoMode));
	if (!(OSGetResetCode() >> 31)) { // a set sign bit keeps what was saved

		gVideoModeResetState->hasSelection    = 0;
		gVideoModeResetState->useStandardMode = 1;
	}
	OSSetSaveRegion(gVideoModeResetState, gVideoModeResetState + 1);
}

#if defined(VERSION_PAL)
f32 gProgressiveFadeFrames = 0.15f * FRAME_RATE;
#endif

/**
 * @note Address: 0x8001D9A8
 * @note Size: 0x8
 */
// CCameraActionMap's GetTarget
XObj* CCameraActionMap::GetTarget()
{
	return mTarget;
}
