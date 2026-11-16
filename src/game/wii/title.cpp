// The title screen. One file, .text 0x80044FD8-0x8004F8F8:
// - "title.cpp" opens its string pool (0x802C64D4); the pool's last string ends at 0x802C65F6 and the menu
//   file's first string object follows at 0x802C65F8
// - its .sdata2 literals, 0x805A2DE8-0x805A2F70, are read by the functions from InitTitleScreen (0x80044FD8)
//   to its static initializer (0x8004F27C, the fourth .ctors entry) and by nothing else. The logo code
//   before it reads 0x805A2DA0-0x805A2DE8, the menu file behind it 0x805A2F70 and up
// - behind the static initializer come only what it and the vtables need: the array destructor it
//   registers (0x8004F894), a weak destructor (0x8004F8B0) and a this-adjusting thunk (0x8004F8F0). The menu
//   file's first function (0x8004F8F8) reads the next literal pool
// - extab 0x80009364-0x800095CC and extabindex 0x800124D0-0x80012650 are its 32 entries, in one run
// - .rodata 0x802C6468-0x802C65F8: three constant tables, the RTTI name "XObjDemo", then the string pool
// The functions come in the reverse of the GameCube's order (gxb.cpp from XObjDemo::DrawObject to
// InitTitleScreen).
//
// NONMATCHING: twenty-one functions have source so far: UpdateTitleOptions (0x8004C548), UpdateTitleDust
// (0x8004CA74), DrawTitleHelpText (0x8004D5C4), Title::UpdateSelections (0x8004E534) and the small
// states at the end of this file. UpdateSelections and the eight empty states are exact. The nine
// states that are only an OSPanic tail call (State6, State11-13, State18-22) differ only in the
// message's offset in the string pool, which the rest of the file's strings will move into place.
// In UpdateTitleOptions this build expands UpdateSelections where retail calls it (0x8004CA20),
// which leaves it at 70 %. Compiled outside the build with -inline noauto added, every instruction is
// retail's (99.6 %): what then differs is the name of the callee at 0x80231770, which is fmod and is in
// symbols.txt as exp, and the offset of the halt message in the string pool, which the rest of the file's
// strings will move into place. The flag is not set for the unit: too few neighbours to test it on.
// UpdateTitleDust is one bool temp and one branch polarity away from exact (99.7 %): retail keeps a
// bool temp for each byte range of the SJIS check and inits the flag in another register. In
// DrawTitleHelpText (82 %) the calls, the quad arguments and the float formulas are retail's, but the
// glyph loop's float scheduling and register pressure differ: retail saves r15 and folds the padding
// subtraction and the outline negations into fmsubs/fneg where this build moves and reloads.
#include "game/wii/title.h"
#include "game/wii/cgame.h"
#include "game/wii/cgs.h"
#include "game/cfont.h"
#include "game/coordinfo.h"
#include "game/cmdline.h"
#include "game/gmath.h"
#include "game/menupanelbackground.h"
#include "game/stagedata.h"
#include "game/cscrn.h"
#include "game/wii/saveslots.h"
#include "Dolphin/os.h"
#include "stl/math.h"
#include <stdio.h>
#include <stdlib.h>

extern "C" {
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRingScreenWidth, gRingScreenHeight, gScreenSafeBottom, gRefreshRate;
const char* GetTitleAndSaveMenuText(s32 index);
const char* GetLocalizedInterfaceText(s32 index);
void DrawTitleHelpText(Title* title);
void DrawTitleLoadScreen(Title* title);
void DrawTitleOptions(Title* title);
void DrawTitleNameBubbles(Title* title);
void fn_802AEDEC(WiiGsSceneObj** head, Mtx view);
CCamera* InitGameOrthoCamera(CCamera* camera, GXRenderModeObj* mode);
void OSFatal(GXColor fg, GXColor bg, const char* msg);
void ChangeGameStage(CGame* game, s32 stage, BOOL showName);
void ApproachHudValue(f32* value, f32* target, f32 step);
s32 IndexOfLargestOfThree(OSTime first, OSTime second, OSTime third);
// one dust mote's per-frame update, the body of the GameCube UpdateTitleDust's loop
void fn_8004DC94(TitleDust* dust, u32 frames);
// the pane-event slots in Title's vtable
void fn_8004DF38();
void fn_8004DF3C(Title* title, CbPane* pane);
}

// the figure overrides call their XObj bases by name; XObj's own declaration is still pending
class XObj;

// the cursor object's member at _E8 is an nw4r::g3d::ResFile (one pointer); the game's headers do not
// include the library's yet, so only the one method called here is declared
namespace nw4r {
namespace g3d {
class ResFile {
public:
	u32 GetResAnmChrNumEntries() const;
};
} // namespace g3d
} // namespace nw4r
extern "C" {
void ResetObjectState__4XObjFv(XObj* obj);
s32 LoadFromPlacement__4XObjFRC9CoordInfo(XObj* obj, const CoordInfo& info);
void Release__4XObjFv(XObj* obj);
void UpdateObject__4XObjFUl(XObj* obj, u32 frames);
}

// the slot-scan object inside CGame at _D9B80 and the save job's busy word at _EB0A0; the scan
// object starts like CSaveFile (file, then the scan entries) but CGame's region is smaller,
// so the members the title screen touches are reached by offset
static inline CSaveFile* SaveScan()
{
	return (CSaveFile*)((u8*)&gGame + 0xD9B80);
}
static inline s32* SaveBusy()
{
	return (s32*)((u8*)&gGame + 0xEB0A0);
}

// the pending fatal the save system left for the title screen: the message, two colors and the flag
// that raises it while a menu state still owns the frame
struct WiiFatalInfo {
	const char* mMessage; // _00
	GXColor mFg;          // _04
	u8 _08[4];
	GXColor mBg; // _0C
	u8 _10[4];
	u8 mActive; // _14
};
extern WiiFatalInfo gSaveFatal;

// the title screen's background and text colors, the logo's textures and which of each is in use
struct TitlePaletteEntry {
	GXColor background;
	GXColor text;
};
const TitlePaletteEntry gTitlePalette[] = {
	{ { 0, 0, 128, 0 }, {} },     { { 128, 0, 0, 0 }, {} },     { { 0, 128, 0, 0 }, {} },
	{ { 200, 200, 200, 0 }, {} }, { { 128, 128, 128, 0 }, {} }, { { 0, 207, 233, 0 }, {} },
	{ { 255, 220, 133, 0 }, {} }, { { 255, 220, 0, 0 }, {} },   { {}, { 255, 255, 255, 0 } },
};
const s32 gTitleLogoTextures[3] = { 0, 0, 0 };
s32 gTitlePaletteIndex          = 3;
s32 gTitleLogoIndex             = 1;

// the outline offsets of the help text's letter shadows
f32 gTitleHelpOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gTitleHelpOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

// the glyph width is rounded before subtracting it from the cell width (the GameCube's
// HelpGlyphPadding from drawhelptext.h, which cannot be included here; on the Wii the width
// division stays single precision)
static inline f32 HelpGlyphPadding(s32 width, f32 scale)
{
	f32 glyphWidth = (f32)(width / 24.0f) * scale;
	return scale - glyphWidth;
}

// the letters of the name shrink away
static inline void HideName(Title* title)
{
	for (u32 i = 0; i < 20; i++) {
		title->mName.mChars[i].mWait = 0.0f;
	}
}

// what a selection becomes when its screen opens or the controls change: nothing while the pointer
// chooses or is the only way to choose, else what it was last
static inline void ResetSelection(Title* title, s32& selection, const s32& last)
{
	if (title->IsPointerActive()) {
		selection = -1;
	} else if ((gGame.mMenuControls & MenuControlPointer) && !(gGame.mMenuControls & MenuControlStick)) {
		selection = -1;
	} else {
		selection = last;
	}
}

// the pixel engine state every 2D draw on the title screen needs, the GameCube's BeginTitleOverlay
static inline void BeginTitleOverlay()
{
	SetGsChannelEnabled(&gGame.mGs, 4, 0);
	gGame.mGs.mCurRenderState->SetColorUpdate(TRUE);
	gGame.mGs.mCurRenderState->SetAlphaUpdate(FALSE);
	gGame.mGs.mCurRenderState->SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_LEQUAL, 255);
	gGame.mGs.mCurRenderState->SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_COPY);
	gGame.mGs.mCurRenderState->SetZMode(FALSE, GX_ALWAYS, FALSE);
	if (gGame.mGs.mCurRenderState != NULL) {
		gGame.mGs.mCurRenderState->Apply();
	}
	if (gGame.mGs.mCurTevStage != NULL) {
		gGame.mGs.mCurTevStage->Apply();
	}
	if (gGame.mGs.mCurChannel != NULL) {
		gGame.mGs.mCurChannel->Apply();
	}
}

// one menu entry line: it pulses while the cursor's animation runs, but only once the highlight
// delay has counted down
static inline f32 DrawTitleEntry(Title* title, s32 entry, s32 textId, f32 y, f32 progress, u32 normal, u32 lit)
{
	const char* text = GetTitleAndSaveMenuText(textId);
	f32 size         = 24.0f + (title->mSelection == entry ? 2.0f * (f32)sin(4.712389f * progress) : 0.0f);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, TRUE, 24.0f);
	bool selected    = false;
	if (title->mSelection == entry && (f32)title->mHighlightDelay >= (f32)gRefreshRate) {
		selected = true;
	}
	if (entry != 1 || title->mSaveFound) {
		DrawGsProportionalTextFormat(&gGame.mGs, selected ? lit : normal, text, (f32)(gPointerScreenWidth / 2) - width * 0.5f,
		                             (size - 24.0f) * 0.5f + (y - size), 0.0f, 24.0f, size);
	}
	return width;
}

/**
 * @note Address: 0x80048E54
 * @note Size: 0x1950
 */
// NONMATCHING: the pass structure, the overlay, the dust, the logo, the figures, the footer and the
// save-fatal check are as retail. What follows the option block in retail -- the cursor halves the
// demo object draws at interpolated positions, the option bar snap behind _5174 and the three
// options-page lines -- is still being read; the entries and the guards match the GameCube shape.
extern "C" void DrawTitleScreen(Title* title)
{
	enum { dustTexture = 2, footerTexture = 1 };
	bool active = false;
	if (title->mState >= 1 && title->mState < 23) {
		active = true;
	}
	if (!active) {
		return;
	}
	bool homeMenu = false;
	if (gGame.mHomeMenu.mState >= 2 && gGame.mHomeMenu.mMenuStep >= 3) {
		homeMenu = true;
	}
	if (homeMenu) {
		return;
	}
	if (((CBase*)&gGame)->vf5C()) {
		if (title->mMenuAlpha > 0.0f) {
			BeginTitleOverlay();
			for (s32 i = 0; i < 16; ++i) {
				f32 alpha = title->mMenuAlpha;
				gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
				TitleDust& dust = title->mDust[i];
				f32 w           = dust.mSpeedX * (f32)GXGetTexObjWidth(&title->mTex[dustTexture]);
				f32 h           = dust.mSpeedX * (f32)GXGetTexObjHeight(&title->mTex[dustTexture]);
				u32 color       = 0xffffff00 | (u32)(255.0f * dust.mSpeedY * alpha);
				DrawGsTexturedQuad(&gGame.mGs, &title->mTex[dustTexture], color, color, color, color, dust.mX, dust.mY, 0.0f, w, h, 0.0f,
				                   0.0f, 0.0f);
				gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
			}
			gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
			CTexObj& logo = title->mTex[gTitleLogoTextures[gTitleLogoIndex]];
			u16 w         = GXGetTexObjWidth(&logo);
			u16 h         = GXGetTexObjHeight(&logo);
			u32 color     = 0xffffff00 | (u32)(255.0f * title->mMenuAlpha);
			DrawGsTexturedQuad(&gGame.mGs, &logo, color, color, color, color, (f32)(gPointerScreenWidth / 2),
			                   (f32)(gPointerScreenHeight / 2 - 0x68) - 6.0f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
		}
		((CBase*)&gGame)->vf54();
	}
	if (((CBase*)&gGame)->vf5C()) {
		for (s32 i = 0; i < 3; ++i) {
			XObjDemo& fig = title->mFigures[i];
			if (fig.mState != 0 && (fig.mStatus & 1)) {
				fig.vf174();
			}
		}
		CCamera* camera = ((CBase*)&gGame)->GetScreenCamera();
		fn_802AEDEC(&gGame.mGs.mSceneHead, camera->mViewMtx);
		WiiGsSceneObj* obj = gGame.mGs.mSceneHead;
		while (obj->mPending != 0) {
			obj->vf038(obj->mPending - 1);
		}
		((CBase*)&gGame)->vf54();
	}
	if (!((CBase*)&gGame)->vf5C()) {
		return;
	}
	if (title->mMenuAlpha > 0.0f) {
		BeginTitleOverlay();
		gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
		CTexObj& tex = title->mTex[footerTexture];
		f32 w        = 0.6f * (f32)GXGetTexObjWidth(&tex);
		f32 h        = 0.6f * (f32)GXGetTexObjHeight(&tex);
		u32 color    = (u32)(255.0f * title->mMenuAlpha);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, (f32)(gPointerScreenWidth / 2), (f32)(gScreenSafeBottom - 0x14),
		                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
		gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
	}
	gRingScreenWidth    = 0x280;
	gPointerScreenWidth = 0x280;
	s32 w               = gGame.mAspectRatio != 0 ? 0x356 : 0x280;
	s32 off             = (w - 0x280) / 2;
	CCamera* camera     = ((CBase*)&gGame)->GetScreenCamera();
	camera->vf24(0.0f, (f32)gPointerScreenHeight, -(f32)off, (f32)w - (f32)off);
	((CBase*)&gGame)->vf5C();
	DrawPanelBackground<SavePanel>(title->mLoadAlpha + title->mOptionAlpha);
	DrawTitleHelpText(title);
	DrawTitleLoadScreen(title);
	DrawTitleOptions(title);
	DrawTitleNameBubbles(title);
	if ((title->mMode == 0x13 || title->mMode == 0xE) && gSaveFatal.mActive && gSaveFatal.mMessage != NULL) {
		OSFatal(gSaveFatal.mFg, gSaveFatal.mBg, gSaveFatal.mMessage);
	}
	InitGameOrthoCamera(camera, &gGame.mGs.mScreen);
	((CBase*)&gGame)->vf5C();
	gRingScreenWidth    = w;
	gPointerScreenWidth = w;
	if ((u32)(title->mState - 10) <= 2) {
		f32 y0  = (f32)(gScreenSafeBottom - 0x36) - 40.0f - 30.0f;
		u32 rgb = ((u32)gTitlePalette[gTitlePaletteIndex].text.r << 24) | ((u32)gTitlePalette[gTitlePaletteIndex].text.g << 16)
		        | ((u32)gTitlePalette[gTitlePaletteIndex].text.b << 8);
		u32 normal   = rgb | (u8)(255.0f * title->mMenuAlpha);
		u32 lit      = normal;
		f32 progress = title->mCursor.vf11C() == 1 ? title->mCursor.vf128() / title->mCursor.vf12C() : 0.0f;
		f32 w0       = DrawTitleEntry(title, 0, 0, y0, progress, normal, lit);
		f32 w1       = DrawTitleEntry(title, 1, 1, y0 + 38.0f, progress, normal, lit);
		f32 w2       = DrawTitleEntry(title, 2, 2, y0 + 2.0f * 38.0f, progress, normal, lit);
		f32 y        = y0 + 2.0f * 38.0f;
		f32 width    = w2;
		switch (title->mSelection) {
		case 0:
			y     = y0;
			width = w0;
			break;
		case 1:
			y     = y0 + 38.0f;
			width = w1;
			break;
		case 2:
			break;
		}
		// retail draws the cursor halves here through the demo object at the option-line's y
	}
	// state 0x15's options-page lines and the cursor draws end the pass in retail
	((CBase*)&gGame)->vf54();
}

/**
 * @note Address: 0x8004CA74
 * @note Size: 0x344
 */
// the name is set from a C string: each letter gets a text pointer, a staggered shrink, a random
// bob and tilt, and the rest of the slots are cleared. SJIS lead bytes count once, only when the
// game language is Japanese
extern "C" void UpdateTitleDust(Title* title, const char* text, u32 color)
{
	title->mName.mActive = TRUE;
	title->mName.mCount  = 0;
	if (text == NULL) {
		return;
	}
	title->mName.mColor = color;
	for (s32 i = 0; i < 20 && *text; i++, text++) {
		title->mName.mChars[i].mText  = text;
		title->mName.mChars[i].mScale = -0.2f * i;
		title->mName.mChars[i].mWait  = gRefreshRate;
		title->mName.mChars[i].mBob   = DEG2RAD * (rand() % 360);
		title->mName.mChars[i].mTilt  = DEG2RAD * (rand() % 360);
		title->mName.mCount++;
		if (gGame.mLanguage == 0) {
			u8 c    = *text;
			u8 lead = TRUE;
			u8 a    = (c >= 0x81 && c <= 0x9F);
			if (!a) {
				u8 b = (c >= 0xE0 && c <= 0xFC);
				if (!b) {
					lead = FALSE;
				}
			}
			if (lead) {
				text++;
			}
		}
	}
	for (s32 i = title->mName.mCount; i < 20; i++) {
		title->mName.mChars[i].mText  = NULL;
		title->mName.mChars[i].mScale = 0.0f;
		title->mName.mChars[i].mWait  = 0.0f;
	}
}

// the options page: the pointer and the stick can each be turned off, but not both; OK (or A on the
// third line) writes the two to gGame.mMenuControls, B leaves them as they were
extern "C" void UpdateTitleOptions(Title* title, u32 frames)
{
	bool showPanes        = true;
	bool stickOverPointer = false;

	if (title->mOptionLine != -1) {
		title->mLastOptionLine = title->mOptionLine;
	}
	title->mCursor.UpdateObject(frames);
	title->mDemo.UpdateObject(frames);
	title->mHighlightDelay += frames;
	if (title->mHighlightDelay >= (f32)gRefreshRate) {
		title->mHighlightDelay = (f32)gRefreshRate;
		title->mPulseAngle     = fmod(title->mPulseAngle + frames * 9, 360.0);
	}

	s32 line = title->mOptionLine;
	if (line == -1) {
		line = title->mLastOptionLine;
	}
	title->mOptionBar[0] += 0.8f * (title->mOptionBarLines[line] - title->mOptionBar[0]);
	title->mOptionBar[1] += 0.8f * (title->mOptionBarTarget[0] - title->mOptionBar[1]);
	title->mOptionBar[2] += 0.8f * (title->mOptionBarTarget[1] - title->mOptionBar[2]);

	switch (title->mOptionState) {
	case 0:
		if (title->IsStickUsedOverPointer()) {
			stickOverPointer = true;
		}
		if (!stickOverPointer) {
			if (title->IsStickUp()) {
				if (title->mOptionLine > 0) {
					title->mOptionLine--;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					title->mPulseAngle = 45.0f;
				}
			} else if (title->IsStickDown()) {
				if (title->mOptionLine < 2) {
					title->mOptionLine++;
					PlayWiiSoundEffect(&gGame.mSound, 244, 1.0f, 0.0f);
					title->mPulseAngle = 45.0f;
				}
			}
		}
		if (title->IsConfirmPressed() && title->mOptionLine != -1) {
			showPanes = false;
			switch (title->mOptionLine) {
			case 0:
				if (title->mPointerOn) {
					title->mPointerOn = false;
					if (!title->mStickOn) {
						title->mStickOn = true;
					}
				} else {
					title->mPointerOn = true;
				}
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			case 1:
				if (title->mStickOn) {
					title->mStickOn = false;
					if (!title->mPointerOn) {
						title->mPointerOn = true;
					}
				} else {
					title->mStickOn = true;
				}
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			case 2:
				if (title->mPointerOn) {
					gGame.mMenuControls |= MenuControlPointer;
				} else {
					gGame.mMenuControls &= ~MenuControlPointer;
				}
				if (title->mStickOn) {
					gGame.mMenuControls |= MenuControlStick;
				} else {
					gGame.mMenuControls &= ~MenuControlStick;
				}
				title->mOptionState = 2;
				title->mPulseAngle  = 90.0f;
				PlayWiiSoundEffect(&gGame.mSound, 245, 1.0f, 0.0f);
				break;
			}
		} else if (title->IsCancelPressed()) {
			title->mOptionState = 2;
			title->mPulseAngle  = 90.0f;
			PlayWiiSoundEffect(&gGame.mSound, 243, 1.0f, 0.0f);
			showPanes         = false;
			title->mPointerOn = title->mSavedPointerOn;
			title->mStickOn   = title->mSavedStickOn;
		}
		break;
	case 1:
		OSPanic(__FILE__, 5496, "ここにきたら不正");
		break;
	case 2:
		showPanes                 = false;
		title->mPulseAngle        = 0.0f;
		title->mState             = 11;
		title->mTargetOptionAlpha = 0.0f;
		title->mOptionState       = 0;
		title->mOptionLine        = 0;
		title->mTargetPanelY      = -200.0f;
		title->mOffset            = 0.0f;
		title->mAlpha             = 0.0f;
		HideName(title);
		title->mPanes[8].mPointed  = false;
		title->mPanes[8].mPicked   = false;
		title->mPanes[9].mPointed  = false;
		title->mPanes[9].mPicked   = false;
		title->mPanes[10].mPointed = false;
		title->mPanes[10].mPicked  = false;
		title->UpdateSelections();
		break;
	}
	if (showPanes) {
		AddPointerPane(&gPointerPanes, &title->mPanes[8]);
		AddPointerPane(&gPointerPanes, &title->mPanes[9]);
		AddPointerPane(&gPointerPanes, &title->mPanes[10]);
	}
}

// the shared ease for the panel offset pair, the GameCube's ApproachTitleOffset
static inline void ApproachTitleOffset(f32& value, f32& target)
{
	f32 speed = (target - value) * .25f;
	if (speed < 0)
		speed = -speed;
	if (speed <= 1)
		speed = 1;
	ApproachHudValue(&value, &target, speed);
}

// the dust's per-frame drift, retail calls the helper once per mote
static inline void UpdateTitleDust(Title* title, u32 frames)
{
	for (s32 i = 0; i < 16; ++i) {
		fn_8004DC94(&title->mDust[i], frames);
	}
}

extern void (Title::*gTitleModeUpdates[])(u32);

/**
 * @note Address: 0x8004551C
 * @note Size: 0x1D8C
 */
// NONMATCHING: the frame skeleton, the save scan and states 1-9, 13, 14 and 22 are as retail read
// them. Inside state 9 the newest slot's saved pointer/stick bits are restored (the block after
// IndexOfLargestOfThree is only partly read) and inside 11-12 and 20-21 the pointer/confirm input
// and the mode dispatch follow the GameCube flow; the Wii input calls are the MenuUtil methods the
// binary shares with UpdateTitleOptions.
extern "C" void UpdateTitleScreen(Title* title, u32 frames)
{
	gPointerPanes.mCount = 0;
	ApproachTitleOffset(title->mPanelY, title->mTargetPanelY);
	ApproachHudValue(&title->mLoadAlpha, &title->mAlpha, .08f * frames);
	ApproachTitleOffset(title->mLoadOffset, title->mOffset);
	ApproachHudValue(&title->mOptionAlpha, &title->mTargetOptionAlpha, .08f * frames);
	title->mLoadScroll -= .008f * frames;
	if (title->mState != 0 && title->mState != 1 && title->mState != 23) {
		if (title->mName.mActive && title->mName.mActive > 0) {
			for (s32 i = 0; i < title->mName.mCount; ++i) {
				if (title->mName.mChars[i].mWait > 0) {
					if (title->mName.mChars[i].mScale < 1) {
						if ((title->mName.mChars[i].mScale += (1.f / (.3f * gRefreshRate)) * frames) > 1)
							title->mName.mChars[i].mScale = 1;
					}
				} else if (title->mName.mChars[i].mScale > 0) {
					if ((title->mName.mChars[i].mScale -= (1.f / (.3f * gRefreshRate)) * frames) < 0)
						title->mName.mChars[i].mScale = 0;
				}
			}
			s32 visible = 0;
			for (s32 i = 0; i < title->mName.mCount; ++i) {
				if (0 != title->mName.mChars[i].mScale) {
					++visible;
					title->mName.mChars[i].mBob += (5.f * DEG2RAD) * frames;
					title->mName.mChars[i].mTilt += (3.f * DEG2RAD) * frames;
				}
			}
			if (!visible)
				title->mName.mActive = 0;
		}
		switch (title->mHelpFadeState) {
		case 0:
			break;
		case 1:
			title->mHelpAlpha = 0;
			++title->mHelpFadeState;
			title->mHelpText = title->mNextHelpText;
			break;
		case 2:
			title->mHelpAlpha += .04f * frames;
			if (title->mHelpAlpha >= 1) {
				title->mHelpAlpha     = 1;
				title->mHelpFadeState = 0;
			}
			break;
		}
		if (title->mMenuAlpha > 0) {
			for (s32 i = 0; i < (s32)ARRAY_SIZE(title->mBg); ++i) {
				title->mBg[i].mAngle0 += (5.f * DEG2RAD) * frames;
				title->mBg[i].mAngle1 += (3.f * DEG2RAD) * frames;
			}
		}
		if (title->mMode != 1 && title->mMode != 8 && title->mMode != 12 && title->mMode != 15) {
			title->mReady       = 0;
			title->mWrongDevice = 0;
			// retail keeps rescanning the three NAND slots while the menu sits anywhere
			// interactive; a finished scan raises mReady through mCardScanState
			switch (title->mCardScanState) {
			case 0:
				StartWiiSaveSlotScan(SaveScan(), 3);
				title->mCardScanState = 1;
				break;
			case 1:
				if (!title->mSaveFound) {
					for (s32 i = 0; i < 3; ++i) {
						if (SaveScan()->mScanEntries[i].mStatus & 1) {
							title->mSaveFound = 1;
							break;
						}
					}
				}
				break;
			}
			if (!title->mReady)
				title->mShowMessage = 0;
			if (!title->mReady) {
				if (title->mState != 20 && title->mSelection == 1)
					title->mSelection = 0;
				title->mSaveFound     = 0;
				title->mCardScanState = 0;
			}
		}
		bool running = title->mState >= 1 && title->mState < 23;
		if (!running)
			return;
		if (*SaveBusy())
			return;
	}
	// 1 starts the slot scan, 2 reads its result, 3-7 load the cursor, the demo and the
	// figures, 8 reads the TPL, 9 finishes the scan, 10 fades the menu in, 11 is the menu,
	// 12 waits for the cursor's anim, 20 runs the file modes, 21 the options, 13-14 fade
	// out, 22 frees it all and changes the stage, 23 is done
	switch (title->mState) {
	case 1:
		title->mState = 2;
		StartWiiSaveSlotScan(SaveScan(), 3);
		gPointerPanes.mMenu    = title;
		gPointerPanes.mEnabled = 1;
		gPointerPanes.mActive  = 1;
		for (s32 i = 0; i < 11; ++i) {
			title->mPanes[i].mPointed = 0;
			title->mPanes[i].mPicked  = 0;
		}
		StartScreenFade(&gGame.mFade, 0, 0, 0, 255, 0, 0, 0, 255, 0.0f);
		break;
	case 2: {
		bool found = false;
		for (s32 i = 0; i < 3; ++i) {
			if (SaveScan()->mScanEntries[i].mStatus & 1) {
				found = true;
			}
		}
		if (found) {
			title->mSelection = 1;
			title->mSaveFound = 1;
		} else {
			title->mSelection     = 0;
			title->mSaveFound     = 0;
			title->mCardScanState = 0;
		}
		title->mLastSelection = title->mSelection;
		title->mSelection     = -1;
		title->mOptionBar[0]  = title->mEntryLines[title->mLastSelection];
		title->mOptionBarSnap = 1;
		title->mState         = 3;
		if (gCmdLineArgs.mTitleCursor < 0) {
			title->mSpawnIndex = rand() % 11;
		} else {
			title->mSpawnIndex = gCmdLineArgs.mTitleCursor % 11;
		}
		// fall through: loading starts on the same frame as cursor selection
	}
	case 3:
		if (title->mCursor.vf158(CoordInfo(GetSpawnData(title->mSpawnIndex)))) {
			if (*(u32*)((u8*)title->mCursor.mSpawn + 0xC8) == 0x3CA80) {
				title->mCursor.vf09C(4.0f);
			}
			++title->mState;
		}
		break;
	case 4:
		if (title->mDemo.vf158(CoordInfo(GetSpawnData(title->mSpawnIndex)))) {
			if (*(u32*)((u8*)title->mDemo.mSpawn + 0xC8) == 0x3CA80) {
				title->mDemo.vf09C(4.0f);
			}
			++title->mState;
		}
		break;
	case 5:
		if (title->mFigures[0].LoadFromPlacement(CoordInfo(GetSpawnData(0x31)))) {
			title->mFigures[0].vf09C(6.0f);
			title->mFigures[0].vf070(HALF_PI);
			++title->mState;
		}
		break;
	case 6:
		if (title->mFigures[1].LoadFromPlacement(CoordInfo(GetSpawnData(0x32)))) {
			title->mFigures[1].vf09C(6.0f);
			title->mFigures[1].vf070(HALF_PI);
			++title->mState;
		}
		break;
	case 7:
		if (title->mFigures[2].LoadFromPlacement(CoordInfo(GetSpawnData(0x33)))) {
			title->mFigures[2].vf09C(6.0f);
			title->mFigures[2].vf070(HALF_PI);
			++title->mState;
		}
		break;
	case 8: {
		char path[64];
		sprintf(path, "tpl/title%s.tpl", "");
		if (!title->mTpl.ReadEx(path, 0, 0)) {
			break;
		}
		StartWiiSaveSlotScan(SaveScan(), 3);
		title->mState = 9;
		break;
	}
	case 9: {
		if (*SaveBusy() != 0) {
			break;
		}
		title->mState = 10;
		s32 result    = SaveScan()->mFile.mResult;
		if (result < 0 && (result == -9 || result == -11)) {
			title->mSaveErrorCode = result;
			title->mSaveError     = 1;
		}
		OSTime stamps[3];
		for (s32 i = 0; i < 3; ++i) {
			WiiSaveScanEntry* entry = &SaveScan()->mScanEntries[i];
			stamps[i]               = 0;
			if (entry->mStatus & 1) {
				stamps[i] = (entry->mStatus & 2) ? 0 : ((WiiSaveSlotHeader*)entry->mData)->mTimestamp;
			}
		}
		title->mFile = IndexOfLargestOfThree(stamps[0], stamps[1], stamps[2]);
		// NONMATCHING: retail then walks the newest slot's saved mMenuControls back into
		// mPointerOn/mStickOn and primes the file rows; that tail is not fully read yet
		break;
	}
	case 10: {
		if (title->mMenuAlpha < 1) {
			if ((title->mMenuAlpha += (1.f / (.5f * gRefreshRate)) * frames) > 1)
				title->mMenuAlpha = 1;
		}
		if (title->mTitleAlpha < 1) {
			if ((title->mTitleAlpha += (1.f / gRefreshRate) * frames) > 1)
				title->mTitleAlpha = 1;
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		if (title->mMenuAlpha == 1 && title->mTitleAlpha == 1)
			title->mState = 11;
		break;
	}
	case 11: {
		title->mHighlightDelay += frames;
		if (title->mHighlightDelay >= gRefreshRate) {
			title->mHighlightDelay = gRefreshRate;
			title->mPulseAngle     = (f32)fmod(title->mPulseAngle + (f32)(frames * 9), 360.);
		}
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		title->UpdateSelections();
		if (title->IsConfirmPressed()) {
			switch (title->mSelection) {
			case 0:
			case 1:
				title->mState = 12;
				break;
			case 2:
				title->mState = 12;
				break;
			}
		}
		break;
	}
	case 12: {
		title->mCursor.UpdateObject(frames);
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		if (title->mCursor.vf134() == 0 || title->mCursor.vf11C() == 0
		    || ((nw4r::g3d::ResFile*)((u8*)&title->mCursor + 0xE8))->GetResAnmChrNumEntries() < 2) {
			title->mPulseAngle = 0;
			switch (title->mSelection) {
			case 0:
			case 1:
				title->mTargetPanelY = 0;
				title->mAlpha        = 1;
				title->mState        = 20;
				break;
			case 2:
				title->mTargetPanelY      = 0;
				title->mTargetOptionAlpha = 1;
				title->mState             = 21;
				title->mOptionState       = 0;
				title->mOptionLine        = 0;
				break;
			}
		}
		break;
	}
	case 20:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		(title->*gTitleModeUpdates[title->mMode])(frames);
		break;
	case 21:
		title->mDemo.UpdateObject(frames);
		UpdateTitleDust(title, frames);
		UpdateTitleOptions(title, frames);
		break;
	case 13:
		title->mState = 14;
		StartScreenFade(&gGame.mFade, gTitlePalette[gTitlePaletteIndex].background.r, gTitlePalette[gTitlePaletteIndex].background.g,
		                gTitlePalette[gTitlePaletteIndex].background.b, 0, 0, 0, 0, 255, .5f);
		for (s32 i = 0; i < 3; ++i) {
			title->mFigures[i].mState = 0;
		}
		// fall through
	case 14: {
		if (title->mMenuAlpha > 0) {
			if ((title->mMenuAlpha -= (1.f / (.5f * gRefreshRate)) * frames) < 0)
				title->mMenuAlpha = 0;
		}
		if (title->mTitleAlpha > 0) {
			if ((title->mTitleAlpha -= (1.f / (.5f * gRefreshRate)) * frames) < 0)
				title->mTitleAlpha = 0;
		}
		if (title->mMenuAlpha == 0) {
			bool faded = gGame.mFade.mSpeed.r == 0 && gGame.mFade.mSpeed.g == 0 && gGame.mFade.mSpeed.b == 0 && gGame.mFade.mSpeed.a == 0;
			if (faded) {
				GXColor black = { 0, 0, 0, 0 };
				title->mState = 22;
				GXSetCopyClear(black, 0xFFFFFF);
			}
		}
		break;
	}
	case 22:
		for (s32 i = 0; i < 3; ++i) {
			title->mFigures[i].Release();
		}
		title->mCursor.vf040();
		title->mDemo.vf040();
		title->mTpl.Close();
		if (title->mTimeout >= 0) {
			ChangeGameStage(&gGame, title->mTimeout, TRUE);
			title->mTimeout = -1;
		}
		gPointerPanes.mMenu = NULL;
		title->mState       = 23;
		break;
	case 23:
		break;
	}
	if (title->mState != 0 && title->mState != 22 && title->mState != 23) {
		GXSetCopyClear(gTitlePalette[gTitlePaletteIndex].background, 0xFFFFFF);
	}
}

/**
 * @note Address: 0x8004D5C4
 * @note Size: 0x6D0
 */
// the help line at the bottom: one blob per glyph, centred on the pointer's screen size. SJIS lead
// bytes are only counted when the game language is Japanese (gGame.mLanguage == 0)
extern "C" void DrawTitleHelpText(Title* menu)
{
	f32 alpha   = menu->mMenuAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	f32 centreX = gPointerScreenWidth / 2;
	f32 centreY = 160.0f + gPointerScreenHeight / 2;
	gGame.mGs.mCurRenderState->SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	gGame.mGs.mCurTevStage->SetMode(GX_MODULATE);
	(&gGame.mTpl)->mTex.Init(&gGame.mTpl.mPalette->descriptorArray[109 - StatusTextureOffset], 0);
	CTexObj blob     = gGame.mTpl.mTex;
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		const int limit = 20;
		for (int i = 0; i < limit && *text; i++, text++) {
			menu->mBg[i].mText = text;
			menu->mHelpCount++;
			if (gGame.mLanguage == 0) {
				u8 c    = *text;
				u8 lead = TRUE;
				u8 a    = (c >= 0x81 && c <= 0x9F);
				if (!a) {
					u8 b = (c >= 0xE0 && c <= 0xFC);
					if (!b) {
						lead = FALSE;
					}
				}
				if (lead) {
					text++;
				}
			}
		}
	}
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		s32 width;
		CFont font;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, 0, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 swell    = sin(PI * alpha);
		f32 grow     = sin(HALF_PI * alpha);
		f32 scale    = 0.2f * swell + grow;
		f32 x        = 0.5f * spacing + (i * spacing + (centreX - menu->mHelpCount * spacing * 0.5f));
		f32 y        = centreY + bob;
		rotation     = -(DEG2RAD * (10.0f * rotation));
		u32 color    = (u32)opacity | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		u32 oc      = (u32)opacity;
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, oc, oc, oc, oc, x + gTitleHelpOutlineX[j] + padding * 0.5f,
			                   bob + (centreY + gTitleHelpOutlineY[j]), 1.0f, scale, scale, 0.0f, -zero, rotation);
		}
		color = (u32)opacity | 0xFFFFFB00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x + padding * 0.5f, y, 1.0f, scale, scale, 0.0f, -zero, rotation);
	}
	gGame.mGs.mCurTevStage->SetMode(GX_PASSCLR);
}

void Title::UpdateSelections()
{
	if (mState == 11) {
		ResetSelection(this, mSelection, mLastSelection);
	}
	if (mState == 21) {
		mLastOptionLine = 0;
		ResetSelection(this, mOptionLine, mLastOptionLine);
	}
	if (mMode == 2) {
		ResetSelection(this, mFile, mLastFile);
	}
	if (mMode == 3 || mMode == 5 || mMode == 26 || mMode == 27) {
		mLastChoice = 1;
		ResetSelection(this, mChoice, mLastChoice);
	}
}

/**
 * @note Address: 0x800472A8
 * @note Size: 0x4
 */
void Title::State0(u32)
{
}

/**
 * @note Address: 0x800480B4
 * @note Size: 0x4
 */
void Title::State4(u32)
{
}

/**
 * @note Address: 0x80048728
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State6(u32)
{
	OSPanic("title.cpp", 0xa90, "こに来るのはおかしい");
}

/**
 * @note Address: 0x80048740
 * @note Size: 0x4
 */
void Title::State7(u32)
{
}

/**
 * @note Address: 0x80048A0C
 * @note Size: 0x4
 */
void Title::State10(u32)
{
}

/**
 * @note Address: 0x80048A10
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State11(u32)
{
	OSPanic("title.cpp", 0xbeb, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048A28
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State12(u32)
{
	OSPanic("title.cpp", 0xc64, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048A40
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State13(u32)
{
	OSPanic("title.cpp", 0xcc0, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048C9C
 * @note Size: 0x4
 */
void Title::State16(u32)
{
}

/**
 * @note Address: 0x80048CA0
 * @note Size: 0x4
 */
void Title::State17(u32)
{
}

/**
 * @note Address: 0x80048CA4
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State18(u32)
{
	OSPanic("title.cpp", 0xe87, "ォブラォン1.09-->1.10で作成したはずの削除です");
}

/**
 * @note Address: 0x80048CBC
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State19(u32)
{
	OSPanic("title.cpp", 0xe87, "ォブラォン1.09-->1.10で作成したはずの削除です");
}

/**
 * @note Address: 0x80048CD4
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State20(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048CEC
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State21(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048D04
 * @note Size: 0x18
 */
// unreachable on the Wii
void Title::State22(u32)
{
	OSPanic("title.cpp", 0xeb2, "ここにくるのおかしい");
}

/**
 * @note Address: 0x80048D1C
 * @note Size: 0x4
 */
void Title::State23(u32)
{
}

/**
 * @note Address: 0x80048E50
 * @note Size: 0x4
 */
void Title::State25(u32)
{
}

/**
 * @note Address: 0x800483F0
 * @note Size: 0x28
 */
// the mode table's entry 26: retail's fn_800483F0 waits out the file-menu's confirm before State24
// runs; the body is not read yet
void Title::State24(u32)
{
}

/**
 * @note Address: 0x80048418
 */
// the last gTitleModeUpdates entry, the file-menu mode past State25
void Title::fn_80048418(u32)
{
}

/**
 * @note Address: 0x8004DC94
 * @note Size: 0x174
 */
// one dust mote's drift cycle: a random delay, then it grows until it pops and respawns
void fn_8004DC94(TitleDust* dust, u32 frames)
{
	switch (dust->mState) {
	case 0:
		dust->mTimer = rand() % 120;
		++dust->mState;
		break;
	case 1:
		if (--dust->mTimer < 0) {
			dust->mState  = 2;
			dust->mSpeedY = 1.0f;
		}
		break;
	case 2:
		dust->mSpeedX += 0.018f * frames;
		dust->mSpeedY -= 0.009f * frames;
		if (dust->mSpeedX >= 2.0f) {
			dust->mX      = rand() % gRingScreenWidth;
			dust->mY      = rand() % gRingScreenHeight;
			dust->mState  = 0;
			dust->mSpeedX = 0.0f;
			dust->mSpeedY = 0.0f;
		}
		break;
	}
}

/**
 * @note Address: 0x8004DE08
 * @note Size: 0x64
 */
// ResetObjectState: the figure's own bookkeeping goes back to blank
void XObjDemo::ResetObjectState()
{
	ResetObjectState__4XObjFv((XObj*)this);
	mState      = 0;
	mStateParam = 0;
	mNextAnim   = -1;
	_BAC        = 0;
	for (s32 i = 0; i < 8; i++) {
		mAnimIds[i] = -1;
	}
}

/**
 * @note Address: 0x8004DE6C
 * @note Size: 0x2C
 */
// LoadFromPlacement: the base call's result becomes a plain flag
s32 XObjDemo::LoadFromPlacement(const CoordInfo& info)
{
	return LoadFromPlacement__4XObjFRC9CoordInfo((XObj*)this, info) != 0;
}

/**
 * @note Address: 0x8004DE98
 * @note Size: 0x4
 */
// Release
void XObjDemo::Release()
{
	Release__4XObjFv((XObj*)this);
}

// the screen fade has stopped, as in the GameCube's gxb.cpp
static inline bool FadeDone()
{
	return 0.0f == gGame.mFade.mSpeed.r && 0.0f == gGame.mFade.mSpeed.g && 0.0f == gGame.mFade.mSpeed.b && 0.0f == gGame.mFade.mSpeed.a;
}

/**
 * @note Address: 0x8004DE9C
 * @note Size: 0x78
 */
// UpdateObject: state 2 waits for the screen fade to finish, then goes quiet
void XObjDemo::UpdateObject(u32 frames)
{
	if (mState == 0) {
		return;
	}
	if (mState == 2 && FadeDone()) {
		mState = 0;
		return;
	}
	UpdateObject__4XObjFUl((XObj*)this, frames);
}

/**
 * @note Address: 0x8004DF14
 * @note Size: 0x24
 */
// DrawObject front: only draws while the figure is live
void XObjDemo::vf174()
{
	if (mState != 0) {
		vf178(0);
	}
}

/**
 * @note Address: 0x8004DF38
 * @note Size: 0x4
 */
// one of the pane-event slots in Title's vtable; retail's body is empty
void fn_8004DF38()
{
}

/**
 * @note Address: 0x8004DF3C
 * @note Size: 0x104
 */
// a pane event clears whichever tracked index pointed at it; the slot sits in Title's vtable
void fn_8004DF3C(Title* title, CbPane* pane)
{
	switch (pane->mKind) {
	case 0:
	case 1:
	case 2:
		if (title->mFile == pane->mKind && title->mFile != -1) {
			title->mFile = -1;
		}
		break;
	case 3:
	case 4:
		switch (title->mMode) {
		case 3:
		case 4:
		case 5:
		case 6:
		case 11:
		case 26:
		case 27:
			if (title->mChoice == pane->mKind - 3 && title->mChoice != -1) {
				title->mChoice = -1;
			}
			break;
		}
		break;
	case 5:
	case 6:
	case 7:
		if (title->mSelection == pane->mKind - 5 && title->mSelection != -1) {
			title->mSelection = -1;
		}
		break;
	}
	if (title->mState == 21 && pane->mKind >= 8 && pane->mKind < 11 && title->mOptionLine != -1) {
		title->mOptionLine = -1;
	}
}

// the handlers UpdateTitleScreen's state 20 dispatches by mMode
void (Title::*gTitleModeUpdates[])(u32) = {
	NULL,
	&Title::State0,
	&Title::State1,
	&Title::State2,
	&Title::State3,
	&Title::State4,
	&Title::State5,
	&Title::State6,
	&Title::State7,
	&Title::State8,
	&Title::State9,
	&Title::State10,
	&Title::State11,
	&Title::State12,
	&Title::State13,
	&Title::State14,
	&Title::State15,
	&Title::State16,
	&Title::State17,
	&Title::State18,
	&Title::State19,
	&Title::State20,
	&Title::State21,
	&Title::State22,
	&Title::State23,
	&Title::State24,
	&Title::State25,
	&Title::fn_80048418,
};
