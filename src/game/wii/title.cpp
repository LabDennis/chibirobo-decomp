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
// NONMATCHING: only UpdateTitleOptions (0x8004C548) and Title::UpdateSelections (0x8004E534) have source so
// far. UpdateSelections is exact. In UpdateTitleOptions this build expands UpdateSelections where retail
// calls it (0x8004CA20), which leaves it at 70 %. Compiled outside the build with -inline noauto added,
// every instruction is retail's (99.6 %): what then differs is the name of the callee at 0x80231770, which
// is fmod and is in symbols.txt as exp, and the offset of the halt message in the string pool, which the
// rest of the file's strings will move into place. The flag is not set for the unit: two functions are
// too few neighbours to test it on.
#include "game/wii/title.h"
#include "game/wii/cgame.h"
#include "Dolphin/os.h"
#include "stl/math.h"

extern "C" {
extern s32 gRefreshRate;
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
