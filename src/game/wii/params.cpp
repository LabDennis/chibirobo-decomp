#include "game/wii/cgame.h"

// the Wii port kept the GameCube Params layout (verified member offsets in WiiPlayerParams)
typedef WiiPlayerParams Params;

extern "C" {
void SpawnPlayerPointParticles(WiiPlayerState* player, s32 type, s32 value);
// the Wii HUD callbacks are empty stubs; the object at CGame+0xAF58 is unreconstructed
void InfoScrn_OnPlayerLevelChanged(void* scrn, s32 level);
void InfoScrn_OnPlayerExperienceChanged(void* scrn, s32 delta);
}

/**
 * @note Address: 0x80113394
 * @note Size: 0xF0
 */
extern "C" void ResetPlayerStats(Params* stats)
{
	stats->mExperience = 0;
	stats->mLevel      = 0;
	stats->mRank       = 1000000; // rank counts down from the max
	stats->mMaxBattery = 80.0f;   // a fresh battery
	stats->mBattery    = stats->mMaxBattery;
	stats->mMoney      = 0;
	stats->mScrap      = 0;
	stats->_1C[0]      = 0;
	stats->_1C[1]      = 0;
	stats->_1C[2]      = 0;
	stats->_1C[3]      = 0;
	stats->_1C[4]      = 0;
	stats->_1C[5]      = 0;
	stats->_1C[6]      = 0;
	stats->_1C[7]      = 0;
	for (u32 i = 0; i < 256; ++i) {
		stats->mInventory.mItems[i].mId    = -1;
		stats->mInventory.mItems[i].mCount = 0;
	}
	stats->mInventory.mCompact = 1;
}

/**
 * @note Address: 0x80113484
 * @note Size: 0x94
 */
extern "C" void AdjustPlayerExperience(Params* stats, s32 delta)
{
	stats->mExperience += delta;
	if (stats->mExperience < 0)
		stats->mExperience = 0;
	if (stats->mExperience > 99999)
		stats->mExperience = 99999;
	if (delta != 0) {
		InfoScrn_OnPlayerExperienceChanged(&gGame.mInfoScrn, delta);
		SpawnPlayerPointParticles(gGame.GetPlayer(), 0, delta);
	}
}

/**
 * @note Address: 0x80113518
 * @note Size: 0x98
 */
extern "C" void AdjustPlayerLevel(Params* stats, s32 delta)
{
	stats->mLevel += delta;
	if (stats->mLevel < 0)
		stats->mLevel = 0;
	if (stats->mLevel > 30)
		stats->mLevel = 30;
	if (delta != 0) {
		InfoScrn_OnPlayerLevelChanged(&gGame.mInfoScrn, stats->mLevel + 1);
		SpawnPlayerPointParticles(gGame.GetPlayer(), 1, stats->mLevel + 1);
	}
}

/**
 * @note Address: 0x801135B0
 * @note Size: 0x38
 */
extern "C" void AddPlayerRank(Params* stats, s32 delta)
{
	stats->mRank += delta;
	if (stats->mRank < 1)
		stats->mRank = 1;
	if (stats->mRank > 1000000)
		stats->mRank = 1000000;
}

/**
 * @note Address: 0x801135E8
 * @note Size: 0x58
 */
extern "C" void AdjustPlayerMoney(Params* stats, s32 delta)
{
	stats->mMoney += delta;
	if (stats->mMoney < 0)
		stats->mMoney = 0;
	if (stats->mMoney > 99999)
		stats->mMoney = 99999;
	if (delta != 0)
		SpawnPlayerPointParticles(gGame.GetPlayer(), 2, delta);
}

/**
 * @note Address: 0x80113640
 * @note Size: 0x58
 */
extern "C" void AddPlayerScrap(Params* stats, s32 delta)
{
	stats->mScrap += delta;
	if (stats->mScrap < 0)
		stats->mScrap = 0;
	if (stats->mScrap > 99999) // the 5-digit cap
		stats->mScrap = 99999;
	if (delta != 0)
		SpawnPlayerPointParticles(gGame.GetPlayer(), 3, delta); // type 3 is the scrap particle
}

/**
 * @note Address: 0x80113698
 * @note Size: 0x13C
 */
extern "C" s32 GetOwnedInventoryItemCount(Params* stats, s16 id)
{
	s32 index = FindOwnedInventoryItemIndex(stats->mInventory.mItems, id);
	return index >= 0 ? stats->mInventory.mItems[index].mCount : 0;
}

/**
 * @note Address: 0x801137D4
 * @note Size: 0x13C
 */
extern "C" s32 FindOwnedInventoryItemIndex(InventoryItem* entries, s16 id)
{
	for (u32 i = 0; i < 256; ++i) {
		if (id == entries[i].mId && entries[i].mCount > 0)
			return i;
	}
	return -1;
}
