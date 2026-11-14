#ifndef GAME_WII_SAVESLOTS_H
#define GAME_WII_SAVESLOTS_H

#include "game/wii/nandfile.h"
#include "Dolphin/vec.h"

struct WiiSaveSlotHeader {
	u32 mSignature;
	u32 mVersion;
	u64 mTimestamp;
	s32 mStages[2];
	u32 _18;
	u32 mStatusFlags; // _1C, the low word of gGame.GetStatusFlags(1); bit 0 means a camera was saved
	f32 mPlayTime;    // _20
	s32 mTimeMode;    // _24
	f32 mTime;        // _28
	f32 mDuration;    // _2C
	s32 mExp;         // _30
	s32 mLevel;       // _34
	s32 mRank;        // _38
	f32 mBattery;     // _3C
	f32 mMaxBattery;  // _40
	s32 mMoney;       // _44
	s32 mScrap;       // _48
	s32 mActive;      // _4C
	u8 _50[0x78 - 0x50];
	u32 mMenuControls; // bit 0 enables pointer navigation, bit 1 enables stick navigation
	u32 mChecksum;
};
STATIC_ASSERT(sizeof(WiiSaveSlotHeader) == 0x80);

// the saved game state, the GameCube SaveData without its header
struct WiiSaveData {
	u8 mPlayer[0x400];
	u8 mEvtFlags[0x200];
	u8 mEvtVars[0x2000];
	u32 mCoinFlags[0x80];
	u32 mItemFlags[0x80];
	u32 mDustFlags[0x80];
	u8 mPlayerRecord[0x48];
	s32 mSuit;
	Vec mPos;
	f32 mRotY;
	s32 mSpecialSuit;
	s32 mCameraAction;
	Vec mCameraEye;
	Vec mCameraInterest;
	Vec mCameraAngle;
	f32 mCameraDistance;
	f32 mCameraFov;
	s32 mSquirterLiquid;
	s32 mItemTint;
	u8 _2C98[0x2CA0 - 0x2C98];
};
STATIC_ASSERT(sizeof(WiiSaveData) == 0x2ca0);

struct WiiSaveSlot {
	WiiSaveSlotHeader mHeader;
	WiiSaveData mData;
};
STATIC_ASSERT(sizeof(WiiSaveSlot) == 0x2d20);

// ScanWiiSaveSlots fills these while probing the slots in the NAND: mData holds the last verified
// slot copy and mStatus is 1 (valid), 3 (unusable), 4 (empty), 8 (no blocks) or 0x10 (no inodes).
struct WiiSaveScanEntry {
	u8 mData[0x80];
	u32 mStatus;
	u8 _84[0x1c];
};
STATIC_ASSERT(sizeof(WiiSaveScanEntry) == 0xa0);

struct WiiSaveManager {
	WiiGameSaveFile mFile;
	u8 _94[0xa0 - sizeof(WiiGameSaveFile)];
	WiiSaveScanEntry mScanEntries[10];
	WiiSaveSlot mSlot;
	char mHomeDir[64];
	char mSlotPath[64];
	NANDBanner mBanner;
	s32 mJobStarted;
};

extern "C" {
void InitializeWiiSaveManager(WiiSaveManager* manager);
void UpdateWiiSaveJob(WiiSaveManager* manager);
int CheckWiiSaveBanner(WiiSaveManager* manager, bool removeInvalid);
s32 WriteWiiSaveBanner(WiiSaveManager* manager);
void StartWiiSaveSlotScan(WiiSaveManager* manager, int count);
s32 WriteWiiSaveSlot(WiiSaveManager* manager, int slot, bool newGame);
void StartWiiSaveSlotWrite(WiiSaveManager* manager, int slot, int newGame);
s32 ReadWiiSaveSlot(WiiSaveManager* manager, int slot);
void StartWiiSaveSlotRead(WiiSaveManager* manager, int slot);
void StartWiiSaveSlotDelete(WiiSaveManager* manager, int slot);
}

#endif
