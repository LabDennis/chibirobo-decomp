#ifndef GAME_WII_HOMEMENU_H
#define GAME_WII_HOMEMENU_H

#include "game/cfile.h"
#include "game/wii/cgs.h"
#include "revolution/hbm.h"

// mFiles, in the order they are read
enum {
	HomeMenuFileLayout,  // hbm/homeBtn*.arc
	HomeMenuFileSpeaker, // hbm/SpeakerSe.arc
	HomeMenuFileMessage, // hbm/home.csv
	HomeMenuFileConfig,  // hbm/config.txt
	HomeMenuFileSound,   // hbm/HomeButtonSe.arc
	HomeMenuFileCount
};

// mState. Below zero the "HOME Menu is not available" icon shows instead of the menu
enum {
	HomeMenuIconFadeOut = -3,
	HomeMenuIconHold    = -2,
	HomeMenuIconFadeIn  = -1,
	HomeMenuClosed      = 0,
	HomeMenuCapture     = 1, // the next draw copies the screen for the menu's background
	HomeMenuOpen        = 2
};

// the game's side of the HOME Button Menu, CGame keeps it at _B148. The names are ours
struct WiiHomeMenu {
	s32 mLoadStep;                   // _000, 0 to 4 read mFiles, 5 hands them to mInfo, 6 creates the menu, 7 is done
	CFile mFiles[HomeMenuFileCount]; // _004
	s32 mState;                      // _4A0
	bool mPausedGame;                // _4A4, opening the menu set the game's pause flag, closing it clears it again
	WiiFbTexture mCapture;           // _4A8, the screen as it was when the menu opened
	f32 mFade;                       // _5CC, the icon's and the background's alpha
	f32 mIconTime;                   // _5D0, frames the icon has held
	HBMDataInfo mInfo;               // _5D4
	HBMControllerData mControllers;  // _614
	s32 mMenuStep;                   // _654, 0 idle, 1 start the library, 3 it runs
	BOOL mAdjust;                    // _658, 16:9
	s32 _65C;                        // _65C, written 0, keeps the menu from starting while set

	bool IsOpen() const { return mState >= HomeMenuOpen && mMenuStep >= 0; }
	// the library runs: the menu draws over the whole frame
	bool IsRunning() const { return mState >= HomeMenuOpen && mMenuStep >= 3; }
};
STATIC_ASSERT(sizeof(WiiHomeMenu) == 0x660);

extern "C" {
void InitWiiHomeMenu(WiiHomeMenu* menu);
BOOL LoadWiiHomeMenu(WiiHomeMenu* menu);
void OpenWiiHomeMenu(WiiHomeMenu* menu);
bool UpdateWiiHomeMenu(WiiHomeMenu* menu, u32 frames);
BOOL DrawWiiHomeMenu(WiiHomeMenu* menu);
}

#endif
