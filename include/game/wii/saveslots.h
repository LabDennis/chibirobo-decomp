#ifndef GAME_WII_SAVESLOTS_H
#define GAME_WII_SAVESLOTS_H

#include "game/wii/nandfile.h"

struct WiiSaveSlotHeader {
	u32 mSignature;
	u32 mVersion;
	u64 mTimestamp;
	s32 mStages[2];
	u8 _18[0x78 - 0x18];
	u32 mMenuControls; // bit 0 enables pointer navigation, bit 1 enables stick navigation
	u32 mChecksum;
};
STATIC_ASSERT(sizeof(WiiSaveSlotHeader) == 0x80);

struct WiiSaveSlot {
	WiiSaveSlotHeader mHeader;
	u8 mPayload[0x2d20 - sizeof(WiiSaveSlotHeader)];
};
STATIC_ASSERT(sizeof(WiiSaveSlot) == 0x2d20);

struct WiiSaveManager {
	WiiGameSaveFile mFile;
	u8 _94[0x6e0 - sizeof(WiiGameSaveFile)];
	WiiSaveSlot mSlot;
	char mHomeDir[64];
	char mSlotPath[64];
	u8 mBanner[0xf0a0];
	s32 mJobStarted;
};

extern "C" {
void StartWiiSaveSlotScan(WiiSaveManager* manager, int count);
s32 WriteWiiSaveSlot(WiiSaveManager* manager, int slot, bool newGame);
void StartWiiSaveSlotWrite(WiiSaveManager* manager, int slot, int newGame);
s32 ReadWiiSaveSlot(WiiSaveManager* manager, int slot);
void StartWiiSaveSlotRead(WiiSaveManager* manager, int slot);
void StartWiiSaveSlotDelete(WiiSaveManager* manager, int slot);
}

#endif
