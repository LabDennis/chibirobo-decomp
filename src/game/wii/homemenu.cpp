// The game's side of the HOME Button Menu: it reads the library's files for the console language, opens
// the menu over a copy of the screen, pauses the game and its sound meanwhile, acts on what the menu was
// closed with, and shows the "not available" icon where the menu may not open.
//
// One file, .text 0x800AAD1C-0x800AC07C:
// - it starts where savemanager.cpp ends, behind that file's static initializer (0x800AACEC), which a
//   file's code always ends with
// - its .sdata2 literals, 0x805A4470-0x805A44D0, come in the order the functions from MoveCursorWithButtons
//   to DrawWiiHomeMenu first use them and nothing else reads them; the two before them (0x805A4468) are
//   ApplyWiiSaveSlot's
// - fn_800AC07C and fn_800AC11C behind it are XObj's InitializeObject and ResetObjectState, which sit next
//   to XObj::LoadFromPlacement (0x800AC238) on the GameCube too; from fn_800AC11C on the code reads the
//   next pool, 0x805A44D0
// - its other data, which only these functions read: .rodata 0x802D9360-0x802D9460 (the eleven paths, then
//   the pool of the strings the code uses), .sdata 0x805A0F50-0x805A0F80 (the path pointers), .sbss
//   0x805A1C38-0x805A1C40 and .bss 0x804569C0-0x80456A50 (the path table)
//
// Built with GC/3.0a5 and -ipa file, and linked: main.dol comes out the same with this file built from
// source, which is what settles the bounds above.
// - GC/3.0a3 to GC/3.0a5 all give retail's code. GC/3.0a5.2 differs in one
//   place: where OpenWiiHomeMenu looks whether the game is paused already, retail loads the 0 it stores
//   into mPausedGame before the branch on the flags (0x800AB3E8), 3.0a5.2 behind it. Wii/1.0 to Wii/1.3 turn
//   the load step's switch into a compare chain retail does not have and reach 91 to 96 % on the four
//   larger functions.
// - without -ipa file the eleven paths join the code's strings in @stringBase0; retail has each as an object
//   of its own in front of that pool.
//
// The cursor code follows the library's sample program: a Classic Controller's stick and buttons move the
// cursor, the Wii Remote's pointer places it otherwise.
#include "game/wii/homemenu.h"
#include "game/wii/cgame.h"
#include "game/wii/cpadact.h"
#include "revolution/mem.h"
#include "Dolphin/os.h"
#include "Dolphin/vi.h"
#include "stl/mem.h"

// the Classic Controller's direction buttons in KPADStatus
enum { KPAD_CL_BUTTON_UP = 0x0001, KPAD_CL_BUTTON_LEFT = 0x0002, KPAD_CL_BUTTON_DOWN = 0x4000, KPAD_CL_BUTTON_RIGHT = 0x8000 };

// the library's sound heap
#define HOME_MENU_SOUND_MEMORY 0x18700

extern "C" {
u8 SCGetLanguage();
extern MEMAllocator gWiiPadAllocator;
extern MEMiHeapHead* gWiiGameHeap;
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRefreshRate;
// 60 over the refresh rate
extern f32 gFrameRateScale;

// these two count, the sounds resume once every pause is taken back
void PauseGameSounds(CGame* game, f32 fadeSeconds);
void ResumeGameSounds(CGame* game, f32 fadeSeconds);
void ClearSoundEffects(XSound* sound, s32 fadeMilliseconds);
void ApplyStageSoundEffects(XSound* sound);
// where the icons of mIconTpl go, the screen's lower right corner
Vec GetCornerIconPosition(CGame* game);

BOOL StepWiiHomeMenuLoad(WiiHomeMenu* menu);
}

// a full stick deflection moves the cursor by one over this in a frame; the direction buttons move it
// as far, split between the axes on a diagonal
static const f32 sStickMoveCoe       = 2048.0f / 72.0f;
static const f32 sButtonMove         = 1.0f / sStickMoveCoe;
static const f32 sButtonMoveDiagonal = sButtonMove * 0.7071f;

static const char* sLayoutJapanese = "hbm/homeBtn.arc";
static const char* sLayoutEnglish  = "hbm/homeBtn_ENG.arc";
static const char* sLayoutGerman   = "hbm/homeBtn_GER.arc";
static const char* sLayoutFrench   = "hbm/homeBtn_FRA.arc";
static const char* sLayoutSpanish  = "hbm/homeBtn_SPA.arc";
static const char* sLayoutItalian  = "hbm/homeBtn_ITA.arc";
static const char* sLayoutDutch    = "hbm/homeBtn_NED.arc";
static const char* sSpeakerSounds  = "hbm/SpeakerSe.arc";
static const char* sMessages       = "hbm/home.csv";
static const char* sConfig         = "hbm/config.txt";
static const char* sMenuSounds     = "hbm/HomeButtonSe.arc";

static void* sSoundMemory;

static inline f32 AbsClamp(f32 value, f32 max)
{
	return value > max ? max : value < -max ? -max : value;
}

// written as a change to the argument: retail divides at run time (0x800AB42C), which this compiler does for
// a variable and not for a constant
static inline f32 MillisecondsToSeconds(f32 time)
{
	time /= 1000.0f;
	return time;
}

// moves the cursor by the direction buttons held; FALSE if there are none
static BOOL MoveCursorWithButtons(u32 buttons, Vec2* pos)
{
	buttons &= KPAD_CL_BUTTON_UP | KPAD_CL_BUTTON_LEFT | KPAD_CL_BUTTON_DOWN | KPAD_CL_BUTTON_RIGHT;
	switch (buttons) {
	case KPAD_CL_BUTTON_UP:
		pos->y -= sButtonMove;
		break;
	case KPAD_CL_BUTTON_LEFT:
		pos->x -= sButtonMove;
		break;
	case KPAD_CL_BUTTON_DOWN:
		pos->y += sButtonMove;
		break;
	case KPAD_CL_BUTTON_RIGHT:
		pos->x += sButtonMove;
		break;
	case KPAD_CL_BUTTON_UP | KPAD_CL_BUTTON_LEFT:
		pos->y -= sButtonMoveDiagonal;
		pos->x -= sButtonMoveDiagonal;
		break;
	case KPAD_CL_BUTTON_UP | KPAD_CL_BUTTON_RIGHT:
		pos->y -= sButtonMoveDiagonal;
		pos->x += sButtonMoveDiagonal;
		break;
	case KPAD_CL_BUTTON_DOWN | KPAD_CL_BUTTON_LEFT:
		pos->y += sButtonMoveDiagonal;
		pos->x -= sButtonMoveDiagonal;
		break;
	case KPAD_CL_BUTTON_DOWN | KPAD_CL_BUTTON_RIGHT:
		pos->y += sButtonMoveDiagonal;
		pos->x += sButtonMoveDiagonal;
		break;
	default:
		return FALSE;
	}
	pos->x = AbsClamp(pos->x, 1.0f);
	pos->y = AbsClamp(pos->y, 1.0f);
	return TRUE;
}

// moves the cursor by the stick; FALSE if it is at rest
static inline BOOL MoveCursorWithStick(f32 stickX, f32 stickY, Vec2* pos)
{
	f32 x = stickX / sStickMoveCoe;
	f32 y = stickY / sStickMoveCoe;
	if (x == 0.0f && y == 0.0f)
		return FALSE;
	pos->x += x;
	pos->y -= y;
	pos->x = AbsClamp(pos->x, 1.0f);
	pos->y = AbsClamp(pos->y, 1.0f);
	return TRUE;
}

// what follows the screen and is taken anew whenever the menu is created or opened
static inline void ResetWiiHomeMenu(WiiHomeMenu* menu)
{
	menu->mMenuStep        = 0;
	menu->mAdjust          = gGame.mAspectRatio != 0;
	menu->_65C             = 0;
	menu->mInfo.frameDelta = gFrameRateScale;
	menu->mInfo.adjust.x   = !gGame.mAspectRatio ? 1.0f : 832.0f / 608.0f;
}

extern "C" void InitWiiHomeMenu(WiiHomeMenu* menu)
{
	menu->mState = HomeMenuClosed;
	FreeFbTexture(&menu->mCapture);
	menu->mAdjust = FALSE;
	for (int i = 0; i < 4; ++i) {
		menu->mControllers.wiiCon[i].pos.x = 0;
		menu->mControllers.wiiCon[i].pos.y = 0;
	}
	menu->mLoadStep            = 0;
	menu->mInfo.mem            = NULL;
	menu->mInfo.sound_callback = NULL;
	menu->mInfo.backFlag       = FALSE;
	menu->mInfo.region         = SCGetLanguage();
	menu->mInfo.cursor         = 0;
	menu->mInfo.messageFlag    = 0;
	menu->mInfo.memSize        = 0x80000;
	menu->mInfo.frameDelta     = 1;
	menu->mInfo.adjust.x       = 1;
	menu->mInfo.adjust.y       = 1;
	menu->mInfo.pAllocator     = &gWiiPadAllocator;
}

// TRUE once the menu's files are read and the menu is created
extern "C" BOOL LoadWiiHomeMenu(WiiHomeMenu* menu)
{
	return StepWiiHomeMenuLoad(menu);
}

extern "C" BOOL StepWiiHomeMenuLoad(WiiHomeMenu* menu)
{
	// by the game's language, in the console's order
	static const char* sPaths[7][HomeMenuFileCount] = {
		{ sLayoutJapanese, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutEnglish, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutGerman, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutFrench, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutSpanish, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutItalian, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
		{ sLayoutDutch, sSpeakerSounds, sMessages, sConfig, sMenuSounds },
	};

	switch (menu->mLoadStep) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		if (menu->mFiles[menu->mLoadStep].ReadEx(sPaths[gGame.mLanguage][menu->mLoadStep], 0, 0))
			menu->mLoadStep++;
		break;
	case 5:
		menu->mInfo.layoutBuf     = menu->mFiles[HomeMenuFileLayout].mBuffer;
		menu->mInfo.spkSeBuf      = menu->mFiles[HomeMenuFileSpeaker].mBuffer;
		menu->mInfo.msgBuf        = menu->mFiles[HomeMenuFileMessage].mBuffer;
		menu->mInfo.configBuf     = menu->mFiles[HomeMenuFileConfig].mBuffer;
		menu->mInfo.configBufSize = menu->mFiles[HomeMenuFileConfig].mSize;
		menu->mLoadStep++;
		// fallthrough
	case 6:
		ResetWiiHomeMenu(menu);
		HBMCreate(&menu->mInfo);
		menu->mLoadStep++;
		break;
	}
	return menu->mLoadStep > 6;
}

// the HOME Button was pressed: opens the menu, or shows the icon where it may not open
extern "C" void OpenWiiHomeMenu(WiiHomeMenu* menu)
{
	if (menu->mState != HomeMenuClosed)
		return;
	if (!gGame.GetStatusFlags(0x0100000000000000ULL))
		return;

	if (gGame.CanOpenHomeMenu()) {
		menu->mState = HomeMenuCapture;
		CreateFbTexture(&menu->mCapture, gGame.mGs.mScreen.fbWidth / 2, gGame.mGs.mScreen.efbHeight / 2, GX_TF_I8, TRUE, NULL, "(hbmenu)");
		menu->mFade = 0.0f;
		ResetWiiHomeMenu(menu);
		if (gGame.GetStatusFlags(2)) {
			menu->mPausedGame = false;
		} else {
			menu->mPausedGame = true;
			gGame.SetStatusFlags(2);
		}
		PauseGameSounds(&gGame, MillisecondsToSeconds(300.0f));
		ClearSoundEffects(&gGame.mSound, 250);
	} else {
		menu->mState    = HomeMenuIconFadeIn;
		menu->mFade     = 0.0f;
		menu->mIconTime = 0.0f;
	}
}

// runs the open menu, or the icon. TRUE while the menu is open
extern "C" bool UpdateWiiHomeMenu(WiiHomeMenu* menu, u32 frames)
{
	if (menu->mState > HomeMenuClosed) {
		HBMControllerData* controllers = &menu->mControllers;
		s32 wpadResult[4];
		u32 padType[4];
		s32 kpadRead[4];
		KPADStatus* kpads[4];

		for (int i = 0; i < 4; ++i) {
			wpadResult[i]                      = WPADProbe(i, &padType[i]);
			controllers->wiiCon[i].use_devtype = 0;
			kpads[i]                           = GetWiiPad(i).mSamples;
			kpadRead[i]                        = GetWiiPad(i).mSampleCount;
			switch (wpadResult[i]) {
			case WPAD_ERR_BUSY:
			case WPAD_ERR_TRANSFER:
				if (wpadResult[i] == WPAD_ERR_BUSY) {
					// nothing was read: an empty sample, but for what tells which controller it is
					u8 device = kpads[i]->device;
					s8 error  = kpads[i]->error;
					memset(kpads[i], 0, sizeof(KPADStatus));
					kpads[i]->device = device;
					kpads[i]->error  = error;
				}
				// fallthrough
			case WPAD_ERR_NONE: {
				controllers->wiiCon[i].kpad = kpads[i];
				BOOL moved
				    = MoveCursorWithButtons(controllers->wiiCon[i].kpad->extension.classic.buttons.hold, &controllers->wiiCon[i].pos);
				moved = moved
				      | MoveCursorWithStick(controllers->wiiCon[i].kpad->extension.classic.leftStick.x,
				                            controllers->wiiCon[i].kpad->extension.classic.leftStick.y, &controllers->wiiCon[i].pos);
				if (!moved) {
					if (controllers->wiiCon[i].kpad->pointerValid > 0) {
						controllers->wiiCon[i].pos.x = controllers->wiiCon[i].kpad->pointer.position.x;
						controllers->wiiCon[i].pos.y = controllers->wiiCon[i].kpad->pointer.position.y;
					}
				}
				break;
			}
			case WPAD_ERR_NO_CONTROLLER:
			default:
				controllers->wiiCon[i].kpad = NULL;
				break;
			}
		}
		// only the first controller works the menu
		controllers->wiiCon[1].kpad = NULL;
		controllers->wiiCon[2].kpad = NULL;
		controllers->wiiCon[3].kpad = NULL;

		if (menu->mMenuStep == 0 && menu->_65C == 0)
			menu->mMenuStep = 1;
		switch (menu->mMenuStep) {
		case 1:
		case 2:
			HBMInit();
			sSoundMemory = MEMAllocFromExpHeapEx(gWiiGameHeap, HOME_MENU_SOUND_MEMORY, 4);
			HBMCreateSound(menu->mFiles[HomeMenuFileSound].mBuffer, sSoundMemory, HOME_MENU_SOUND_MEMORY);
			menu->mMenuStep += 2;
			// fallthrough
		case 3:
			HBMUpdateSound();
			if (HBMCalc(controllers) >= HBM_SELECT_HOMEBTN) {
				menu->mMenuStep        = 0;
				HBMSelectBtnNum button = HBMGetSelectBtnNum();
				menu->mState           = HomeMenuClosed;
				// back to the first load step
				menu->mLoadStep = 0;
				HBMDeleteSound();
				MEMFreeToExpHeap(gWiiGameHeap, sSoundMemory);
				FreeFbTexture(&menu->mCapture);
				if (menu->mPausedGame)
					gGame.ClearStatusFlags(2);
				if (button != HBM_SELECT_BTN1 && button != HBM_SELECT_BTN2)
					ResumeGameSounds(&gGame, MillisecondsToSeconds(300.0f));
				ApplyStageSoundEffects(&gGame.mSound);
				gWiiPointer.ResetRumble(TRUE);
				// the library played through the Wii Remotes' speakers, set them up again
				for (int i = 0; i < 4; ++i) {
					CPadOne& pad = GetWiiPad(i);
					if (pad.IsRemoteActive()) {
						pad.DisableRemoteSpeaker();
						while (nw4r::snd::detail::RemoteSpeakerManager::GetInstance().GetRemoteSpeaker(pad.mPort).IsAvailable())
							OSYieldThread();
						pad.EnableRemoteSpeaker();
					}
				}
				switch (button) {
				case HBM_SELECT_HOMEBTN:
					break;
				case HBM_SELECT_BTN1:
					OSReport("Return to WiiMenu.\n");
					gGame.mResetRequested = true;
					gGame.mResetType      = 4;
					break;
				case HBM_SELECT_BTN2:
					OSReport("Reset.\n");
					StartScreenFade(&gGame.mFade, 0, 0, 0, 0, 0, 0, 0, 255, 0.0f);
					gGame.RequestReset(3);
					VISetBlack(FALSE);
					break;
				case HBM_SELECT_BTN3:
					break;
				}
			} else if (gGame.mResetRequested) {
				HBMStartBlackOut();
			}
			break;
		}
		menu->mFade = 1.0f;
	} else if (menu->mState < HomeMenuClosed) {
		// the icon fades in over a quarter of a second, holds for one and fades out again
		switch (menu->mState) {
		case HomeMenuIconFadeIn:
			menu->mFade += frames * (1.0f / (0.25f * gRefreshRate));
			if (menu->mFade >= 1.0f) {
				menu->mFade  = 1.0f;
				menu->mState = HomeMenuIconHold;
			} else {
				break;
			}
			// fallthrough
		case HomeMenuIconHold: {
			s32 hold = gRefreshRate;
			menu->mIconTime += frames;
			if (menu->mIconTime > hold) {
				menu->mState    = HomeMenuIconFadeOut;
				menu->mIconTime = hold;
			} else {
				break;
			}
		}
			// fallthrough
		case HomeMenuIconFadeOut:
			menu->mFade -= frames * (1.0f / (0.25f * gRefreshRate));
			if (menu->mFade <= 0.0f) {
				menu->mFade  = 0.0f;
				menu->mState = HomeMenuClosed;
			}
			break;
		}
	}
	return menu->IsOpen();
}

extern "C" BOOL DrawWiiHomeMenu(WiiHomeMenu* menu)
{
	if (menu->mState > HomeMenuClosed) {
		if (menu->mState == HomeMenuCapture) {
			CopyFbTexture(&menu->mCapture, menu->mCapture.mFormat, FALSE, TRUE);
			menu->mState = HomeMenuOpen;
		} else {
			// the copied screen behind the menu
			GXSetProjection(gGame.GetScreenCamera()->mProjection, GX_ORTHOGRAPHIC);
			Mtx identity;
			PSMTXIdentity(identity);
			GXLoadPosMtxImm(identity, GX_PNMTX0);
			GXSetCurrentMtx(GX_PNMTX0);
			GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
			gGame.mGs.mCurTevStage->SetOrder(GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
			gGame.mGs.mCurTevStage->SetColorIn(GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
			gGame.mGs.mCurTevStage->SetColorOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			gGame.mGs.mCurTevStage->SetAlphaIn(GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
			gGame.mGs.mCurTevStage->SetAlphaOp(GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
			u32 color = (u32)(255.0f * menu->mFade) | 0xFFFFFF00;
			DrawFbTexture(&menu->mCapture, color, gPointerScreenWidth / 2, gPointerScreenHeight / 2, 0.0f, gPointerScreenWidth,
			              gPointerScreenHeight, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
			gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		}
		if (menu->mMenuStep >= 3) {
			// the library draws in its own 608x456 space, 832 wide on a 16:9 screen
			Mtx44 projection;
			if (menu->mAdjust)
				C_MTXOrtho(projection, 228.0f, -228.0f, -416.0f, 416.0f, 0.0f, 500.0f);
			else
				C_MTXOrtho(projection, 228.0f, -228.0f, -304.0f, 304.0f, 0.0f, 500.0f);
			GXSetProjection(projection, GX_ORTHOGRAPHIC);
			HBMSetAdjustFlag(menu->mAdjust);
			HBMDraw();
			GXSetProjection(gGame.GetScreenCamera()->mProjection, GX_ORTHOGRAPHIC);
		}
	} else if (menu->mState < HomeMenuClosed) {
		gGame.mIconTpl.mTex.Init(gGame.mIconTpl.mPalette->descriptorArray, 0);
		CTexObj icon = gGame.mIconTpl.mTex;
		Vec pos      = GetCornerIconPosition(&gGame);
		f32 width    = GXGetTexObjWidth(&icon);
		f32 height   = GXGetTexObjHeight(&icon);
		f32 x        = pos.x + width / 2.0f;
		f32 y        = pos.y - height / 2.0f;
		u32 color    = (u32)(255.0f * menu->mFade) | 0xFFFFFF00;
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		DrawGsTexturedQuad(&gGame.mGs, &icon, color, color, color, color, x, y, 0.0f, width, height, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
		gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	}
	return TRUE;
}
