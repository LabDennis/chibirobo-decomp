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
#include "game/gmath.h"
#include "Dolphin/os.h"
#include "stl/math.h"
#include <stdlib.h>

extern "C" {
extern s32 gPointerScreenWidth, gPointerScreenHeight, gRefreshRate;
}

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
		title->mOptionPanes[0].mPointed = false;
		title->mOptionPanes[0].mPicked  = false;
		title->mOptionPanes[1].mPointed = false;
		title->mOptionPanes[1].mPicked  = false;
		title->mOptionPanes[2].mPointed = false;
		title->mOptionPanes[2].mPicked  = false;
		title->UpdateSelections();
		break;
	}
	if (showPanes) {
		AddPointerPane(&gPointerPanes, &title->mOptionPanes[0]);
		AddPointerPane(&gPointerPanes, &title->mOptionPanes[1]);
		AddPointerPane(&gPointerPanes, &title->mOptionPanes[2]);
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
